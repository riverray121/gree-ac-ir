#include "kelvinator_ac.h"

#include <cmath>

#include <IRremoteESP8266.h>
#include <ir_Kelvinator.h>

#include "esphome/core/log.h"

namespace esphome {
namespace kelvinator_ac {

static const char *const TAG = "kelvinator_ac";

void KelvinatorAC::setup() {
  this->ac_ = new IRKelvinatorAC(this->pin_);
  // With the RMT path the transmitter owns the pin; begin() would claim it
  // as a plain output for the library's bit-banged send.
  if (this->rmt_ == nullptr)
    this->ac_->begin();

  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->target_temperature = 26;
    this->fan_mode = climate::CLIMATE_FAN_AUTO;
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  }
}

void KelvinatorAC::dump_config() {
  ESP_LOGCONFIG(TAG, "Kelvinator A/C (IRremoteESP8266) on GPIO%u", this->pin_);
  if (this->rmt_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  RMT transmit: hdr %u/%u bit %u one %u zero %u gap %u", this->t_hdr_mark_,
                  this->t_hdr_space_, this->t_bit_mark_, this->t_one_space_, this->t_zero_space_, this->t_gap_);
  }
}

climate::ClimateTraits KelvinatorAC::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
      climate::CLIMATE_MODE_HEAT_COOL,
  });
  traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
  });
  traits.set_supported_swing_modes({
      climate::CLIMATE_SWING_OFF,
      climate::CLIMATE_SWING_VERTICAL,
  });
  traits.set_visual_min_temperature(16);
  traits.set_visual_max_temperature(30);
  traits.set_visual_target_temperature_step(1);
  return traits;
}

void KelvinatorAC::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value())
    this->mode = *call.get_mode();
  // A temperature change while off implies power-on: "set the AC to 28"
  // must work without a separate turn-on command, from every control path.
  else if (call.get_target_temperature().has_value() &&
           this->mode == climate::CLIMATE_MODE_OFF)
    this->mode = climate::CLIMATE_MODE_COOL;
  if (call.get_target_temperature().has_value())
    this->target_temperature = *call.get_target_temperature();
  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();
  if (call.get_swing_mode().has_value())
    this->swing_mode = *call.get_swing_mode();

  this->publish_state();
  // Scheduling through a named timeout coalesces rapid commands (only the
  // final state is transmitted) and applies this unit's transmit slot.
  this->set_timeout("transmit", this->tx_delay_ms_, [this]() { this->transmit_state_(); });
}

void KelvinatorAC::transmit_state_() {
  auto *ac = this->ac_;

  if (this->mode == climate::CLIMATE_MODE_OFF) {
    ac->setPower(false);
  } else {
    ac->setPower(true);
    switch (this->mode) {
      case climate::CLIMATE_MODE_COOL:
        ac->setMode(kKelvinatorCool);
        break;
      case climate::CLIMATE_MODE_HEAT:
        ac->setMode(kKelvinatorHeat);
        break;
      case climate::CLIMATE_MODE_DRY:
        ac->setMode(kKelvinatorDry);
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        ac->setMode(kKelvinatorFan);
        break;
      default:
        ac->setMode(kKelvinatorAuto);
        break;
    }
  }

  ac->setTemp((uint8_t) lroundf(this->target_temperature));

  climate::ClimateFanMode fan =
      this->fan_mode.has_value() ? *this->fan_mode : climate::CLIMATE_FAN_AUTO;
  switch (fan) {
    case climate::CLIMATE_FAN_LOW:
      ac->setFan(kKelvinatorFanMin);
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      ac->setFan(3);
      break;
    case climate::CLIMATE_FAN_HIGH:
      ac->setFan(kKelvinatorFanMax);
      break;
    default:
      ac->setFan(kKelvinatorFanAuto);
      break;
  }

  if (this->swing_mode == climate::CLIMATE_SWING_VERTICAL) {
    ac->setSwingVertical(true, kKelvinatorSwingVAuto);
  } else {
    ac->setSwingVertical(false, kKelvinatorSwingVOff);
  }

  ac->setLight(true);
  // One repeat: every state goes out as two back-to-back frames, so a
  // marginal signal path (distance, angle) still lands the command.
  if (this->rmt_ != nullptr) {
    this->send_rmt_(ac->getRaw());
  } else {
    ac->send(1);
  }
  ESP_LOGD(TAG, "Sent Kelvinator state: %s", ac->toString().c_str());
  if (this->tx_log_ != nullptr) {
    this->tx_count_++;
    char buf[240];
    snprintf(buf, sizeof(buf), "#%u %s", (unsigned) this->tx_count_, ac->toString().c_str());
    this->tx_log_->publish_state(buf);
  }
}

// Same frame layout as IRsend::sendKelvinator: two halves, each a header,
// 4 command bytes LSB first, the 3-bit footer 010, a gap, 4 data bytes and
// a double gap; the whole frame once more as the repeat.
void KelvinatorAC::send_rmt_(const uint8_t *data) {
  auto call = this->rmt_->transmit();
  auto *out = call.get_data();
  out->set_carrier_frequency(this->carrier_hz_);
  const int32_t b = this->mark_bias_;
  auto mark = [&](uint32_t us) { out->mark(us + b); };
  auto space = [&](uint32_t us) { out->space(us - b); };
  auto bits = [&](const uint8_t *bytes, size_t n) {
    for (size_t i = 0; i < n; i++) {
      for (uint8_t bit = 0; bit < 8; bit++) {
        mark(this->t_bit_mark_);
        space(((bytes[i] >> bit) & 1) ? this->t_one_space_ : this->t_zero_space_);
      }
    }
  };
  for (int r = 0; r < this->repeats_; r++) {
    for (int half = 0; half < 2; half++) {
      const uint8_t *blk = data + half * 8;
      mark(this->t_hdr_mark_);
      space(this->t_hdr_space_);
      bits(blk, 4);
      // footer bits 0b010, LSB first: 0, 1, 0
      mark(this->t_bit_mark_);
      space(this->t_zero_space_);
      mark(this->t_bit_mark_);
      space(this->t_one_space_);
      mark(this->t_bit_mark_);
      space(this->t_zero_space_);
      mark(this->t_bit_mark_);
      out->space(this->t_gap_);
      bits(blk + 4, 4);
      mark(this->t_bit_mark_);
      out->space(this->half_gap_);
    }
  }
  call.perform();
}

}  // namespace kelvinator_ac
}  // namespace esphome

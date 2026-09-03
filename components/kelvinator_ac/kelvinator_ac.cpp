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
  ac->send(1);
  ESP_LOGD(TAG, "Sent Kelvinator state: %s", ac->toString().c_str());
}

}  // namespace kelvinator_ac
}  // namespace esphome

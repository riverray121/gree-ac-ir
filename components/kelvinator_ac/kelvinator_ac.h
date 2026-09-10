#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

class IRKelvinatorAC;

namespace esphome {
namespace kelvinator_ac {

// Climate entity that transmits Gree/Kelvinator long-format (128-bit) IR
// frames via IRremoteESP8266's IRKelvinatorAC. The library bit-bangs the
// carrier on the given GPIO itself, so the pin must not also be claimed by
// a remote_transmitter.
class KelvinatorAC : public climate::Climate, public Component {
 public:
  explicit KelvinatorAC(uint8_t pin) : pin_(pin) {}

  void setup() override;
  void dump_config() override;
  // Per-unit transmit slot: units with different delays never put IR on the
  // air simultaneously, so a multi-AC command cannot collide at a receiver
  // that can see more than one emitter.
  void set_tx_delay(uint32_t delay_ms) { this->tx_delay_ms_ = delay_ms; }
  // Publishes a numbered entry for every IR transmission so HA's recorder
  // keeps a history of what actually went on the air.
  void set_tx_log(text_sensor::TextSensor *ts) { this->tx_log_ = ts; }
  // Hardware-timed path: frames go out through an RMT transmitter with these
  // pulse lengths (microseconds); the library only supplies the state bytes.
  void set_rmt(remote_base::RemoteTransmitterBase *tx, uint32_t header_mark, uint32_t header_space,
               uint32_t bit_mark, uint32_t one_space, uint32_t zero_space, uint32_t gap) {
    this->rmt_ = tx;
    this->t_hdr_mark_ = header_mark;
    this->t_hdr_space_ = header_space;
    this->t_bit_mark_ = bit_mark;
    this->t_one_space_ = one_space;
    this->t_zero_space_ = zero_space;
    this->t_gap_ = gap;
  }

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  void transmit_state_();
  void send_rmt_(const uint8_t *data);

 public:
  // Live-tunable RMT carrier and frame count, for finding what a marginal
  // receiver accepts.
  void set_carrier_hz(uint32_t hz) { this->carrier_hz_ = hz; }
  void set_repeats(uint8_t n) { this->repeats_ = n; }
  // Adds to every mark and subtracts from every bit space (bit periods stay
  // constant): compensates a receiver that shortens weak-signal marks.
  void set_mark_bias(int32_t us) { this->mark_bias_ = us; }
  // Silence between the two halves of a message and before the repeat.
  void set_half_gap(uint32_t us) { this->half_gap_ = us; }

 protected:
  uint32_t carrier_hz_{38000};
  uint8_t repeats_{2};
  int32_t mark_bias_{0};
  uint32_t half_gap_{40000};

  uint8_t pin_;
  uint32_t tx_delay_ms_{0};
  uint32_t tx_count_{0};
  text_sensor::TextSensor *tx_log_{nullptr};
  IRKelvinatorAC *ac_{nullptr};
  remote_base::RemoteTransmitterBase *rmt_{nullptr};
  uint32_t t_hdr_mark_{0}, t_hdr_space_{0}, t_bit_mark_{0}, t_one_space_{0}, t_zero_space_{0}, t_gap_{0};
};

}  // namespace kelvinator_ac
}  // namespace esphome

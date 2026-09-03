#pragma once

#include "esphome/components/climate/climate.h"
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

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  void transmit_state_();

  uint8_t pin_;
  uint32_t tx_delay_ms_{0};
  IRKelvinatorAC *ac_{nullptr};
};

}  // namespace kelvinator_ac
}  // namespace esphome

#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

class IRrecv;

namespace esphome {
namespace kelvinator_rx {

// Text sensor that decodes every IR burst seen by a demodulating receiver
// (TL1838 class, active low) with IRremoteESP8266's IRrecv and publishes
// one numbered entry per burst. Kelvinator frames are rendered in the same
// format as the transmitters' IR TX Log, so a transmit entry and the
// matching receive entry can be compared field by field. Bursts that do
// not decode are published as INVALID with the raw symbol count, which is
// the evidence for a corrupted or truncated frame.
class KelvinatorRx : public text_sensor::TextSensor, public Component {
 public:
  explicit KelvinatorRx(uint8_t pin) : pin_(pin) {}

  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  uint8_t pin_;
  uint32_t rx_count_{0};
  IRrecv *rx_{nullptr};
};

}  // namespace kelvinator_rx
}  // namespace esphome

#include "kelvinator_rx.h"

#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <ir_Kelvinator.h>

#include "esphome/core/log.h"

namespace esphome {
namespace kelvinator_rx {

static const char *const TAG = "kelvinator_rx";

// A Kelvinator transmission is two 64-bit halves separated by a ~20 ms gap,
// and the transmitters follow each frame with a repeat after a ~25 ms gap.
// The capture must not split at the 20 ms gap, so the idle timeout sits
// well above it; frame and repeat therefore land in one capture and the
// raw symbol count (~265 per frame) tells how many frames arrived.
static const uint16_t kCaptureBuffer = 1024;
static const uint8_t kTimeoutMs = 50;
// Anything shorter than a protocol header plus a few bits is receiver noise.
static const uint16_t kMinRawLen = 20;

void KelvinatorRx::setup() {
  this->rx_ = new IRrecv(this->pin_, kCaptureBuffer, kTimeoutMs, true);
  this->rx_->enableIRIn();
}

void KelvinatorRx::dump_config() {
  ESP_LOGCONFIG(TAG, "Kelvinator IR receiver (IRremoteESP8266) on GPIO%u", this->pin_);
}

void KelvinatorRx::loop() {
  decode_results results;
  if (!this->rx_->decode(&results))
    return;
  if (results.rawlen >= kMinRawLen) {
    this->rx_count_++;
    char buf[240];
    if (results.decode_type == decode_type_t::KELVINATOR) {
      IRKelvinatorAC ac(0);
      ac.setRaw(results.state);
      snprintf(buf, sizeof(buf), "#%u %s | raw=%u", (unsigned) this->rx_count_, ac.toString().c_str(),
               (unsigned) results.rawlen);
    } else {
      snprintf(buf, sizeof(buf), "#%u INVALID type=%s bits=%u raw=%u", (unsigned) this->rx_count_,
               typeToString(results.decode_type).c_str(), (unsigned) results.bits, (unsigned) results.rawlen);
    }
    ESP_LOGD(TAG, "%s", buf);
    this->publish_state(buf);
  }
  this->rx_->resume();
}

}  // namespace kelvinator_rx
}  // namespace esphome

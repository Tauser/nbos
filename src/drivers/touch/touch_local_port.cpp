#include "drivers/touch/touch_local_port.hpp"

namespace ncos::drivers::touch {

bool TouchLocalPort::ensure_ready() {
  return bringup_.init();
}

bool TouchLocalPort::calibrate_idle() {
  if (!ensure_ready()) {
    return false;
  }

  TouchStats stats{};
  if (!bringup_.sample_idle_stats(16, 8, &stats)) {
    return false;
  }

  baseline_raw_ = stats.avg_raw;
  trigger_delta_ = stats.suggested_delta > 0 ? stats.suggested_delta : 400;
  calibrated_ = true;
  return true;
}

bool TouchLocalPort::read_raw(uint32_t* out_raw) {
  if (!calibrated_) {
    (void)calibrate_idle();
  }
  return bringup_.read_raw(out_raw);
}

uint32_t TouchLocalPort::baseline_raw() const {
  return baseline_raw_;
}

uint32_t TouchLocalPort::trigger_delta() const {
  return trigger_delta_;
}

ncos::interfaces::sensing::TouchPort* acquire_shared_touch_port() {
  static TouchLocalPort shared{};
  return &shared;
}

}  // namespace ncos::drivers::touch

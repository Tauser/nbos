#include "drivers/led/led_local_port.hpp"

namespace ncos::drivers::led {

bool LedLocalPort::ensure_ready() {
  ready_ = true;
  return true;
}

bool LedLocalPort::apply_state(const ncos::core::contracts::LedState& state) {
  (void)state;
  return ready_;
}

ncos::interfaces::led::LedPort* acquire_shared_led_port() {
  static LedLocalPort shared{};
  return &shared;
}

}  // namespace ncos::drivers::led

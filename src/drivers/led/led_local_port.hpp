#pragma once

#include "interfaces/led/led_port.hpp"

namespace ncos::drivers::led {

class LedLocalPort final : public ncos::interfaces::led::LedPort {
 public:
  bool ensure_ready() override;
  bool apply_state(const ncos::core::contracts::LedState& state) override;

 private:
  bool ready_ = false;
};

ncos::interfaces::led::LedPort* acquire_shared_led_port();

}  // namespace ncos::drivers::led

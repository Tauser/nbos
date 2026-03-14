#pragma once

#include "core/contracts/led_orchestration_contracts.hpp"

namespace ncos::interfaces::led {

class LedPort {
 public:
  virtual ~LedPort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool apply_state(const ncos::core::contracts::LedState& state) = 0;
};

}  // namespace ncos::interfaces::led

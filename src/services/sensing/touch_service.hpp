#pragma once

#include <stdint.h>

#include "core/contracts/sensing_runtime_contracts.hpp"
#include "interfaces/sensing/touch_port.hpp"

namespace ncos::services::sensing {

class TouchService final {
 public:
  void bind_port(ncos::interfaces::sensing::TouchPort* port);
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms);
  [[nodiscard]] const ncos::core::contracts::TouchRuntimeState& state() const;

 private:
  static uint16_t normalize_touch_level(uint32_t raw, uint32_t baseline, uint32_t trigger_delta);

  ncos::interfaces::sensing::TouchPort* port_ = nullptr;
  ncos::core::contracts::TouchRuntimeState state_ =
      ncos::core::contracts::make_touch_runtime_baseline();
  uint64_t next_probe_ms_ = 0;
};

}  // namespace ncos::services::sensing

#pragma once

#include <stdint.h>

#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

struct PowerRuntimeState {
  bool initialized = false;
  bool sample_ready = false;
  bool sensor_backed = false;
  bool external_power = false;

  bool electrical_guard_active = false;
  bool thermal_guard_active = false;
  bool electrical_guard_latched = false;
  bool thermal_guard_latched = false;

  uint16_t battery_mv = 0;
  uint8_t battery_percent = 0;
  uint8_t thermal_load_percent = 0;
  EnergyMode mode = EnergyMode::kNominal;

  uint64_t last_update_ms = 0;
  uint32_t sample_success_total = 0;
  uint32_t sample_failure_total = 0;
  uint8_t consecutive_sample_failures = 0;
  uint32_t guard_trip_total = 0;
};

constexpr PowerRuntimeState make_power_runtime_baseline() {
  return PowerRuntimeState{};
}

inline uint8_t battery_percent_from_mv(uint16_t mv, uint16_t min_mv, uint16_t max_mv) {
  if (max_mv <= min_mv) {
    return 0;
  }
  if (mv <= min_mv) {
    return 0;
  }
  if (mv >= max_mv) {
    return 100;
  }

  const uint32_t span = static_cast<uint32_t>(max_mv - min_mv);
  const uint32_t value = static_cast<uint32_t>(mv - min_mv);
  return static_cast<uint8_t>((value * 100U) / span);
}

}  // namespace ncos::core::contracts

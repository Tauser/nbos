#pragma once

#include <stdint.h>

#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/power_runtime_contracts.hpp"
#include "interfaces/power/power_port.hpp"

namespace ncos::services::power {

class PowerService final {
 public:
  void bind_port(ncos::interfaces::power::PowerPort* port);
  bool initialize(uint16_t service_id, uint64_t now_ms);

  bool tick(const ncos::core::contracts::CompanionSnapshot& companion,
            uint64_t now_ms,
            ncos::core::contracts::CompanionEnergeticSignal* out_energetic);

  [[nodiscard]] const ncos::core::contracts::PowerRuntimeState& state() const;

 private:
  static ncos::core::contracts::EnergyMode choose_mode(const ncos::interfaces::power::PowerSampleRaw& sample,
                                                       uint8_t battery_percent,
                                                       bool runtime_safe_mode,
                                                       bool electrical_guard,
                                                       bool thermal_guard,
                                                       bool thermal_critical_guard);

  static constexpr uint16_t BatteryMinMv = 3300;
  static constexpr uint16_t BatteryMaxMv = 4200;

  uint16_t service_id_ = 0;
  ncos::interfaces::power::PowerPort* port_ = nullptr;
  ncos::core::contracts::PowerRuntimeState state_ = ncos::core::contracts::make_power_runtime_baseline();
  uint64_t next_probe_ms_ = 0;
};

}  // namespace ncos::services::power

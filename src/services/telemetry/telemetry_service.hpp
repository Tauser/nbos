#pragma once

#include <stdint.h>

#include "config/system_config.hpp"
#include "core/contracts/cloud_bridge_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/telemetry_runtime_contracts.hpp"
#include "interfaces/telemetry/telemetry_port.hpp"

namespace ncos::services::telemetry {

class TelemetryService final {
 public:
  void bind_port(ncos::interfaces::telemetry::TelemetryPort* port);
  bool initialize(uint16_t service_id, uint64_t now_ms, const ncos::config::RuntimeConfig& config);

  bool tick(const ncos::core::contracts::TelemetryRuntimeInput& runtime,
            const ncos::core::contracts::CompanionSnapshot& companion,
            const ncos::core::contracts::CloudBridgeRuntimeState& cloud,
            uint64_t now_ms);

  [[nodiscard]] const ncos::core::contracts::TelemetryRuntimeState& state() const;

 private:
  ncos::core::contracts::TelemetryCollectionSurface surface_from_config() const;
  ncos::core::contracts::TelemetrySample make_sample(
      const ncos::core::contracts::TelemetryRuntimeInput& runtime,
      const ncos::core::contracts::CompanionSnapshot& companion,
      const ncos::core::contracts::CloudBridgeRuntimeState& cloud,
      uint64_t now_ms) const;

  uint16_t service_id_ = 0;
  const ncos::config::RuntimeConfig* config_ = nullptr;
  ncos::interfaces::telemetry::TelemetryPort* port_ = nullptr;
  ncos::core::contracts::TelemetryRuntimeState state_ =
      ncos::core::contracts::make_telemetry_runtime_baseline();
};

}  // namespace ncos::services::telemetry

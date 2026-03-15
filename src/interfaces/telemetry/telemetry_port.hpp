#pragma once

#include "core/contracts/telemetry_runtime_contracts.hpp"

namespace ncos::interfaces::telemetry {

class TelemetryPort {
 public:
  virtual ~TelemetryPort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool publish_sample(const ncos::core::contracts::TelemetrySample& sample) = 0;
};

}  // namespace ncos::interfaces::telemetry

#pragma once

#include "interfaces/telemetry/telemetry_port.hpp"

namespace ncos::drivers::telemetry {

class TelemetryLocalPort final : public ncos::interfaces::telemetry::TelemetryPort {
 public:
  bool ensure_ready() override;
  bool publish_sample(const ncos::core::contracts::TelemetrySample& sample) override;
};

ncos::interfaces::telemetry::TelemetryPort* acquire_shared_telemetry_port();

}  // namespace ncos::drivers::telemetry

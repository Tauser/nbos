#include "drivers/telemetry/telemetry_local_port.hpp"

#include "hal/platform/runtime_build_target.hpp"

namespace ncos::drivers::telemetry {

bool TelemetryLocalPort::ensure_ready() {
  return ncos::hal::platform::running_native_tests();
}

bool TelemetryLocalPort::publish_sample(const ncos::core::contracts::TelemetrySample&) {
  return ncos::hal::platform::running_native_tests();
}

ncos::interfaces::telemetry::TelemetryPort* acquire_shared_telemetry_port() {
  static TelemetryLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::telemetry

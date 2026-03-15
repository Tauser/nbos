#include "drivers/telemetry/telemetry_local_port.hpp"

namespace ncos::drivers::telemetry {

bool TelemetryLocalPort::ensure_ready() {
#ifdef NCOS_NATIVE_TESTS
  return true;
#else
  return false;
#endif
}

bool TelemetryLocalPort::publish_sample(const ncos::core::contracts::TelemetrySample&) {
#ifdef NCOS_NATIVE_TESTS
  return true;
#else
  return false;
#endif
}

ncos::interfaces::telemetry::TelemetryPort* acquire_shared_telemetry_port() {
  static TelemetryLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::telemetry

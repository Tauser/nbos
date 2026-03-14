#include "core/runtime/diagnostics.hpp"

#include "esp_log.h"

namespace {
constexpr const char* kTag = "NCOS_DIAG";

const char* to_fault_code_name(ncos::core::runtime::FaultCode code) {
  switch (code) {
    case ncos::core::runtime::FaultCode::kLifecycleFaulted:
      return "lifecycle_faulted";
    case ncos::core::runtime::FaultCode::kLifecycleWatchdogRegistrationFailed:
      return "watchdog_registration_failed";
    case ncos::core::runtime::FaultCode::kDiagnosticsTaskRegistrationFailed:
      return "diagnostics_registration_failed";
    case ncos::core::runtime::FaultCode::kUnknown:
    default:
      return "unknown";
  }
}
}

namespace ncos::core::runtime {

DiagnosticsSnapshot Diagnostics::snapshot(const HealthMonitor& health, const SafeModeController& safe_mode,
                                          const FaultHistory& fault_history) const {
  DiagnosticsSnapshot out{};
  out.health = health.snapshot();
  out.safe_mode = safe_mode.active();
  out.safe_mode_reason = safe_mode.reason();
  out.has_recent_fault = fault_history.latest(&out.recent_fault);
  return out;
}

void Diagnostics::log_heartbeat(const DiagnosticsSnapshot& snapshot) const {
  if (snapshot.has_recent_fault) {
    ESP_LOGI(kTag,
             "health uptime=%llu tasks=%u faults=%u safe_mode=%d reason=%s last_fault=%s at=%llu msg=%s",
             static_cast<unsigned long long>(snapshot.health.uptime_ms),
             static_cast<unsigned>(snapshot.health.scheduler_tasks),
             static_cast<unsigned>(snapshot.health.fault_count), snapshot.safe_mode ? 1 : 0,
             snapshot.safe_mode_reason, to_fault_code_name(snapshot.recent_fault.code),
             static_cast<unsigned long long>(snapshot.recent_fault.timestamp_ms), snapshot.recent_fault.message);
    return;
  }

  ESP_LOGI(kTag, "health uptime=%llu tasks=%u faults=%u safe_mode=%d reason=%s",
           static_cast<unsigned long long>(snapshot.health.uptime_ms),
           static_cast<unsigned>(snapshot.health.scheduler_tasks),
           static_cast<unsigned>(snapshot.health.fault_count), snapshot.safe_mode ? 1 : 0,
           snapshot.safe_mode_reason);
}

}  // namespace ncos::core::runtime
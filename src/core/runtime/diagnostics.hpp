#pragma once

#include "core/runtime/fault_history.hpp"
#include "core/runtime/health.hpp"
#include "core/runtime/safe_mode.hpp"

namespace ncos::core::runtime {

struct DiagnosticsSnapshot {
  HealthStatus health{};
  bool safe_mode = false;
  const char* safe_mode_reason = "none";
  bool has_recent_fault = false;
  FaultEvent recent_fault{};
};

class Diagnostics final {
 public:
  DiagnosticsSnapshot snapshot(const HealthMonitor& health, const SafeModeController& safe_mode,
                               const FaultHistory& fault_history) const;
  void log_heartbeat(const DiagnosticsSnapshot& snapshot) const;
};

}  // namespace ncos::core::runtime
#pragma once

#include <stdint.h>

#include "app/lifecycle/system_lifecycle.hpp"
#include "config/system_config.hpp"
#include "core/runtime/diagnostics.hpp"
#include "core/runtime/fault_history.hpp"
#include "core/runtime/health.hpp"
#include "core/runtime/safe_mode.hpp"
#include "core/runtime/scheduler_base.hpp"

namespace ncos::core::runtime {

class SystemManager final {
 public:
  SystemManager() = default;

  bool initialize(const ncos::app::lifecycle::SystemLifecycle* lifecycle,
                  const ncos::config::GlobalConfig* config);
  void start(uint64_t now_ms);
  void tick(uint64_t now_ms);

 private:
  static void lifecycle_watchdog_task(void* context);
  static void diagnostics_heartbeat_task(void* context);

  void handle_lifecycle_fault();
  void refresh_health(uint64_t now_ms);

  const ncos::app::lifecycle::SystemLifecycle* lifecycle_ = nullptr;
  const ncos::config::GlobalConfig* config_ = nullptr;
  SchedulerBase scheduler_{};
  HealthMonitor health_{};
  FaultHistory fault_history_{};
  SafeModeController safe_mode_{};
  Diagnostics diagnostics_{};
  uint64_t last_tick_ms_ = 0;
  bool lifecycle_fault_latched_ = false;
  bool initialized_ = false;
  bool started_ = false;
};

}  // namespace ncos::core::runtime

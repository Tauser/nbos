#pragma once

#include <stdint.h>

#include "app/lifecycle/system_lifecycle.hpp"
#include "config/system_config.hpp"
#include "core/messaging/event_bus_v2.hpp"
#include "core/runtime/diagnostics.hpp"
#include "core/runtime/fault_history.hpp"
#include "core/runtime/health.hpp"
#include "core/runtime/safe_mode.hpp"
#include "core/runtime/scheduler_base.hpp"

namespace ncos::core::runtime {

struct RuntimeStatus {
  bool initialized = false;
  bool started = false;
  uint32_t scheduler_tasks = 0;
  bool safe_mode = false;
  uint32_t fault_count = 0;
  uint64_t last_tick_ms = 0;
  uint32_t bus_published_total = 0;
  uint32_t bus_dispatched_total = 0;
  uint32_t bus_dropped_total = 0;
};

class SystemManager final {
 public:
  SystemManager() = default;

  bool initialize(const ncos::app::lifecycle::SystemLifecycle* lifecycle,
                  const ncos::config::GlobalConfig* config);
  void start(uint64_t now_ms);
  void tick(uint64_t now_ms);
  RuntimeStatus status() const;

 private:
  static void lifecycle_watchdog_task(void* context);
  static void diagnostics_heartbeat_task(void* context);

  void handle_lifecycle_fault();
  void refresh_health(uint64_t now_ms);

  const ncos::app::lifecycle::SystemLifecycle* lifecycle_ = nullptr;
  const ncos::config::GlobalConfig* config_ = nullptr;
  SchedulerBase scheduler_{};
  ncos::core::messaging::EventBusV2 event_bus_{};
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

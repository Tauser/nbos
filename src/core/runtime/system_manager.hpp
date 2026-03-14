#pragma once

#include <stdint.h>

#include "app/lifecycle/system_lifecycle.hpp"
#include "core/runtime/scheduler_base.hpp"

namespace ncos::core::runtime {

class SystemManager final {
 public:
  SystemManager() = default;

  bool initialize(const ncos::app::lifecycle::SystemLifecycle* lifecycle);
  void start(uint64_t now_ms);
  void tick(uint64_t now_ms);

 private:
  static void lifecycle_watchdog_task(void* context);

  const ncos::app::lifecycle::SystemLifecycle* lifecycle_ = nullptr;
  SchedulerBase scheduler_{};
  bool initialized_ = false;
  bool started_ = false;
};

}  // namespace ncos::core::runtime

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::core::runtime {

using TaskCallback = void (*)(void* context);

struct SchedulerTask {
  const char* name = nullptr;
  uint32_t period_ms = 0;
  TaskCallback callback = nullptr;
  void* context = nullptr;
  uint64_t next_run_ms = 0;
  bool enabled = false;
};

class SchedulerBase final {
 public:
  static constexpr size_t kMaxTasks = 8;

  bool register_task(const char* name, uint32_t period_ms, TaskCallback callback, void* context,
                     uint64_t start_at_ms);
  void tick(uint64_t now_ms);
  size_t task_count() const;

 private:
  SchedulerTask tasks_[kMaxTasks] = {};
  size_t used_ = 0;
};

}  // namespace ncos::core::runtime

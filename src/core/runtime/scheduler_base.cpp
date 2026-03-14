#include "core/runtime/scheduler_base.hpp"

namespace ncos::core::runtime {

bool SchedulerBase::register_task(const char* name, uint32_t period_ms, TaskCallback callback,
                                  void* context, uint64_t start_at_ms) {
  if (used_ >= kMaxTasks || period_ms == 0 || callback == nullptr) {
    return false;
  }

  SchedulerTask task{};
  task.name = name;
  task.period_ms = period_ms;
  task.callback = callback;
  task.context = context;
  task.next_run_ms = start_at_ms;
  task.enabled = true;

  tasks_[used_] = task;
  ++used_;
  return true;
}

void SchedulerBase::tick(uint64_t now_ms) {
  for (size_t i = 0; i < used_; ++i) {
    SchedulerTask& task = tasks_[i];
    if (!task.enabled) {
      continue;
    }

    if (now_ms < task.next_run_ms) {
      continue;
    }

    task.callback(task.context);
    task.next_run_ms = now_ms + task.period_ms;
  }
}

size_t SchedulerBase::task_count() const {
  return used_;
}

}  // namespace ncos::core::runtime

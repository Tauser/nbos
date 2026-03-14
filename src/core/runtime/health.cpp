#include "core/runtime/health.hpp"

namespace ncos::core::runtime {

void HealthMonitor::reset(uint64_t start_ms) {
  start_ms_ = start_ms;
  current_ = HealthStatus{};
}

void HealthMonitor::update(uint64_t now_ms, uint32_t scheduler_tasks, uint32_t fault_count, bool safe_mode) {
  current_.uptime_ms = now_ms >= start_ms_ ? (now_ms - start_ms_) : 0;
  current_.scheduler_tasks = scheduler_tasks;
  current_.fault_count = fault_count;
  current_.safe_mode = safe_mode;
}

HealthStatus HealthMonitor::snapshot() const {
  return current_;
}

}  // namespace ncos::core::runtime

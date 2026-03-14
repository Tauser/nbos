#pragma once

#include <stdint.h>

namespace ncos::core::runtime {

struct HealthStatus {
  uint64_t uptime_ms = 0;
  uint32_t scheduler_tasks = 0;
  uint32_t fault_count = 0;
  bool safe_mode = false;
};

class HealthMonitor final {
 public:
  void reset(uint64_t start_ms);
  void update(uint64_t now_ms, uint32_t scheduler_tasks, uint32_t fault_count, bool safe_mode);
  HealthStatus snapshot() const;

 private:
  uint64_t start_ms_ = 0;
  HealthStatus current_{};
};

}  // namespace ncos::core::runtime

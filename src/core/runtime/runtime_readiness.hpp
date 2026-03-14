#pragma once

#include "app/lifecycle/system_lifecycle.hpp"
#include "config/system_config.hpp"
#include "core/runtime/system_manager.hpp"

namespace ncos::core::runtime {

enum class RuntimeReadinessLevel {
  kReady,
  kConditionallyReady,
  kNotReady,
};

struct RuntimeReadinessReport {
  bool config_valid = false;
  bool board_profile_bound = false;
  bool lifecycle_allows_runtime = false;
  bool runtime_initialized = false;
  bool runtime_started = false;
  bool scheduler_has_minimum_tasks = false;
  bool safe_mode_inactive = false;
  bool no_faults_recorded = false;
  RuntimeReadinessLevel level = RuntimeReadinessLevel::kNotReady;

  const char* level_name() const;
};

RuntimeReadinessReport evaluate_runtime_readiness(const ncos::config::GlobalConfig& config,
                                                  const ncos::app::lifecycle::SystemLifecycle& lifecycle,
                                                  const RuntimeStatus& runtime_status);

}  // namespace ncos::core::runtime

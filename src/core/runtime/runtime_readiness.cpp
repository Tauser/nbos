#include "core/runtime/runtime_readiness.hpp"

namespace {

bool is_board_profile_bound(const ncos::config::BoardProfile& board) {
  return board.board_name != nullptr && board.board_name[0] != '\0' && board.display_mosi >= 0 &&
         board.display_sck >= 0 && board.display_dc >= 0 && board.display_rst >= 0 && board.touch >= 0;
}

bool is_lifecycle_state_acceptable(ncos::app::lifecycle::SystemState state) {
  return state == ncos::app::lifecycle::SystemState::kRunning ||
         state == ncos::app::lifecycle::SystemState::kDegraded;
}

bool is_governance_stable(const ncos::core::runtime::RuntimeStatus& status) {
  const uint32_t accepted = status.governance_allowed_total + status.governance_preempted_total;
  return !(status.governance_rejected_total >= 3 && accepted == 0);
}

}  // namespace

namespace ncos::core::runtime {

const char* RuntimeReadinessReport::level_name() const {
  switch (level) {
    case RuntimeReadinessLevel::kReady:
      return "ready";
    case RuntimeReadinessLevel::kConditionallyReady:
      return "conditionally_ready";
    case RuntimeReadinessLevel::kNotReady:
    default:
      return "not_ready";
  }
}

RuntimeReadinessReport evaluate_runtime_readiness(const ncos::config::GlobalConfig& config,
                                                  const ncos::app::lifecycle::SystemLifecycle& lifecycle,
                                                  const RuntimeStatus& runtime_status) {
  RuntimeReadinessReport report{};
  report.config_valid = config.config_ready;
  report.board_profile_bound = is_board_profile_bound(config.board);
  report.lifecycle_allows_runtime = is_lifecycle_state_acceptable(lifecycle.state());
  report.runtime_initialized = runtime_status.initialized;
  report.runtime_started = runtime_status.started;
  report.scheduler_has_minimum_tasks = runtime_status.scheduler_tasks >= 1;
  report.safe_mode_inactive = !runtime_status.safe_mode;
  report.no_faults_recorded = runtime_status.fault_count == 0;
  report.governance_stable = is_governance_stable(runtime_status);

  const bool core_gate = report.config_valid && report.board_profile_bound && report.lifecycle_allows_runtime &&
                         report.runtime_initialized && report.runtime_started &&
                         report.scheduler_has_minimum_tasks;

  if (!core_gate) {
    report.level = RuntimeReadinessLevel::kNotReady;
    return report;
  }

  if (report.safe_mode_inactive && report.no_faults_recorded && report.governance_stable) {
    report.level = RuntimeReadinessLevel::kReady;
    return report;
  }

  report.level = RuntimeReadinessLevel::kConditionallyReady;
  return report;
}

}  // namespace ncos::core::runtime

#include "services/motion/motion_service.hpp"

#ifndef NCOS_NATIVE_TESTS
#include "esp_log.h"
#else
#define ESP_LOGW(tag, fmt, ...) ((void)0)
#endif

namespace {
constexpr const char* kTag = "NCOS_MOTION_SVC";
}

namespace ncos::services::motion {

void MotionService::bind_port(ncos::interfaces::motion::MotionPort* port) {
  port_ = port;
}

bool MotionService::initialize(uint64_t now_ms) {
  state_ = ncos::core::contracts::make_motion_runtime_baseline();
  state_.last_update_ms = now_ms;

  if (port_ == nullptr) {
    ESP_LOGW(kTag, "Porta de motion nao conectada");
    return false;
  }

  state_.console_pin_conflict = port_->has_console_pin_conflict();
  if (state_.console_pin_conflict) {
    ESP_LOGW(kTag, "Motion bloqueado por conflito com UART de console");
    return false;
  }

  state_.initialized = port_->ensure_ready();
  if (!state_.initialized) {
    return false;
  }

  return apply_neutral_pose(now_ms);
}

bool MotionService::apply_neutral_pose(uint64_t now_ms) {
  return request_motion(ncos::core::contracts::make_neutral_hold_command(), now_ms);
}

bool MotionService::recover_to_neutral(uint64_t now_ms) {
  return request_motion(ncos::core::contracts::make_recovery_command(), now_ms);
}

bool MotionService::apply_pose(const ncos::core::contracts::MotionPoseCommand& pose, uint64_t now_ms) {
  ncos::core::contracts::MotionCommand command{};
  command.intent = ncos::core::contracts::MotionIntent::kDirectPose;
  command.origin = ncos::core::contracts::MotionCommandOrigin::kRuntime;
  command.priority = ncos::core::contracts::MotionPriority::kNormal;
  command.pose = pose;
  command.hold_ms = 120;
  return request_motion(command, now_ms);
}

bool MotionService::request_motion(const ncos::core::contracts::MotionCommand& command,
                                   uint64_t now_ms) {
  if (!state_.initialized || port_ == nullptr || !ncos::core::contracts::is_motion_command_valid(command) ||
      !ncos::core::contracts::are_safety_limits_valid(state_.safety_limits)) {
    state_.last_apply_ok = false;
    ++state_.apply_failure_total;
    ++state_.plan_rejected_total;
    state_.last_update_ms = now_ms;
    return false;
  }

  const ncos::core::contracts::MotionCommand safe_command = sanitize_command(command, now_ms);

  if (state_.has_active_plan &&
      !ncos::core::contracts::should_preempt_plan(state_.active_plan, safe_command, now_ms)) {
    state_.rejected_for_priority = true;
    ++state_.plan_rejected_total;
    state_.last_update_ms = now_ms;
    return false;
  }

  state_.rejected_for_priority = false;
  const ncos::core::contracts::MotionExecutionPlan plan =
      ncos::core::contracts::make_execution_plan(safe_command, now_ms);
  return apply_plan(plan, now_ms);
}

void MotionService::update_companion_signal(const ncos::core::contracts::MotionCompanionSignal& signal,
                                            uint64_t now_ms) {
  state_.companion_signal = signal;
  state_.last_update_ms = now_ms;
}

void MotionService::update_face_signal(const ncos::core::contracts::MotionFaceSignal& signal,
                                       uint64_t now_ms) {
  state_.face_signal = signal;
  state_.last_update_ms = now_ms;
}

void MotionService::tick(uint64_t now_ms) {
  if (state_.has_active_plan && now_ms >= state_.active_plan.hold_until_ms) {
    state_.has_active_plan = false;
  }
}

const ncos::core::contracts::MotionRuntimeState& MotionService::state() const {
  return state_;
}

ncos::core::contracts::MotionCommand MotionService::sanitize_command(
    const ncos::core::contracts::MotionCommand& command, uint64_t now_ms) {
  ncos::core::contracts::MotionCommand safe = command;

  if (state_.companion_signal.safe_mode &&
      safe.priority != ncos::core::contracts::MotionPriority::kCritical) {
    safe.pose.speed_percent =
        safe.pose.speed_percent > 45 ? 45 : safe.pose.speed_percent;
    safe.hold_ms = safe.hold_ms > 180 ? 180 : safe.hold_ms;
  }

  if (safe.intent == ncos::core::contracts::MotionIntent::kRecovery) {
    safe.pose = ncos::core::contracts::make_neutral_pose();
    safe.pose.speed_percent = state_.safety_limits.recovery_speed_percent;
  }

  ncos::core::contracts::MotionSafetyReport report{};
  safe.pose =
      ncos::core::contracts::clamp_pose_to_safety(safe.pose, state_.safety_limits, &report);

  state_.safety_clamp_applied = report.clamped_yaw || report.clamped_pitch || report.clamped_speed;
  if (state_.safety_clamp_applied) {
    ++state_.safety_clamp_total;
  }

  state_.last_update_ms = now_ms;
  return safe;
}

bool MotionService::apply_plan(const ncos::core::contracts::MotionExecutionPlan& plan, uint64_t now_ms) {
  size_t tx_bytes = 0;
  const bool ok = port_->apply_pose(plan.target_pose, &tx_bytes);

  state_.last_apply_ok = ok;
  state_.last_tx_bytes = tx_bytes;
  state_.last_pose = plan.target_pose;
  state_.last_update_ms = now_ms;

  if (!ok) {
    ++state_.apply_failure_total;
    ++state_.plan_rejected_total;
    return false;
  }

  state_.active_plan = plan;
  state_.has_active_plan = true;
  state_.neutral_applied =
      (plan.target_pose.yaw_permille == 0 && plan.target_pose.pitch_permille == 0);
  ++state_.apply_success_total;
  ++state_.plan_applied_total;
  return true;
}

}  // namespace ncos::services::motion

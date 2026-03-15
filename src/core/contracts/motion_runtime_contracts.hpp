#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::core::contracts {

enum class MotionIntent : uint8_t {
  kDirectPose = 1,
  kNeutralHold = 2,
  kAttendUser = 3,
  kObserveStimulus = 4,
  kAcknowledgeTouch = 5,
  kPowerSave = 6,
};

enum class MotionCommandOrigin : uint8_t {
  kRuntime = 1,
  kFace = 2,
  kCompanionState = 3,
  kSafety = 4,
  kExternal = 5,
};

enum class MotionPriority : uint8_t {
  kLow = 1,
  kNormal = 2,
  kHigh = 3,
  kCritical = 4,
};

struct MotionPoseCommand {
  int16_t yaw_permille = 0;    // -1000..1000
  int16_t pitch_permille = 0;  // -1000..1000
  uint16_t speed_percent = 40;
};

struct MotionCommand {
  MotionIntent intent = MotionIntent::kDirectPose;
  MotionCommandOrigin origin = MotionCommandOrigin::kRuntime;
  MotionPriority priority = MotionPriority::kNormal;
  MotionPoseCommand pose{};
  uint32_t trace_id = 0;
  uint32_t hold_ms = 0;
};

struct MotionCompanionSignal {
  bool safe_mode = false;
  bool attention_lock = false;
  uint8_t emotional_arousal_percent = 0;
};

struct MotionFaceSignal {
  int8_t gaze_x_percent = 0;
  int8_t gaze_y_percent = 0;
  bool clip_active = false;
};

struct MotionExecutionPlan {
  MotionIntent intent = MotionIntent::kDirectPose;
  MotionCommandOrigin origin = MotionCommandOrigin::kRuntime;
  MotionPriority priority = MotionPriority::kNormal;
  MotionPoseCommand target_pose{};
  uint32_t trace_id = 0;
  uint64_t hold_until_ms = 0;
};

struct MotionRuntimeState {
  bool initialized = false;
  bool console_pin_conflict = false;
  bool neutral_applied = false;
  bool last_apply_ok = false;
  bool has_active_plan = false;
  bool rejected_for_priority = false;
  MotionPoseCommand last_pose{};
  MotionExecutionPlan active_plan{};
  MotionCompanionSignal companion_signal{};
  MotionFaceSignal face_signal{};
  size_t last_tx_bytes = 0;
  uint64_t last_update_ms = 0;
  uint32_t apply_success_total = 0;
  uint32_t apply_failure_total = 0;
  uint32_t plan_applied_total = 0;
  uint32_t plan_rejected_total = 0;
};

constexpr MotionPoseCommand make_neutral_pose() {
  return MotionPoseCommand{};
}

constexpr MotionCommand make_neutral_hold_command() {
  MotionCommand cmd{};
  cmd.intent = MotionIntent::kNeutralHold;
  cmd.priority = MotionPriority::kNormal;
  cmd.pose = make_neutral_pose();
  cmd.hold_ms = 250;
  return cmd;
}

constexpr MotionRuntimeState make_motion_runtime_baseline() {
  return MotionRuntimeState{};
}

constexpr bool is_pose_valid(const MotionPoseCommand& pose) {
  return pose.yaw_permille >= -1000 && pose.yaw_permille <= 1000 &&
         pose.pitch_permille >= -1000 && pose.pitch_permille <= 1000 && pose.speed_percent <= 100;
}

constexpr bool is_motion_command_valid(const MotionCommand& cmd) {
  return is_pose_valid(cmd.pose);
}

constexpr bool should_preempt_plan(const MotionExecutionPlan& active, const MotionCommand& incoming,
                                   uint64_t now_ms) {
  if (now_ms >= active.hold_until_ms) {
    return true;
  }

  return static_cast<uint8_t>(incoming.priority) > static_cast<uint8_t>(active.priority);
}

constexpr MotionExecutionPlan make_execution_plan(const MotionCommand& cmd, uint64_t now_ms) {
  MotionExecutionPlan plan{};
  plan.intent = cmd.intent;
  plan.origin = cmd.origin;
  plan.priority = cmd.priority;
  plan.target_pose = cmd.pose;
  plan.trace_id = cmd.trace_id;
  plan.hold_until_ms = now_ms + cmd.hold_ms;
  return plan;
}

}  // namespace ncos::core::contracts

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::core::contracts {

struct MotionPoseCommand {
  int16_t yaw_permille = 0;    // -1000..1000
  int16_t pitch_permille = 0;  // -1000..1000
  uint16_t speed_percent = 40;
};

struct MotionRuntimeState {
  bool initialized = false;
  bool console_pin_conflict = false;
  bool neutral_applied = false;
  bool last_apply_ok = false;
  MotionPoseCommand last_pose{};
  size_t last_tx_bytes = 0;
  uint64_t last_update_ms = 0;
  uint32_t apply_success_total = 0;
  uint32_t apply_failure_total = 0;
};

constexpr MotionPoseCommand make_neutral_pose() {
  return MotionPoseCommand{};
}

constexpr MotionRuntimeState make_motion_runtime_baseline() {
  return MotionRuntimeState{};
}

constexpr bool is_pose_valid(const MotionPoseCommand& pose) {
  return pose.yaw_permille >= -1000 && pose.yaw_permille <= 1000 &&
         pose.pitch_permille >= -1000 && pose.pitch_permille <= 1000 && pose.speed_percent <= 100;
}

}  // namespace ncos::core::contracts

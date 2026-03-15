#pragma once

#include <stdint.h>

#include "core/contracts/motion_runtime_contracts.hpp"
#include "interfaces/motion/motion_port.hpp"

namespace ncos::services::motion {

class MotionService final {
 public:
  void bind_port(ncos::interfaces::motion::MotionPort* port);
  bool initialize(uint64_t now_ms);

  bool apply_neutral_pose(uint64_t now_ms);
  bool recover_to_neutral(uint64_t now_ms);
  bool apply_pose(const ncos::core::contracts::MotionPoseCommand& pose, uint64_t now_ms);
  bool request_motion(const ncos::core::contracts::MotionCommand& command, uint64_t now_ms);

  void update_companion_signal(const ncos::core::contracts::MotionCompanionSignal& signal,
                               uint64_t now_ms);
  void update_face_signal(const ncos::core::contracts::MotionFaceSignal& signal, uint64_t now_ms);

  void tick(uint64_t now_ms);
  [[nodiscard]] const ncos::core::contracts::MotionRuntimeState& state() const;

 private:
  ncos::core::contracts::MotionCommand sanitize_command(const ncos::core::contracts::MotionCommand& command,
                                                        uint64_t now_ms);
  bool apply_plan(const ncos::core::contracts::MotionExecutionPlan& plan, uint64_t now_ms);

  ncos::interfaces::motion::MotionPort* port_ = nullptr;
  ncos::core::contracts::MotionRuntimeState state_ =
      ncos::core::contracts::make_motion_runtime_baseline();
  uint64_t next_embodiment_ms_ = 0;
};

}  // namespace ncos::services::motion

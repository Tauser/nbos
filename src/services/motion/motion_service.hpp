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
  bool apply_pose(const ncos::core::contracts::MotionPoseCommand& pose, uint64_t now_ms);
  void tick(uint64_t now_ms);
  [[nodiscard]] const ncos::core::contracts::MotionRuntimeState& state() const;

 private:
  ncos::interfaces::motion::MotionPort* port_ = nullptr;
  ncos::core::contracts::MotionRuntimeState state_ =
      ncos::core::contracts::make_motion_runtime_baseline();
};

}  // namespace ncos::services::motion

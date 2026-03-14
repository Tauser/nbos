#pragma once

#include <stddef.h>

#include "core/contracts/motion_runtime_contracts.hpp"

namespace ncos::interfaces::motion {

class MotionPort {
 public:
  virtual ~MotionPort() = default;

  virtual bool has_console_pin_conflict() const = 0;
  virtual bool ensure_ready() = 0;
  virtual bool apply_pose(const ncos::core::contracts::MotionPoseCommand& pose, size_t* out_tx_bytes) = 0;
};

}  // namespace ncos::interfaces::motion

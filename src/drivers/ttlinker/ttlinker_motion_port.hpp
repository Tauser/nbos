#pragma once

#include "drivers/ttlinker/ttlinker_bringup.hpp"
#include "interfaces/motion/motion_port.hpp"

namespace ncos::drivers::ttlinker {

class TtlinkerMotionPort final : public ncos::interfaces::motion::MotionPort {
 public:
  bool has_transport_conflict() const override;
  bool ensure_ready() override;
  bool apply_pose(const ncos::core::contracts::MotionPoseCommand& pose, size_t* out_tx_bytes) override;

 private:
  static uint8_t map_permille_to_servo_byte(int16_t value);
  TtlinkerBringup bringup_{};
};

ncos::interfaces::motion::MotionPort* acquire_shared_motion_port();

}  // namespace ncos::drivers::ttlinker


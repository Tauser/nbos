#include "drivers/ttlinker/ttlinker_motion_port.hpp"

#include <limits.h>

namespace ncos::drivers::ttlinker {

bool TtlinkerMotionPort::has_console_pin_conflict() const {
  return !bringup_.can_run_probe_with_console();
}

bool TtlinkerMotionPort::ensure_ready() {
  return bringup_.init();
}

bool TtlinkerMotionPort::apply_pose(const ncos::core::contracts::MotionPoseCommand& pose,
                                    size_t* out_tx_bytes) {
  if (!ncos::core::contracts::is_pose_valid(pose) || !ensure_ready()) {
    return false;
  }

  const uint8_t yaw = map_permille_to_servo_byte(pose.yaw_permille);
  const uint8_t pitch = map_permille_to_servo_byte(pose.pitch_permille);
  const uint8_t speed = static_cast<uint8_t>(pose.speed_percent > 100 ? 100 : pose.speed_percent);

  // Protocolo inicial de bridge: frame simples para trilho de motion.
  const uint8_t frame[] = {0xFA, 0xAF, 0x01, yaw, 0x02, pitch, 0x03, speed, 0x0D};
  return bringup_.send_frame(frame, sizeof(frame), out_tx_bytes);
}

uint8_t TtlinkerMotionPort::map_permille_to_servo_byte(int16_t value) {
  int32_t clamped = value;
  if (clamped > 1000) {
    clamped = 1000;
  }
  if (clamped < -1000) {
    clamped = -1000;
  }

  const int32_t mapped = ((clamped + 1000) * 255) / 2000;
  if (mapped < 0) {
    return 0;
  }
  if (mapped > UCHAR_MAX) {
    return UCHAR_MAX;
  }
  return static_cast<uint8_t>(mapped);
}

ncos::interfaces::motion::MotionPort* acquire_shared_motion_port() {
  static TtlinkerMotionPort shared{};
  return &shared;
}

}  // namespace ncos::drivers::ttlinker

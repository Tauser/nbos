#pragma once

#include <stdint.h>

namespace ncos::core::contracts {

struct CameraRuntimeState {
  bool initialized = false;
  bool component_available = false;
  bool capture_ready = false;
  bool last_capture_ok = false;
  uint16_t last_frame_width = 0;
  uint16_t last_frame_height = 0;
  uint32_t last_frame_bytes = 0;
  uint8_t last_motion_delta_percent = 0;
  uint64_t last_frame_sequence = 0;
  uint64_t last_capture_ms = 0;
  uint32_t capture_success_total = 0;
  uint32_t capture_failure_total = 0;
};

constexpr CameraRuntimeState make_camera_runtime_baseline() {
  return CameraRuntimeState{};
}

}  // namespace ncos::core::contracts

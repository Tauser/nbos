#pragma once

#include <stdint.h>

namespace ncos::drivers::camera {

struct CameraProbeResult {
  bool camera_component_available = false;
  bool init_ok = false;
  bool frame_ok = false;
  int frame_len = 0;
  int frame_width = 0;
  int frame_height = 0;
  uint8_t motion_delta_percent = 0;
};

class CameraBringup final {
 public:
  bool initialize();
  bool capture_frame(CameraProbeResult* out_result);
  void deinit();
  bool is_initialized() const { return initialized_; }

 private:
  bool initialized_ = false;
  uint8_t prev_frame_downsampled_[768]{};
  bool has_prev_frame_ = false;
};

}  // namespace ncos::drivers::camera

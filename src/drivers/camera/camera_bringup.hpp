#pragma once

namespace ncos::drivers::camera {

struct CameraProbeResult {
  bool camera_component_available = false;
  bool init_ok = false;
  bool frame_ok = false;
  int frame_len = 0;
  int frame_width = 0;
  int frame_height = 0;
};

class CameraBringup final {
 public:
  bool run_probe(CameraProbeResult* out_result);
};

}  // namespace ncos::drivers::camera

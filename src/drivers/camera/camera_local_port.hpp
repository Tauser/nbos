#pragma once

#include "drivers/camera/camera_bringup.hpp"
#include "interfaces/vision/camera_port.hpp"

namespace ncos::drivers::camera {

class CameraLocalPort final : public ncos::interfaces::vision::CameraPort {
 public:
  bool ensure_ready() override;
  bool is_component_available() const override;
  bool capture_frame(ncos::interfaces::vision::CameraFrameMeta* out_frame) override;

 private:
  CameraBringup bringup_{};
  CameraProbeResult last_probe_{};
};

ncos::interfaces::vision::CameraPort* acquire_shared_camera_port();

}  // namespace ncos::drivers::camera

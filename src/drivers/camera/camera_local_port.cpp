#include "drivers/camera/camera_local_port.hpp"

namespace ncos::drivers::camera {

bool CameraLocalPort::ensure_ready() {
  if (!bringup_.is_initialized()) {
    return bringup_.initialize();
  }
  return true;
}

bool CameraLocalPort::is_component_available() const {
  return true;
}

bool CameraLocalPort::capture_frame(ncos::interfaces::vision::CameraFrameMeta* out_frame) {
  if (out_frame == nullptr) {
    return false;
  }

  CameraProbeResult probe{};
  if (!bringup_.capture_frame(&probe)) {
    return false;
  }

  last_probe_ = probe;

  out_frame->width = static_cast<uint16_t>(probe.frame_width);
  out_frame->height = static_cast<uint16_t>(probe.frame_height);
  out_frame->payload_bytes = static_cast<uint32_t>(probe.frame_len);
  out_frame->motion_delta_percent = probe.motion_delta_percent;

  return probe.camera_component_available && probe.init_ok && probe.frame_ok;
}

ncos::interfaces::vision::CameraPort* acquire_shared_camera_port() {
  static CameraLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::camera

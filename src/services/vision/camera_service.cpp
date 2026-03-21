#include "services/vision/camera_service.hpp"

#include "config/system_config.hpp"
#include "hal/platform/runtime_logging.hpp"

namespace {
constexpr const char* Tag = "NCOS_CAM_SVC";
}

namespace ncos::services::vision {

void CameraService::bind_port(ncos::interfaces::vision::CameraPort* port) {
  port_ = port;
}

bool CameraService::initialize(uint64_t now_ms) {
  if (port_ == nullptr) {
    NCOS_LOGW(Tag, "Porta de camera nao conectada");
    return false;
  }

  state_.component_available = port_->is_component_available();
  state_.capture_ready = port_->ensure_ready();
  state_.component_available = state_.component_available || port_->is_component_available();
  state_.initialized = true;
  state_.last_capture_ok = false;
  state_.last_capture_ms = now_ms;
  state_.last_frame_sequence = 0;
  state_.capture_success_total = 0;
  state_.capture_failure_total = 0;
  next_capture_ms_ = now_ms;

  return state_.capture_ready;
}

void CameraService::tick(uint64_t now_ms) {
  if (!state_.initialized || !state_.capture_ready || port_ == nullptr) {
    return;
  }

  if (now_ms < next_capture_ms_) {
    return;
  }

  ncos::interfaces::vision::CameraFrameMeta frame{};
  const bool ok = port_->capture_frame(&frame);
  state_.last_capture_ok = ok;
  state_.last_capture_ms = now_ms;

  if (ok) {
    state_.last_frame_width = frame.width;
    state_.last_frame_height = frame.height;
    state_.last_frame_bytes = frame.payload_bytes;
    state_.last_motion_delta_percent = frame.motion_delta_percent;
    ++state_.last_frame_sequence;
    ++state_.capture_success_total;
  } else {
    ++state_.capture_failure_total;
  }

  state_.component_available = port_->is_component_available();
  next_capture_ms_ = now_ms + ncos::config::kGlobalConfig.runtime.camera_probe_interval_ms;
}

const ncos::core::contracts::CameraRuntimeState& CameraService::state() const {
  return state_;
}

}  // namespace ncos::services::vision

#include "services/motion/motion_service.hpp"

#ifndef NCOS_NATIVE_TESTS
#include "esp_log.h"
#else
#define ESP_LOGW(tag, fmt, ...) ((void)0)
#endif

namespace {
constexpr const char* kTag = "NCOS_MOTION_SVC";
}

namespace ncos::services::motion {

void MotionService::bind_port(ncos::interfaces::motion::MotionPort* port) {
  port_ = port;
}

bool MotionService::initialize(uint64_t now_ms) {
  state_ = ncos::core::contracts::make_motion_runtime_baseline();
  state_.last_update_ms = now_ms;

  if (port_ == nullptr) {
    ESP_LOGW(kTag, "Porta de motion nao conectada");
    return false;
  }

  state_.console_pin_conflict = port_->has_console_pin_conflict();
  if (state_.console_pin_conflict) {
    ESP_LOGW(kTag, "Motion bloqueado por conflito com UART de console");
    return false;
  }

  state_.initialized = port_->ensure_ready();
  if (!state_.initialized) {
    return false;
  }

  return apply_neutral_pose(now_ms);
}

bool MotionService::apply_neutral_pose(uint64_t now_ms) {
  return apply_pose(ncos::core::contracts::make_neutral_pose(), now_ms);
}

bool MotionService::apply_pose(const ncos::core::contracts::MotionPoseCommand& pose, uint64_t now_ms) {
  if (!state_.initialized || port_ == nullptr || !ncos::core::contracts::is_pose_valid(pose)) {
    state_.last_apply_ok = false;
    ++state_.apply_failure_total;
    state_.last_update_ms = now_ms;
    return false;
  }

  size_t tx_bytes = 0;
  const bool ok = port_->apply_pose(pose, &tx_bytes);
  state_.last_apply_ok = ok;
  state_.last_tx_bytes = tx_bytes;
  state_.last_pose = pose;
  state_.last_update_ms = now_ms;

  if (ok) {
    state_.neutral_applied = (pose.yaw_permille == 0 && pose.pitch_permille == 0);
    ++state_.apply_success_total;
  } else {
    ++state_.apply_failure_total;
  }

  return ok;
}

void MotionService::tick(uint64_t now_ms) {
  (void)now_ms;
}

const ncos::core::contracts::MotionRuntimeState& MotionService::state() const {
  return state_;
}

}  // namespace ncos::services::motion

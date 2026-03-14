#include "services/face/face_gaze_controller.hpp"

namespace {

uint8_t clamp_percent(uint8_t value) {
  return value > 100 ? 100 : value;
}

}  // namespace

namespace ncos::services::face {

FaceGazeController::FaceGazeController(uint16_t owner_service) : owner_service_(owner_service) {}

void FaceGazeController::bind_owner(uint16_t owner_service) {
  owner_service_ = owner_service;
}

bool FaceGazeController::set_target(const ncos::models::face::FaceGazeTarget& target, uint64_t now_ms) {
  if (target.hold_ms == 0) {
    return false;
  }

  target_ = target;
  target_.focus_percent = clamp_percent(target.focus_percent);
  target_.salience_percent = clamp_percent(target.salience_percent);
  expires_at_ms_ = now_ms + target.hold_ms;
  active_target_ = true;
  return true;
}

void FaceGazeController::clear_target() {
  active_target_ = false;
  expires_at_ms_ = 0;
}

bool FaceGazeController::owner_can_write(const ncos::core::contracts::FaceRenderState& state) const {
  if (owner_service_ == 0) {
    return false;
  }

  const size_t idx =
      ncos::models::face::face_layer_index(ncos::models::face::FaceLayer::kGaze);
  const auto& gaze_layer = state.composition.layers[idx];

  return gaze_layer.owner_role == ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner &&
         gaze_layer.owner_service == owner_service_;
}

bool FaceGazeController::tick(uint64_t now_ms, ncos::core::contracts::FaceRenderState* state) {
  if (state == nullptr || !ncos::core::contracts::is_valid(*state) || !owner_can_write(*state) ||
      !active_target_) {
    return false;
  }

  if (now_ms > expires_at_ms_) {
    active_target_ = false;
    return false;
  }

  state->eyes.anchor = target_.anchor;
  state->eyes.direction = target_.direction;
  state->eyes.focus_percent = target_.focus_percent;
  state->updated_at_ms = now_ms;
  state->owner_service = owner_service_;
  ++state->revision;
  return true;
}

}  // namespace ncos::services::face

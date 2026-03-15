#include "services/face/face_graphics_pipeline.hpp"

#include "drivers/display/display_runtime.hpp"

namespace ncos::services::face {

bool FaceGraphicsPipeline::initialize(uint64_t now_ms) {
  if (initialized_) {
    return true;
  }

  auto* display = ncos::drivers::display::acquire_shared_display();
  if (display == nullptr || !renderer_.bind(display)) {
    return false;
  }

  state_ = ncos::core::contracts::make_face_render_state_baseline();

  if (!apply_face_preset(exploratory_preset_, &state_, now_ms)) {
    return false;
  }

  ncos::core::contracts::FaceLayerClaim base_claim{};
  base_claim.layer = ncos::models::face::FaceLayer::kBase;
  base_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
  base_claim.requester_service = kFaceServiceId;
  base_claim.priority = 3;

  if (!ncos::core::contracts::apply_layer_claim(&state_, base_claim, now_ms)) {
    return false;
  }

  ncos::core::contracts::FaceLayerClaim gaze_claim{};
  gaze_claim.layer = ncos::models::face::FaceLayer::kGaze;
  gaze_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  gaze_claim.requester_service = kFaceServiceId;
  gaze_claim.priority = 5;

  if (!ncos::core::contracts::apply_layer_claim(&state_, gaze_claim, now_ms)) {
    return false;
  }

  next_render_ms_ = now_ms;
  next_gaze_target_ms_ = now_ms;
  initialized_ = true;
  return true;
}

void FaceGraphicsPipeline::tick(uint64_t now_ms) {
  if (!initialized_ || now_ms < next_render_ms_) {
    return;
  }

  if (now_ms >= next_gaze_target_ms_) {
    ncos::models::face::FaceGazeTarget target{};
    target.anchor = ncos::models::face::GazeAnchor::kUser;
    target.direction = gaze_left_ ? ncos::models::face::GazeDirection::kLeft
                                  : ncos::models::face::GazeDirection::kRight;
    target.focus_percent = 48;
    target.salience_percent = 30;
    target.hold_ms = 420;
    target.origin = ncos::models::face::FaceGazeTargetOrigin::kSystem;

    (void)gaze_controller_.set_target(target, now_ms);
    gaze_left_ = !gaze_left_;
    next_gaze_target_ms_ = now_ms + 700;
  }

  (void)gaze_controller_.tick(now_ms, &state_);

  FaceFrame frame{};
  if (composer_.compose(state_, &frame)) {
    (void)renderer_.render(frame);
  }

  next_render_ms_ = now_ms + kRenderPeriodMs;
}

bool FaceGraphicsPipeline::initialized() const {
  return initialized_;
}

}  // namespace ncos::services::face

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

  ncos::core::contracts::FaceLayerClaim base_claim{};
  base_claim.layer = ncos::models::face::FaceLayer::kBase;
  base_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
  base_claim.requester_service = kFaceServiceId;
  base_claim.priority = 3;

  if (!ncos::core::contracts::apply_layer_claim(&state_, base_claim, now_ms)) {
    return false;
  }

  next_render_ms_ = now_ms;
  initialized_ = true;
  return true;
}

void FaceGraphicsPipeline::tick(uint64_t now_ms) {
  if (!initialized_ || now_ms < next_render_ms_) {
    return;
  }

  // Minimal animation validates end-to-end flow face->frame->display.
  gaze_left_ = !gaze_left_;
  state_.eyes.direction = gaze_left_ ? ncos::models::face::GazeDirection::kLeft
                                     : ncos::models::face::GazeDirection::kRight;

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

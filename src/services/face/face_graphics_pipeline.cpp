#include "services/face/face_graphics_pipeline.hpp"

#include "drivers/display/display_runtime.hpp"

namespace {

constexpr ncos::models::face::FaceClipKeyframe kSignatureClipFrames[] = {
    {0, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kCenter, 54, 96}},
    {180, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kUpRight, 74, 92}},
    {360, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kUpLeft, 70, 88}},
    {540, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kCenter, 50, 96}},
};

constexpr ncos::models::face::FaceClip kSignatureClip = {
    1001,
    "signature_glance_arc",
    kSignatureClipFrames,
    sizeof(kSignatureClipFrames) / sizeof(kSignatureClipFrames[0]),
    620,
    220,
    8,
    760,
    280,
};

}  // namespace

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
  if (!compositor_.bind_state(&state_)) {
    return false;
  }

  FaceLayerRequest base_request{};
  base_request.layer = ncos::models::face::FaceLayer::kBase;
  base_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
  base_request.requester_service = kPresetOwnerServiceId;
  base_request.priority = 3;

  if (!compositor_.request_layer(base_request, now_ms).granted) {
    return false;
  }

  if (!apply_official_face_preset(official_preset_, &state_, now_ms)) {
    return false;
  }

  FaceLayerRequest gaze_request{};
  gaze_request.layer = ncos::models::face::FaceLayer::kGaze;
  gaze_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  gaze_request.requester_service = kGazeOwnerServiceId;
  gaze_request.priority = 5;

  if (!compositor_.request_layer(gaze_request, now_ms).granted) {
    return false;
  }

  FaceLayerRequest modulation_request{};
  modulation_request.layer = ncos::models::face::FaceLayer::kModulation;
  modulation_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kModulationOwner;
  modulation_request.requester_service = kModulationOwnerServiceId;
  modulation_request.priority = 4;

  if (!compositor_.request_layer(modulation_request, now_ms).granted) {
    return false;
  }

  preview_snapshot_ = make_face_preview_snapshot(state_, false, now_ms);
  next_render_ms_ = now_ms;
  next_gaze_target_ms_ = now_ms;
  next_clip_start_ms_ = now_ms + 3800;
  initialized_ = true;
  return true;
}

void FaceGraphicsPipeline::tick(uint64_t now_ms,
                                const ncos::core::contracts::FaceMultimodalInput& multimodal) {
  if (!initialized_ || now_ms < next_render_ms_) {
    return;
  }

  compositor_.tick(now_ms);

  if (!clip_player_.active() && now_ms >= next_clip_start_ms_) {
    (void)clip_player_.play(kSignatureClip, &compositor_, &state_, now_ms);
    next_clip_start_ms_ = now_ms + 6200;
  }

  const bool clip_updated = clip_player_.tick(now_ms, &compositor_, &state_);

  if (!clip_player_.active()) {
    if (now_ms >= next_gaze_target_ms_) {
      FaceLayerRequest gaze_request{};
      gaze_request.layer = ncos::models::face::FaceLayer::kGaze;
      gaze_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
      gaze_request.requester_service = kGazeOwnerServiceId;
      gaze_request.priority = 5;

      if (compositor_.request_layer(gaze_request, now_ms).granted) {
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
      }

      next_gaze_target_ms_ = now_ms + 700;
    }

    (void)gaze_controller_.tick(now_ms, &state_);
  } else if (!clip_updated) {
    // If clip is active but did not write, keep timeline ownership healthy.
    (void)clip_player_.tick(now_ms, &compositor_, &state_);
  }

  (void)multimodal_sync_.apply(multimodal, &compositor_, &state_, kModulationOwnerServiceId, now_ms);

  FaceFrame frame{};
  if (composer_.compose(state_, &frame)) {
    (void)renderer_.render(frame);
  }

  preview_snapshot_ = make_face_preview_snapshot(state_, clip_player_.active(), now_ms);
  next_render_ms_ = now_ms + kRenderPeriodMs;
}

bool FaceGraphicsPipeline::initialized() const {
  return initialized_;
}

size_t FaceGraphicsPipeline::export_preview_json(char* out_buffer, size_t out_buffer_size) const {
  return export_face_preview_json(preview_snapshot_, out_buffer, out_buffer_size);
}

}  // namespace ncos::services::face

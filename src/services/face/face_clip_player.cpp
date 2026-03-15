#include "services/face/face_clip_player.hpp"

namespace ncos::services::face {

FaceClipPlayer::FaceClipPlayer(uint16_t owner_service) : owner_service_(owner_service) {}

void FaceClipPlayer::bind_owner(uint16_t owner_service) {
  owner_service_ = owner_service;
}

uint8_t FaceClipPlayer::clamp_percent(uint8_t value) {
  return value > 100 ? 100 : value;
}

uint8_t FaceClipPlayer::lerp_u8(uint8_t from, uint8_t to, uint64_t num, uint64_t den) {
  if (den == 0 || num >= den) {
    return to;
  }

  const int32_t from_i = static_cast<int32_t>(from);
  const int32_t to_i = static_cast<int32_t>(to);
  const int32_t delta = to_i - from_i;
  const int32_t value =
      from_i + static_cast<int32_t>((delta * static_cast<int64_t>(num)) / static_cast<int64_t>(den));
  return static_cast<uint8_t>(value < 0 ? 0 : (value > 100 ? 100 : value));
}

void FaceClipPlayer::snapshot_from_state(PoseSnapshot* out,
                                         const ncos::core::contracts::FaceRenderState& state) const {
  if (out == nullptr) {
    return;
  }

  out->anchor = state.eyes.anchor;
  out->direction = state.eyes.direction;
  out->focus_percent = state.eyes.focus_percent;
  out->lid_openness_percent = state.lids.openness_percent;
}

void FaceClipPlayer::apply_pose(const ncos::models::face::FaceClipPose& pose,
                                ncos::core::contracts::FaceRenderState* state,
                                uint64_t now_ms) const {
  if (state == nullptr) {
    return;
  }

  state->eyes.anchor = pose.anchor;
  state->eyes.direction = pose.direction;
  state->eyes.focus_percent = clamp_percent(pose.focus_percent);
  state->lids.openness_percent = clamp_percent(pose.lid_openness_percent);
  state->lids.phase = ncos::models::face::BlinkPhase::kOpen;
  state->updated_at_ms = now_ms;
  state->owner_service = owner_service_;
  ++state->revision;
}

void FaceClipPlayer::apply_snapshot(const PoseSnapshot& pose,
                                    ncos::core::contracts::FaceRenderState* state,
                                    uint64_t now_ms) const {
  ncos::models::face::FaceClipPose clip_pose{};
  clip_pose.anchor = pose.anchor;
  clip_pose.direction = pose.direction;
  clip_pose.focus_percent = pose.focus_percent;
  clip_pose.lid_openness_percent = pose.lid_openness_percent;
  apply_pose(clip_pose, state, now_ms);
}

const ncos::models::face::FaceClipKeyframe* FaceClipPlayer::current_keyframe(uint64_t elapsed_ms) const {
  if (active_clip_ == nullptr || active_clip_->keyframes == nullptr || active_clip_->keyframe_count == 0) {
    return nullptr;
  }

  const ncos::models::face::FaceClipKeyframe* selected = &active_clip_->keyframes[0];
  for (size_t i = 0; i < active_clip_->keyframe_count; ++i) {
    const auto& frame = active_clip_->keyframes[i];
    if (frame.offset_ms <= elapsed_ms) {
      selected = &frame;
    } else {
      break;
    }
  }
  return selected;
}

bool FaceClipPlayer::play(const ncos::models::face::FaceClip& clip,
                          FaceCompositor* compositor,
                          ncos::core::contracts::FaceRenderState* state,
                          uint64_t now_ms) {
  if (compositor == nullptr || state == nullptr || owner_service_ == 0 || clip.id == 0 ||
      clip.keyframes == nullptr || clip.keyframe_count == 0 || clip.duration_ms == 0) {
    return false;
  }

  FaceLayerRequest request{};
  request.layer = ncos::models::face::FaceLayer::kClip;
  request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kClipOwner;
  request.requester_service = owner_service_;
  request.priority = clip.layer_priority;
  request.source_clip_id = clip.id;
  request.hold_ms = clip.hold_ms;
  request.cooldown_ms = clip.cooldown_ms;

  const auto decision = compositor->request_layer(request, now_ms);
  if (!decision.granted || !compositor->can_write(ncos::models::face::FaceLayer::kClip, owner_service_)) {
    return false;
  }

  active_clip_ = &clip;
  phase_ = FaceClipPlayerPhase::kPlaying;
  clip_start_ms_ = now_ms;
  recovery_start_ms_ = 0;
  recovery_end_ms_ = 0;

  snapshot_from_state(&baseline_pose_, *state);

  const auto* keyframe = current_keyframe(0);
  if (keyframe != nullptr) {
    apply_pose(keyframe->pose, state, now_ms);
  }

  return true;
}

bool FaceClipPlayer::begin_recovery(uint64_t now_ms, ncos::core::contracts::FaceRenderState* state) {
  if (active_clip_ == nullptr || state == nullptr) {
    return false;
  }

  snapshot_from_state(&recovery_from_, *state);
  recovery_start_ms_ = now_ms;
  recovery_end_ms_ = now_ms + (active_clip_->recovery_ms == 0 ? 1 : active_clip_->recovery_ms);
  phase_ = FaceClipPlayerPhase::kRecovering;
  return true;
}

void FaceClipPlayer::finish_and_release(FaceCompositor* compositor,
                                        ncos::core::contracts::FaceRenderState* state,
                                        uint64_t now_ms) {
  if (compositor != nullptr && state != nullptr && owner_service_ != 0) {
    (void)compositor->release_layer(ncos::models::face::FaceLayer::kClip,
                                    owner_service_,
                                    now_ms,
                                    active_clip_ == nullptr ? 0 : active_clip_->cooldown_ms);
  }

  active_clip_ = nullptr;
  phase_ = FaceClipPlayerPhase::kIdle;
  clip_start_ms_ = 0;
  recovery_start_ms_ = 0;
  recovery_end_ms_ = 0;
}

bool FaceClipPlayer::cancel(uint64_t now_ms, ncos::core::contracts::FaceRenderState* state) {
  if (active_clip_ == nullptr || state == nullptr) {
    return false;
  }

  if (phase_ == FaceClipPlayerPhase::kRecovering) {
    return true;
  }

  return begin_recovery(now_ms, state);
}

bool FaceClipPlayer::tick(uint64_t now_ms,
                          FaceCompositor* compositor,
                          ncos::core::contracts::FaceRenderState* state) {
  if (active_clip_ == nullptr || compositor == nullptr || state == nullptr || owner_service_ == 0) {
    return false;
  }

  if (!compositor->can_write(ncos::models::face::FaceLayer::kClip, owner_service_)) {
    finish_and_release(compositor, state, now_ms);
    return false;
  }

  if (phase_ == FaceClipPlayerPhase::kPlaying) {
    const uint64_t elapsed_ms = now_ms - clip_start_ms_;
    if (elapsed_ms >= active_clip_->duration_ms) {
      if (!begin_recovery(now_ms, state)) {
        finish_and_release(compositor, state, now_ms);
      }
      return true;
    }

    const auto* keyframe = current_keyframe(elapsed_ms);
    if (keyframe != nullptr) {
      apply_pose(keyframe->pose, state, now_ms);
      return true;
    }

    return false;
  }

  if (phase_ == FaceClipPlayerPhase::kRecovering) {
    const uint64_t den = recovery_end_ms_ - recovery_start_ms_;
    const uint64_t num = now_ms <= recovery_start_ms_ ? 0 : (now_ms - recovery_start_ms_);

    PoseSnapshot current{};
    current.anchor = baseline_pose_.anchor;
    current.direction = baseline_pose_.direction;
    current.focus_percent = lerp_u8(recovery_from_.focus_percent, baseline_pose_.focus_percent, num, den);
    current.lid_openness_percent =
        lerp_u8(recovery_from_.lid_openness_percent, baseline_pose_.lid_openness_percent, num, den);

    apply_snapshot(current, state, now_ms);

    if (now_ms >= recovery_end_ms_) {
      apply_snapshot(baseline_pose_, state, now_ms);
      finish_and_release(compositor, state, now_ms);
    }

    return true;
  }

  return false;
}

bool FaceClipPlayer::active() const {
  return active_clip_ != nullptr;
}

bool FaceClipPlayer::recovering() const {
  return phase_ == FaceClipPlayerPhase::kRecovering;
}

FaceClipPlayerPhase FaceClipPlayer::phase() const {
  return phase_;
}

uint32_t FaceClipPlayer::active_clip_id() const {
  return active_clip_ == nullptr ? 0 : active_clip_->id;
}

}  // namespace ncos::services::face

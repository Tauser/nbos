#include "services/face/face_visual_fallback.hpp"

namespace {

ncos::models::face::GazeDirection fallback_direction(ncos::models::face::GazeDirection direction) {
  using ncos::models::face::GazeDirection;
  switch (direction) {
    case GazeDirection::kUpLeft:
    case GazeDirection::kDownLeft:
      return GazeDirection::kLeft;
    case GazeDirection::kUpRight:
    case GazeDirection::kDownRight:
      return GazeDirection::kRight;
    default:
      return direction;
  }
}

uint8_t clamp_focus_for_fallback(uint8_t focus_percent) {
  return focus_percent > 36 ? 36 : focus_percent;
}

}  // namespace

namespace ncos::services::face {

bool should_enter_face_visual_fallback(const FaceTuningTelemetry& tuning) {
  const bool over_budget = has_face_visual_degradation(tuning.degradation,
                                                       FaceVisualDegradationFlag::kFrameOverBudget);
  const bool large_dirty_rect = has_face_visual_degradation(tuning.degradation,
                                                            FaceVisualDegradationFlag::kLargeDirtyRect);
  const bool difficult_motion = has_face_visual_degradation(tuning.degradation,
                                                            FaceVisualDegradationFlag::kHighContrastMotion) ||
                                has_face_visual_degradation(tuning.degradation,
                                                            FaceVisualDegradationFlag::kDiagonalMotion) ||
                                has_face_visual_degradation(tuning.degradation,
                                                            FaceVisualDegradationFlag::kFullRedraw);
  return (over_budget || large_dirty_rect) && difficult_motion;
}

bool should_exit_face_visual_fallback(const FaceTuningTelemetry& tuning) {
  return !has_face_visual_degradation(tuning.degradation, FaceVisualDegradationFlag::kFrameOverBudget) &&
         !has_face_visual_degradation(tuning.degradation, FaceVisualDegradationFlag::kLargeDirtyRect) &&
         !has_face_visual_degradation(tuning.degradation, FaceVisualDegradationFlag::kFullRedraw);
}

bool set_face_visual_fallback_active(FaceVisualFallbackStatus* status,
                                     bool active,
                                     uint64_t now_ms,
                                     FaceVisualDegradationFlag trigger) {
  if (status == nullptr || status->active == active) {
    return false;
  }

  status->active = active;
  status->last_change_ms = now_ms;
  status->last_trigger = trigger;
  if (active) {
    ++status->entry_count;
  } else {
    ++status->exit_count;
  }
  return true;
}

void apply_face_visual_fallback(ncos::core::contracts::FaceRenderState* state) {
  if (state == nullptr) {
    return;
  }

  bool changed = false;
  if (state->safety_mode != ncos::core::contracts::FaceRenderSafetyMode::kSafeFallback) {
    state->safety_mode = ncos::core::contracts::FaceRenderSafetyMode::kSafeFallback;
    changed = true;
  }

  const auto direction = fallback_direction(state->eyes.direction);
  if (direction != state->eyes.direction) {
    state->eyes.direction = direction;
    changed = true;
  }

  const uint8_t focus = clamp_focus_for_fallback(state->eyes.focus_percent);
  if (focus != state->eyes.focus_percent) {
    state->eyes.focus_percent = focus;
    changed = true;
  }

  if (changed) {
    ++state->revision;
  }
}

}  // namespace ncos::services::face

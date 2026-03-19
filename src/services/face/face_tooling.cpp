#include "services/face/face_tooling.hpp"

#include <stdio.h>

namespace {

const char* degradation_label(ncos::services::face::FaceVisualDegradationFlag flag) {
  using ncos::services::face::FaceVisualDegradationFlag;
  switch (flag) {
    case FaceVisualDegradationFlag::kFrameOverBudget:
      return "frame_over_budget";
    case FaceVisualDegradationFlag::kFullRedraw:
      return "full_redraw";
    case FaceVisualDegradationFlag::kHighContrastMotion:
      return "high_contrast_motion";
    case FaceVisualDegradationFlag::kLargeDirtyRect:
      return "large_dirty_rect";
    case FaceVisualDegradationFlag::kDiagonalMotion:
      return "diagonal_motion";
    case FaceVisualDegradationFlag::kNone:
    default:
      return "none";
  }
}

size_t append_degradation_labels(char* buffer,
                                 size_t buffer_size,
                                 ncos::services::face::FaceVisualDegradationFlag flags) {
  if (buffer == nullptr || buffer_size == 0) {
    return 0;
  }

  buffer[0] = '\0';
  if (flags == ncos::services::face::FaceVisualDegradationFlag::kNone) {
    return static_cast<size_t>(snprintf(buffer, buffer_size, "none"));
  }

  const ncos::services::face::FaceVisualDegradationFlag ordered_flags[] = {
      ncos::services::face::FaceVisualDegradationFlag::kFrameOverBudget,
      ncos::services::face::FaceVisualDegradationFlag::kFullRedraw,
      ncos::services::face::FaceVisualDegradationFlag::kHighContrastMotion,
      ncos::services::face::FaceVisualDegradationFlag::kLargeDirtyRect,
      ncos::services::face::FaceVisualDegradationFlag::kDiagonalMotion,
  };

  size_t used = 0;
  bool first = true;
  for (const auto flag : ordered_flags) {
    if (!ncos::services::face::has_face_visual_degradation(flags, flag)) {
      continue;
    }

    const int written = snprintf(buffer + used, buffer_size - used, "%s%s", first ? "" : "|",
                                 degradation_label(flag));
    if (written <= 0) {
      break;
    }

    const size_t chunk = static_cast<size_t>(written);
    if (used + chunk >= buffer_size) {
      used = buffer_size - 1;
      buffer[used] = '\0';
      break;
    }

    used += chunk;
    first = false;
  }

  return used;
}

}  // namespace

namespace ncos::services::face {

FacePreviewSnapshot make_face_preview_snapshot(const ncos::core::contracts::FaceRenderState& state,
                                               bool clip_active,
                                               uint64_t now_ms,
                                               const FaceTuningTelemetry& tuning) {
  FacePreviewSnapshot snapshot{};
  snapshot.stability = FaceToolingStability::kExploratory;
  snapshot.captured_at_ms = now_ms;
  snapshot.state_revision = state.revision;
  snapshot.active_owner_service = state.owner_service;
  snapshot.focus_percent = state.eyes.focus_percent;
  snapshot.lid_open_percent = state.lids.openness_percent;
  snapshot.brow_intensity_percent = state.brows.intensity_percent;
  snapshot.clip_active = clip_active;
  snapshot.tuning = tuning;
  return snapshot;
}

size_t export_face_preview_json(const FacePreviewSnapshot& snapshot,
                                char* out_buffer,
                                size_t out_buffer_size) {
  if (out_buffer == nullptr || out_buffer_size == 0) {
    return 0;
  }

  const char* stability =
      snapshot.stability == FaceToolingStability::kBaselineStable ? "baseline_stable" : "exploratory";

  char degradation[128] = {};
  (void)append_degradation_labels(degradation, sizeof(degradation), snapshot.tuning.degradation);

  const int written = snprintf(
      out_buffer,
      out_buffer_size,
      "{\"tooling\":\"%s\",\"captured_at_ms\":%llu,\"state_revision\":%lu,\"owner_service\":%u,"
      "\"focus\":%u,\"lid_open\":%u,\"brow_intensity\":%u,\"clip_active\":%s,"
      "\"tuning\":{\"frame_budget_us\":%lu,\"total_us\":%lu,\"compositor_us\":%lu,\"clip_us\":%lu,"
      "\"gaze_us\":%lu,\"modulation_us\":%lu,\"compose_us\":%lu,\"render_us\":%lu,"
      "\"avg_frame_time_us\":%lu,\"peak_frame_time_us\":%lu,\"dirty_area_px\":%lu,"
      "\"rendered_frames\":%lu,\"skipped_duplicate_frames\":%lu,\"frame_skipped\":%s,"
      "\"full_redraw\":%s,\"high_contrast_motion\":%s,\"degradation\":\"%s\"}}",
      stability,
      static_cast<unsigned long long>(snapshot.captured_at_ms),
      static_cast<unsigned long>(snapshot.state_revision),
      snapshot.active_owner_service,
      snapshot.focus_percent,
      snapshot.lid_open_percent,
      snapshot.brow_intensity_percent,
      snapshot.clip_active ? "true" : "false",
      static_cast<unsigned long>(snapshot.tuning.frame_budget_us),
      static_cast<unsigned long>(snapshot.tuning.stages.total_us),
      static_cast<unsigned long>(snapshot.tuning.stages.compositor_us),
      static_cast<unsigned long>(snapshot.tuning.stages.clip_us),
      static_cast<unsigned long>(snapshot.tuning.stages.gaze_us),
      static_cast<unsigned long>(snapshot.tuning.stages.modulation_us),
      static_cast<unsigned long>(snapshot.tuning.stages.compose_us),
      static_cast<unsigned long>(snapshot.tuning.stages.render_us),
      static_cast<unsigned long>(snapshot.tuning.avg_frame_time_us),
      static_cast<unsigned long>(snapshot.tuning.peak_frame_time_us),
      static_cast<unsigned long>(snapshot.tuning.dirty_area_px),
      static_cast<unsigned long>(snapshot.tuning.rendered_frames),
      static_cast<unsigned long>(snapshot.tuning.skipped_duplicate_frames),
      snapshot.tuning.frame_skipped ? "true" : "false",
      snapshot.tuning.full_redraw ? "true" : "false",
      snapshot.tuning.high_contrast_motion ? "true" : "false",
      degradation);

  if (written <= 0) {
    out_buffer[0] = '\0';
    return 0;
  }

  const size_t length = static_cast<size_t>(written);
  if (length >= out_buffer_size) {
    out_buffer[out_buffer_size - 1] = '\0';
    return out_buffer_size - 1;
  }

  return length;
}

}  // namespace ncos::services::face

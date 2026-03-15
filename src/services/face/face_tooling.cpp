#include "services/face/face_tooling.hpp"

#include <stdio.h>

namespace ncos::services::face {

FacePreviewSnapshot make_face_preview_snapshot(const ncos::core::contracts::FaceRenderState& state,
                                               bool clip_active,
                                               uint64_t now_ms) {
  FacePreviewSnapshot snapshot{};
  snapshot.stability = FaceToolingStability::kExploratory;
  snapshot.captured_at_ms = now_ms;
  snapshot.state_revision = state.revision;
  snapshot.active_owner_service = state.owner_service;
  snapshot.focus_percent = state.eyes.focus_percent;
  snapshot.lid_open_percent = state.lids.openness_percent;
  snapshot.brow_intensity_percent = state.brows.intensity_percent;
  snapshot.clip_active = clip_active;
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

  const int written = snprintf(
      out_buffer,
      out_buffer_size,
      "{\"tooling\":\"%s\",\"captured_at_ms\":%llu,\"state_revision\":%lu,\"owner_service\":%u,"
      "\"focus\":%u,\"lid_open\":%u,\"brow_intensity\":%u,\"clip_active\":%s}",
      stability,
      static_cast<unsigned long long>(snapshot.captured_at_ms),
      static_cast<unsigned long>(snapshot.state_revision),
      snapshot.active_owner_service,
      snapshot.focus_percent,
      snapshot.lid_open_percent,
      snapshot.brow_intensity_percent,
      snapshot.clip_active ? "true" : "false");

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

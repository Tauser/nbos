#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"

namespace ncos::services::face {

enum class FaceToolingStability : uint8_t {
  kExploratory = 1,
  kBaselineStable = 2,
};

enum class FaceVisualDegradationFlag : uint8_t {
  kNone = 0,
  kFrameOverBudget = 1 << 0,
  kFullRedraw = 1 << 1,
  kHighContrastMotion = 1 << 2,
  kLargeDirtyRect = 1 << 3,
  kDiagonalMotion = 1 << 4,
};

constexpr FaceVisualDegradationFlag operator|(FaceVisualDegradationFlag lhs,
                                              FaceVisualDegradationFlag rhs) {
  return static_cast<FaceVisualDegradationFlag>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr bool has_face_visual_degradation(FaceVisualDegradationFlag mask,
                                           FaceVisualDegradationFlag flag) {
  return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(flag)) != 0;
}

struct FaceStageTelemetry {
  uint32_t compositor_us = 0;
  uint32_t clip_us = 0;
  uint32_t gaze_us = 0;
  uint32_t modulation_us = 0;
  uint32_t compose_us = 0;
  uint32_t render_us = 0;
  uint32_t total_us = 0;
};

struct FaceTuningTelemetry {
  FaceStageTelemetry stages{};
  uint32_t frame_budget_us = 0;
  uint32_t dirty_area_px = 0;
  uint32_t avg_frame_time_us = 0;
  uint32_t peak_frame_time_us = 0;
  uint32_t rendered_frames = 0;
  uint32_t skipped_duplicate_frames = 0;
  bool frame_skipped = false;
  bool full_redraw = false;
  bool high_contrast_motion = false;
  FaceVisualDegradationFlag degradation = FaceVisualDegradationFlag::kNone;
};

struct FacePreviewSnapshot {
  FaceToolingStability stability = FaceToolingStability::kExploratory;
  uint64_t captured_at_ms = 0;
  uint32_t state_revision = 0;
  uint16_t active_owner_service = 0;
  uint8_t focus_percent = 0;
  uint8_t lid_open_percent = 100;
  uint8_t brow_intensity_percent = 0;
  bool clip_active = false;
  FaceTuningTelemetry tuning{};
};

FacePreviewSnapshot make_face_preview_snapshot(const ncos::core::contracts::FaceRenderState& state,
                                               bool clip_active,
                                               uint64_t now_ms,
                                               const FaceTuningTelemetry& tuning = {});

size_t export_face_preview_json(const FacePreviewSnapshot& snapshot,
                                char* out_buffer,
                                size_t out_buffer_size);

}  // namespace ncos::services::face

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"

namespace ncos::services::face {

enum class FaceToolingStability : uint8_t {
  kExploratory = 1,
  kBaselineStable = 2,
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
};

FacePreviewSnapshot make_face_preview_snapshot(const ncos::core::contracts::FaceRenderState& state,
                                               bool clip_active,
                                               uint64_t now_ms);

size_t export_face_preview_json(const FacePreviewSnapshot& snapshot,
                                char* out_buffer,
                                size_t out_buffer_size);

}  // namespace ncos::services::face

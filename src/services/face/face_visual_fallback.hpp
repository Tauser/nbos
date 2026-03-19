#pragma once

#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_tooling.hpp"

namespace ncos::services::face {

struct FaceVisualFallbackStatus {
  bool active = false;
  uint32_t entry_count = 0;
  uint32_t exit_count = 0;
  uint64_t last_change_ms = 0;
  FaceVisualDegradationFlag last_trigger = FaceVisualDegradationFlag::kNone;
};

bool should_enter_face_visual_fallback(const FaceTuningTelemetry& tuning);
bool should_exit_face_visual_fallback(const FaceTuningTelemetry& tuning);
bool set_face_visual_fallback_active(FaceVisualFallbackStatus* status,
                                     bool active,
                                     uint64_t now_ms,
                                     FaceVisualDegradationFlag trigger);
void apply_face_visual_fallback(ncos::core::contracts::FaceRenderState* state);

}  // namespace ncos::services::face

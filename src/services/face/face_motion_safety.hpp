#pragma once

#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"

namespace ncos::services::face {

struct FaceMotionSafetyStatus {
  ncos::models::face::GazeDirection last_direction = ncos::models::face::GazeDirection::kCenter;
  uint8_t last_focus_percent = 0;
  uint64_t last_update_ms = 0;
};

struct FaceMotionSafetyResult {
  bool active = false;
  bool limited_amplitude = false;
  bool limited_velocity = false;
  bool limited_diagonal = false;
  bool limited_blink_interaction = false;
  uint8_t effective_focus_percent = 0;
};

void reset_face_motion_safety_status(FaceMotionSafetyStatus* status,
                                     const ncos::core::contracts::FaceRenderState& state,
                                     uint64_t now_ms);

bool apply_face_motion_safety(ncos::core::contracts::FaceRenderState* state,
                              FaceMotionSafetyStatus* status,
                              uint64_t now_ms,
                              FaceMotionSafetyResult* out_result = nullptr);

}  // namespace ncos::services::face

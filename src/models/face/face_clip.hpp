#pragma once

#include <stddef.h>
#include <stdint.h>

#include "models/face/face_models.hpp"

namespace ncos::models::face {

struct FaceClipPose {
  GazeAnchor anchor = GazeAnchor::kCenter;
  GazeDirection direction = GazeDirection::kCenter;
  uint8_t focus_percent = 0;
  uint8_t lid_openness_percent = 100;
};

struct FaceClipKeyframe {
  uint32_t offset_ms = 0;
  FaceClipPose pose{};
};

struct FaceClip {
  uint32_t id = 0;
  const char* name = "";
  const FaceClipKeyframe* keyframes = nullptr;
  size_t keyframe_count = 0;
  uint32_t duration_ms = 0;
  uint32_t recovery_ms = 180;
  uint8_t layer_priority = 8;
  uint32_t hold_ms = 700;
  uint32_t cooldown_ms = 240;
};

}  // namespace ncos::models::face

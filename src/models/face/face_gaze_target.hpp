#pragma once

#include <stdint.h>

#include "models/face/face_models.hpp"

namespace ncos::models::face {

enum class FaceGazeTargetOrigin : uint8_t {
  kSystem = 1,
  kAttention = 2,
  kInteraction = 3,
  kSafety = 4,
};

struct FaceGazeTarget {
  GazeAnchor anchor = GazeAnchor::kCenter;
  GazeDirection direction = GazeDirection::kCenter;
  uint8_t focus_percent = 0;
  uint8_t salience_percent = 0;
  uint16_t hold_ms = 0;
  FaceGazeTargetOrigin origin = FaceGazeTargetOrigin::kSystem;
};

}  // namespace ncos::models::face

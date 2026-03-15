#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"

namespace ncos::services::face {

enum class FaceReadabilityTier : uint8_t {
  kTierAImmediate = 1,
  kTierBBalanced = 2,
  kTierCNuanced = 3,
};

enum class FaceExploratoryPresetId : uint8_t {
  kClarityNeutral = 1,
  kFocusedAttend = 2,
  kCalmLowPower = 3,
  kCuriousObserve = 4,
  kPlayfulLift = 5,
  kAttentiveLock = 6,
};

struct FacePresetSpec {
  FaceExploratoryPresetId id = FaceExploratoryPresetId::kClarityNeutral;
  const char* name = "";
  FaceReadabilityTier readability_tier = FaceReadabilityTier::kTierAImmediate;
  bool exploratory_only = true;

  ncos::models::face::FacePresetId compatibility_preset =
      ncos::models::face::FacePresetId::kNeutralBaseline;
  ncos::models::face::FaceShapeProfile shape_profile =
      ncos::models::face::FaceShapeProfile::kCompanionBalanced;

  uint8_t eye_size_percent = 50;
  uint8_t eye_spacing_percent = 50;
  uint8_t eye_line_height_percent = 50;

  ncos::models::face::GazeAnchor gaze_anchor = ncos::models::face::GazeAnchor::kCenter;
  ncos::models::face::GazeDirection gaze_direction = ncos::models::face::GazeDirection::kCenter;
  uint8_t gaze_focus_percent = 0;
  uint8_t eyelid_openness_percent = 100;

  const char* contrast_note = "";
};

size_t face_preset_library_count();
const FacePresetSpec* face_preset_library_items();
const FacePresetSpec* find_face_preset(FaceExploratoryPresetId id);
bool apply_face_preset(FaceExploratoryPresetId id,
                       ncos::core::contracts::FaceRenderState* state,
                       uint64_t now_ms);

}  // namespace ncos::services::face

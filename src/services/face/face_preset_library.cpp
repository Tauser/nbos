#include "services/face/face_preset_library.hpp"

namespace {

using ncos::models::face::FacePresetId;
using ncos::models::face::FaceShapeProfile;
using ncos::models::face::GazeAnchor;
using ncos::models::face::GazeDirection;
using ncos::services::face::FaceExploratoryPresetId;
using ncos::services::face::FaceOfficialPresetId;
using ncos::services::face::FaceOfficialPresetSpec;
using ncos::services::face::FacePresetSpec;
using ncos::services::face::FaceReadabilityTier;

constexpr FacePresetSpec kPresetLibrary[] = {
    {
        FaceExploratoryPresetId::kClarityNeutral,
        "clarity_neutral",
        FaceReadabilityTier::kTierAImmediate,
        true,
        FacePresetId::kNeutralBaseline,
        FaceShapeProfile::kCompanionBalanced,
        58,
        56,
        50,
        GazeAnchor::kCenter,
        GazeDirection::kCenter,
        40,
        100,
        "tier A baseline: immediate read with symmetric square-open eyes",
    },
    {
        FaceExploratoryPresetId::kFocusedAttend,
        "focused_attend",
        FaceReadabilityTier::kTierAImmediate,
        true,
        FacePresetId::kFocusedAttend,
        FaceShapeProfile::kHeroicWide,
        54,
        64,
        48,
        GazeAnchor::kUser,
        GazeDirection::kRight,
        74,
        88,
        "tier A contrast by direct attention, higher focus and tighter lids",
    },
    {
        FaceExploratoryPresetId::kSocialGlance,
        "social_glance",
        FaceReadabilityTier::kTierAImmediate,
        true,
        FacePresetId::kWarmWelcome,
        FaceShapeProfile::kCompanionBalanced,
        56,
        60,
        49,
        GazeAnchor::kUser,
        GazeDirection::kLeft,
        62,
        94,
        "tier A social signal: quick side attention while keeping friendly openness",
    },
    {
        FaceExploratoryPresetId::kCalmLowPower,
        "calm_low_power",
        FaceReadabilityTier::kTierBBalanced,
        true,
        FacePresetId::kLowPowerCalm,
        FaceShapeProfile::kCompanionBalanced,
        46,
        52,
        54,
        GazeAnchor::kInternal,
        GazeDirection::kDown,
        24,
        76,
        "tier B energetic contrast: lower focus and reduced aperture",
    },
    {
        FaceExploratoryPresetId::kCuriousObserve,
        "curious_observe",
        FaceReadabilityTier::kTierBBalanced,
        true,
        FacePresetId::kWarmWelcome,
        FaceShapeProfile::kCuriousTall,
        62,
        50,
        44,
        GazeAnchor::kStimulus,
        GazeDirection::kUpLeft,
        56,
        96,
        "tier B curiosity: higher eye-line and open lids with upward diagonal gaze",
    },
    {
        FaceExploratoryPresetId::kPlayfulLift,
        "playful_lift",
        FaceReadabilityTier::kTierCNuanced,
        true,
        FacePresetId::kWarmWelcome,
        FaceShapeProfile::kPlayfulRound,
        64,
        58,
        52,
        GazeAnchor::kUser,
        GazeDirection::kUpRight,
        50,
        92,
        "tier C nuance: playful contour with subtle upbeat attention",
    },
    {
        FaceExploratoryPresetId::kAttentiveLock,
        "attentive_lock",
        FaceReadabilityTier::kTierCNuanced,
        true,
        FacePresetId::kFocusedAttend,
        FaceShapeProfile::kHeroicWide,
        52,
        66,
        50,
        GazeAnchor::kUser,
        GazeDirection::kLeft,
        84,
        82,
        "tier C lock: strong commitment signal with controlled aperture",
    },
    {
        FaceExploratoryPresetId::kIdleDrift,
        "idle_drift",
        FaceReadabilityTier::kTierCNuanced,
        true,
        FacePresetId::kNeutralBaseline,
        FaceShapeProfile::kPlayfulRound,
        60,
        54,
        51,
        GazeAnchor::kInternal,
        GazeDirection::kDownRight,
        34,
        90,
        "tier C idle nuance: soft internal attention to support long-run liveliness",
    },
};

constexpr FaceOfficialPresetSpec kOfficialPresetLibrary[] = {
    {
        FaceOfficialPresetId::kCoreNeutral,
        FaceExploratoryPresetId::kClarityNeutral,
        "core_neutral",
        FaceReadabilityTier::kTierAImmediate,
        true,
        "frozen after stable baseline behavior with compositor ownership and clip recovery",
    },
    {
        FaceOfficialPresetId::kCoreAttend,
        FaceExploratoryPresetId::kFocusedAttend,
        "core_attend",
        FaceReadabilityTier::kTierAImmediate,
        true,
        "frozen for strong attentional signal under gaze/clip coexistence",
    },
    {
        FaceOfficialPresetId::kCoreCalm,
        FaceExploratoryPresetId::kCalmLowPower,
        "core_calm",
        FaceReadabilityTier::kTierBBalanced,
        true,
        "frozen for low-energy readability without destabilizing clip or transient space",
    },
    {
        FaceOfficialPresetId::kCoreCurious,
        FaceExploratoryPresetId::kCuriousObserve,
        "core_curious",
        FaceReadabilityTier::kTierBBalanced,
        true,
        "frozen for curiosity signature preserving compositional legibility",
    },
    {
        FaceOfficialPresetId::kCoreLock,
        FaceExploratoryPresetId::kAttentiveLock,
        "core_lock",
        FaceReadabilityTier::kTierCNuanced,
        true,
        "frozen as high-intent state validated against clip preemption and recovery",
    },
};

constexpr size_t kPresetCount = sizeof(kPresetLibrary) / sizeof(kPresetLibrary[0]);
constexpr size_t kOfficialPresetCount = sizeof(kOfficialPresetLibrary) / sizeof(kOfficialPresetLibrary[0]);

uint8_t clamp_percent(uint8_t value) {
  return value > 100 ? 100 : value;
}

void apply_official_eye_signature(FaceOfficialPresetId id,
                                  ncos::core::contracts::FaceRenderState* state) {
  if (state == nullptr) {
    return;
  }

  state->eyes.left_adjust = {};
  state->eyes.right_adjust = {};

  switch (id) {
    case FaceOfficialPresetId::kCoreNeutral:
    case FaceOfficialPresetId::kCoreCalm:
      break;
    case FaceOfficialPresetId::kCoreAttend:
      state->eyes.left_adjust.size_delta_percent = -4;
      state->eyes.left_adjust.openness_delta_percent = -4;
      state->eyes.right_adjust.x_offset_px = 1;
      state->eyes.right_adjust.size_delta_percent = 4;
      state->eyes.right_adjust.openness_delta_percent = 4;
      break;
    case FaceOfficialPresetId::kCoreCurious:
      state->eyes.left_adjust.x_offset_px = -1;
      state->eyes.left_adjust.y_offset_px = -1;
      state->eyes.left_adjust.size_delta_percent = 4;
      state->eyes.left_adjust.openness_delta_percent = 4;
      state->eyes.right_adjust.size_delta_percent = -2;
      state->eyes.right_adjust.openness_delta_percent = -2;
      break;
    case FaceOfficialPresetId::kCoreLock:
      state->eyes.left_adjust.x_offset_px = -1;
      state->eyes.left_adjust.size_delta_percent = 4;
      state->eyes.left_adjust.openness_delta_percent = 4;
      state->eyes.right_adjust.size_delta_percent = -3;
      state->eyes.right_adjust.openness_delta_percent = -4;
      break;
  }
}

}  // namespace

namespace ncos::services::face {

size_t face_preset_library_count() {
  return kPresetCount;
}

const FacePresetSpec* face_preset_library_items() {
  return kPresetLibrary;
}

const FacePresetSpec* find_face_preset(FaceExploratoryPresetId id) {
  for (size_t i = 0; i < kPresetCount; ++i) {
    if (kPresetLibrary[i].id == id) {
      return &kPresetLibrary[i];
    }
  }
  return nullptr;
}

bool apply_face_preset(FaceExploratoryPresetId id,
                       ncos::core::contracts::FaceRenderState* state,
                       uint64_t now_ms) {
  if (state == nullptr || !ncos::core::contracts::is_valid(*state)) {
    return false;
  }

  const FacePresetSpec* spec = find_face_preset(id);
  if (spec == nullptr) {
    return false;
  }

  auto geometry = ncos::core::contracts::make_shape_geometry_profile(spec->shape_profile);
  geometry.eye_size_percent = clamp_percent(spec->eye_size_percent);
  geometry.eye_spacing_percent = clamp_percent(spec->eye_spacing_percent);
  geometry.eye_line_height_percent = clamp_percent(spec->eye_line_height_percent);

  state->preset = spec->compatibility_preset;
  state->geometry = geometry;

  state->eyes.anchor = spec->gaze_anchor;
  state->eyes.direction = spec->gaze_direction;
  state->eyes.focus_percent = clamp_percent(spec->gaze_focus_percent);
  state->eyes.left_adjust = {};
  state->eyes.right_adjust = {};

  state->lids.phase = ncos::models::face::BlinkPhase::kOpen;
  state->lids.openness_percent = clamp_percent(spec->eyelid_openness_percent);

  state->updated_at_ms = now_ms;
  ++state->revision;

  return ncos::core::contracts::is_valid(*state);
}

size_t face_official_preset_library_count() {
  return kOfficialPresetCount;
}

const FaceOfficialPresetSpec* face_official_preset_library_items() {
  return kOfficialPresetLibrary;
}

const FaceOfficialPresetSpec* find_official_face_preset(FaceOfficialPresetId id) {
  for (size_t i = 0; i < kOfficialPresetCount; ++i) {
    if (kOfficialPresetLibrary[i].id == id) {
      return &kOfficialPresetLibrary[i];
    }
  }
  return nullptr;
}

bool apply_official_face_preset(FaceOfficialPresetId id,
                                ncos::core::contracts::FaceRenderState* state,
                                uint64_t now_ms) {
  const FaceOfficialPresetSpec* spec = find_official_face_preset(id);
  if (spec == nullptr || !spec->frozen_for_release) {
    return false;
  }

  if (!apply_face_preset(spec->source, state, now_ms)) {
    return false;
  }

  apply_official_eye_signature(id, state);
  return ncos::core::contracts::is_valid(*state);
}

}  // namespace ncos::services::face


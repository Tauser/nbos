#include "services/face/face_preset_library.hpp"

namespace {

using ncos::models::face::FacePresetId;
using ncos::models::face::FaceShapeProfile;
using ncos::models::face::GazeAnchor;
using ncos::models::face::GazeDirection;
using ncos::services::face::FaceExploratoryPresetId;
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
        54,
        50,
        GazeAnchor::kCenter,
        GazeDirection::kCenter,
        42,
        100,
        "baseline de leitura imediata: olhos grandes e simetricos",
    },
    {
        FaceExploratoryPresetId::kFocusedAttend,
        "focused_attend",
        FaceReadabilityTier::kTierAImmediate,
        true,
        FacePresetId::kFocusedAttend,
        FaceShapeProfile::kHeroicWide,
        54,
        62,
        48,
        GazeAnchor::kUser,
        GazeDirection::kRight,
        72,
        90,
        "contraste por postura atencional: abertura menor e foco alto",
    },
    {
        FaceExploratoryPresetId::kCalmLowPower,
        "calm_low_power",
        FaceReadabilityTier::kTierBBalanced,
        true,
        FacePresetId::kLowPowerCalm,
        FaceShapeProfile::kCompanionBalanced,
        46,
        50,
        54,
        GazeAnchor::kInternal,
        GazeDirection::kDown,
        28,
        78,
        "contraste energetico: olhos menores e foco reduzido",
    },
    {
        FaceExploratoryPresetId::kCuriousObserve,
        "curious_observe",
        FaceReadabilityTier::kTierBBalanced,
        true,
        FacePresetId::kWarmWelcome,
        FaceShapeProfile::kCuriousTall,
        62,
        48,
        44,
        GazeAnchor::kStimulus,
        GazeDirection::kUpLeft,
        58,
        96,
        "curiosidade legivel por eixo vertical alto e abertura ampla",
    },
    {
        FaceExploratoryPresetId::kPlayfulLift,
        "playful_lift",
        FaceReadabilityTier::kTierCNuanced,
        true,
        FacePresetId::kWarmWelcome,
        FaceShapeProfile::kPlayfulRound,
        64,
        56,
        52,
        GazeAnchor::kUser,
        GazeDirection::kUpRight,
        52,
        92,
        "nuance ludica: roundness alta com gaze diagonal leve",
    },
    {
        FaceExploratoryPresetId::kAttentiveLock,
        "attentive_lock",
        FaceReadabilityTier::kTierCNuanced,
        true,
        FacePresetId::kFocusedAttend,
        FaceShapeProfile::kHeroicWide,
        52,
        60,
        50,
        GazeAnchor::kUser,
        GazeDirection::kLeft,
        80,
        84,
        "nuance de lock: foco maximo com abertura controlada",
    },
};

constexpr size_t kPresetCount = sizeof(kPresetLibrary) / sizeof(kPresetLibrary[0]);

uint8_t clamp_percent(uint8_t value) {
  return value > 100 ? 100 : value;
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

  state->lids.phase = ncos::models::face::BlinkPhase::kOpen;
  state->lids.openness_percent = clamp_percent(spec->eyelid_openness_percent);

  state->updated_at_ms = now_ms;
  ++state->revision;

  return ncos::core::contracts::is_valid(*state);
}

}  // namespace ncos::services::face

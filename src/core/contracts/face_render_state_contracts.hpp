#pragma once

#include <stdint.h>

#include "models/face/face_models.hpp"

namespace ncos::core::contracts {

enum class FaceRenderStateVersion : uint8_t {
  kV1 = 1,
};

enum class FaceCompositionMode : uint8_t {
  kSinglePreset = 1,
  kLayered = 2,
};

enum class FaceRenderSafetyMode : uint8_t {
  kNominal = 1,
  kSafeFallback = 2,
};

struct FaceLayerOwnership {
  ncos::models::face::FaceLayer layer = ncos::models::face::FaceLayer::kBase;
  uint16_t owner_service = 0;
  uint8_t priority = 0;
  uint32_t source_clip_id = 0;
};

struct FaceCompositionState {
  FaceCompositionMode mode = FaceCompositionMode::kSinglePreset;
  bool composition_locked = false;
  FaceLayerOwnership layers[ncos::models::face::kFaceLayerCount] = {};
};

struct FaceRenderState {
  FaceRenderStateVersion version = FaceRenderStateVersion::kV1;
  uint32_t revision = 0;
  uint64_t updated_at_ms = 0;

  FaceRenderSafetyMode safety_mode = FaceRenderSafetyMode::kNominal;
  ncos::models::face::FacePresetId preset = ncos::models::face::FacePresetId::kNeutralBaseline;

  ncos::models::face::EyePose eyes{};
  ncos::models::face::EyelidPose lids{};
  ncos::models::face::MouthPose mouth{};
  ncos::models::face::BrowPose brows{};

  FaceCompositionState composition{};

  uint32_t active_trace_id = 0;
  uint16_t owner_service = 0;
};

struct FaceLayerClaim {
  ncos::models::face::FaceLayer layer = ncos::models::face::FaceLayer::kBase;
  uint16_t requester_service = 0;
  uint8_t priority = 0;
  uint32_t source_clip_id = 0;
};

FaceRenderState make_face_render_state_baseline();
bool is_valid(const FaceRenderState& state);
bool can_apply_layer_claim(const FaceRenderState& state, const FaceLayerClaim& claim);
bool apply_layer_claim(FaceRenderState* state, const FaceLayerClaim& claim, uint64_t now_ms);

}  // namespace ncos::core::contracts

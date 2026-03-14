#pragma once

#include <stdint.h>

#include "models/face/face_models.hpp"

namespace ncos::core::contracts {

enum class FaceRenderStateVersion : uint8_t {
  kV1 = 1,
  kV2 = 2,
};

enum class FaceCompositionMode : uint8_t {
  kSinglePreset = 1,
  kLayered = 2,
};

enum class FaceRenderSafetyMode : uint8_t {
  kNominal = 1,
  kSafeFallback = 2,
};

enum class FaceLayerOwnerRole : uint8_t {
  kBaseOwner = 1,
  kBlinkOwner = 2,
  kGazeOwner = 3,
  kModulationOwner = 4,
  kTransientOwner = 5,
  kClipOwner = 6,
};

struct FaceShapeGeometry {
  ncos::models::face::FaceShapeProfile profile =
      ncos::models::face::FaceShapeProfile::kCompanionBalanced;
  uint8_t eye_size_percent = 50;
  uint8_t eye_spacing_percent = 50;
  uint8_t eye_line_height_percent = 50;
  uint8_t brow_height_percent = 50;
  uint8_t mouth_width_percent = 50;
  uint8_t mouth_height_percent = 50;
  uint8_t jaw_width_percent = 50;
  uint8_t silhouette_roundness_percent = 50;
};

struct FaceLayerPolicy {
  ncos::models::face::FaceLayer layer = ncos::models::face::FaceLayer::kBase;
  FaceLayerOwnerRole required_owner_role = FaceLayerOwnerRole::kBaseOwner;
  uint8_t default_priority = 0;
};

struct FaceLayerOwnership {
  ncos::models::face::FaceLayer layer = ncos::models::face::FaceLayer::kBase;
  FaceLayerOwnerRole owner_role = FaceLayerOwnerRole::kBaseOwner;
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
  FaceRenderStateVersion version = FaceRenderStateVersion::kV2;
  uint32_t revision = 0;
  uint64_t updated_at_ms = 0;

  FaceRenderSafetyMode safety_mode = FaceRenderSafetyMode::kNominal;
  ncos::models::face::FacePresetId preset = ncos::models::face::FacePresetId::kNeutralBaseline;

  FaceShapeGeometry geometry{};

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
  FaceLayerOwnerRole requester_role = FaceLayerOwnerRole::kBaseOwner;
  uint16_t requester_service = 0;
  uint8_t priority = 0;
  uint32_t source_clip_id = 0;
};

FaceShapeGeometry make_shape_geometry_profile(ncos::models::face::FaceShapeProfile profile);
FaceRenderState make_face_render_state_baseline();
FaceLayerPolicy face_layer_policy(ncos::models::face::FaceLayer layer);
bool is_valid(const FaceRenderState& state);
bool can_apply_layer_claim(const FaceRenderState& state, const FaceLayerClaim& claim);
bool apply_layer_claim(FaceRenderState* state, const FaceLayerClaim& claim, uint64_t now_ms);

}  // namespace ncos::core::contracts

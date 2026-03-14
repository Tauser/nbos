#include "core/contracts/face_render_state_contracts.hpp"

namespace {

bool percent_in_range(uint8_t value) {
  return value <= 100;
}

}  // namespace

namespace ncos::core::contracts {

FaceLayerPolicy face_layer_policy(ncos::models::face::FaceLayer layer) {
  switch (layer) {
    case ncos::models::face::FaceLayer::kBase:
      return {layer, FaceLayerOwnerRole::kBaseOwner, 2};
    case ncos::models::face::FaceLayer::kBlink:
      return {layer, FaceLayerOwnerRole::kBlinkOwner, 6};
    case ncos::models::face::FaceLayer::kGaze:
      return {layer, FaceLayerOwnerRole::kGazeOwner, 5};
    case ncos::models::face::FaceLayer::kModulation:
      return {layer, FaceLayerOwnerRole::kModulationOwner, 4};
    case ncos::models::face::FaceLayer::kTransient:
      return {layer, FaceLayerOwnerRole::kTransientOwner, 7};
    case ncos::models::face::FaceLayer::kClip:
      return {layer, FaceLayerOwnerRole::kClipOwner, 8};
    default:
      return {ncos::models::face::FaceLayer::kBase, FaceLayerOwnerRole::kBaseOwner, 2};
  }
}

FaceRenderState make_face_render_state_baseline() {
  FaceRenderState baseline{};
  baseline.version = FaceRenderStateVersion::kV1;
  baseline.revision = 1;
  baseline.updated_at_ms = 0;
  baseline.safety_mode = FaceRenderSafetyMode::kSafeFallback;
  baseline.preset = ncos::models::face::FacePresetId::kNeutralBaseline;

  baseline.eyes.anchor = ncos::models::face::GazeAnchor::kCenter;
  baseline.eyes.direction = ncos::models::face::GazeDirection::kCenter;
  baseline.eyes.focus_percent = 0;

  baseline.lids.phase = ncos::models::face::BlinkPhase::kOpen;
  baseline.lids.openness_percent = 100;

  baseline.mouth.shape = ncos::models::face::MouthShape::kNeutral;
  baseline.mouth.openness_percent = 0;

  baseline.brows.expression = ncos::models::face::BrowExpression::kNeutral;
  baseline.brows.intensity_percent = 0;

  baseline.composition.mode = FaceCompositionMode::kLayered;
  baseline.composition.composition_locked = false;

  for (size_t i = 0; i < ncos::models::face::kFaceLayerCount; ++i) {
    const ncos::models::face::FaceLayer layer = static_cast<ncos::models::face::FaceLayer>(i + 1);
    const FaceLayerPolicy policy = face_layer_policy(layer);

    FaceLayerOwnership& slot = baseline.composition.layers[i];
    slot.layer = layer;
    slot.owner_role = policy.required_owner_role;
    slot.owner_service = 0;
    slot.priority = policy.default_priority;
    slot.source_clip_id = 0;
  }

  baseline.active_trace_id = 0;
  baseline.owner_service = 0;
  return baseline;
}

bool is_valid(const FaceRenderState& state) {
  if (state.version != FaceRenderStateVersion::kV1) {
    return false;
  }

  if (!percent_in_range(state.eyes.focus_percent) || !percent_in_range(state.lids.openness_percent) ||
      !percent_in_range(state.mouth.openness_percent) ||
      !percent_in_range(state.brows.intensity_percent)) {
    return false;
  }

  for (size_t i = 0; i < ncos::models::face::kFaceLayerCount; ++i) {
    const FaceLayerOwnership& slot = state.composition.layers[i];
    const ncos::models::face::FaceLayer expected_layer =
        static_cast<ncos::models::face::FaceLayer>(i + 1);
    const FaceLayerPolicy policy = face_layer_policy(expected_layer);

    if (slot.layer != expected_layer) {
      return false;
    }

    if (slot.owner_role != policy.required_owner_role) {
      return false;
    }
  }

  return true;
}

bool can_apply_layer_claim(const FaceRenderState& state, const FaceLayerClaim& claim) {
  if (claim.requester_service == 0) {
    return false;
  }

  const FaceLayerPolicy policy = face_layer_policy(claim.layer);
  if (claim.requester_role != policy.required_owner_role) {
    return false;
  }

  const size_t idx = ncos::models::face::face_layer_index(claim.layer);
  const FaceLayerOwnership& current = state.composition.layers[idx];

  if (state.composition.composition_locked && current.owner_service != claim.requester_service) {
    return false;
  }

  if (current.owner_service == 0 || current.owner_service == claim.requester_service) {
    return true;
  }

  return claim.priority > current.priority;
}

bool apply_layer_claim(FaceRenderState* state, const FaceLayerClaim& claim, uint64_t now_ms) {
  if (state == nullptr || !is_valid(*state) || !can_apply_layer_claim(*state, claim)) {
    return false;
  }

  const size_t idx = ncos::models::face::face_layer_index(claim.layer);
  FaceLayerOwnership& current = state->composition.layers[idx];

  current.owner_service = claim.requester_service;
  current.priority = claim.priority;
  current.source_clip_id = claim.source_clip_id;

  state->owner_service = claim.requester_service;
  state->updated_at_ms = now_ms;
  ++state->revision;
  return true;
}

}  // namespace ncos::core::contracts

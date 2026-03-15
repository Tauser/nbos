#include "services/face/face_multimodal_sync.hpp"

namespace {

uint8_t clamp_percent_sync(uint32_t value) {
  return static_cast<uint8_t>(value > 100U ? 100U : value);
}

}  // namespace

namespace ncos::services::face {

bool FaceMultimodalSync::apply(const ncos::core::contracts::FaceMultimodalInput& input,
                               FaceCompositor* compositor,
                               ncos::core::contracts::FaceRenderState* state,
                               uint16_t owner_service,
                               uint64_t now_ms,
                               FaceMultimodalSyncResult* out_result) {
  if (compositor == nullptr || state == nullptr || owner_service == 0 ||
      !ncos::core::contracts::is_valid(*state)) {
    return false;
  }

  FaceLayerRequest request{};
  request.layer = ncos::models::face::FaceLayer::kModulation;
  request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kModulationOwner;
  request.requester_service = owner_service;
  request.priority = 4;
  request.hold_ms = 180;
  request.cooldown_ms = 100;

  if (!compositor->request_layer(request, now_ms).granted ||
      !compositor->can_write(ncos::models::face::FaceLayer::kModulation, owner_service)) {
    return false;
  }

  FaceMultimodalSyncResult result{};
  result.applied = true;

  const uint32_t expressiveness_mix =
      (static_cast<uint32_t>(input.audio_energy_percent) * 6U +
       static_cast<uint32_t>(input.touch_intensity_percent) * 3U +
       static_cast<uint32_t>(input.motion_intensity_percent) * 1U) /
      10U;

  result.target_focus_percent = clamp_percent_sync(32U + expressiveness_mix / 2U);
  result.target_lid_open_percent =
      clamp_percent_sync(100U - (input.motion_active ? input.motion_intensity_percent / 3U : 0U));
  result.target_brow_intensity_percent = clamp_percent_sync(expressiveness_mix / 2U);

  state->eyes.focus_percent = result.target_focus_percent;
  state->lids.openness_percent = result.target_lid_open_percent;
  state->brows.intensity_percent = result.target_brow_intensity_percent;
  state->updated_at_ms = now_ms;
  ++state->revision;

  if (out_result != nullptr) {
    *out_result = result;
  }

  return true;
}

}  // namespace ncos::services::face

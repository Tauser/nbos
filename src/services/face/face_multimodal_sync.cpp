#include "services/face/face_multimodal_sync.hpp"

namespace {

uint8_t clamp_percent_sync(uint32_t value) {
  return static_cast<uint8_t>(value > 100U ? 100U : value);
}

struct BlinkPattern {
  ncos::models::face::BlinkPhase phase = ncos::models::face::BlinkPhase::kOpen;
  uint8_t openness_percent = 100;
};

BlinkPattern resolve_blink_pattern(const ncos::core::contracts::FaceMultimodalInput& input,
                                   uint64_t now_ms,
                                   uint8_t baseline_open_percent) {
  const uint32_t engagement = (static_cast<uint32_t>(input.social_engagement_percent) * 3U +
                               static_cast<uint32_t>(input.behavior_activation_percent) * 2U +
                               static_cast<uint32_t>(input.emotional_arousal_percent)) /
                              6U;
  const uint32_t blink_period_ms = 3200U + (100U - engagement) * 18U;
  const uint32_t cycle = blink_period_ms == 0 ? 0 : static_cast<uint32_t>(now_ms % blink_period_ms);

  if (engagement >= 55U && cycle < 60U) {
    return {ncos::models::face::BlinkPhase::kClosing,
            clamp_percent_sync(static_cast<uint32_t>(baseline_open_percent) / 3U)};
  }
  if (engagement >= 55U && cycle < 115U) {
    return {ncos::models::face::BlinkPhase::kClosed, 0};
  }
  if (engagement >= 55U && cycle < 185U) {
    return {ncos::models::face::BlinkPhase::kOpening,
            clamp_percent_sync(static_cast<uint32_t>(baseline_open_percent) * 3U / 4U)};
  }

  const uint32_t micro_period_ms = 1400U + input.audio_energy_percent * 5U;
  const uint32_t micro_cycle = micro_period_ms == 0 ? 0 : static_cast<uint32_t>((now_ms + engagement * 17U) % micro_period_ms);
  if (micro_cycle < 35U) {
    return {ncos::models::face::BlinkPhase::kClosing,
            clamp_percent_sync(static_cast<uint32_t>(baseline_open_percent) * 3U / 5U)};
  }
  if (micro_cycle < 70U) {
    return {ncos::models::face::BlinkPhase::kOpening,
            clamp_percent_sync(static_cast<uint32_t>(baseline_open_percent) * 4U / 5U)};
  }

  return {ncos::models::face::BlinkPhase::kOpen, baseline_open_percent};
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

  const uint32_t sensor_mix =
      (static_cast<uint32_t>(input.audio_energy_percent) * 4U +
       static_cast<uint32_t>(input.touch_intensity_percent) * 2U +
       static_cast<uint32_t>(input.motion_intensity_percent) * 1U) /
      7U;

  const uint32_t semantic_mix =
      (static_cast<uint32_t>(input.emotional_arousal_percent) * 5U +
       static_cast<uint32_t>(input.social_engagement_percent) * 3U +
       static_cast<uint32_t>(input.behavior_activation_percent) * 2U) /
      10U;

  result.target_focus_percent = clamp_percent_sync(26U + sensor_mix / 3U + semantic_mix / 2U);
  const uint8_t baseline_lid_open_percent = clamp_percent_sync(
      100U - (input.motion_active ? input.motion_intensity_percent / 4U : 0U) -
      input.emotional_arousal_percent / 6U);
  const BlinkPattern blink = resolve_blink_pattern(input, now_ms, baseline_lid_open_percent);
  result.target_lid_open_percent = blink.openness_percent;
  result.target_brow_intensity_percent = clamp_percent_sync((sensor_mix + semantic_mix) / 2U);
  result.target_blink_phase = blink.phase;

  state->eyes.focus_percent = result.target_focus_percent;
  state->lids.phase = result.target_blink_phase;
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

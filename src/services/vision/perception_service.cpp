#include "services/vision/perception_service.hpp"

namespace {
constexpr uint8_t PresenceDetectedThreshold = 22;
constexpr uint8_t AttentionLockedThreshold = 56;
constexpr uint8_t TouchDominantThreshold = 35;
constexpr uint64_t CameraFreshnessWindowMs = 1600;
constexpr uint64_t TouchReleaseHoldMs = 180;
}

namespace ncos::services::vision {

bool PerceptionService::initialize(uint16_t service_id, uint64_t now_ms) {
  if (service_id == 0) {
    return false;
  }

  service_id_ = service_id;
  state_ = ncos::core::contracts::make_perception_runtime_baseline();
  state_.initialized = true;
  state_.last_update_ms = now_ms;
  return true;
}

bool PerceptionService::tick(const ncos::core::contracts::AudioRuntimeState& audio,
                             const ncos::core::contracts::TouchRuntimeState& touch,
                             const ncos::core::contracts::CameraRuntimeState& camera,
                             const ncos::core::contracts::CompanionSnapshot& companion,
                             uint64_t now_ms,
                             ncos::core::contracts::CompanionAttentionalSignal* out_attention,
                             ncos::core::contracts::CompanionInteractionSignal* out_interaction) {
  if (!state_.initialized || out_attention == nullptr || out_interaction == nullptr) {
    return false;
  }

  const bool had_presence = state_.presence_active;
  const bool had_attention = state_.attention_active;

  *out_attention = ncos::core::contracts::CompanionAttentionalSignal{};
  *out_interaction = ncos::core::contracts::CompanionInteractionSignal{};

  state_.last_update_ms = now_ms;
  ++state_.updates_total;

  if (companion.runtime.safe_mode ||
      companion.energetic.mode == ncos::core::contracts::EnergyMode::kCritical) {
    state_.presence_active = false;
    state_.attention_active = false;
    state_.presence_confidence_percent = 0;
    state_.attention_confidence_percent = 0;
    state_.attention_target = ncos::core::contracts::AttentionTarget::kNone;
    state_.attention_channel = ncos::core::contracts::AttentionChannel::kVisual;
    state_.stage = ncos::core::contracts::PerceptionStage::Dormant;
    state_.touch_release_hold_until_ms = 0;
    return had_presence || had_attention;
  }

  const uint8_t visual_confidence = visual_presence_confidence(camera, now_ms);
  const uint8_t auditory_confidence = auditory_presence_confidence(audio);
  uint8_t touch_confidence = touch_presence_confidence(touch);

  if (touch.trigger_active) {
    state_.touch_release_hold_until_ms = now_ms + TouchReleaseHoldMs;
  } else if (touch_confidence == 0 && state_.touch_release_hold_until_ms > now_ms &&
             visual_confidence < PresenceDetectedThreshold &&
             auditory_confidence < PresenceDetectedThreshold) {
    touch_confidence = TouchDominantThreshold;
  }

  const uint8_t presence_confidence =
      ncos::core::contracts::clamp_percent_u8(static_cast<uint16_t>(
          (static_cast<uint16_t>(visual_confidence) * 45U +
           static_cast<uint16_t>(auditory_confidence) * 25U +
           static_cast<uint16_t>(touch_confidence) * 30U) /
          100U));

  state_.presence_confidence_percent = presence_confidence;
  state_.presence_active = presence_confidence >= PresenceDetectedThreshold;

  choose_attention_channel(visual_confidence, auditory_confidence, touch_confidence, out_attention,
                           out_interaction);

  state_.attention_target = out_attention->target;
  state_.attention_channel = out_attention->channel;
  state_.attention_confidence_percent = out_attention->focus_confidence_percent;
  state_.attention_active = out_attention->lock_active ||
                            state_.attention_confidence_percent >= AttentionLockedThreshold;

  if (state_.attention_active) {
    state_.stage = ncos::core::contracts::PerceptionStage::AttentionLocked;
    state_.last_attention_ms = now_ms;
  } else if (state_.presence_active) {
    state_.stage = ncos::core::contracts::PerceptionStage::PresenceDetected;
    state_.last_presence_ms = now_ms;
  } else {
    state_.stage = ncos::core::contracts::PerceptionStage::Dormant;
  }

  return state_.presence_active || state_.attention_active || had_presence || had_attention;
}

const ncos::core::contracts::PerceptionRuntimeState& PerceptionService::state() const {
  return state_;
}

uint8_t PerceptionService::visual_presence_confidence(
    const ncos::core::contracts::CameraRuntimeState& camera, uint64_t now_ms) {
  if (!camera.initialized || !camera.capture_ready || !camera.last_capture_ok) {
    return 0;
  }

  if (camera.last_capture_ms == 0 || (now_ms - camera.last_capture_ms) > CameraFreshnessWindowMs) {
    return 0;
  }

  uint16_t confidence = 35;
  if (camera.last_frame_width >= 200 && camera.last_frame_height >= 120) {
    confidence += 20;
  }
  if (camera.last_frame_bytes >= 900) {
    confidence += 15;
  }

  return ncos::core::contracts::clamp_percent_u8(confidence);
}

uint8_t PerceptionService::auditory_presence_confidence(
    const ncos::core::contracts::AudioRuntimeState& audio) {
  if (!audio.initialized || !audio.input_ready || !audio.last_capture_ok ||
      audio.last_capture_samples == 0) {
    return 0;
  }

  const uint32_t peak = audio.last_peak_level < 0 ? static_cast<uint32_t>(-audio.last_peak_level)
                                                   : static_cast<uint32_t>(audio.last_peak_level);
  const uint16_t scaled = static_cast<uint16_t>((peak * 100U) / 32767U);
  return ncos::core::contracts::clamp_percent_u8(scaled);
}

uint8_t PerceptionService::touch_presence_confidence(
    const ncos::core::contracts::TouchRuntimeState& touch) {
  if (!touch.initialized) {
    return 0;
  }

  const uint8_t normalized_confidence =
      ncos::core::contracts::clamp_percent_u8(touch.normalized_level / 10U);
  if (touch.trigger_active) {
    return normalized_confidence >= TouchDominantThreshold ? normalized_confidence
                                                           : TouchDominantThreshold;
  }

  if (!touch.last_read_ok) {
    return 0;
  }

  return normalized_confidence;
}

void PerceptionService::choose_attention_channel(
    uint8_t visual_confidence,
    uint8_t auditory_confidence,
    uint8_t touch_confidence,
    ncos::core::contracts::CompanionAttentionalSignal* out_attention,
    ncos::core::contracts::CompanionInteractionSignal* out_interaction) {
  if (out_attention == nullptr || out_interaction == nullptr) {
    return;
  }

  if (touch_confidence >= TouchDominantThreshold) {
    out_attention->target = ncos::core::contracts::AttentionTarget::kUser;
    out_attention->channel = ncos::core::contracts::AttentionChannel::kTouch;
    out_attention->focus_confidence_percent = touch_confidence;
    out_attention->lock_active = true;

    out_interaction->phase = ncos::core::contracts::InteractionPhase::kActing;
    out_interaction->turn_owner = ncos::core::contracts::TurnOwner::kUser;
    out_interaction->session_active = true;
    out_interaction->response_pending = false;
    return;
  }

  out_interaction->session_active = false;

  if (visual_confidence >= auditory_confidence && visual_confidence >= PresenceDetectedThreshold) {
    out_attention->target = ncos::core::contracts::AttentionTarget::kUser;
    out_attention->channel = ncos::core::contracts::AttentionChannel::kVisual;
    out_attention->focus_confidence_percent = visual_confidence;
    out_attention->lock_active = visual_confidence >= AttentionLockedThreshold;

    out_interaction->phase = ncos::core::contracts::InteractionPhase::kListening;
    out_interaction->turn_owner = ncos::core::contracts::TurnOwner::kUser;
    out_interaction->session_active = true;
    out_interaction->response_pending = false;
    return;
  }

  if (auditory_confidence >= PresenceDetectedThreshold) {
    out_attention->target = ncos::core::contracts::AttentionTarget::kUser;
    out_attention->channel = ncos::core::contracts::AttentionChannel::kAuditory;
    out_attention->focus_confidence_percent = auditory_confidence;
    out_attention->lock_active = auditory_confidence >= AttentionLockedThreshold;

    out_interaction->phase = ncos::core::contracts::InteractionPhase::kListening;
    out_interaction->turn_owner = ncos::core::contracts::TurnOwner::kUser;
    out_interaction->session_active = true;
    out_interaction->response_pending = auditory_confidence >= 70;
    return;
  }

  out_attention->target = ncos::core::contracts::AttentionTarget::kNone;
  out_attention->channel = ncos::core::contracts::AttentionChannel::kVisual;
  out_attention->focus_confidence_percent = 0;
  out_attention->lock_active = false;

  out_interaction->phase = ncos::core::contracts::InteractionPhase::kIdle;
  out_interaction->turn_owner = ncos::core::contracts::TurnOwner::kNone;
  out_interaction->session_active = false;
  out_interaction->response_pending = false;
}

}  // namespace ncos::services::vision


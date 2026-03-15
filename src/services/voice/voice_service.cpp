#include "services/voice/voice_service.hpp"

namespace ncos::services::voice {

bool VoiceService::initialize(uint16_t service_id, uint64_t now_ms) {
  if (service_id == 0) {
    return false;
  }

  service_id_ = service_id;
  state_ = ncos::core::contracts::make_voice_runtime_baseline();
  state_.initialized = true;
  state_.last_update_ms = now_ms;
  return true;
}

bool VoiceService::tick(const ncos::core::contracts::AudioRuntimeState& audio,
                        const ncos::core::contracts::CompanionSnapshot& companion,
                        uint64_t now_ms,
                        ncos::core::contracts::CompanionAttentionalSignal* out_attention,
                        ncos::core::contracts::CompanionInteractionSignal* out_interaction) {
  if (!state_.initialized || out_attention == nullptr || out_interaction == nullptr) {
    return false;
  }

  *out_attention = ncos::core::contracts::CompanionAttentionalSignal{};
  *out_interaction = ncos::core::contracts::CompanionInteractionSignal{};

  state_.input_available = audio.input_ready;
  state_.last_update_ms = now_ms;
  state_.energy_percent = ncos::core::contracts::voice_energy_percent_from_audio(audio);

  if (!audio.input_ready || companion.runtime.safe_mode ||
      companion.energetic.mode == ncos::core::contracts::EnergyMode::kCritical) {
    state_.speech_active = false;
    state_.trigger_candidate = false;
    state_.consecutive_speech_frames = 0;
    state_.silence_frames = 0;
    state_.stage = ncos::core::contracts::VoiceStage::Dormant;
    return false;
  }

  state_.speech_active = state_.energy_percent >= SpeechThresholdPercent;

  if (state_.speech_active) {
    ++state_.consecutive_speech_frames;
    state_.silence_frames = 0;
    ++state_.speech_frames_total;
  } else {
    state_.consecutive_speech_frames = 0;
    ++state_.silence_frames;
  }

  const bool cooldown_elapsed =
      (state_.last_trigger_ms == 0) || ((now_ms - state_.last_trigger_ms) >= TriggerCooldownMs);

  state_.trigger_candidate = state_.energy_percent >= TriggerThresholdPercent &&
                             state_.consecutive_speech_frames >= TriggerSpeechFrames &&
                             cooldown_elapsed;

  if (state_.trigger_candidate) {
    state_.stage = ncos::core::contracts::VoiceStage::TriggerCandidate;
    state_.last_trigger_ms = now_ms;
    ++state_.trigger_candidates_total;
  } else if (state_.speech_active) {
    state_.stage = ncos::core::contracts::VoiceStage::Listening;
  } else {
    state_.stage = ncos::core::contracts::VoiceStage::Dormant;
  }

  if (!(state_.speech_active || state_.trigger_candidate)) {
    return false;
  }

  out_attention->target = ncos::core::contracts::AttentionTarget::kUser;
  out_attention->channel = ncos::core::contracts::AttentionChannel::kAuditory;
  out_attention->focus_confidence_percent = static_cast<uint8_t>(45 + state_.energy_percent / 2);
  out_attention->lock_active = state_.trigger_candidate;

  out_interaction->phase = ncos::core::contracts::InteractionPhase::kListening;
  out_interaction->turn_owner = ncos::core::contracts::TurnOwner::kUser;
  out_interaction->session_active = true;
  out_interaction->response_pending = state_.trigger_candidate;

  return true;
}

const ncos::core::contracts::VoiceRuntimeState& VoiceService::state() const {
  return state_;
}

}  // namespace ncos::services::voice


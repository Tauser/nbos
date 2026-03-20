#include "services/voice/voice_service.hpp"

namespace {

bool is_capture_fresh(const ncos::core::contracts::AudioRuntimeState& audio, uint64_t now_ms,
                      uint64_t freshness_window_ms) {
  if (!audio.last_capture_ok || audio.last_capture_ms == 0 || now_ms < audio.last_capture_ms) {
    return false;
  }

  return (now_ms - audio.last_capture_ms) <= freshness_window_ms;
}

bool is_capture_usable(const ncos::core::contracts::AudioRuntimeState& audio, uint64_t now_ms,
                       size_t min_capture_samples, uint64_t freshness_window_ms) {
  return audio.input_ready && audio.last_capture_samples >= min_capture_samples &&
         is_capture_fresh(audio, now_ms, freshness_window_ms);
}

ncos::core::contracts::IntentTopic classify_constrained_intent(
    const ncos::core::contracts::CompanionSnapshot& companion) {
  if (companion.energetic.mode == ncos::core::contracts::EnergyMode::kConstrained) {
    return ncos::core::contracts::IntentTopic::kPreserveEnergy;
  }

  if (companion.attentional.target == ncos::core::contracts::AttentionTarget::kStimulus ||
      companion.session.recent_stimulus.target == ncos::core::contracts::AttentionTarget::kStimulus) {
    return ncos::core::contracts::IntentTopic::kInspectStimulus;
  }

  if (companion.interactional.session_active || companion.session.warm ||
      companion.session.last_turn_owner == ncos::core::contracts::TurnOwner::kUser ||
      companion.session.recent_interaction.turn_owner == ncos::core::contracts::TurnOwner::kUser) {
    return ncos::core::contracts::IntentTopic::kAcknowledgeUser;
  }

  return ncos::core::contracts::IntentTopic::kAttendUser;
}

ncos::core::contracts::VoiceResponsePlan make_response_plan(
    ncos::core::contracts::IntentTopic intent) {
  ncos::core::contracts::VoiceResponsePlan plan{};
  plan.valid = true;
  plan.intent = intent;
  plan.interaction_phase = ncos::core::contracts::InteractionPhase::kResponding;
  plan.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  plan.session_active = true;
  plan.response_pending = false;

  switch (intent) {
    case ncos::core::contracts::IntentTopic::kAcknowledgeUser:
      plan.cue = ncos::core::contracts::VoiceResponseCue::AcknowledgeChirp;
      plan.tone_frequency_hz = 880.0F;
      plan.tone_duration_ms = 90;
      break;

    case ncos::core::contracts::IntentTopic::kInspectStimulus:
      plan.cue = ncos::core::contracts::VoiceResponseCue::StimulusChirp;
      plan.tone_frequency_hz = 620.0F;
      plan.tone_duration_ms = 110;
      break;

    case ncos::core::contracts::IntentTopic::kPreserveEnergy:
      plan.cue = ncos::core::contracts::VoiceResponseCue::EnergySoftChirp;
      plan.tone_frequency_hz = 440.0F;
      plan.tone_duration_ms = 80;
      break;

    case ncos::core::contracts::IntentTopic::kAttendUser:
    default:
      plan.cue = ncos::core::contracts::VoiceResponseCue::WakeChirp;
      plan.tone_frequency_hz = 740.0F;
      plan.tone_duration_ms = 70;
      break;
  }

  return plan;
}

}  // namespace

namespace ncos::services::voice {

bool VoiceService::initialize(uint16_t service_id, uint64_t now_ms) {
  if (service_id == 0) {
    return false;
  }

  service_id_ = service_id;
  state_ = ncos::core::contracts::make_voice_runtime_baseline();
  pending_response_plan_ = ncos::core::contracts::VoiceResponsePlan{};
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

  const bool capture_usable =
      is_capture_usable(audio, now_ms, MinCaptureSamples, CaptureFreshnessMs);

  state_.input_available = capture_usable;
  state_.last_update_ms = now_ms;
  state_.energy_percent = ncos::core::contracts::voice_energy_percent_from_audio(audio);

  if (!capture_usable || companion.runtime.safe_mode ||
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
  const bool trigger_energy_ready = state_.energy_percent >= TriggerThresholdPercent;

  state_.trigger_candidate =
      trigger_energy_ready && state_.consecutive_speech_frames >= TriggerSpeechFrames &&
      cooldown_elapsed;

  if (state_.trigger_candidate) {
    state_.stage = ncos::core::contracts::VoiceStage::TriggerCandidate;
    state_.last_trigger_ms = now_ms;
    ++state_.trigger_candidates_total;

    const auto detected_intent = classify_constrained_intent(companion);
    pending_response_plan_ = make_response_plan(detected_intent);
    state_.last_detected_intent = detected_intent;
    state_.last_response_cue = pending_response_plan_.cue;
    ++state_.response_plan_total;
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

bool VoiceService::take_response_plan(ncos::core::contracts::VoiceResponsePlan* out_plan) {
  if (out_plan == nullptr || !pending_response_plan_.valid) {
    return false;
  }

  *out_plan = pending_response_plan_;
  pending_response_plan_ = ncos::core::contracts::VoiceResponsePlan{};
  return true;
}

const ncos::core::contracts::VoiceRuntimeState& VoiceService::state() const {
  return state_;
}

}  // namespace ncos::services::voice

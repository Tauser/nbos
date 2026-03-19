#include "core/contracts/companion_state_contracts.hpp"

namespace {

ncos::models::emotion::EmotionVector legacy_vector_from_tone(
    ncos::core::contracts::EmotionalTone tone,
    ncos::core::contracts::EmotionalArousal arousal,
    uint8_t intensity_percent) {
  ncos::models::emotion::EmotionVector vector{};

  switch (tone) {
    case ncos::core::contracts::EmotionalTone::kAlert:
      vector.valence_percent = -20;
      vector.social_engagement_percent = 35;
      break;
    case ncos::core::contracts::EmotionalTone::kAffiliative:
      vector.valence_percent = 40;
      vector.social_engagement_percent = 78;
      break;
    case ncos::core::contracts::EmotionalTone::kCurious:
      vector.valence_percent = 10;
      vector.social_engagement_percent = 58;
      break;
    case ncos::core::contracts::EmotionalTone::kNeutral:
    default:
      vector.valence_percent = 0;
      vector.social_engagement_percent = 45;
      break;
  }

  switch (arousal) {
    case ncos::core::contracts::EmotionalArousal::kHigh:
      vector.arousal_percent = 82;
      break;
    case ncos::core::contracts::EmotionalArousal::kMedium:
      vector.arousal_percent = 52;
      break;
    case ncos::core::contracts::EmotionalArousal::kLow:
    default:
      vector.arousal_percent = 22;
      break;
  }

  const uint8_t weighted_intensity = ncos::models::emotion::clamp_percent(intensity_percent / 3U);
  vector.arousal_percent = ncos::models::emotion::clamp_percent(
      static_cast<int16_t>(vector.arousal_percent + weighted_intensity));

  return ncos::models::emotion::normalize_vector(vector);
}

}  // namespace

namespace ncos::core::contracts {

GovernanceHealth evaluate_governance_health(uint32_t allowed_total, uint32_t preempted_total,
                                            uint32_t rejected_total) {
  const uint32_t accepted = allowed_total + preempted_total;
  if (rejected_total == 0 && accepted == 0) {
    return GovernanceHealth::kUnknown;
  }

  if (rejected_total >= 3 && accepted == 0) {
    return GovernanceHealth::kContended;
  }

  return GovernanceHealth::kStable;
}

EmotionalTone tone_from_emotion_model(const ncos::models::emotion::EmotionModelState& model) {
  const auto& vector = model.vector;

  if (vector.arousal_percent >= 70 && vector.valence_percent <= 0) {
    return EmotionalTone::kAlert;
  }

  if (vector.social_engagement_percent >= 62 && vector.valence_percent >= 15) {
    return EmotionalTone::kAffiliative;
  }

  if (vector.arousal_percent >= 34 || (vector.valence_percent > -18 && vector.valence_percent < 18)) {
    return EmotionalTone::kCurious;
  }

  return EmotionalTone::kNeutral;
}

EmotionalArousal arousal_from_emotion_model(const ncos::models::emotion::EmotionModelState& model) {
  if (model.vector.arousal_percent >= 70) {
    return EmotionalArousal::kHigh;
  }

  if (model.vector.arousal_percent >= 35) {
    return EmotionalArousal::kMedium;
  }

  return EmotionalArousal::kLow;
}

CompanionEmotionalSignal normalize_emotional_signal(const CompanionEmotionalSignal& signal) {
  CompanionEmotionalSignal out = signal;

  ncos::models::emotion::EmotionModelState model{};
  model.intensity_percent = ncos::models::emotion::clamp_percent(signal.intensity_percent);
  model.stability_percent = ncos::models::emotion::clamp_percent(signal.stability_percent);

  if (signal.vector_authoritative) {
    model.vector = ncos::models::emotion::normalize_vector(signal.vector);
  } else {
    model.vector = legacy_vector_from_tone(signal.tone, signal.arousal, model.intensity_percent);
  }

  model = ncos::models::emotion::normalize_model(model);

  out.vector = model.vector;
  out.tone = tone_from_emotion_model(model);
  out.arousal = arousal_from_emotion_model(model);
  out.intensity_percent = model.intensity_percent;
  out.stability_percent = model.stability_percent;
  return out;
}

bool can_writer_mutate_domain(CompanionStateWriter writer, CompanionStateDomain domain) {
  switch (writer) {
    case CompanionStateWriter::kBootstrap:
      return domain == CompanionStateDomain::kStructural ||
             domain == CompanionStateDomain::kPersonality;
    case CompanionStateWriter::kRuntimeCore:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance;
    case CompanionStateWriter::kGovernanceCore:
      return domain == CompanionStateDomain::kTransient;
    case CompanionStateWriter::kEmotionService:
      return domain == CompanionStateDomain::kEmotional;
    case CompanionStateWriter::kAttentionService:
      return domain == CompanionStateDomain::kAttentional;
    case CompanionStateWriter::kPowerService:
      return domain == CompanionStateDomain::kEnergetic;
    case CompanionStateWriter::kInteractionService:
      return domain == CompanionStateDomain::kInteractional;
    default:
      return false;
  }
}

bool can_reader_observe_domain(CompanionStateReader reader, CompanionStateDomain domain) {
  switch (reader) {
    case CompanionStateReader::kRuntimeCore:
      return true;
    case CompanionStateReader::kBehaviorService:
      return domain != CompanionStateDomain::kStructural;
    case CompanionStateReader::kFaceService:
      return domain == CompanionStateDomain::kPersonality ||
             domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kEmotional ||
             domain == CompanionStateDomain::kAttentional ||
             domain == CompanionStateDomain::kInteractional ||
             domain == CompanionStateDomain::kSessionMemory;
    case CompanionStateReader::kMotionService:
      return domain == CompanionStateDomain::kPersonality ||
             domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kAttentional ||
             domain == CompanionStateDomain::kEnergetic ||
             domain == CompanionStateDomain::kInteractional ||
             domain == CompanionStateDomain::kSessionMemory;
    case CompanionStateReader::kVoiceService:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kEmotional ||
             domain == CompanionStateDomain::kAttentional ||
             domain == CompanionStateDomain::kEnergetic ||
             domain == CompanionStateDomain::kInteractional ||
             domain == CompanionStateDomain::kSessionMemory;
    case CompanionStateReader::kPowerService:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kEnergetic;
    case CompanionStateReader::kDiagnostics:
      return domain == CompanionStateDomain::kStructural || domain == CompanionStateDomain::kPersonality ||
             domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kEnergetic ||
             domain == CompanionStateDomain::kTransient ||
             domain == CompanionStateDomain::kSessionMemory;
    case CompanionStateReader::kCloudBridge:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kInteractional;
    default:
      return false;
  }
}

CompanionSnapshot redact_snapshot_for_reader(const CompanionSnapshot& source,
                                             CompanionStateReader reader) {
  CompanionSnapshot redacted = source;

  if (!can_reader_observe_domain(reader, CompanionStateDomain::kStructural)) {
    redacted.structural = CompanionStructuralState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kPersonality)) {
    redacted.personality = CompanionPersonalityState{};
    redacted.personality.profile_name = nullptr;
    redacted.personality.warmth_percent = 0;
    redacted.personality.curiosity_percent = 0;
    redacted.personality.composure_percent = 0;
    redacted.personality.initiative_percent = 0;
    redacted.personality.assertiveness_percent = 0;
    redacted.personality.adaptive_social_warmth_bias_percent = 0;
    redacted.personality.adaptive_response_energy_bias_percent = 0;
    redacted.personality.adaptive_continuity_window_bias_ms = 0;
    redacted.personality.behavior_energy_protect_ttl_ms = 0;
    redacted.personality.behavior_alert_scan_ttl_ms = 0;
    redacted.personality.behavior_attend_user_ttl_ms = 0;
    redacted.personality.reengagement_ttl_ms = 0;
    redacted.personality.user_continuity_window_ms = 0;
    redacted.personality.stimulus_continuity_window_ms = 0;
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kRuntime)) {
    redacted.runtime = CompanionRuntimeState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kGovernance)) {
    redacted.governance = CompanionGovernanceState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kEmotional)) {
    redacted.emotional = CompanionEmotionalState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kAttentional)) {
    redacted.attentional = CompanionAttentionalState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kEnergetic)) {
    redacted.energetic = CompanionEnergeticState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kInteractional)) {
    redacted.interactional = CompanionInteractionState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kTransient)) {
    redacted.transient = CompanionTransientState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kSessionMemory)) {
    redacted.session = CompanionSessionMemoryState{};
  }

  return redacted;
}

}  // namespace ncos::core::contracts


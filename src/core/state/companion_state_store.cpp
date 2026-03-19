#include "core/state/companion_state_store.hpp"

#include "core/contracts/companion_personality_contracts.hpp"

namespace {
constexpr uint64_t AttendUserHoldMs = 1400;
constexpr uint64_t AlertScanHoldMs = 1600;
constexpr uint64_t RespondingHoldMs = 900;
constexpr uint64_t IdleToSleepMs = 12000;
constexpr uint64_t SessionMemoryRetentionMs = 18000;
constexpr int8_t AdaptiveBiasStepPercent = 2;
constexpr int16_t AdaptiveContinuityStepMs = 120;

int8_t move_toward_i8(int8_t current, int8_t target, int8_t step) {
  if (current < target) {
    const int16_t advanced = static_cast<int16_t>(current) + step;
    return advanced > target ? target : static_cast<int8_t>(advanced);
  }
  if (current > target) {
    const int16_t reduced = static_cast<int16_t>(current) - step;
    return reduced < target ? target : static_cast<int8_t>(reduced);
  }
  return current;
}

int16_t move_toward_i16(int16_t current, int16_t target, int16_t step) {
  if (current < target) {
    const int32_t advanced = static_cast<int32_t>(current) + step;
    return advanced > target ? target : static_cast<int16_t>(advanced);
  }
  if (current > target) {
    const int32_t reduced = static_cast<int32_t>(current) - step;
    return reduced < target ? target : static_cast<int16_t>(reduced);
  }
  return current;
}

uint8_t engagement_bonus(uint8_t engagement_percent, uint8_t floor_percent, uint8_t divisor, uint8_t cap) {
  if (engagement_percent <= floor_percent || divisor == 0) {
    return 0;
  }
  const uint8_t bonus = static_cast<uint8_t>((engagement_percent - floor_percent) / divisor);
  return bonus > cap ? cap : bonus;
}
}

namespace ncos::core::state {

bool CompanionStateStore::initialize(const ncos::core::contracts::CompanionStructuralState& structural,
                                     ncos::core::contracts::CompanionStateWriter writer,
                                     uint64_t now_ms) {
  if (!authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kStructural)) {
    return false;
  }

  snapshot_.structural = structural;
  snapshot_.personality = ncos::core::contracts::make_companion_personality_state();
  snapshot_.runtime.last_transition_cause =
      ncos::core::contracts::CompanionStateTransitionCause::kBootstrap;
  snapshot_.runtime.last_state_change_ms = now_ms;
  refresh_derived_runtime_state(now_ms);
  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
  return true;
}

bool CompanionStateStore::ingest_runtime(const ncos::core::contracts::CompanionRuntimeSignal& runtime,
                                         ncos::core::contracts::CompanionStateWriter writer,
                                         uint64_t now_ms) {
  if (!authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kRuntime) ||
      !authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kGovernance)) {
    return false;
  }

  snapshot_.runtime.initialized = runtime.initialized;
  snapshot_.runtime.started = runtime.started;
  snapshot_.runtime.safe_mode = runtime.safe_mode;
  snapshot_.runtime.scheduler_tasks = runtime.scheduler_tasks;
  snapshot_.runtime.fault_count = runtime.fault_count;

  snapshot_.governance.allowed_total = runtime.governance_allowed_total;
  snapshot_.governance.preempted_total = runtime.governance_preempted_total;
  snapshot_.governance.rejected_total = runtime.governance_rejected_total;
  snapshot_.governance.health = ncos::core::contracts::evaluate_governance_health(
      runtime.governance_allowed_total, runtime.governance_preempted_total,
      runtime.governance_rejected_total);

  if (runtime.safe_mode) {
    snapshot_.energetic.mode = ncos::core::contracts::EnergyMode::kCritical;
    snapshot_.interactional.phase = ncos::core::contracts::InteractionPhase::kIdle;
    snapshot_.interactional.response_pending = false;
  }

  if (!runtime.started) {
    snapshot_.interactional.session_active = false;
    snapshot_.interactional.turn_owner = ncos::core::contracts::TurnOwner::kNone;
    snapshot_.interactional.phase = ncos::core::contracts::InteractionPhase::kIdle;
  }

  refresh_derived_runtime_state(now_ms);
  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
  return true;
}

bool CompanionStateStore::ingest_emotional(
    const ncos::core::contracts::CompanionEmotionalSignal& emotional,
    ncos::core::contracts::CompanionStateWriter writer, uint64_t now_ms) {
  if (!authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kEmotional)) {
    return false;
  }

  const ncos::core::contracts::CompanionEmotionalSignal normalized =
      ncos::core::contracts::normalize_emotional_signal(emotional);

  ncos::models::emotion::EmotionModelState emotion_model{};
  emotion_model.vector = normalized.vector;
  emotion_model.intensity_percent = normalized.intensity_percent;
  emotion_model.stability_percent = normalized.stability_percent;
  emotion_model = ncos::models::emotion::normalize_model(emotion_model);

  snapshot_.emotional.tone = normalized.tone;
  snapshot_.emotional.arousal = normalized.arousal;
  snapshot_.emotional.vector = emotion_model.vector;
  snapshot_.emotional.phase = emotion_model.phase;
  snapshot_.emotional.intensity_percent = emotion_model.intensity_percent;
  snapshot_.emotional.stability_percent = emotion_model.stability_percent;

  refresh_derived_runtime_state(now_ms);
  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
  return true;
}

bool CompanionStateStore::ingest_attentional(
    const ncos::core::contracts::CompanionAttentionalSignal& attentional,
    ncos::core::contracts::CompanionStateWriter writer, uint64_t now_ms) {
  if (!authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kAttentional)) {
    return false;
  }

  snapshot_.attentional.target = attentional.target;
  snapshot_.attentional.channel = attentional.channel;
  snapshot_.attentional.focus_confidence_percent = attentional.focus_confidence_percent;
  snapshot_.attentional.lock_active = attentional.lock_active;

  if (attentional.target == ncos::core::contracts::AttentionTarget::kUser &&
      attentional.focus_confidence_percent >= 40) {
    snapshot_.interactional.session_active = true;
  }

  refresh_derived_runtime_state(now_ms);
  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
  return true;
}

bool CompanionStateStore::ingest_energetic(
    const ncos::core::contracts::CompanionEnergeticSignal& energetic,
    ncos::core::contracts::CompanionStateWriter writer, uint64_t now_ms) {
  if (!authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kEnergetic)) {
    return false;
  }

  snapshot_.energetic.mode = energetic.mode;
  snapshot_.energetic.battery_percent = energetic.battery_percent;
  snapshot_.energetic.thermal_load_percent = energetic.thermal_load_percent;
  snapshot_.energetic.external_power = energetic.external_power;

  refresh_derived_runtime_state(now_ms);
  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
  return true;
}

bool CompanionStateStore::ingest_interactional(
    const ncos::core::contracts::CompanionInteractionSignal& interactional,
    ncos::core::contracts::CompanionStateWriter writer, uint64_t now_ms) {
  if (!authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kInteractional)) {
    return false;
  }

  snapshot_.interactional.phase = interactional.phase;
  snapshot_.interactional.turn_owner = interactional.turn_owner;
  snapshot_.interactional.session_active = interactional.session_active;
  snapshot_.interactional.response_pending = interactional.response_pending;

  refresh_derived_runtime_state(now_ms);
  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
  return true;
}

bool CompanionStateStore::ingest_governance_decision(
    const ncos::core::contracts::GovernanceDecision& decision,
    ncos::core::contracts::CompanionStateWriter writer, uint64_t now_ms) {
  if (!authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kTransient)) {
    return false;
  }

  if (decision.kind == ncos::core::contracts::GovernanceDecisionKind::kAllow ||
      decision.kind == ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow) {
    snapshot_.transient.has_active_trace = true;
    snapshot_.transient.active_trace_id = decision.proposal_trace_id;
    snapshot_.transient.active_domain = decision.domain;
    snapshot_.transient.active_owner_service = decision.owner_service;
    snapshot_.transient.last_transition_ms = now_ms;
  }

  if (decision.kind == ncos::core::contracts::GovernanceDecisionKind::kReject &&
      decision.reject_reason == ncos::core::contracts::GovernanceRejectReason::kSemanticDebounced) {
    snapshot_.transient.last_transition_ms = now_ms;
  }

  refresh_derived_runtime_state(now_ms);
  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
  return true;
}

ncos::core::contracts::CompanionSnapshot CompanionStateStore::snapshot_for(
    ncos::core::contracts::CompanionStateReader reader) const {
  return ncos::core::contracts::redact_snapshot_for_reader(snapshot_, reader);
}

bool CompanionStateStore::energy_protect_active(const ncos::core::contracts::CompanionSnapshot& snapshot) {
  return snapshot.runtime.safe_mode ||
         snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kCritical ||
         (snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kConstrained &&
          snapshot.energetic.battery_percent <= 18 && !snapshot.energetic.external_power);
}

bool CompanionStateStore::user_attention_active(const ncos::core::contracts::CompanionSnapshot& snapshot) {
  return snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kUser &&
         (snapshot.attentional.lock_active || snapshot.attentional.focus_confidence_percent >= 45 ||
          snapshot.interactional.session_active);
}

bool CompanionStateStore::stimulus_attention_active(const ncos::core::contracts::CompanionSnapshot& snapshot) {
  return snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kStimulus &&
         snapshot.attentional.focus_confidence_percent >= 50;
}

bool CompanionStateStore::response_active(const ncos::core::contracts::CompanionSnapshot& snapshot) {
  return snapshot.interactional.phase == ncos::core::contracts::InteractionPhase::kResponding ||
         snapshot.interactional.turn_owner == ncos::core::contracts::TurnOwner::kCompanion ||
         snapshot.interactional.response_pending;
}

ncos::core::contracts::CompanionPresenceMode CompanionStateStore::derive_presence_mode(
    const ncos::core::contracts::CompanionSnapshot& snapshot) {
  if (!snapshot.runtime.started || snapshot.runtime.safe_mode ||
      snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kCritical) {
    return ncos::core::contracts::CompanionPresenceMode::kDormant;
  }

  switch (snapshot.runtime.product_state) {
    case ncos::core::contracts::CompanionProductState::kAttendUser:
    case ncos::core::contracts::CompanionProductState::kAlertScan:
    case ncos::core::contracts::CompanionProductState::kResponding:
      return ncos::core::contracts::CompanionPresenceMode::kAttending;
    case ncos::core::contracts::CompanionProductState::kBooting:
    case ncos::core::contracts::CompanionProductState::kEnergyProtect:
      return ncos::core::contracts::CompanionPresenceMode::kDormant;
    case ncos::core::contracts::CompanionProductState::kSleep:
    case ncos::core::contracts::CompanionProductState::kIdleObserve:
    default:
      return ncos::core::contracts::CompanionPresenceMode::kIdle;
  }
}

uint64_t CompanionStateStore::hold_duration_for_state(
    ncos::core::contracts::CompanionProductState state) {
  switch (state) {
    case ncos::core::contracts::CompanionProductState::kAttendUser:
      return AttendUserHoldMs;
    case ncos::core::contracts::CompanionProductState::kAlertScan:
      return AlertScanHoldMs;
    case ncos::core::contracts::CompanionProductState::kResponding:
      return RespondingHoldMs;
    default:
      return 0;
  }
}

bool CompanionStateStore::session_activity_present(
    const ncos::core::contracts::CompanionSnapshot& snapshot) {
  return user_attention_active(snapshot) || stimulus_attention_active(snapshot) ||
         response_active(snapshot) || snapshot.interactional.session_active;
}

uint8_t CompanionStateStore::derive_session_engagement_percent(
    const ncos::core::contracts::CompanionSnapshot& snapshot, bool user_now, bool stimulus_now,
    bool response_now) {
  uint8_t engagement = 0;

  if (user_now) {
    engagement = snapshot.attentional.focus_confidence_percent > 55
                     ? snapshot.attentional.focus_confidence_percent
                     : static_cast<uint8_t>(55);
  }

  if (stimulus_now) {
    const uint8_t stimulus_engagement = snapshot.attentional.focus_confidence_percent > 42
                                            ? snapshot.attentional.focus_confidence_percent
                                            : static_cast<uint8_t>(42);
    engagement = stimulus_engagement > engagement ? stimulus_engagement : engagement;
  }

  if (snapshot.interactional.session_active && engagement < 60) {
    engagement = 60;
  }

  if (snapshot.interactional.turn_owner == ncos::core::contracts::TurnOwner::kCompanion &&
      engagement < 68) {
    engagement = 68;
  }

  if (response_now && engagement < 72) {
    engagement = 72;
  }

  return engagement;
}

bool CompanionStateStore::user_session_continuity_active(
    const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  if (!snapshot.session.warm || snapshot.session.last_activity_ms == 0 ||
      snapshot.session.last_activity_ms > now_ms) {
    return false;
  }

  const bool user_anchored =
      snapshot.session.anchor_target == ncos::core::contracts::AttentionTarget::kUser ||
      snapshot.session.recent_stimulus.target == ncos::core::contracts::AttentionTarget::kUser ||
      snapshot.session.recent_interaction.phase == ncos::core::contracts::InteractionPhase::kResponding ||
      snapshot.session.recent_interaction.turn_owner != ncos::core::contracts::TurnOwner::kNone;

  return user_anchored &&
         snapshot.session.engagement_recent_percent >=
             ncos::core::contracts::personality_continuity_engagement_threshold_percent(
                 snapshot.personality, ncos::core::contracts::PersonalityContinuityKind::kUser) &&
         (now_ms - snapshot.session.last_activity_ms) <=
             ncos::core::contracts::personality_continuity_window_ms(
                 snapshot.personality, ncos::core::contracts::PersonalityContinuityKind::kUser);
}

bool CompanionStateStore::stimulus_session_continuity_active(
    const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  if (!snapshot.session.warm || snapshot.session.last_activity_ms == 0 ||
      snapshot.session.last_activity_ms > now_ms) {
    return false;
  }

  return snapshot.session.recent_stimulus.target == ncos::core::contracts::AttentionTarget::kStimulus &&
         snapshot.session.engagement_recent_percent >=
             ncos::core::contracts::personality_continuity_engagement_threshold_percent(
                 snapshot.personality, ncos::core::contracts::PersonalityContinuityKind::kStimulus) &&
         (now_ms - snapshot.session.last_activity_ms) <=
             ncos::core::contracts::personality_continuity_window_ms(
                 snapshot.personality, ncos::core::contracts::PersonalityContinuityKind::kStimulus);
}

int8_t CompanionStateStore::derive_target_social_warmth_bias_percent(
    const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  if (!snapshot.runtime.initialized || !snapshot.runtime.started) {
    return 0;
  }
  if (energy_protect_active(snapshot)) {
    return -6;
  }
  if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kSleep) {
    return -4;
  }

  int8_t target = 0;
  if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kAttendUser) {
    target = 4;
  }
  if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kResponding &&
      snapshot.session.anchor_target == ncos::core::contracts::AttentionTarget::kUser) {
    target = 5;
  }
  if (user_session_continuity_active(snapshot, now_ms)) {
    const int8_t continuity_target = static_cast<int8_t>(
        3 + engagement_bonus(snapshot.session.engagement_recent_percent, 56, 8, 3));
    target = continuity_target > target ? continuity_target : target;
  }

  return ncos::core::contracts::personality_clamp_i8(
      target, ncos::core::contracts::personality_adaptive_social_warmth_bias_min_percent(),
      ncos::core::contracts::personality_adaptive_social_warmth_bias_max_percent());
}

int8_t CompanionStateStore::derive_target_response_energy_bias_percent(
    const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  if (!snapshot.runtime.initialized || !snapshot.runtime.started) {
    return 0;
  }
  if (energy_protect_active(snapshot)) {
    return -8;
  }
  if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kSleep) {
    return -4;
  }

  int8_t target = 0;
  if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kResponding) {
    target = static_cast<int8_t>(
        4 + engagement_bonus(snapshot.session.engagement_recent_percent, 60, 8, 2));
  } else if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kAlertScan) {
    target = 3;
  } else if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kAttendUser) {
    target = 2;
  } else if (stimulus_session_continuity_active(snapshot, now_ms) ||
             user_session_continuity_active(snapshot, now_ms)) {
    target = 1;
  }

  return ncos::core::contracts::personality_clamp_i8(
      target, ncos::core::contracts::personality_adaptive_response_energy_bias_min_percent(),
      ncos::core::contracts::personality_adaptive_response_energy_bias_max_percent());
}

int16_t CompanionStateStore::derive_target_continuity_window_bias_ms(
    const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  if (!snapshot.runtime.initialized || !snapshot.runtime.started) {
    return 0;
  }
  if (energy_protect_active(snapshot)) {
    return -600;
  }
  if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kSleep) {
    return -300;
  }
  if (snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kResponding &&
      snapshot.session.anchor_target == ncos::core::contracts::AttentionTarget::kUser) {
    return 360;
  }
  if (user_session_continuity_active(snapshot, now_ms)) {
    return static_cast<int16_t>(
        260 + engagement_bonus(snapshot.session.engagement_recent_percent, 56, 4, 2) * 60);
  }
  if (stimulus_session_continuity_active(snapshot, now_ms)) {
    return static_cast<int16_t>(
        120 + engagement_bonus(snapshot.session.engagement_recent_percent, 50, 5, 2) * 40);
  }
  return 0;
}

void CompanionStateStore::refresh_session_memory(
    uint64_t now_ms,
    ncos::core::contracts::CompanionProductState previous_state,
    ncos::core::contracts::CompanionProductState next_state,
    bool user_now,
    bool stimulus_now,
    bool response_now) {
  auto& session = snapshot_.session;

  if (!snapshot_.runtime.initialized || !snapshot_.runtime.started || energy_protect_active(snapshot_)) {
    session = ncos::core::contracts::CompanionSessionMemoryState{};
    return;
  }

  const bool active_context = session_activity_present(snapshot_);
  if (!active_context) {
    if (session.warm && session.retention_until_ms > 0 && now_ms >= session.retention_until_ms) {
      session = ncos::core::contracts::CompanionSessionMemoryState{};
    }
    return;
  }

  if (!session.warm || (session.retention_until_ms > 0 && now_ms >= session.retention_until_ms)) {
    session = ncos::core::contracts::CompanionSessionMemoryState{};
    session.opened_at_ms = now_ms;
  } else if (session.opened_at_ms == 0) {
    session.opened_at_ms = now_ms;
  }

  session.warm = true;
  session.last_activity_ms = now_ms;
  session.retention_until_ms = now_ms + SessionMemoryRetentionMs;

  const uint8_t engagement_recent =
      derive_session_engagement_percent(snapshot_, user_now, stimulus_now, response_now);
  if (engagement_recent > 0) {
    session.engagement_recent_percent = engagement_recent;
    session.last_engagement_ms = now_ms;
  }

  if (snapshot_.attentional.target != ncos::core::contracts::AttentionTarget::kNone) {
    session.anchor_target = snapshot_.attentional.target;
    session.anchor_channel = snapshot_.attentional.channel;
    session.recent_stimulus.target = snapshot_.attentional.target;
    session.recent_stimulus.channel = snapshot_.attentional.channel;
    session.recent_stimulus.confidence_percent = snapshot_.attentional.focus_confidence_percent;
    session.recent_stimulus.observed_at_ms = now_ms;
  }

  if (snapshot_.emotional.intensity_percent > 0) {
    session.anchor_tone = snapshot_.emotional.tone;
  }

  if (next_state != ncos::core::contracts::CompanionProductState::kBooting &&
      next_state != ncos::core::contracts::CompanionProductState::kIdleObserve &&
      next_state != ncos::core::contracts::CompanionProductState::kSleep &&
      next_state != ncos::core::contracts::CompanionProductState::kEnergyProtect) {
    session.anchor_state = next_state;
  }

  if (snapshot_.interactional.phase != ncos::core::contracts::InteractionPhase::kIdle ||
      snapshot_.interactional.turn_owner != ncos::core::contracts::TurnOwner::kNone ||
      snapshot_.interactional.response_pending) {
    session.recent_interaction.phase = snapshot_.interactional.phase;
    session.recent_interaction.turn_owner = snapshot_.interactional.turn_owner;
    session.recent_interaction.response_pending = snapshot_.interactional.response_pending;
    session.recent_interaction.updated_at_ms = now_ms;
  }

  if (snapshot_.interactional.turn_owner != ncos::core::contracts::TurnOwner::kNone) {
    session.last_turn_owner = snapshot_.interactional.turn_owner;
  }

  if (next_state == ncos::core::contracts::CompanionProductState::kAttendUser &&
      previous_state != ncos::core::contracts::CompanionProductState::kAttendUser) {
    ++session.user_trigger_count;
    session.last_user_trigger_ms = now_ms;
  }

  if (next_state == ncos::core::contracts::CompanionProductState::kResponding &&
      previous_state != ncos::core::contracts::CompanionProductState::kResponding) {
    ++session.companion_response_count;
    session.last_companion_response_ms = now_ms;
  }
}

ncos::core::contracts::CompanionProductState CompanionStateStore::derive_product_state(
    const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  const auto previous_state = snapshot.runtime.product_state;

  if (!snapshot.runtime.initialized || !snapshot.runtime.started) {
    return ncos::core::contracts::CompanionProductState::kBooting;
  }

  if (energy_protect_active(snapshot)) {
    return ncos::core::contracts::CompanionProductState::kEnergyProtect;
  }

  if (response_active(snapshot) ||
      (previous_state == ncos::core::contracts::CompanionProductState::kResponding &&
       snapshot.runtime.state_hold_until_ms > now_ms)) {
    return ncos::core::contracts::CompanionProductState::kResponding;
  }

  if (stimulus_attention_active(snapshot) ||
      (previous_state == ncos::core::contracts::CompanionProductState::kAlertScan &&
       snapshot.runtime.state_hold_until_ms > now_ms)) {
    return ncos::core::contracts::CompanionProductState::kAlertScan;
  }

  if (user_attention_active(snapshot) ||
      (previous_state == ncos::core::contracts::CompanionProductState::kAttendUser &&
       snapshot.runtime.state_hold_until_ms > now_ms)) {
    return ncos::core::contracts::CompanionProductState::kAttendUser;
  }

  if (previous_state == ncos::core::contracts::CompanionProductState::kSleep) {
    return ncos::core::contracts::CompanionProductState::kSleep;
  }

  if (snapshot.runtime.idle_since_ms > 0 && (now_ms - snapshot.runtime.idle_since_ms) >= IdleToSleepMs) {
    return ncos::core::contracts::CompanionProductState::kSleep;
  }

  return ncos::core::contracts::CompanionProductState::kIdleObserve;
}

ncos::core::contracts::CompanionStateTransitionCause CompanionStateStore::derive_transition_cause(
    ncos::core::contracts::CompanionProductState previous_state,
    ncos::core::contracts::CompanionProductState next_state) {
  if (next_state == ncos::core::contracts::CompanionProductState::kBooting) {
    return previous_state == ncos::core::contracts::CompanionProductState::kBooting
               ? ncos::core::contracts::CompanionStateTransitionCause::kBootstrap
               : ncos::core::contracts::CompanionStateTransitionCause::kRuntimeStopped;
  }

  if (next_state == ncos::core::contracts::CompanionProductState::kEnergyProtect) {
    return ncos::core::contracts::CompanionStateTransitionCause::kEnergyGuard;
  }

  if (next_state == ncos::core::contracts::CompanionProductState::kResponding) {
    return ncos::core::contracts::CompanionStateTransitionCause::kCompanionResponding;
  }

  if (next_state == ncos::core::contracts::CompanionProductState::kAlertScan) {
    return ncos::core::contracts::CompanionStateTransitionCause::kStimulusObserved;
  }

  if (next_state == ncos::core::contracts::CompanionProductState::kAttendUser) {
    return ncos::core::contracts::CompanionStateTransitionCause::kUserTrigger;
  }

  if (next_state == ncos::core::contracts::CompanionProductState::kSleep) {
    return ncos::core::contracts::CompanionStateTransitionCause::kIdleDecayToSleep;
  }

  if (next_state == ncos::core::contracts::CompanionProductState::kIdleObserve) {
    if (previous_state == ncos::core::contracts::CompanionProductState::kBooting) {
      return ncos::core::contracts::CompanionStateTransitionCause::kRuntimeStarted;
    }

    if (previous_state == ncos::core::contracts::CompanionProductState::kAttendUser) {
      return ncos::core::contracts::CompanionStateTransitionCause::kUserRelease;
    }

    return ncos::core::contracts::CompanionStateTransitionCause::kRecoveryToIdle;
  }

  return ncos::core::contracts::CompanionStateTransitionCause::kRecoveryToIdle;
}

void CompanionStateStore::refresh_adaptive_personality(uint64_t now_ms) {
  const int8_t target_social_bias = derive_target_social_warmth_bias_percent(snapshot_, now_ms);
  const int8_t target_response_bias = derive_target_response_energy_bias_percent(snapshot_, now_ms);
  const int16_t target_continuity_bias = derive_target_continuity_window_bias_ms(snapshot_, now_ms);

  snapshot_.personality.adaptive_social_warmth_bias_percent = move_toward_i8(
      snapshot_.personality.adaptive_social_warmth_bias_percent, target_social_bias,
      AdaptiveBiasStepPercent);
  snapshot_.personality.adaptive_response_energy_bias_percent = move_toward_i8(
      snapshot_.personality.adaptive_response_energy_bias_percent, target_response_bias,
      AdaptiveBiasStepPercent);
  snapshot_.personality.adaptive_continuity_window_bias_ms = move_toward_i16(
      snapshot_.personality.adaptive_continuity_window_bias_ms, target_continuity_bias,
      AdaptiveContinuityStepMs);
}

void CompanionStateStore::refresh_derived_runtime_state(uint64_t now_ms) {
  const auto previous_state = snapshot_.runtime.product_state;

  const bool response_now = response_active(snapshot_);
  const bool stimulus_now = stimulus_attention_active(snapshot_);
  const bool user_now = user_attention_active(snapshot_);

  if (response_now || stimulus_now || user_now || energy_protect_active(snapshot_) ||
      !snapshot_.runtime.started || !snapshot_.runtime.initialized) {
    snapshot_.runtime.idle_since_ms = 0;
  } else if (snapshot_.runtime.idle_since_ms == 0) {
    snapshot_.runtime.idle_since_ms = now_ms;
  }

  const auto next_state = derive_product_state(snapshot_, now_ms);
  snapshot_.runtime.product_state = next_state;
  snapshot_.runtime.presence_mode = derive_presence_mode(snapshot_);
  refresh_session_memory(now_ms, previous_state, next_state, user_now, stimulus_now, response_now);
  refresh_adaptive_personality(now_ms);

  if (snapshot_.runtime.last_state_change_ms == 0) {
    snapshot_.runtime.last_state_change_ms = now_ms;
  }

  if (next_state != previous_state) {
    snapshot_.runtime.last_transition_cause = derive_transition_cause(previous_state, next_state);
    snapshot_.runtime.last_state_change_ms = now_ms;
    const uint64_t hold_ms = hold_duration_for_state(next_state);
    snapshot_.runtime.state_hold_until_ms = hold_ms > 0 ? (now_ms + hold_ms) : 0;
    ++snapshot_.runtime.state_transition_total;
  } else {
    if ((next_state == ncos::core::contracts::CompanionProductState::kResponding && response_now) ||
        (next_state == ncos::core::contracts::CompanionProductState::kAlertScan && stimulus_now) ||
        (next_state == ncos::core::contracts::CompanionProductState::kAttendUser && user_now)) {
      snapshot_.runtime.state_hold_until_ms = now_ms + hold_duration_for_state(next_state);
    } else if (hold_duration_for_state(next_state) == 0) {
      snapshot_.runtime.state_hold_until_ms = 0;
    }
  }

  snapshot_.runtime.state_dwell_ms =
      now_ms >= snapshot_.runtime.last_state_change_ms ? (now_ms - snapshot_.runtime.last_state_change_ms) : 0;
}

bool CompanionStateStore::authorize_write(ncos::core::contracts::CompanionStateWriter writer,
                                          ncos::core::contracts::CompanionStateDomain domain) const {
  return ncos::core::contracts::can_writer_mutate_domain(writer, domain);
}

}  // namespace ncos::core::state

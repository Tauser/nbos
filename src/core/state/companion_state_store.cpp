#include "core/state/companion_state_store.hpp"

namespace ncos::core::state {

bool CompanionStateStore::initialize(const ncos::core::contracts::CompanionStructuralState& structural,
                                     ncos::core::contracts::CompanionStateWriter writer,
                                     uint64_t now_ms) {
  if (!authorize_write(writer, ncos::core::contracts::CompanionStateDomain::kStructural)) {
    return false;
  }

  snapshot_.structural = structural;
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

ncos::core::contracts::CompanionPresenceMode CompanionStateStore::derive_presence_mode(
    const ncos::core::contracts::CompanionSnapshot& snapshot) {
  if (!snapshot.runtime.started || snapshot.runtime.safe_mode ||
      snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kCritical) {
    return ncos::core::contracts::CompanionPresenceMode::kDormant;
  }

  switch (derive_product_state(snapshot)) {
    case ncos::core::contracts::CompanionProductState::kAttendUser:
    case ncos::core::contracts::CompanionProductState::kAlertScan:
    case ncos::core::contracts::CompanionProductState::kResponding:
      return ncos::core::contracts::CompanionPresenceMode::kAttending;
    case ncos::core::contracts::CompanionProductState::kBooting:
    case ncos::core::contracts::CompanionProductState::kEnergyProtect:
      return ncos::core::contracts::CompanionPresenceMode::kDormant;
    case ncos::core::contracts::CompanionProductState::kIdleObserve:
    default:
      return ncos::core::contracts::CompanionPresenceMode::kIdle;
  }
}

ncos::core::contracts::CompanionProductState CompanionStateStore::derive_product_state(
    const ncos::core::contracts::CompanionSnapshot& snapshot) {
  if (!snapshot.runtime.initialized || !snapshot.runtime.started) {
    return ncos::core::contracts::CompanionProductState::kBooting;
  }

  if (snapshot.runtime.safe_mode || snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kCritical ||
      (snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kConstrained &&
       snapshot.energetic.battery_percent <= 18 && !snapshot.energetic.external_power)) {
    return ncos::core::contracts::CompanionProductState::kEnergyProtect;
  }

  if (snapshot.interactional.phase == ncos::core::contracts::InteractionPhase::kResponding ||
      snapshot.interactional.turn_owner == ncos::core::contracts::TurnOwner::kCompanion ||
      snapshot.interactional.response_pending) {
    return ncos::core::contracts::CompanionProductState::kResponding;
  }

  if (snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kStimulus &&
      snapshot.attentional.focus_confidence_percent >= 50) {
    return ncos::core::contracts::CompanionProductState::kAlertScan;
  }

  if (snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kUser &&
      (snapshot.attentional.lock_active || snapshot.attentional.focus_confidence_percent >= 45 ||
       snapshot.interactional.session_active)) {
    return ncos::core::contracts::CompanionProductState::kAttendUser;
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

void CompanionStateStore::refresh_derived_runtime_state(uint64_t now_ms) {
  const auto previous_state = snapshot_.runtime.product_state;
  const auto next_state = derive_product_state(snapshot_);

  snapshot_.runtime.presence_mode = derive_presence_mode(snapshot_);
  snapshot_.runtime.product_state = next_state;

  if (snapshot_.runtime.last_state_change_ms == 0) {
    snapshot_.runtime.last_state_change_ms = now_ms;
  }

  if (next_state != previous_state) {
    snapshot_.runtime.last_transition_cause = derive_transition_cause(previous_state, next_state);
    snapshot_.runtime.last_state_change_ms = now_ms;
    ++snapshot_.runtime.state_transition_total;
  }
}

bool CompanionStateStore::authorize_write(ncos::core::contracts::CompanionStateWriter writer,
                                          ncos::core::contracts::CompanionStateDomain domain) const {
  return ncos::core::contracts::can_writer_mutate_domain(writer, domain);
}

}  // namespace ncos::core::state

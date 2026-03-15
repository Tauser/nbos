#include "services/routine/routine_service.hpp"

namespace {

ncos::core::contracts::RoutineProposal make_routine_proposal(
    ncos::core::contracts::IdleRoutine routine,
    ncos::core::contracts::AttentionMode mode,
    uint16_t service_id,
    ncos::core::contracts::ActionDomain domain,
    ncos::core::contracts::CommandTopic action,
    ncos::core::contracts::IntentTopic intent,
    uint8_t priority,
    uint32_t ttl_ms,
    const char* rationale) {
  ncos::core::contracts::RoutineProposal out{};
  out.valid = true;
  out.routine = routine;
  out.attention_mode = mode;
  out.rationale = rationale;
  out.proposal.origin = ncos::core::contracts::ProposalOrigin::kIntent;
  out.proposal.requester_service = service_id;
  out.proposal.domain = domain;
  out.proposal.action = action;
  out.proposal.intent_context = intent;
  out.proposal.priority = priority;
  out.proposal.ttl_ms = ttl_ms;
  out.proposal.preemption_policy = ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority;
  return out;
}

}  // namespace

namespace ncos::services::routine {

bool RoutineService::initialize(uint16_t service_id, uint64_t now_ms) {
  if (service_id == 0) {
    return false;
  }

  service_id_ = service_id;
  state_ = ncos::core::contracts::RoutineRuntimeState{};
  state_.initialized = true;
  state_.last_tick_ms = now_ms;
  state_.last_emit_ms = now_ms;
  return true;
}

bool RoutineService::tick(const ncos::core::contracts::CompanionSnapshot& snapshot,
                          const ncos::core::contracts::BehaviorRuntimeState& behavior_state,
                          uint64_t now_ms,
                          ncos::core::contracts::RoutineProposal* out_proposal) {
  if (!state_.initialized || out_proposal == nullptr) {
    return false;
  }

  state_.last_tick_ms = now_ms;
  out_proposal->valid = false;

  const ncos::core::contracts::AttentionMode mode = resolve_attention_mode(snapshot);
  state_.attention_mode = mode;

  if (should_suppress_by_behavior(behavior_state, now_ms)) {
    return false;
  }

  if ((now_ms - state_.last_emit_ms) < kRoutineCooldownMs) {
    return false;
  }

  ncos::core::contracts::RoutineProposal proposal = propose_for_mode(mode, snapshot, now_ms);
  if (!proposal.valid) {
    return false;
  }

  proposal.proposal.trace_id = static_cast<uint32_t>(now_ms & 0xFFFFFFFFULL);
  *out_proposal = proposal;

  state_.has_pending = true;
  state_.active_routine = proposal.routine;
  state_.last_emit_ms = now_ms;
  ++state_.emitted_total;
  return true;
}

void RoutineService::on_governance_decision(
    const ncos::core::contracts::GovernanceDecision& decision,
    uint64_t now_ms) {
  if (!state_.initialized) {
    return;
  }

  state_.last_tick_ms = now_ms;
  state_.has_pending = false;

  switch (decision.kind) {
    case ncos::core::contracts::GovernanceDecisionKind::kAllow:
      ++state_.accepted_total;
      state_.last_accept_ms = now_ms;
      break;
    case ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow:
      ++state_.accepted_total;
      ++state_.preempted_total;
      state_.last_accept_ms = now_ms;
      break;
    case ncos::core::contracts::GovernanceDecisionKind::kReject:
      ++state_.rejected_total;
      break;
    case ncos::core::contracts::GovernanceDecisionKind::kDefer:
    default:
      break;
  }
}

const ncos::core::contracts::RoutineRuntimeState& RoutineService::state() const {
  return state_;
}

ncos::core::contracts::AttentionMode RoutineService::resolve_attention_mode(
    const ncos::core::contracts::CompanionSnapshot& snapshot) const {
  if (snapshot.runtime.safe_mode ||
      snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kCritical ||
      snapshot.energetic.battery_percent <= 18) {
    return ncos::core::contracts::AttentionMode::kEnergyConserve;
  }

  if (snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kUser &&
      snapshot.attentional.focus_confidence_percent >= 45) {
    return ncos::core::contracts::AttentionMode::kUserEngaged;
  }

  if (snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kStimulus &&
      snapshot.attentional.focus_confidence_percent >= 35) {
    return ncos::core::contracts::AttentionMode::kStimulusTracking;
  }

  return ncos::core::contracts::AttentionMode::kAmbient;
}

ncos::core::contracts::RoutineProposal RoutineService::propose_for_mode(
    ncos::core::contracts::AttentionMode mode,
    const ncos::core::contracts::CompanionSnapshot& snapshot,
    uint64_t now_ms) const {
  (void)now_ms;

  switch (mode) {
    case ncos::core::contracts::AttentionMode::kEnergyConserve:
      return make_routine_proposal(
          ncos::core::contracts::IdleRoutine::kEnergySaveSettle,
          mode,
          service_id_,
          ncos::core::contracts::ActionDomain::kMotion,
          ncos::core::contracts::CommandTopic::kMotionExecute,
          ncos::core::contracts::IntentTopic::kPreserveEnergy,
          4,
          400,
          "idle_energy_settle");

    case ncos::core::contracts::AttentionMode::kStimulusTracking:
      return make_routine_proposal(
          ncos::core::contracts::IdleRoutine::kStimulusScanNudge,
          mode,
          service_id_,
          ncos::core::contracts::ActionDomain::kMotion,
          ncos::core::contracts::CommandTopic::kMotionExecute,
          ncos::core::contracts::IntentTopic::kInspectStimulus,
          4,
          320,
          "idle_stimulus_nudge");

    case ncos::core::contracts::AttentionMode::kUserEngaged:
      return make_routine_proposal(
          ncos::core::contracts::IdleRoutine::kUserPresencePulse,
          mode,
          service_id_,
          ncos::core::contracts::ActionDomain::kFace,
          ncos::core::contracts::CommandTopic::kFaceRenderExecute,
          ncos::core::contracts::IntentTopic::kAttendUser,
          4,
          300,
          "idle_user_presence");

    case ncos::core::contracts::AttentionMode::kAmbient:
    default: {
      const bool engaged_emotion =
          snapshot.emotional.phase == ncos::models::emotion::EmotionPhase::kEngaged ||
          snapshot.emotional.tone == ncos::core::contracts::EmotionalTone::kCurious;

      return make_routine_proposal(
          ncos::core::contracts::IdleRoutine::kAmbientGazeSweep,
          mode,
          service_id_,
          ncos::core::contracts::ActionDomain::kFace,
          ncos::core::contracts::CommandTopic::kFaceRenderExecute,
          engaged_emotion ? ncos::core::contracts::IntentTopic::kAcknowledgeUser
                          : ncos::core::contracts::IntentTopic::kAttendUser,
          3,
          260,
          "idle_ambient_sweep");
    }
  }
}

bool RoutineService::should_suppress_by_behavior(
    const ncos::core::contracts::BehaviorRuntimeState& behavior_state,
    uint64_t now_ms) const {
  if (!behavior_state.initialized) {
    return false;
  }

  if (behavior_state.has_pending) {
    return true;
  }

  if (behavior_state.active_profile == ncos::core::contracts::BehaviorProfile::kIdleObserve) {
    return false;
  }

  return (now_ms - behavior_state.last_accept_ms) < kBehaviorSuppressionMs;
}

}  // namespace ncos::services::routine

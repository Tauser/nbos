#include "services/behavior/behavior_service.hpp"

namespace {

ncos::core::contracts::BehaviorProposal make_behavior_proposal(
    ncos::core::contracts::BehaviorProfile profile, uint16_t service_id,
    ncos::core::contracts::ActionDomain domain, ncos::core::contracts::CommandTopic action,
    ncos::core::contracts::IntentTopic intent, uint8_t priority, uint32_t ttl_ms,
    ncos::core::contracts::PreemptionPolicy preemption, const char* rationale) {
  ncos::core::contracts::BehaviorProposal out{};
  out.valid = true;
  out.profile = profile;
  out.rationale = rationale;
  out.proposal.origin = ncos::core::contracts::ProposalOrigin::kIntent;
  out.proposal.requester_service = service_id;
  out.proposal.domain = domain;
  out.proposal.action = action;
  out.proposal.intent_context = intent;
  out.proposal.priority = priority;
  out.proposal.ttl_ms = ttl_ms;
  out.proposal.preemption_policy = preemption;
  return out;
}

}  // namespace

namespace ncos::services::behavior {

bool BehaviorService::initialize(uint16_t service_id, uint64_t now_ms) {
  if (service_id == 0) {
    return false;
  }

  service_id_ = service_id;
  state_ = ncos::core::contracts::BehaviorRuntimeState{};
  state_.initialized = true;
  state_.last_tick_ms = now_ms;
  state_.last_emit_ms = now_ms;
  return true;
}

bool BehaviorService::tick(const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms,
                           ncos::core::contracts::BehaviorProposal* out_proposal) {
  if (!state_.initialized || out_proposal == nullptr) {
    return false;
  }

  state_.last_tick_ms = now_ms;
  out_proposal->valid = false;

  if ((now_ms - state_.last_emit_ms) < BehaviorCooldownMs) {
    return false;
  }

  ncos::core::contracts::BehaviorProposal proposal = propose_energy_protect(snapshot);
  if (!proposal.valid) {
    proposal = propose_alert_scan(snapshot);
  }
  if (!proposal.valid) {
    proposal = propose_attend_user(snapshot);
  }

  if (!proposal.valid) {
    return false;
  }

  proposal.proposal.trace_id = static_cast<uint32_t>(now_ms & 0xFFFFFFFFULL);
  *out_proposal = proposal;

  state_.has_pending = true;
  state_.last_emit_ms = now_ms;
  state_.active_profile = proposal.profile;
  ++state_.emitted_total;
  return true;
}

void BehaviorService::on_governance_decision(
    const ncos::core::contracts::GovernanceDecision& decision, uint64_t now_ms) {
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

const ncos::core::contracts::BehaviorRuntimeState& BehaviorService::state() const {
  return state_;
}

ncos::core::contracts::BehaviorProposal BehaviorService::propose_energy_protect(
    const ncos::core::contracts::CompanionSnapshot& snapshot) const {
  if (snapshot.runtime.safe_mode ||
      snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kCritical ||
      snapshot.energetic.battery_percent <= 18) {
    return make_behavior_proposal(
        ncos::core::contracts::BehaviorProfile::kEnergyProtect, service_id_,
        ncos::core::contracts::ActionDomain::kPower,
        ncos::core::contracts::CommandTopic::kPowerModeSet,
        ncos::core::contracts::IntentTopic::kPreserveEnergy,
        10,
        420,
        ncos::core::contracts::PreemptionPolicy::kAllowAlways,
        "energy_protect");
  }

  return ncos::core::contracts::BehaviorProposal{};
}

ncos::core::contracts::BehaviorProposal BehaviorService::propose_alert_scan(
    const ncos::core::contracts::CompanionSnapshot& snapshot) const {
  const bool alert_emotion =
      snapshot.emotional.phase == ncos::models::emotion::EmotionPhase::kAlerting ||
      snapshot.emotional.arousal == ncos::core::contracts::EmotionalArousal::kHigh;

  const bool world_attention = snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kStimulus &&
                               snapshot.attentional.focus_confidence_percent >= 55;

  if (alert_emotion || world_attention) {
    return make_behavior_proposal(
        ncos::core::contracts::BehaviorProfile::kAlertScan, service_id_,
        ncos::core::contracts::ActionDomain::kMotion,
        ncos::core::contracts::CommandTopic::kMotionExecute,
        ncos::core::contracts::IntentTopic::kInspectStimulus,
        7,
        260,
        ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority,
        "alert_scan");
  }

  return ncos::core::contracts::BehaviorProposal{};
}

ncos::core::contracts::BehaviorProposal BehaviorService::propose_attend_user(
    const ncos::core::contracts::CompanionSnapshot& snapshot) const {
  const bool user_attention = snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kUser &&
                              snapshot.attentional.focus_confidence_percent >= 45;
  const bool auditory_trigger_context =
      snapshot.attentional.channel == ncos::core::contracts::AttentionChannel::kAuditory &&
      snapshot.interactional.response_pending;

  if (user_attention && !snapshot.runtime.safe_mode) {
    return make_behavior_proposal(
        ncos::core::contracts::BehaviorProfile::kAttendUser, service_id_,
        ncos::core::contracts::ActionDomain::kFace,
        ncos::core::contracts::CommandTopic::kFaceRenderExecute,
        ncos::core::contracts::IntentTopic::kAttendUser,
        auditory_trigger_context ? 7 : 6,
        220,
        ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority,
        auditory_trigger_context ? "attend_user_voice_trigger" : "attend_user");
  }

  return ncos::core::contracts::BehaviorProposal{};
}

}  // namespace ncos::services::behavior




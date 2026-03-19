#include "services/behavior/behavior_service.hpp"

#include "core/contracts/companion_personality_contracts.hpp"

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


bool has_warm_user_continuity(const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  if (!snapshot.session.warm || snapshot.runtime.safe_mode || snapshot.session.last_activity_ms == 0 ||
      now_ms < snapshot.session.last_activity_ms ||
      (now_ms - snapshot.session.last_activity_ms) >
          ncos::core::contracts::personality_continuity_window_ms(
              snapshot.personality, ncos::core::contracts::PersonalityContinuityKind::kUser)) {
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
                 snapshot.personality, ncos::core::contracts::PersonalityContinuityKind::kUser);
}

bool should_return_to_idle(const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  return snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kNone &&
         snapshot.interactional.phase == ncos::core::contracts::InteractionPhase::kIdle &&
         !snapshot.interactional.session_active && !has_warm_user_continuity(snapshot, now_ms);
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
    proposal = propose_attend_user(snapshot, now_ms);
  }

  if (!proposal.valid) {
    if (should_return_to_idle(snapshot, now_ms)) {
      state_.active_profile = ncos::core::contracts::BehaviorProfile::kIdleObserve;
    }
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
        ncos::core::contracts::personality_behavior_priority(
            snapshot.personality, ncos::core::contracts::BehaviorProfile::kEnergyProtect, 10),
        ncos::core::contracts::personality_behavior_ttl_ms(
            snapshot.personality, ncos::core::contracts::BehaviorProfile::kEnergyProtect),
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
        ncos::core::contracts::personality_behavior_priority(
            snapshot.personality, ncos::core::contracts::BehaviorProfile::kAlertScan, 7),
        ncos::core::contracts::personality_behavior_ttl_ms(
            snapshot.personality, ncos::core::contracts::BehaviorProfile::kAlertScan),
        ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority,
        "alert_scan");
  }

  return ncos::core::contracts::BehaviorProposal{};
}

ncos::core::contracts::BehaviorProposal BehaviorService::propose_attend_user(
    const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) const {
  const bool user_attention = snapshot.attentional.target == ncos::core::contracts::AttentionTarget::kUser &&
                              snapshot.attentional.focus_confidence_percent >= 45;
  const bool auditory_trigger_context =
      snapshot.attentional.channel == ncos::core::contracts::AttentionChannel::kAuditory &&
      snapshot.interactional.response_pending;
  const bool warm_user_context =
      snapshot.runtime.product_state == ncos::core::contracts::CompanionProductState::kIdleObserve &&
      has_warm_user_continuity(snapshot, now_ms);

  if (user_attention && !snapshot.runtime.safe_mode) {
    return make_behavior_proposal(
        ncos::core::contracts::BehaviorProfile::kAttendUser, service_id_,
        ncos::core::contracts::ActionDomain::kFace,
        ncos::core::contracts::CommandTopic::kFaceRenderExecute,
        ncos::core::contracts::IntentTopic::kAttendUser,
        ncos::core::contracts::personality_behavior_priority(
            snapshot.personality, ncos::core::contracts::BehaviorProfile::kAttendUser,
            auditory_trigger_context ? 7 : 6),
        ncos::core::contracts::personality_behavior_ttl_ms(
            snapshot.personality, ncos::core::contracts::BehaviorProfile::kAttendUser),
        ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority,
        auditory_trigger_context ? "attend_user_voice_trigger" : "attend_user");
  }

  if (warm_user_context && !snapshot.runtime.safe_mode) {
    return make_behavior_proposal(
        ncos::core::contracts::BehaviorProfile::kAttendUser, service_id_,
        ncos::core::contracts::ActionDomain::kFace,
        ncos::core::contracts::CommandTopic::kFaceRenderExecute,
        ncos::core::contracts::IntentTopic::kAttendUser,
        ncos::core::contracts::personality_behavior_priority(
            snapshot.personality, ncos::core::contracts::BehaviorProfile::kAttendUser, 5),
        ncos::core::contracts::personality_reengagement_ttl_ms(snapshot.personality),
        ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority,
        "attend_user_continuity");
  }

  return ncos::core::contracts::BehaviorProposal{};
}

}  // namespace ncos::services::behavior


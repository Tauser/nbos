#include "core/governance/action_governor.hpp"

namespace {

ncos::core::contracts::GovernanceDecision make_reject(
    const ncos::core::contracts::ActionProposal& proposal,
    ncos::core::contracts::GovernanceRejectReason reject_reason, const char* reason) {
  ncos::core::contracts::GovernanceDecision decision{};
  decision.kind = ncos::core::contracts::GovernanceDecisionKind::kReject;
  decision.proposal_trace_id = proposal.trace_id;
  decision.domain = proposal.domain;
  decision.owner_service = proposal.requester_service;
  decision.reject_reason = reject_reason;
  decision.reason = reason;
  return decision;
}

}  // namespace

namespace ncos::core::governance {

ncos::core::contracts::GovernanceDecision ActionGovernor::evaluate(
    const ncos::core::contracts::ActionProposal& proposal, uint64_t now_ms) {
  if (!ncos::core::contracts::is_valid(proposal)) {
    ++stats_.rejected;
    return make_reject(proposal, ncos::core::contracts::GovernanceRejectReason::kInvalidProposal,
                       "invalid_proposal");
  }

  const size_t idx = domain_index(proposal.domain);
  expire_lease_if_needed(idx, now_ms);

  ncos::core::contracts::DomainLease& current = leases_[idx];

  if (!current.active) {
    current.active = true;
    current.domain = proposal.domain;
    current.owner_service = proposal.requester_service;
    current.priority = proposal.priority;
    current.proposal_trace_id = proposal.trace_id;
    current.expires_at_ms = now_ms + proposal.ttl_ms;

    ++stats_.allowed;

    ncos::core::contracts::GovernanceDecision decision{};
    decision.kind = ncos::core::contracts::GovernanceDecisionKind::kAllow;
    decision.proposal_trace_id = proposal.trace_id;
    decision.domain = proposal.domain;
    decision.owner_service = proposal.requester_service;
    decision.reason = "granted_domain_free";
    return decision;
  }

  if (current.owner_service == proposal.requester_service) {
    current.priority = proposal.priority;
    current.proposal_trace_id = proposal.trace_id;
    current.expires_at_ms = now_ms + proposal.ttl_ms;

    ++stats_.allowed;

    ncos::core::contracts::GovernanceDecision decision{};
    decision.kind = ncos::core::contracts::GovernanceDecisionKind::kAllow;
    decision.proposal_trace_id = proposal.trace_id;
    decision.domain = proposal.domain;
    decision.owner_service = proposal.requester_service;
    decision.reason = "owner_refresh";
    return decision;
  }

  bool can_preempt = false;
  if (proposal.preemption_policy == ncos::core::contracts::PreemptionPolicy::kAllowAlways) {
    can_preempt = true;
  } else if (proposal.preemption_policy ==
             ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority) {
    can_preempt = proposal.priority > current.priority;
  }

  if (!can_preempt) {
    ++stats_.rejected;

    if (proposal.priority <= current.priority) {
      return make_reject(proposal,
                         ncos::core::contracts::GovernanceRejectReason::kInsufficientPriority,
                         "insufficient_priority_for_preemption");
    }

    return make_reject(proposal, ncos::core::contracts::GovernanceRejectReason::kDomainOwnedByOther,
                       "preemption_forbidden");
  }

  const uint16_t old_owner = current.owner_service;
  current.owner_service = proposal.requester_service;
  current.priority = proposal.priority;
  current.proposal_trace_id = proposal.trace_id;
  current.expires_at_ms = now_ms + proposal.ttl_ms;

  ++stats_.preempted;

  ncos::core::contracts::GovernanceDecision decision{};
  decision.kind = ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow;
  decision.proposal_trace_id = proposal.trace_id;
  decision.domain = proposal.domain;
  decision.owner_service = proposal.requester_service;
  decision.preempted_owner_service = old_owner;
  decision.reason = "preempted";
  return decision;
}

bool ActionGovernor::release(ncos::core::contracts::ActionDomain domain, uint16_t owner_service) {
  const size_t idx = domain_index(domain);
  ncos::core::contracts::DomainLease& current = leases_[idx];

  if (!current.active || current.owner_service != owner_service) {
    return false;
  }

  current.active = false;
  current.owner_service = 0;
  current.priority = 0;
  current.proposal_trace_id = 0;
  current.expires_at_ms = 0;
  return true;
}

ncos::core::contracts::DomainLease ActionGovernor::lease(
    ncos::core::contracts::ActionDomain domain) const {
  return leases_[domain_index(domain)];
}

ActionGovernanceStats ActionGovernor::stats() const {
  return stats_;
}

size_t ActionGovernor::domain_index(ncos::core::contracts::ActionDomain domain) {
  switch (domain) {
    case ncos::core::contracts::ActionDomain::kFace:
      return 0;
    case ncos::core::contracts::ActionDomain::kMotion:
      return 1;
    case ncos::core::contracts::ActionDomain::kAudio:
      return 2;
    case ncos::core::contracts::ActionDomain::kPower:
      return 3;
    case ncos::core::contracts::ActionDomain::kLed:
      return 4;
    default:
      return 0;
  }
}

void ActionGovernor::expire_lease_if_needed(size_t domain_idx, uint64_t now_ms) {
  ncos::core::contracts::DomainLease& lease = leases_[domain_idx];
  if (!lease.active) {
    return;
  }

  if (now_ms >= lease.expires_at_ms) {
    lease.active = false;
    lease.owner_service = 0;
    lease.priority = 0;
    lease.proposal_trace_id = 0;
    lease.expires_at_ms = 0;
  }
}

}  // namespace ncos::core::governance

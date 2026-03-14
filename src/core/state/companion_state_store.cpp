#include "core/state/companion_state_store.hpp"

namespace ncos::core::state {

void CompanionStateStore::initialize(const ncos::core::contracts::CompanionStructuralState& structural,
                                     uint64_t now_ms) {
  snapshot_.structural = structural;
  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
}

void CompanionStateStore::ingest_runtime(const ncos::core::contracts::CompanionRuntimeSignal& runtime,
                                         uint64_t now_ms) {
  snapshot_.runtime.initialized = runtime.initialized;
  snapshot_.runtime.started = runtime.started;
  snapshot_.runtime.safe_mode = runtime.safe_mode;
  snapshot_.runtime.scheduler_tasks = runtime.scheduler_tasks;
  snapshot_.runtime.fault_count = runtime.fault_count;
  snapshot_.runtime.presence_mode = presence_from_runtime(runtime);

  snapshot_.governance.allowed_total = runtime.governance_allowed_total;
  snapshot_.governance.preempted_total = runtime.governance_preempted_total;
  snapshot_.governance.rejected_total = runtime.governance_rejected_total;
  snapshot_.governance.health = ncos::core::contracts::evaluate_governance_health(
      runtime.governance_allowed_total, runtime.governance_preempted_total,
      runtime.governance_rejected_total);

  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
}

void CompanionStateStore::ingest_governance_decision(
    const ncos::core::contracts::GovernanceDecision& decision, uint64_t now_ms) {
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

  snapshot_.captured_at_ms = now_ms;
  ++snapshot_.revision;
}

ncos::core::contracts::CompanionSnapshot CompanionStateStore::snapshot() const {
  return snapshot_;
}

ncos::core::contracts::CompanionPresenceMode CompanionStateStore::presence_from_runtime(
    const ncos::core::contracts::CompanionRuntimeSignal& runtime) {
  if (!runtime.started || runtime.safe_mode) {
    return ncos::core::contracts::CompanionPresenceMode::kDormant;
  }

  if (runtime.scheduler_tasks > 1) {
    return ncos::core::contracts::CompanionPresenceMode::kAttending;
  }

  return ncos::core::contracts::CompanionPresenceMode::kIdle;
}

}  // namespace ncos::core::state

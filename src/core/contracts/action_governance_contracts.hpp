#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/interaction_taxonomy.hpp"

namespace ncos::core::contracts {

enum class ProposalOrigin : uint8_t {
  kCommand = 1,
  kIntent = 2,
};

enum class ActionDomain : uint8_t {
  kFace = 1,
  kMotion = 2,
  kAudio = 3,
  kPower = 4,
  kLed = 5,
};

enum class PreemptionPolicy : uint8_t {
  kForbid = 1,
  kAllowIfHigherPriority = 2,
  kAllowAlways = 3,
};

struct ActionProposal {
  ProposalOrigin origin = ProposalOrigin::kIntent;
  uint32_t trace_id = 0;
  uint16_t requester_service = 0;
  ActionDomain domain = ActionDomain::kFace;
  CommandTopic action = CommandTopic::kMotionExecute;
  IntentTopic intent_context = IntentTopic::kAttendUser;
  uint8_t priority = 0;
  uint32_t ttl_ms = 0;
  PreemptionPolicy preemption_policy = PreemptionPolicy::kAllowIfHigherPriority;
};

enum class GovernanceDecisionKind : uint8_t {
  kAllow = 1,
  kPreemptAndAllow = 2,
  kReject = 3,
  kDefer = 4,
};

enum class GovernanceRejectReason : uint8_t {
  kNone = 0,
  kInvalidProposal = 1,
  kDomainOwnedByOther = 2,
  kInsufficientPriority = 3,
  kSemanticDebounced = 4,
};

struct DomainLease {
  bool active = false;
  ActionDomain domain = ActionDomain::kFace;
  uint16_t owner_service = 0;
  uint8_t priority = 0;
  uint32_t proposal_trace_id = 0;
  uint64_t expires_at_ms = 0;
};

struct GovernanceDecision {
  GovernanceDecisionKind kind = GovernanceDecisionKind::kDefer;
  uint32_t proposal_trace_id = 0;
  ActionDomain domain = ActionDomain::kFace;
  uint16_t owner_service = 0;
  uint16_t preempted_owner_service = 0;
  GovernanceRejectReason reject_reason = GovernanceRejectReason::kNone;
  const char* reason = "none";
};

inline constexpr bool is_valid(const ActionProposal& proposal) {
  return proposal.requester_service != 0 && proposal.ttl_ms > 0;
}

constexpr size_t kActionDomainCount = 5;

ActionDomain command_topic_to_domain(CommandTopic topic);
const char* action_domain_name(ActionDomain domain);
const char* governance_reject_reason_name(GovernanceRejectReason reason);

}  // namespace ncos::core::contracts

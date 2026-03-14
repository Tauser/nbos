#pragma once

#include "core/contracts/interaction_taxonomy.hpp"

namespace ncos::core::contracts {

enum class ProposalOrigin : uint8_t {
  kCommand = 1,
  kIntent = 2,
};

struct ActionProposal {
  ProposalOrigin origin = ProposalOrigin::kIntent;
  uint32_t trace_id = 0;
  CommandTopic action = CommandTopic::kMotionExecute;
  IntentTopic intent_context = IntentTopic::kAttendUser;
  uint8_t priority = 0;
  uint32_t ttl_ms = 0;
};

enum class GovernanceDecisionKind : uint8_t {
  kAllow = 1,
  kReject = 2,
  kDefer = 3,
};

struct GovernanceDecision {
  GovernanceDecisionKind kind = GovernanceDecisionKind::kDefer;
  uint32_t proposal_trace_id = 0;
  const char* reason = "none";
};

inline constexpr bool is_valid(const ActionProposal& proposal) {
  return proposal.ttl_ms > 0;
}

}  // namespace ncos::core::contracts

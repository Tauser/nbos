#include <unity.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/governance/action_governor.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/governance/action_governor.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

ncos::core::contracts::ActionProposal make_base_proposal(uint16_t service, uint8_t priority,
                                                         uint32_t trace) {
  ncos::core::contracts::ActionProposal proposal{};
  proposal.origin = ncos::core::contracts::ProposalOrigin::kCommand;
  proposal.trace_id = trace;
  proposal.requester_service = service;
  proposal.domain = ncos::core::contracts::ActionDomain::kFace;
  proposal.action = ncos::core::contracts::CommandTopic::kFaceRenderExecute;
  proposal.priority = priority;
  proposal.ttl_ms = 1000;
  proposal.preemption_policy = ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority;
  return proposal;
}

}  // namespace

void test_action_governance_allows_when_domain_is_free() {
  ncos::core::governance::ActionGovernor governor;
  const auto proposal = make_base_proposal(10, 5, 100);

  const auto decision = governor.evaluate(proposal, 1000);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kAllow),
                        static_cast<int>(decision.kind));
  TEST_ASSERT_EQUAL_UINT16(10, decision.owner_service);
  TEST_ASSERT_EQUAL_UINT16(10, governor.lease(ncos::core::contracts::ActionDomain::kFace).owner_service);
}

void test_action_governance_rejects_equal_or_lower_priority_preemption() {
  ncos::core::governance::ActionGovernor governor;

  const auto first = make_base_proposal(10, 7, 100);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kAllow),
                        static_cast<int>(governor.evaluate(first, 1000).kind));

  auto challenger = make_base_proposal(20, 7, 101);
  const auto decision = governor.evaluate(challenger, 1100);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kReject),
                        static_cast<int>(decision.kind));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceRejectReason::kInsufficientPriority),
                        static_cast<int>(decision.reject_reason));
}

void test_action_governance_preempts_when_priority_is_higher() {
  ncos::core::governance::ActionGovernor governor;

  const auto first = make_base_proposal(10, 3, 100);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kAllow),
                        static_cast<int>(governor.evaluate(first, 1000).kind));

  auto challenger = make_base_proposal(20, 9, 101);
  const auto decision = governor.evaluate(challenger, 1050);

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow),
      static_cast<int>(decision.kind));
  TEST_ASSERT_EQUAL_UINT16(20, decision.owner_service);
  TEST_ASSERT_EQUAL_UINT16(10, decision.preempted_owner_service);
}

void test_action_governance_debounces_duplicate_semantic_signal() {
  ncos::core::governance::ActionGovernor governor;

  const auto first = make_base_proposal(10, 5, 100);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kAllow),
                        static_cast<int>(governor.evaluate(first, 1000).kind));

  auto duplicate = make_base_proposal(10, 5, 101);
  const auto decision = governor.evaluate(duplicate, 1030);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kReject),
                        static_cast<int>(decision.kind));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceRejectReason::kSemanticDebounced),
                        static_cast<int>(decision.reject_reason));
  TEST_ASSERT_EQUAL_UINT32(1, governor.stats().debounced);
}

void test_action_governance_accepts_after_debounce_window() {
  ncos::core::governance::ActionGovernor governor;

  const auto first = make_base_proposal(10, 5, 100);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kAllow),
                        static_cast<int>(governor.evaluate(first, 1000).kind));

  auto later = make_base_proposal(10, 5, 101);
  const auto decision = governor.evaluate(
      later, 1000 + ncos::core::governance::ActionGovernor::kSemanticDebounceWindowMs + 1);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceDecisionKind::kAllow),
                        static_cast<int>(decision.kind));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_action_governance_allows_when_domain_is_free);
  RUN_TEST(test_action_governance_rejects_equal_or_lower_priority_preemption);
  RUN_TEST(test_action_governance_preempts_when_priority_is_higher);
  RUN_TEST(test_action_governance_debounces_duplicate_semantic_signal);
  RUN_TEST(test_action_governance_accepts_after_debounce_window);
  return UNITY_END();
}

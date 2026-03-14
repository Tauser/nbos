#include <unity.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/state/companion_state_store.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "core/state/companion_state_store.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_companion_state_keeps_structural_source_of_truth() {
  ncos::core::state::CompanionStateStore store;

  ncos::core::contracts::CompanionStructuralState structural{};
  structural.offline_first = true;
  structural.semantic_taxonomy_version = 1;
  structural.board_name = "esp32s3_dev";

  store.initialize(structural, 1000);
  const auto snap = store.snapshot();

  TEST_ASSERT_TRUE(snap.structural.offline_first);
  TEST_ASSERT_EQUAL_UINT16(1, snap.structural.semantic_taxonomy_version);
  TEST_ASSERT_EQUAL_STRING("esp32s3_dev", snap.structural.board_name);
}

void test_companion_state_updates_runtime_and_governance_snapshot() {
  ncos::core::state::CompanionStateStore store;

  ncos::core::contracts::CompanionStructuralState structural{};
  structural.semantic_taxonomy_version = 1;
  structural.board_name = "esp32s3_dev";
  store.initialize(structural, 1000);

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  runtime.safe_mode = false;
  runtime.fault_count = 0;
  runtime.governance_allowed_total = 2;
  runtime.governance_preempted_total = 0;
  runtime.governance_rejected_total = 0;

  store.ingest_runtime(runtime, 1050);
  const auto snap = store.snapshot();

  TEST_ASSERT_TRUE(snap.runtime.started);
  TEST_ASSERT_EQUAL_UINT32(2, snap.runtime.scheduler_tasks);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionPresenceMode::kAttending),
                        static_cast<int>(snap.runtime.presence_mode));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::GovernanceHealth::kStable),
                        static_cast<int>(snap.governance.health));
}

void test_companion_state_tracks_transient_governance_transition() {
  ncos::core::state::CompanionStateStore store;

  ncos::core::contracts::CompanionStructuralState structural{};
  structural.semantic_taxonomy_version = 1;
  store.initialize(structural, 1000);

  ncos::core::contracts::GovernanceDecision decision{};
  decision.kind = ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow;
  decision.proposal_trace_id = 777;
  decision.domain = ncos::core::contracts::ActionDomain::kMotion;
  decision.owner_service = 42;

  store.ingest_governance_decision(decision, 1100);
  const auto snap = store.snapshot();

  TEST_ASSERT_TRUE(snap.transient.has_active_trace);
  TEST_ASSERT_EQUAL_UINT32(777, snap.transient.active_trace_id);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::ActionDomain::kMotion),
                        static_cast<int>(snap.transient.active_domain));
  TEST_ASSERT_EQUAL_UINT16(42, snap.transient.active_owner_service);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_companion_state_keeps_structural_source_of_truth);
  RUN_TEST(test_companion_state_updates_runtime_and_governance_snapshot);
  RUN_TEST(test_companion_state_tracks_transient_governance_transition);
  return UNITY_END();
}

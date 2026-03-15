#include <unity.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/interaction_taxonomy.hpp"
#include "core/governance/action_governor.hpp"
#include "core/messaging/event_bus.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/interaction_taxonomy.cpp"
#include "core/contracts/action_governance_contracts.cpp"
#include "core/messaging/event_bus.cpp"
#include "core/governance/action_governor.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

struct LaneCounters {
  uint32_t events = 0;
  uint32_t commands = 0;
  uint32_t intents = 0;
  uint32_t reactions = 0;
};

void on_event(const ncos::core::contracts::EventMessage&, void* context) {
  auto* counters = static_cast<LaneCounters*>(context);
  if (counters != nullptr) {
    ++counters->events;
  }
}

void on_command(const ncos::core::contracts::CommandMessage&, void* context) {
  auto* counters = static_cast<LaneCounters*>(context);
  if (counters != nullptr) {
    ++counters->commands;
  }
}

void on_intent(const ncos::core::contracts::IntentMessage&, void* context) {
  auto* counters = static_cast<LaneCounters*>(context);
  if (counters != nullptr) {
    ++counters->intents;
  }
}

void on_reaction(const ncos::core::contracts::ReactionMessage&, void* context) {
  auto* counters = static_cast<LaneCounters*>(context);
  if (counters != nullptr) {
    ++counters->reactions;
  }
}

ncos::core::contracts::ActionProposal make_motion_proposal(uint16_t service_id,
                                                           uint8_t priority,
                                                           uint32_t trace_id,
                                                           ncos::core::contracts::PreemptionPolicy preemption) {
  ncos::core::contracts::ActionProposal proposal{};
  proposal.origin = ncos::core::contracts::ProposalOrigin::kIntent;
  proposal.trace_id = trace_id;
  proposal.requester_service = service_id;
  proposal.domain = ncos::core::contracts::ActionDomain::kMotion;
  proposal.action = ncos::core::contracts::CommandTopic::kMotionExecute;
  proposal.intent_context = ncos::core::contracts::IntentTopic::kInspectStimulus;
  proposal.priority = priority;
  proposal.ttl_ms = 120;
  proposal.preemption_policy = preemption;
  return proposal;
}

}  // namespace

void test_stress_event_bus_v2_handles_high_throughput_with_accountable_drops() {
  ncos::core::messaging::EventBusV2 bus;
  LaneCounters counters{};

  TEST_ASSERT_TRUE(bus.subscribe_events(&on_event, &counters));
  TEST_ASSERT_TRUE(bus.subscribe_commands(&on_command, &counters));
  TEST_ASSERT_TRUE(bus.subscribe_intents(&on_intent, &counters));
  TEST_ASSERT_TRUE(bus.subscribe_reactions(&on_reaction, &counters));

  constexpr uint32_t kIterations = 2500;

  for (uint32_t i = 1; i <= kIterations; ++i) {
    ncos::core::contracts::EventMessage event{};
    event.header.kind = ncos::core::contracts::SignalKind::kEvent;
    event.header.trace_id = i;
    event.header.timestamp_ms = i;
    event.topic = ncos::core::contracts::EventTopic::kSystemObservation;
    event.source_service = 31;

    ncos::core::contracts::CommandMessage command{};
    command.header.kind = ncos::core::contracts::SignalKind::kCommand;
    command.header.trace_id = i;
    command.header.timestamp_ms = i;
    command.topic = ncos::core::contracts::CommandTopic::kMotionExecute;
    command.issuer_service = 41;
    command.priority = static_cast<uint8_t>((i % 10) + 1);
    command.ttl_ms = 80;

    ncos::core::contracts::IntentMessage intent{};
    intent.header.kind = ncos::core::contracts::SignalKind::kIntent;
    intent.header.trace_id = i;
    intent.header.timestamp_ms = i;
    intent.topic = ncos::core::contracts::IntentTopic::kAttendUser;
    intent.derived_from_trace_id = i - 1;
    intent.confidence_percent = static_cast<uint8_t>(60 + (i % 40));

    ncos::core::contracts::ReactionMessage reaction{};
    reaction.header.kind = ncos::core::contracts::SignalKind::kReaction;
    reaction.header.trace_id = i;
    reaction.header.timestamp_ms = i;
    reaction.topic = ncos::core::contracts::ReactionTopic::kBlinkPulse;
    reaction.caused_by_trace_id = i;
    reaction.ttl_ms = 50;

    (void)bus.publish_event(event);
    (void)bus.publish_command(command);
    (void)bus.publish_intent(intent);
    (void)bus.publish_reaction(reaction);

    if ((i % 7U) != 0U) {
      bus.drain(2);
    }
  }

  for (uint32_t i = 0; i < 64; ++i) {
    bus.drain(4);
  }

  const auto stats = bus.stats();

  TEST_ASSERT_EQUAL_UINT32(kIterations, stats.published_events + stats.dropped_events);
  TEST_ASSERT_EQUAL_UINT32(kIterations, stats.published_commands + stats.dropped_commands);
  TEST_ASSERT_EQUAL_UINT32(kIterations, stats.published_intents + stats.dropped_intents);
  TEST_ASSERT_EQUAL_UINT32(kIterations, stats.published_reactions + stats.dropped_reactions);

  TEST_ASSERT_EQUAL_UINT32(stats.dispatched_events, counters.events);
  TEST_ASSERT_EQUAL_UINT32(stats.dispatched_commands, counters.commands);
  TEST_ASSERT_EQUAL_UINT32(stats.dispatched_intents, counters.intents);
  TEST_ASSERT_EQUAL_UINT32(stats.dispatched_reactions, counters.reactions);

  TEST_ASSERT_TRUE(stats.published_events > 0);
  TEST_ASSERT_TRUE(stats.published_commands > 0);
  TEST_ASSERT_TRUE(stats.published_intents > 0);
  TEST_ASSERT_TRUE(stats.published_reactions > 0);
}

void test_stress_action_governance_handles_conflict_churn_without_invalid_leases() {
  ncos::core::governance::ActionGovernor governor;

  constexpr uint32_t kIterations = 3200;
  uint64_t now_ms = 1000;

  for (uint32_t i = 1; i <= kIterations; ++i) {
    const uint16_t service_id = (i % 3U == 0U) ? 77 : 66;
    const uint8_t priority = static_cast<uint8_t>((i % 12U) + 1U);
    const auto preemption = (i % 4U == 0U)
                                ? ncos::core::contracts::PreemptionPolicy::kAllowAlways
                                : ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority;

    const auto decision = governor.evaluate(
        make_motion_proposal(service_id, priority, i, preemption), now_ms);

    TEST_ASSERT_TRUE(decision.kind == ncos::core::contracts::GovernanceDecisionKind::kAllow ||
                     decision.kind == ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow ||
                     decision.kind == ncos::core::contracts::GovernanceDecisionKind::kReject);

    const auto lease = governor.lease(ncos::core::contracts::ActionDomain::kMotion);
    if (lease.active) {
      TEST_ASSERT_TRUE(lease.owner_service == 66 || lease.owner_service == 77);
      TEST_ASSERT_TRUE(lease.priority <= 12);
    }

    now_ms += 40;
  }

  const auto stats = governor.stats();
  TEST_ASSERT_TRUE(stats.allowed > 0);
  TEST_ASSERT_TRUE(stats.preempted > 0);
  TEST_ASSERT_TRUE(stats.rejected > 0);

  const auto final_lease = governor.lease(ncos::core::contracts::ActionDomain::kMotion);
  TEST_ASSERT_TRUE(final_lease.active);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_stress_event_bus_v2_handles_high_throughput_with_accountable_drops);
  RUN_TEST(test_stress_action_governance_handles_conflict_churn_without_invalid_leases);
  return UNITY_END();
}

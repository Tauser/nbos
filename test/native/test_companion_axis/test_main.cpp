#include <unity.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/interaction_taxonomy.hpp"
#include "core/governance/action_governor.hpp"
#include "core/messaging/event_bus.hpp"
#include "core/runtime/companion_state_axis.hpp"
#include "core/state/companion_state_store.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "core/contracts/interaction_taxonomy.cpp"
#include "core/governance/action_governor.cpp"
#include "core/messaging/event_bus.cpp"
#include "core/runtime/companion_state_axis.cpp"
#include "core/state/companion_state_store.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_companion_axis_maps_intent_to_state_domains() {
  ncos::core::messaging::EventBusV2 bus;
  ncos::core::governance::ActionGovernor governor;
  ncos::core::state::CompanionStateStore state;
  ncos::core::runtime::CompanionStateAxis axis;

  TEST_ASSERT_TRUE(state.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));
  TEST_ASSERT_TRUE(axis.bind(&bus, &state, &governor));

  ncos::core::contracts::IntentMessage intent{};
  intent.header.kind = ncos::core::contracts::SignalKind::kIntent;
  intent.header.trace_id = 10;
  intent.header.timestamp_ms = 1100;
  intent.topic = ncos::core::contracts::IntentTopic::kAttendUser;
  intent.confidence_percent = 80;

  TEST_ASSERT_TRUE(bus.publish_intent(intent));
  bus.drain(4);

  const auto snapshot = state.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kListening),
                        static_cast<int>(snapshot.interactional.phase));
  TEST_ASSERT_TRUE(snapshot.interactional.session_active);
}

void test_companion_axis_routes_command_through_governance_to_transient_state() {
  ncos::core::messaging::EventBusV2 bus;
  ncos::core::governance::ActionGovernor governor;
  ncos::core::state::CompanionStateStore state;
  ncos::core::runtime::CompanionStateAxis axis;

  TEST_ASSERT_TRUE(state.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));
  TEST_ASSERT_TRUE(axis.bind(&bus, &state, &governor));

  ncos::core::contracts::CommandMessage command{};
  command.header.kind = ncos::core::contracts::SignalKind::kCommand;
  command.header.trace_id = 77;
  command.header.timestamp_ms = 1200;
  command.topic = ncos::core::contracts::CommandTopic::kFaceRenderExecute;
  command.issuer_service = 21;
  command.priority = 8;
  command.ttl_ms = 300;

  TEST_ASSERT_TRUE(bus.publish_command(command));
  bus.drain(4);

  const auto snapshot = state.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_TRUE(snapshot.transient.has_active_trace);
  TEST_ASSERT_EQUAL_UINT32(77, snapshot.transient.active_trace_id);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::ActionDomain::kFace),
                        static_cast<int>(snapshot.transient.active_domain));
  TEST_ASSERT_EQUAL_UINT16(21, snapshot.transient.active_owner_service);
}

void test_companion_axis_maps_reaction_to_emotional_state() {
  ncos::core::messaging::EventBusV2 bus;
  ncos::core::governance::ActionGovernor governor;
  ncos::core::state::CompanionStateStore state;
  ncos::core::runtime::CompanionStateAxis axis;

  TEST_ASSERT_TRUE(state.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));
  TEST_ASSERT_TRUE(axis.bind(&bus, &state, &governor));

  ncos::core::contracts::ReactionMessage reaction{};
  reaction.header.kind = ncos::core::contracts::SignalKind::kReaction;
  reaction.header.trace_id = 88;
  reaction.header.timestamp_ms = 1300;
  reaction.topic = ncos::core::contracts::ReactionTopic::kEarconChirp;
  reaction.ttl_ms = 50;

  TEST_ASSERT_TRUE(bus.publish_reaction(reaction));
  bus.drain(4);

  const auto snapshot = state.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EmotionalTone::kAffiliative),
                        static_cast<int>(snapshot.emotional.tone));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::emotion::EmotionPhase::kEngaged),
                        static_cast<int>(snapshot.emotional.phase));
  TEST_ASSERT_EQUAL_INT8(42, snapshot.emotional.vector.valence_percent);
  TEST_ASSERT_EQUAL_UINT8(76, snapshot.emotional.vector.social_engagement_percent);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_companion_axis_maps_intent_to_state_domains);
  RUN_TEST(test_companion_axis_routes_command_through_governance_to_transient_state);
  RUN_TEST(test_companion_axis_maps_reaction_to_emotional_state);
  return UNITY_END();
}

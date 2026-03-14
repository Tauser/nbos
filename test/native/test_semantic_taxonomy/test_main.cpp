#include <unity.h>

#include "core/contracts/interaction_taxonomy.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/interaction_taxonomy.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_messages_keep_semantic_kind_separation() {
  ncos::core::contracts::EventMessage event{};
  event.header.kind = ncos::core::contracts::SignalKind::kEvent;
  event.header.timestamp_ms = 10;

  ncos::core::contracts::CommandMessage command{};
  command.header.kind = ncos::core::contracts::SignalKind::kCommand;
  command.header.timestamp_ms = 10;
  command.ttl_ms = 100;

  ncos::core::contracts::IntentMessage intent{};
  intent.header.kind = ncos::core::contracts::SignalKind::kIntent;
  intent.header.timestamp_ms = 10;
  intent.confidence_percent = 80;

  ncos::core::contracts::ReactionMessage reaction{};
  reaction.header.kind = ncos::core::contracts::SignalKind::kReaction;
  reaction.header.timestamp_ms = 10;
  reaction.ttl_ms = 80;

  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(event));
  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(command));
  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(intent));
  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(reaction));
}

void test_invalid_when_signal_kind_is_mixed() {
  ncos::core::contracts::EventMessage event{};
  event.header.kind = ncos::core::contracts::SignalKind::kCommand;
  event.header.timestamp_ms = 10;

  ncos::core::contracts::IntentMessage intent{};
  intent.header.kind = ncos::core::contracts::SignalKind::kIntent;
  intent.header.timestamp_ms = 10;
  intent.confidence_percent = 101;

  TEST_ASSERT_FALSE(ncos::core::contracts::is_valid(event));
  TEST_ASSERT_FALSE(ncos::core::contracts::is_valid(intent));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_messages_keep_semantic_kind_separation);
  RUN_TEST(test_invalid_when_signal_kind_is_mixed);
  return UNITY_END();
}

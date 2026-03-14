#include <unity.h>

#include "core/contracts/interaction_taxonomy.hpp"
#include "core/messaging/event_bus_v2.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/interaction_taxonomy.cpp"
#include "core/messaging/event_bus_v2.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

uint32_t g_event_counter = 0;
uint32_t g_command_counter = 0;
uint8_t g_last_command_priority = 0;

void on_event(const ncos::core::contracts::EventMessage&, void*) {
  ++g_event_counter;
}

void on_command(const ncos::core::contracts::CommandMessage& message, void*) {
  ++g_command_counter;
  g_last_command_priority = message.priority;
}

}  // namespace

void test_event_bus_v2_delivers_semantic_event_lane() {
  ncos::core::messaging::EventBusV2 bus;
  g_event_counter = 0;

  TEST_ASSERT_TRUE(bus.subscribe_events(&on_event, nullptr));

  ncos::core::contracts::EventMessage event{};
  event.header.kind = ncos::core::contracts::SignalKind::kEvent;
  event.header.timestamp_ms = 100;
  event.header.trace_id = 1;
  event.topic = ncos::core::contracts::EventTopic::kSystemObservation;

  TEST_ASSERT_TRUE(bus.publish_event(event));
  bus.drain(4);

  TEST_ASSERT_EQUAL_UINT32(1, g_event_counter);
}

void test_event_bus_v2_sorts_commands_by_priority() {
  ncos::core::messaging::EventBusV2 bus;
  g_command_counter = 0;
  g_last_command_priority = 0;

  TEST_ASSERT_TRUE(bus.subscribe_commands(&on_command, nullptr));

  ncos::core::contracts::CommandMessage low{};
  low.header.kind = ncos::core::contracts::SignalKind::kCommand;
  low.header.timestamp_ms = 100;
  low.header.trace_id = 10;
  low.priority = 1;
  low.ttl_ms = 200;

  ncos::core::contracts::CommandMessage high = low;
  high.header.trace_id = 11;
  high.priority = 9;

  TEST_ASSERT_TRUE(bus.publish_command(low));
  TEST_ASSERT_TRUE(bus.publish_command(high));

  bus.drain(1);

  TEST_ASSERT_EQUAL_UINT32(1, g_command_counter);
  TEST_ASSERT_EQUAL_UINT8(9, g_last_command_priority);
}

void test_event_bus_v2_rejects_invalid_message_kind() {
  ncos::core::messaging::EventBusV2 bus;

  ncos::core::contracts::EventMessage invalid{};
  invalid.header.kind = ncos::core::contracts::SignalKind::kIntent;
  invalid.header.timestamp_ms = 10;

  TEST_ASSERT_FALSE(bus.publish_event(invalid));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_event_bus_v2_delivers_semantic_event_lane);
  RUN_TEST(test_event_bus_v2_sorts_commands_by_priority);
  RUN_TEST(test_event_bus_v2_rejects_invalid_message_kind);
  return UNITY_END();
}

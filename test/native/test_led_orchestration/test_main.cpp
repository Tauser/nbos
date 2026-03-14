#include <unity.h>

#include "services/led/led_service.hpp"

// Native tests run with test_build_src = no.
#include "services/led/led_service.cpp"

namespace {

class FakeLedPort final : public ncos::interfaces::led::LedPort {
 public:
  bool ensure_ready() override {
    return ready;
  }

  bool apply_state(const ncos::core::contracts::LedState& state) override {
    ++apply_calls;
    last_state = state;
    return apply_ok;
  }

  bool ready = true;
  bool apply_ok = true;
  uint32_t apply_calls = 0;
  ncos::core::contracts::LedState last_state{};
};

ncos::core::contracts::LedRequest make_request(uint16_t owner, ncos::core::contracts::LedPriority prio,
                                               uint64_t expires_ms, uint8_t r, uint8_t g, uint8_t b) {
  ncos::core::contracts::LedRequest request{};
  request.active = true;
  request.owner_service = owner;
  request.priority = prio;
  request.expires_at_ms = expires_ms;
  request.state.red = r;
  request.state.green = g;
  request.state.blue = b;
  request.state.intensity_percent = 40;
  request.state.pattern = ncos::core::contracts::LedPattern::kSolid;
  return request;
}

}  // namespace

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_led_orchestration_picks_higher_priority() {
  FakeLedPort port{};
  ncos::services::led::LedService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(100));

  TEST_ASSERT_TRUE(service.submit_request(make_request(11, ncos::core::contracts::LedPriority::kLow, 1000, 1, 2, 3), 100));
  TEST_ASSERT_TRUE(service.submit_request(make_request(22, ncos::core::contracts::LedPriority::kHigh, 1000, 9, 9, 9), 100));

  service.tick(100, 50);

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.last_apply_ok);
  TEST_ASSERT_EQUAL_UINT16(22, state.applied_owner);
  TEST_ASSERT_EQUAL_UINT8(9, state.applied_state.red);
}

void test_led_orchestration_falls_back_to_off_when_requests_expire() {
  FakeLedPort port{};
  ncos::services::led::LedService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(100));

  TEST_ASSERT_TRUE(service.submit_request(make_request(33, ncos::core::contracts::LedPriority::kMedium, 120, 8, 1, 1), 100));
  service.tick(100, 50);
  TEST_ASSERT_EQUAL_UINT16(33, service.state().applied_owner);

  service.tick(200, 50);
  TEST_ASSERT_EQUAL_UINT16(0, service.state().applied_owner);
  TEST_ASSERT_EQUAL_UINT8(0, service.state().applied_state.intensity_percent);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_led_orchestration_picks_higher_priority);
  RUN_TEST(test_led_orchestration_falls_back_to_off_when_requests_expire);
  return UNITY_END();
}

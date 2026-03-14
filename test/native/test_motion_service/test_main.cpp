#include <unity.h>

#include "services/motion/motion_service.hpp"

// Native tests run with test_build_src = no.
#include "services/motion/motion_service.cpp"

namespace {

class FakeMotionPort final : public ncos::interfaces::motion::MotionPort {
 public:
  bool has_console_pin_conflict() const override {
    return conflict;
  }

  bool ensure_ready() override {
    return ready;
  }

  bool apply_pose(const ncos::core::contracts::MotionPoseCommand& pose, size_t* out_tx_bytes) override {
    ++apply_calls;
    last_pose = pose;
    if (out_tx_bytes != nullptr) {
      *out_tx_bytes = tx_bytes;
    }
    return apply_ok;
  }

  bool conflict = false;
  bool ready = true;
  bool apply_ok = true;
  size_t tx_bytes = 9;
  uint32_t apply_calls = 0;
  ncos::core::contracts::MotionPoseCommand last_pose{};
};

}  // namespace

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_motion_service_applies_neutral_pose_on_initialize() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);

  TEST_ASSERT_TRUE(service.initialize(100));
  const auto& state = service.state();

  TEST_ASSERT_TRUE(state.initialized);
  TEST_ASSERT_TRUE(state.neutral_applied);
  TEST_ASSERT_TRUE(state.last_apply_ok);
  TEST_ASSERT_EQUAL_UINT32(1, fake.apply_calls);
  TEST_ASSERT_EQUAL_INT16(0, fake.last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(0, fake.last_pose.pitch_permille);
}

void test_motion_service_blocks_when_console_conflicts_with_ttlinker() {
  FakeMotionPort fake{};
  fake.conflict = true;

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);

  TEST_ASSERT_FALSE(service.initialize(200));
  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.console_pin_conflict);
  TEST_ASSERT_FALSE(state.initialized);
  TEST_ASSERT_EQUAL_UINT32(0, fake.apply_calls);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_motion_service_applies_neutral_pose_on_initialize);
  RUN_TEST(test_motion_service_blocks_when_console_conflicts_with_ttlinker);
  return UNITY_END();
}

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
  TEST_ASSERT_TRUE(state.has_active_plan);
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

void test_motion_service_rejects_lower_priority_during_hold_window() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(1000));

  ncos::core::contracts::MotionCommand high_priority{};
  high_priority.intent = ncos::core::contracts::MotionIntent::kAttendUser;
  high_priority.priority = ncos::core::contracts::MotionPriority::kHigh;
  high_priority.pose.yaw_permille = 200;
  high_priority.hold_ms = 500;

  TEST_ASSERT_TRUE(service.request_motion(high_priority, 1100));

  ncos::core::contracts::MotionCommand low_priority{};
  low_priority.intent = ncos::core::contracts::MotionIntent::kObserveStimulus;
  low_priority.priority = ncos::core::contracts::MotionPriority::kLow;
  low_priority.pose.yaw_permille = -200;
  low_priority.hold_ms = 100;

  TEST_ASSERT_FALSE(service.request_motion(low_priority, 1200));
  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.rejected_for_priority);
  TEST_ASSERT_EQUAL_UINT32(1, state.plan_rejected_total);
  TEST_ASSERT_EQUAL_INT16(200, state.last_pose.yaw_permille);
}

void test_motion_service_updates_companion_and_face_signals() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(2000));

  ncos::core::contracts::MotionCompanionSignal companion{};
  companion.safe_mode = true;
  companion.attention_lock = true;
  companion.emotional_arousal_percent = 78;
  service.update_companion_signal(companion, 2100);

  ncos::core::contracts::MotionFaceSignal face{};
  face.gaze_x_percent = 40;
  face.gaze_y_percent = -10;
  face.clip_active = true;
  service.update_face_signal(face, 2200);

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.companion_signal.safe_mode);
  TEST_ASSERT_TRUE(state.companion_signal.attention_lock);
  TEST_ASSERT_EQUAL_UINT8(78, state.companion_signal.emotional_arousal_percent);
  TEST_ASSERT_EQUAL_INT8(40, state.face_signal.gaze_x_percent);
  TEST_ASSERT_EQUAL_INT8(-10, state.face_signal.gaze_y_percent);
  TEST_ASSERT_TRUE(state.face_signal.clip_active);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_motion_service_applies_neutral_pose_on_initialize);
  RUN_TEST(test_motion_service_blocks_when_console_conflicts_with_ttlinker);
  RUN_TEST(test_motion_service_rejects_lower_priority_during_hold_window);
  RUN_TEST(test_motion_service_updates_companion_and_face_signals);
  return UNITY_END();
}

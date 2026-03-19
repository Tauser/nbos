#include <unity.h>

#include "services/motion/motion_service.hpp"

// Native tests run with test_build_src = no.
#include "services/motion/motion_service.cpp"

namespace {

class FakeMotionPort final : public ncos::interfaces::motion::MotionPort {
 public:
  bool has_transport_conflict() const override {
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
  TEST_ASSERT_TRUE(state.transport_conflict);
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

  TEST_ASSERT_TRUE(service.request_motion(high_priority, 1300));

  ncos::core::contracts::MotionCommand low_priority{};
  low_priority.intent = ncos::core::contracts::MotionIntent::kObserveStimulus;
  low_priority.priority = ncos::core::contracts::MotionPriority::kLow;
  low_priority.pose.yaw_permille = -200;
  low_priority.hold_ms = 100;

  TEST_ASSERT_FALSE(service.request_motion(low_priority, 1400));
  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.rejected_for_priority);
  TEST_ASSERT_EQUAL_UINT32(1, state.plan_rejected_total);
  TEST_ASSERT_EQUAL_INT16(140, state.last_pose.yaw_permille);
}

void test_motion_service_clamps_pose_to_safe_limits() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(2000));

  ncos::core::contracts::MotionCommand command{};
  command.intent = ncos::core::contracts::MotionIntent::kDirectPose;
  command.priority = ncos::core::contracts::MotionPriority::kHigh;
  command.pose.yaw_permille = 900;
  command.pose.pitch_permille = -900;
  command.pose.speed_percent = 95;
  command.hold_ms = 200;

  TEST_ASSERT_TRUE(service.request_motion(command, 2300));

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.safety_clamp_applied);
  TEST_ASSERT_EQUAL_UINT32(1, state.safety_clamp_total);
  TEST_ASSERT_EQUAL_INT16(140, state.last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(-120, state.last_pose.pitch_permille);
  TEST_ASSERT_EQUAL_UINT16(70, state.last_pose.speed_percent);
}

void test_motion_service_recovery_command_returns_to_neutral() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(3000));

  ncos::core::contracts::MotionCommand command{};
  command.intent = ncos::core::contracts::MotionIntent::kDirectPose;
  command.priority = ncos::core::contracts::MotionPriority::kHigh;
  command.pose.yaw_permille = 260;
  command.pose.pitch_permille = 120;
  command.hold_ms = 200;
  TEST_ASSERT_TRUE(service.request_motion(command, 3300));

  TEST_ASSERT_TRUE(service.recover_to_neutral(3600));
  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.neutral_applied);
  TEST_ASSERT_EQUAL_INT16(0, state.last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(0, state.last_pose.pitch_permille);
  TEST_ASSERT_EQUAL_UINT16(35, state.last_pose.speed_percent);
}

void test_motion_service_tick_aligns_head_with_face_signal() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(4000));

  ncos::core::contracts::MotionFaceSignal face{};
  face.gaze_x_percent = 30;
  face.gaze_y_percent = -20;
  face.clip_active = false;
  service.update_face_signal(face, 4200);

  service.tick(4300);

  const auto& state = service.state();
  TEST_ASSERT_GREATER_THAN_UINT32(1, state.apply_success_total);
  TEST_ASSERT_EQUAL_INT16(140, state.last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(-100, state.last_pose.pitch_permille);
}

void test_motion_service_tick_uses_companion_safe_mode_for_recovery() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(5000));

  ncos::core::contracts::MotionCommand command{};
  command.intent = ncos::core::contracts::MotionIntent::kDirectPose;
  command.priority = ncos::core::contracts::MotionPriority::kHigh;
  command.pose.yaw_permille = 220;
  command.pose.pitch_permille = 120;
  command.hold_ms = 100;
  TEST_ASSERT_TRUE(service.request_motion(command, 5300));

  ncos::core::contracts::MotionCompanionSignal companion{};
  companion.safe_mode = true;
  service.update_companion_signal(companion, 5400);

  service.tick(5800);

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.neutral_applied);
  TEST_ASSERT_EQUAL_INT16(0, state.last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(0, state.last_pose.pitch_permille);
}

void test_motion_service_stale_face_signal_guard_returns_to_neutral() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(6000));

  ncos::core::contracts::MotionCommand command{};
  command.intent = ncos::core::contracts::MotionIntent::kDirectPose;
  command.priority = ncos::core::contracts::MotionPriority::kHigh;
  command.pose.yaw_permille = 180;
  command.pose.pitch_permille = 90;
  command.hold_ms = 100;
  TEST_ASSERT_TRUE(service.request_motion(command, 6300));

  ncos::core::contracts::MotionFaceSignal face{};
  face.gaze_x_percent = 20;
  face.gaze_y_percent = 5;
  service.update_face_signal(face, 6400);

  service.tick(8001);

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.stale_face_guard_active);
  TEST_ASSERT_TRUE(state.neutral_applied);
  TEST_ASSERT_EQUAL_INT16(0, state.last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(0, state.last_pose.pitch_permille);
}

void test_motion_service_fail_safe_guard_blocks_non_recovery_until_neutral() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(7000));

  fake.apply_ok = false;

  ncos::core::contracts::MotionCommand command{};
  command.intent = ncos::core::contracts::MotionIntent::kDirectPose;
  command.priority = ncos::core::contracts::MotionPriority::kNormal;
  command.pose.yaw_permille = 100;
  command.pose.pitch_permille = 40;
  command.hold_ms = 100;

  TEST_ASSERT_FALSE(service.request_motion(command, 7300));
  TEST_ASSERT_FALSE(service.request_motion(command, 7400));
  TEST_ASSERT_FALSE(service.request_motion(command, 7500));

  TEST_ASSERT_TRUE(service.state().fail_safe_guard_active);

  fake.apply_ok = true;
  TEST_ASSERT_FALSE(service.request_motion(command, 7600));
  TEST_ASSERT_TRUE(service.recover_to_neutral(7700));
  TEST_ASSERT_FALSE(service.state().fail_safe_guard_active);

  TEST_ASSERT_TRUE(service.request_motion(command, 8400));
}

void test_motion_service_attention_lock_drives_attentive_pose_and_returns_to_neutral() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(9500));

  ncos::core::contracts::MotionCompanionSignal companion{};
  companion.attention_lock = true;
  companion.emotional_arousal_percent = 60;
  companion.product_state = ncos::core::contracts::CompanionProductState::kAttendUser;
  service.update_companion_signal(companion, 9600);

  service.tick(9800);

  TEST_ASSERT_GREATER_THAN_UINT32(1, service.state().apply_success_total);
  TEST_ASSERT_EQUAL_INT16(0, service.state().last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(55, service.state().last_pose.pitch_permille);

  companion.attention_lock = false;
  companion.product_state = ncos::core::contracts::CompanionProductState::kIdleObserve;
  service.update_companion_signal(companion, 10000);
  service.tick(10200);

  TEST_ASSERT_TRUE(service.state().neutral_applied);
  TEST_ASSERT_EQUAL_INT16(0, service.state().last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(0, service.state().last_pose.pitch_permille);
}

void test_motion_service_sleep_state_drives_power_save_pose() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(10400));

  ncos::core::contracts::MotionCompanionSignal companion{};
  companion.product_state = ncos::core::contracts::CompanionProductState::kSleep;
  companion.emotional_arousal_percent = 20;
  service.update_companion_signal(companion, 10500);

  service.tick(10700);

  TEST_ASSERT_FALSE(service.state().neutral_applied);
  TEST_ASSERT_EQUAL_INT16(0, service.state().last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(-40, service.state().last_pose.pitch_permille);
  TEST_ASSERT_LESS_THAN_UINT16(25, service.state().last_pose.speed_percent);
}

void test_motion_service_responding_state_uses_stronger_attend_pose() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(11200));

  ncos::core::contracts::MotionCompanionSignal companion{};
  companion.product_state = ncos::core::contracts::CompanionProductState::kResponding;
  companion.emotional_arousal_percent = 60;
  service.update_companion_signal(companion, 11300);

  service.tick(11500);

  TEST_ASSERT_EQUAL_INT16(0, service.state().last_pose.yaw_permille);
  TEST_ASSERT_EQUAL_INT16(70, service.state().last_pose.pitch_permille);
}
void test_motion_service_updates_companion_and_face_signals() {
  FakeMotionPort fake{};

  ncos::services::motion::MotionService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(9000));

  ncos::core::contracts::MotionCompanionSignal companion{};
  companion.safe_mode = true;
  companion.attention_lock = true;
  companion.emotional_arousal_percent = 78;
  companion.product_state = ncos::core::contracts::CompanionProductState::kAlertScan;
  service.update_companion_signal(companion, 9100);

  ncos::core::contracts::MotionFaceSignal face{};
  face.gaze_x_percent = 40;
  face.gaze_y_percent = -10;
  face.clip_active = true;
  service.update_face_signal(face, 9200);

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.companion_signal.safe_mode);
  TEST_ASSERT_TRUE(state.companion_signal.attention_lock);
  TEST_ASSERT_EQUAL_UINT8(78, state.companion_signal.emotional_arousal_percent);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kAlertScan),
                        static_cast<int>(state.companion_signal.product_state));
  TEST_ASSERT_EQUAL_INT8(40, state.face_signal.gaze_x_percent);
  TEST_ASSERT_EQUAL_INT8(-10, state.face_signal.gaze_y_percent);
  TEST_ASSERT_TRUE(state.face_signal.clip_active);
  TEST_ASSERT_EQUAL_UINT64(9100, state.last_companion_signal_ms);
  TEST_ASSERT_EQUAL_UINT64(9200, state.last_face_signal_ms);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_motion_service_applies_neutral_pose_on_initialize);
  RUN_TEST(test_motion_service_blocks_when_console_conflicts_with_ttlinker);
  RUN_TEST(test_motion_service_rejects_lower_priority_during_hold_window);
  RUN_TEST(test_motion_service_clamps_pose_to_safe_limits);
  RUN_TEST(test_motion_service_recovery_command_returns_to_neutral);
  RUN_TEST(test_motion_service_tick_aligns_head_with_face_signal);
  RUN_TEST(test_motion_service_tick_uses_companion_safe_mode_for_recovery);
  RUN_TEST(test_motion_service_stale_face_signal_guard_returns_to_neutral);
  RUN_TEST(test_motion_service_fail_safe_guard_blocks_non_recovery_until_neutral);
  RUN_TEST(test_motion_service_attention_lock_drives_attentive_pose_and_returns_to_neutral);
  RUN_TEST(test_motion_service_sleep_state_drives_power_save_pose);
  RUN_TEST(test_motion_service_responding_state_uses_stronger_attend_pose);
  RUN_TEST(test_motion_service_updates_companion_and_face_signals);
  return UNITY_END();
}


#include <unity.h>

#include "core/contracts/companion_personality_contracts.hpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_companion_personality_face_profiles_keep_response_focused_and_warm() {
  const auto idle = ncos::core::contracts::personality_face_profile(
      ncos::core::contracts::PersonalityFaceMode::kIdleObserve);
  const auto warm_user = ncos::core::contracts::personality_face_profile(
      ncos::core::contracts::PersonalityFaceMode::kWarmUser);
  const auto attend = ncos::core::contracts::personality_face_profile(
      ncos::core::contracts::PersonalityFaceMode::kAttendUser);
  const auto responding = ncos::core::contracts::personality_face_profile(
      ncos::core::contracts::PersonalityFaceMode::kResponding);

  TEST_ASSERT_GREATER_THAN_UINT8(idle.focus_percent, attend.focus_percent);
  TEST_ASSERT_GREATER_THAN_UINT8(attend.focus_percent, responding.focus_percent);
  TEST_ASSERT_GREATER_THAN_UINT8(idle.salience_percent, warm_user.salience_percent);
  TEST_ASSERT_LESS_THAN_UINT16(idle.cadence_ms, responding.cadence_ms);
}

void test_companion_personality_motion_profiles_preserve_composure_and_sociability() {
  const auto rest = ncos::core::contracts::personality_motion_profile(
      ncos::core::contracts::PersonalityMotionMode::kRest);
  const auto attend = ncos::core::contracts::personality_motion_profile(
      ncos::core::contracts::PersonalityMotionMode::kAttendUser);
  const auto responding = ncos::core::contracts::personality_motion_profile(
      ncos::core::contracts::PersonalityMotionMode::kResponding);
  const auto alert = ncos::core::contracts::personality_motion_profile(
      ncos::core::contracts::PersonalityMotionMode::kAlertScan);

  TEST_ASSERT_LESS_THAN_INT16(0, rest.pitch_permille);
  TEST_ASSERT_GREATER_THAN_INT16(alert.pitch_permille, attend.pitch_permille);
  TEST_ASSERT_GREATER_THAN_INT16(attend.pitch_permille, responding.pitch_permille);
  TEST_ASSERT_GREATER_THAN_UINT16(rest.base_speed_percent, attend.base_speed_percent);
  TEST_ASSERT_GREATER_THAN_UINT16(attend.base_speed_percent, responding.base_speed_percent);
}

void test_companion_personality_timing_keeps_continuity_short_and_predictable() {
  TEST_ASSERT_GREATER_THAN_UINT32(
      ncos::core::contracts::personality_continuity_window_ms(
          ncos::core::contracts::PersonalityContinuityKind::kStimulus),
      ncos::core::contracts::personality_continuity_window_ms(
          ncos::core::contracts::PersonalityContinuityKind::kUser));
  TEST_ASSERT_GREATER_THAN_UINT32(0, ncos::core::contracts::personality_reengagement_ttl_ms());
  TEST_ASSERT_LESS_THAN_UINT32(
      ncos::core::contracts::personality_behavior_ttl_ms(
          ncos::core::contracts::BehaviorProfile::kAttendUser),
      ncos::core::contracts::personality_reengagement_ttl_ms());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_companion_personality_face_profiles_keep_response_focused_and_warm);
  RUN_TEST(test_companion_personality_motion_profiles_preserve_composure_and_sociability);
  RUN_TEST(test_companion_personality_timing_keeps_continuity_short_and_predictable);
  return UNITY_END();
}

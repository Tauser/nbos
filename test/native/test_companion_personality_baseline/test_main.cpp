#include <unity.h>

#include "core/contracts/companion_personality_contracts.hpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_companion_personality_face_profiles_keep_response_focused_and_warm() {
  const auto personality = ncos::core::contracts::make_companion_personality_state();
  const auto idle = ncos::core::contracts::personality_face_profile(
      personality, ncos::core::contracts::PersonalityFaceMode::kIdleObserve);
  const auto warm_user = ncos::core::contracts::personality_face_profile(
      personality, ncos::core::contracts::PersonalityFaceMode::kWarmUser);
  const auto attend = ncos::core::contracts::personality_face_profile(
      personality, ncos::core::contracts::PersonalityFaceMode::kAttendUser);
  const auto responding = ncos::core::contracts::personality_face_profile(
      personality, ncos::core::contracts::PersonalityFaceMode::kResponding);

  TEST_ASSERT_GREATER_THAN_UINT8(idle.focus_percent, attend.focus_percent);
  TEST_ASSERT_GREATER_THAN_UINT8(attend.focus_percent, responding.focus_percent);
  TEST_ASSERT_GREATER_THAN_UINT8(idle.salience_percent, warm_user.salience_percent);
  TEST_ASSERT_LESS_THAN_UINT16(idle.cadence_ms, responding.cadence_ms);
}

void test_companion_personality_motion_profiles_preserve_composure_and_sociability() {
  const auto personality = ncos::core::contracts::make_companion_personality_state();
  const auto rest = ncos::core::contracts::personality_motion_profile(
      personality, ncos::core::contracts::PersonalityMotionMode::kRest);
  const auto attend = ncos::core::contracts::personality_motion_profile(
      personality, ncos::core::contracts::PersonalityMotionMode::kAttendUser);
  const auto responding = ncos::core::contracts::personality_motion_profile(
      personality, ncos::core::contracts::PersonalityMotionMode::kResponding);
  const auto alert = ncos::core::contracts::personality_motion_profile(
      personality, ncos::core::contracts::PersonalityMotionMode::kAlertScan);

  TEST_ASSERT_LESS_THAN_INT16(0, rest.pitch_permille);
  TEST_ASSERT_GREATER_THAN_INT16(alert.pitch_permille, attend.pitch_permille);
  TEST_ASSERT_GREATER_THAN_INT16(attend.pitch_permille, responding.pitch_permille);
  TEST_ASSERT_GREATER_THAN_UINT16(rest.base_speed_percent, attend.base_speed_percent);
  TEST_ASSERT_GREATER_THAN_UINT16(attend.base_speed_percent, responding.base_speed_percent);
}

void test_companion_personality_continuity_gates_stay_consistent_across_channels() {
  const auto personality = ncos::core::contracts::make_companion_personality_state();

  TEST_ASSERT_GREATER_THAN_UINT8(
      ncos::core::contracts::personality_continuity_engagement_threshold_percent(
          personality, ncos::core::contracts::PersonalityContinuityKind::kStimulus),
      ncos::core::contracts::personality_continuity_engagement_threshold_percent(
          personality, ncos::core::contracts::PersonalityContinuityKind::kUser));
  TEST_ASSERT_EQUAL_UINT8(56, ncos::core::contracts::personality_continuity_engagement_threshold_percent(
                                   personality, ncos::core::contracts::PersonalityContinuityKind::kUser));
  TEST_ASSERT_EQUAL_UINT8(50, ncos::core::contracts::personality_continuity_engagement_threshold_percent(
                                   personality, ncos::core::contracts::PersonalityContinuityKind::kStimulus));
}

void test_companion_personality_timing_keeps_continuity_short_and_predictable() {
  TEST_ASSERT_GREATER_THAN_UINT32(
      ncos::core::contracts::personality_continuity_window_ms(
          ncos::core::contracts::make_companion_personality_state(),
          ncos::core::contracts::PersonalityContinuityKind::kStimulus),
      ncos::core::contracts::personality_continuity_window_ms(
          ncos::core::contracts::make_companion_personality_state(),
          ncos::core::contracts::PersonalityContinuityKind::kUser));
  TEST_ASSERT_GREATER_THAN_UINT32(0, ncos::core::contracts::personality_reengagement_ttl_ms(
                                     ncos::core::contracts::make_companion_personality_state()));
  TEST_ASSERT_LESS_THAN_UINT32(
      ncos::core::contracts::personality_behavior_ttl_ms(
          ncos::core::contracts::make_companion_personality_state(),
          ncos::core::contracts::BehaviorProfile::kAttendUser),
      ncos::core::contracts::personality_reengagement_ttl_ms(
          ncos::core::contracts::make_companion_personality_state()));
}

void test_companion_personality_adaptive_layer_is_bounded_and_separate_from_fixed_traits() {
  auto personality = ncos::core::contracts::make_companion_personality_state();
  personality.adaptive_social_warmth_bias_percent = 22;
  personality.adaptive_response_energy_bias_percent = -17;
  personality.adaptive_continuity_window_bias_ms = 1800;

  TEST_ASSERT_EQUAL_UINT8(68, personality.warmth_percent);
  TEST_ASSERT_EQUAL_UINT8(58, personality.curiosity_percent);
  TEST_ASSERT_EQUAL_INT8(10,
                         ncos::core::contracts::personality_social_warmth_bias_percent(personality));
  TEST_ASSERT_EQUAL_INT8(
      -8, ncos::core::contracts::personality_response_energy_bias_percent(personality));
  TEST_ASSERT_EQUAL_INT16(
      600, ncos::core::contracts::personality_continuity_window_bias_ms(personality));
  TEST_ASSERT_EQUAL_UINT64(
      3800, ncos::core::contracts::personality_continuity_window_ms(
                personality, ncos::core::contracts::PersonalityContinuityKind::kUser));
}

void test_companion_personality_adaptive_biases_modulate_outputs_without_changing_baseline_traits() {
  auto personality = ncos::core::contracts::make_companion_personality_state();
  personality.adaptive_social_warmth_bias_percent = 8;
  personality.adaptive_response_energy_bias_percent = 6;
  personality.adaptive_continuity_window_bias_ms = 400;

  const auto baseline = ncos::core::contracts::make_companion_personality_state();
  const auto warm_user = ncos::core::contracts::personality_face_profile(
      personality, ncos::core::contracts::PersonalityFaceMode::kWarmUser);
  const auto baseline_warm_user = ncos::core::contracts::personality_face_profile(
      baseline, ncos::core::contracts::PersonalityFaceMode::kWarmUser);
  const auto responding = ncos::core::contracts::personality_motion_profile(
      personality, ncos::core::contracts::PersonalityMotionMode::kResponding);
  const auto baseline_responding = ncos::core::contracts::personality_motion_profile(
      baseline, ncos::core::contracts::PersonalityMotionMode::kResponding);

  TEST_ASSERT_GREATER_THAN_UINT8(baseline_warm_user.salience_percent, warm_user.salience_percent);
  TEST_ASSERT_GREATER_THAN_UINT16(baseline_responding.base_speed_percent,
                                  responding.base_speed_percent);
  TEST_ASSERT_GREATER_THAN_UINT32(
      ncos::core::contracts::personality_reengagement_ttl_ms(baseline),
      ncos::core::contracts::personality_reengagement_ttl_ms(personality));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_companion_personality_face_profiles_keep_response_focused_and_warm);
  RUN_TEST(test_companion_personality_motion_profiles_preserve_composure_and_sociability);
  RUN_TEST(test_companion_personality_continuity_gates_stay_consistent_across_channels);
  RUN_TEST(test_companion_personality_timing_keeps_continuity_short_and_predictable);
  RUN_TEST(test_companion_personality_adaptive_layer_is_bounded_and_separate_from_fixed_traits);
  RUN_TEST(test_companion_personality_adaptive_biases_modulate_outputs_without_changing_baseline_traits);
  return UNITY_END();
}

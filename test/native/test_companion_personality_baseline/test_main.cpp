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

void test_companion_personality_adaptive_biases_raise_behavior_priority_within_bounds() {
  auto personality = ncos::core::contracts::make_companion_personality_state();
  personality.adaptive_social_warmth_bias_percent = 8;
  personality.adaptive_response_energy_bias_percent = 6;
  personality.adaptive_continuity_window_bias_ms = 400;

  TEST_ASSERT_EQUAL_UINT8(
      8, ncos::core::contracts::personality_behavior_priority(
             personality, ncos::core::contracts::BehaviorProfile::kAttendUser, 6));
  TEST_ASSERT_EQUAL_UINT8(
      8, ncos::core::contracts::personality_behavior_priority(
             personality, ncos::core::contracts::BehaviorProfile::kAlertScan, 7));
  TEST_ASSERT_EQUAL_UINT8(
      10, ncos::core::contracts::personality_behavior_priority(
              personality, ncos::core::contracts::BehaviorProfile::kEnergyProtect, 10));
  TEST_ASSERT_EQUAL_UINT32(
      288, ncos::core::contracts::personality_behavior_ttl_ms(
               personality, ncos::core::contracts::BehaviorProfile::kAttendUser));
  TEST_ASSERT_EQUAL_UINT32(242,
                           ncos::core::contracts::personality_reengagement_ttl_ms(personality));
}

void test_companion_personality_adaptive_variation_stays_inside_identity_band() {
  auto warm_personality = ncos::core::contracts::make_companion_personality_state();
  warm_personality.adaptive_social_warmth_bias_percent = 8;
  warm_personality.adaptive_response_energy_bias_percent = 6;
  warm_personality.adaptive_continuity_window_bias_ms = 400;

  auto calm_personality = ncos::core::contracts::make_companion_personality_state();
  calm_personality.adaptive_social_warmth_bias_percent = -6;
  calm_personality.adaptive_response_energy_bias_percent = -6;
  calm_personality.adaptive_continuity_window_bias_ms = -300;

  const auto warm_user = ncos::core::contracts::personality_face_profile(
      warm_personality, ncos::core::contracts::PersonalityFaceMode::kWarmUser);
  const auto calm_user = ncos::core::contracts::personality_face_profile(
      calm_personality, ncos::core::contracts::PersonalityFaceMode::kWarmUser);
  const auto warm_response = ncos::core::contracts::personality_motion_profile(
      warm_personality, ncos::core::contracts::PersonalityMotionMode::kResponding);
  const auto calm_response = ncos::core::contracts::personality_motion_profile(
      calm_personality, ncos::core::contracts::PersonalityMotionMode::kResponding);

  TEST_ASSERT_LESS_OR_EQUAL_UINT8(16,
                                  static_cast<uint8_t>(warm_user.salience_percent -
                                                       calm_user.salience_percent));
  TEST_ASSERT_LESS_OR_EQUAL_UINT16(120,
                                   static_cast<uint16_t>(warm_user.hold_ms - calm_user.hold_ms));
  TEST_ASSERT_LESS_OR_EQUAL_UINT16(12,
                                   static_cast<uint16_t>(warm_response.base_speed_percent -
                                                         calm_response.base_speed_percent));
  TEST_ASSERT_LESS_OR_EQUAL_UINT16(80,
                                   static_cast<uint16_t>(warm_response.hold_ms -
                                                         calm_response.hold_ms));
}
void test_companion_personality_adaptive_biases_modulate_expression_without_changing_baseline_traits() {
  auto warm_personality = ncos::core::contracts::make_companion_personality_state();
  warm_personality.adaptive_social_warmth_bias_percent = 8;
  warm_personality.adaptive_response_energy_bias_percent = 6;
  warm_personality.adaptive_continuity_window_bias_ms = 400;

  auto calm_personality = ncos::core::contracts::make_companion_personality_state();
  calm_personality.adaptive_social_warmth_bias_percent = -6;
  calm_personality.adaptive_response_energy_bias_percent = -6;
  calm_personality.adaptive_continuity_window_bias_ms = -300;

  const auto baseline = ncos::core::contracts::make_companion_personality_state();
  const auto baseline_warm_stimulus = ncos::core::contracts::personality_face_profile(
      baseline, ncos::core::contracts::PersonalityFaceMode::kWarmStimulus);
  const auto warm_stimulus = ncos::core::contracts::personality_face_profile(
      warm_personality, ncos::core::contracts::PersonalityFaceMode::kWarmStimulus);
  const auto baseline_sleep = ncos::core::contracts::personality_face_profile(
      baseline, ncos::core::contracts::PersonalityFaceMode::kSleep);
  const auto calm_sleep = ncos::core::contracts::personality_face_profile(
      calm_personality, ncos::core::contracts::PersonalityFaceMode::kSleep);
  const auto baseline_alert_motion = ncos::core::contracts::personality_motion_profile(
      baseline, ncos::core::contracts::PersonalityMotionMode::kAlertScan);
  const auto warm_alert_motion = ncos::core::contracts::personality_motion_profile(
      warm_personality, ncos::core::contracts::PersonalityMotionMode::kAlertScan);
  const auto baseline_rest_motion = ncos::core::contracts::personality_motion_profile(
      baseline, ncos::core::contracts::PersonalityMotionMode::kRest);
  const auto calm_rest_motion = ncos::core::contracts::personality_motion_profile(
      calm_personality, ncos::core::contracts::PersonalityMotionMode::kRest);

  TEST_ASSERT_GREATER_THAN_UINT8(baseline_warm_stimulus.salience_percent, warm_stimulus.salience_percent);
  TEST_ASSERT_LESS_THAN_UINT16(baseline_warm_stimulus.cadence_ms, warm_stimulus.cadence_ms);
  TEST_ASSERT_GREATER_THAN_UINT16(baseline_sleep.cadence_ms, calm_sleep.cadence_ms);
  TEST_ASSERT_GREATER_THAN_UINT16(baseline_alert_motion.base_speed_percent,
                               warm_alert_motion.base_speed_percent);
  TEST_ASSERT_GREATER_THAN_UINT16(baseline_rest_motion.hold_ms, calm_rest_motion.hold_ms);
  TEST_ASSERT_LESS_THAN_UINT16(baseline_rest_motion.base_speed_percent,
                               calm_rest_motion.base_speed_percent);
  TEST_ASSERT_EQUAL_UINT8(68, warm_personality.warmth_percent);
  TEST_ASSERT_EQUAL_UINT8(68, calm_personality.warmth_percent);
}

void test_companion_personality_historical_expression_boosts_stay_bounded() {
  auto personality = ncos::core::contracts::make_companion_personality_state();
  personality.persistent_memory_applied = true;
  personality.persistent_social_warmth_bias_percent = 8;
  personality.persistent_response_energy_bias_percent = 7;
  personality.persistent_reinforced_sessions = 9;
  personality.persistent_preferred_attention_channel = ncos::core::contracts::AttentionChannel::kTouch;

  TEST_ASSERT_TRUE(ncos::core::contracts::personality_historical_user_affinity(personality));
  TEST_ASSERT_EQUAL_UINT8(12,
                          ncos::core::contracts::personality_historical_user_expression_boost_percent(
                              personality));

  personality.persistent_preferred_attention_channel =
      ncos::core::contracts::AttentionChannel::kMultimodal;
  TEST_ASSERT_TRUE(ncos::core::contracts::personality_historical_stimulus_affinity(personality));
  TEST_ASSERT_EQUAL_UINT8(10,
                          ncos::core::contracts::personality_historical_stimulus_expression_boost_percent(
                              personality));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_companion_personality_face_profiles_keep_response_focused_and_warm);
  RUN_TEST(test_companion_personality_motion_profiles_preserve_composure_and_sociability);
  RUN_TEST(test_companion_personality_continuity_gates_stay_consistent_across_channels);
  RUN_TEST(test_companion_personality_timing_keeps_continuity_short_and_predictable);
  RUN_TEST(test_companion_personality_adaptive_layer_is_bounded_and_separate_from_fixed_traits);
  RUN_TEST(test_companion_personality_adaptive_biases_raise_behavior_priority_within_bounds);
  RUN_TEST(test_companion_personality_adaptive_variation_stays_inside_identity_band);
  RUN_TEST(test_companion_personality_adaptive_biases_modulate_expression_without_changing_baseline_traits);
  RUN_TEST(test_companion_personality_historical_expression_boosts_stay_bounded);
  return UNITY_END();
}

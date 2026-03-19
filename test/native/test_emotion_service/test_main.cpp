#include <unity.h>

#include "services/emotion/emotion_service.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "services/emotion/emotion_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_emotion_service_raise_arousal_on_alert_behavior() {
  ncos::services::emotion::EmotionService service;
  TEST_ASSERT_TRUE(service.initialize(63, 1000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.emotional.vector.arousal_percent = 28;
  snapshot.emotional.vector.social_engagement_percent = 40;
  snapshot.emotional.intensity_percent = 20;
  snapshot.emotional.stability_percent = 90;

  ncos::core::contracts::BehaviorRuntimeState behavior{};
  behavior.initialized = true;
  behavior.active_profile = ncos::core::contracts::BehaviorProfile::kAlertScan;

  ncos::core::contracts::RoutineRuntimeState routine{};
  routine.initialized = true;

  ncos::core::contracts::CompanionEmotionalSignal out{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior, routine, 1200, &out));
  TEST_ASSERT_TRUE(out.vector_authoritative);
  TEST_ASSERT_GREATER_THAN_UINT8(28, out.vector.arousal_percent);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EmotionalArousal::kMedium),
                        static_cast<int>(out.arousal));
}

void test_emotion_service_boost_social_on_attend_user_with_user_routine() {
  ncos::services::emotion::EmotionService service;
  TEST_ASSERT_TRUE(service.initialize(63, 2000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.emotional.vector.social_engagement_percent = 35;
  snapshot.emotional.vector.valence_percent = 2;
  snapshot.emotional.intensity_percent = 24;
  snapshot.emotional.stability_percent = 92;

  ncos::core::contracts::BehaviorRuntimeState behavior{};
  behavior.initialized = true;
  behavior.active_profile = ncos::core::contracts::BehaviorProfile::kAttendUser;

  ncos::core::contracts::RoutineRuntimeState routine{};
  routine.initialized = true;
  routine.attention_mode = ncos::core::contracts::AttentionMode::kUserEngaged;

  ncos::core::contracts::CompanionEmotionalSignal out{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior, routine, 2300, &out));
  TEST_ASSERT_GREATER_THAN_UINT8(35, out.vector.social_engagement_percent);
  TEST_ASSERT_GREATER_THAN_INT8(2, out.vector.valence_percent);
}


void test_emotion_service_keeps_ambient_tone_warm_and_composed() {
  ncos::services::emotion::EmotionService service;
  TEST_ASSERT_TRUE(service.initialize(63, 3000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.emotional.vector.valence_percent = -4;
  snapshot.emotional.vector.arousal_percent = 4;
  snapshot.emotional.vector.social_engagement_percent = 10;
  snapshot.emotional.intensity_percent = 8;
  snapshot.emotional.stability_percent = 40;

  ncos::core::contracts::BehaviorRuntimeState behavior{};
  behavior.initialized = true;
  behavior.active_profile = ncos::core::contracts::BehaviorProfile::kIdleObserve;

  ncos::core::contracts::RoutineRuntimeState routine{};
  routine.initialized = true;
  routine.attention_mode = ncos::core::contracts::AttentionMode::kAmbient;

  ncos::core::contracts::CompanionEmotionalSignal out{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior, routine, 3300, &out));
  TEST_ASSERT_GREATER_OR_EQUAL_INT8(6, out.vector.valence_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT8(34, out.vector.social_engagement_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT8(18, out.intensity_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT8(72, out.stability_percent);
}

void test_emotion_service_keeps_alert_scan_curious_without_mania() {
  ncos::services::emotion::EmotionService service;
  TEST_ASSERT_TRUE(service.initialize(63, 4000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.emotional.vector.valence_percent = -40;
  snapshot.emotional.vector.arousal_percent = 92;
  snapshot.emotional.vector.social_engagement_percent = 6;
  snapshot.emotional.intensity_percent = 88;
  snapshot.emotional.stability_percent = 20;

  ncos::core::contracts::BehaviorRuntimeState behavior{};
  behavior.initialized = true;
  behavior.active_profile = ncos::core::contracts::BehaviorProfile::kAlertScan;

  ncos::core::contracts::RoutineRuntimeState routine{};
  routine.initialized = true;
  routine.attention_mode = ncos::core::contracts::AttentionMode::kStimulusTracking;

  ncos::core::contracts::CompanionEmotionalSignal out{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior, routine, 4300, &out));
  TEST_ASSERT_GREATER_OR_EQUAL_INT8(-10, out.vector.valence_percent);
  TEST_ASSERT_LESS_OR_EQUAL_UINT8(78, out.vector.arousal_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT8(25, out.vector.social_engagement_percent);
  TEST_ASSERT_LESS_OR_EQUAL_UINT8(76, out.intensity_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT8(72, out.stability_percent);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_emotion_service_raise_arousal_on_alert_behavior);
  RUN_TEST(test_emotion_service_boost_social_on_attend_user_with_user_routine);
  RUN_TEST(test_emotion_service_keeps_ambient_tone_warm_and_composed);
  RUN_TEST(test_emotion_service_keeps_alert_scan_curious_without_mania);
  return UNITY_END();
}


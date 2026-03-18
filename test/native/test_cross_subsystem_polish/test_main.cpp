#include <unity.h>
#include <string.h>

#include "services/observability/system_polish_tooling.hpp"

// Native tests run with test_build_src = no.
#include "services/observability/system_polish_tooling.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_cross_subsystem_polish_reports_coherent_when_signals_are_aligned() {
  ncos::services::observability::CrossSubsystemInput input{};

  input.companion.structural.offline_first = true;
  input.companion.runtime.safe_mode = false;
  input.companion.attentional.lock_active = true;
  input.companion.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  input.companion.energetic.mode = ncos::core::contracts::EnergyMode::kNominal;

  input.behavior.active_profile = ncos::core::contracts::BehaviorProfile::kAttendUser;

  input.perception.attention_active = true;
  input.voice.trigger_candidate = false;

  input.motion.neutral_applied = true;
  input.motion.companion_signal.safe_mode = false;
  input.motion.face_signal.gaze_x_percent = 55;
  input.motion.face_signal.gaze_y_percent = 0;

  input.face_signal.gaze_x_percent = 45;
  input.face_signal.gaze_y_percent = 0;

  input.cloud.offline_authoritative = true;

  const auto review = ncos::services::observability::review_cross_subsystem_coherence(input, 1200);

  TEST_ASSERT_TRUE(review.coherent);
  TEST_ASSERT_EQUAL_UINT8(100, review.polish_score);
  TEST_ASSERT_TRUE(review.safe_mode_aligned);
  TEST_ASSERT_TRUE(review.energy_aligned);
  TEST_ASSERT_TRUE(review.attention_aligned);
  TEST_ASSERT_TRUE(review.face_motion_aligned);
  TEST_ASSERT_TRUE(review.offline_authority_aligned);
}

void test_cross_subsystem_polish_flags_energy_and_attention_divergence() {
  ncos::services::observability::CrossSubsystemInput input{};

  input.companion.structural.offline_first = true;
  input.companion.energetic.mode = ncos::core::contracts::EnergyMode::kCritical;
  input.companion.attentional.lock_active = false;
  input.companion.attentional.target = ncos::core::contracts::AttentionTarget::kNone;

  input.behavior.active_profile = ncos::core::contracts::BehaviorProfile::kIdleObserve;
  input.perception.attention_active = true;

  input.motion.companion_signal.safe_mode = false;
  input.motion.neutral_applied = false;
  input.motion.face_signal.gaze_x_percent = 45;
  input.motion.face_signal.gaze_y_percent = 0;

  input.face_signal.gaze_x_percent = -45;
  input.face_signal.gaze_y_percent = 0;

  input.cloud.offline_authoritative = false;

  const auto review = ncos::services::observability::review_cross_subsystem_coherence(input, 2400);

  TEST_ASSERT_FALSE(review.coherent);
  TEST_ASSERT_LESS_THAN_UINT8(100, review.polish_score);
  TEST_ASSERT_FALSE(review.energy_aligned);
  TEST_ASSERT_FALSE(review.attention_aligned);
  TEST_ASSERT_FALSE(review.face_motion_aligned);
  TEST_ASSERT_FALSE(review.offline_authority_aligned);
  TEST_ASSERT_TRUE(ncos::services::observability::has_issue(
      review.issues, ncos::services::observability::CrossSubsystemIssue::kEnergyCriticalWithoutProtect));
  TEST_ASSERT_TRUE(ncos::services::observability::has_issue(
      review.issues, ncos::services::observability::CrossSubsystemIssue::kAttentionLockDivergence));
  TEST_ASSERT_TRUE(ncos::services::observability::has_issue(
      review.issues, ncos::services::observability::CrossSubsystemIssue::kFaceMotionDivergence));
  TEST_ASSERT_TRUE(ncos::services::observability::has_issue(
      review.issues, ncos::services::observability::CrossSubsystemIssue::kOfflineAuthorityDivergence));
}

void test_cross_subsystem_polish_exports_json_summary() {
  ncos::services::observability::CrossSubsystemReview review{};
  review.coherent = false;
  review.polish_score = 74;
  review.issues = ncos::services::observability::CrossSubsystemIssue::kFaceMotionDivergence;
  review.safe_mode_aligned = true;
  review.energy_aligned = true;
  review.attention_aligned = true;
  review.face_motion_aligned = false;
  review.offline_authority_aligned = true;
  review.evaluated_at_ms = 8800;

  char json[256] = {};
  const size_t written =
      ncos::services::observability::export_cross_subsystem_review_json(review, json, sizeof(json));

  TEST_ASSERT_GREATER_THAN_UINT32(0, static_cast<uint32_t>(written));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"polish_score\":74"));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"face_motion_aligned\":false"));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"evaluated_at_ms\":8800"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_cross_subsystem_polish_reports_coherent_when_signals_are_aligned);
  RUN_TEST(test_cross_subsystem_polish_flags_energy_and_attention_divergence);
  RUN_TEST(test_cross_subsystem_polish_exports_json_summary);
  return UNITY_END();
}

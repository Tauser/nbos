#include "services/observability/system_polish_tooling.hpp"

#include <stdio.h>

namespace {

constexpr uint8_t kPenaltySafeMode = 28;
constexpr uint8_t kPenaltyEnergy = 22;
constexpr uint8_t kPenaltyAttention = 18;
constexpr uint8_t kPenaltyFaceMotion = 20;
constexpr uint8_t kPenaltyOffline = 30;

constexpr int8_t sign_i8(int8_t value) {
  if (value > 0) {
    return 1;
  }
  if (value < 0) {
    return -1;
  }
  return 0;
}

const char* bool_text(bool value) {
  return value ? "true" : "false";
}

}  // namespace

namespace ncos::services::observability {

CrossSubsystemReview review_cross_subsystem_coherence(const CrossSubsystemInput& input,
                                                      uint64_t now_ms) {
  CrossSubsystemReview out{};
  out.evaluated_at_ms = now_ms;

  const bool runtime_safe = input.companion.runtime.safe_mode;
  const bool motion_safe = input.motion.companion_signal.safe_mode || input.motion.fail_safe_guard_active;
  out.safe_mode_aligned = (runtime_safe == motion_safe) || (!runtime_safe && input.motion.neutral_applied);

  const bool critical_energy =
      input.companion.energetic.mode == ncos::core::contracts::EnergyMode::kCritical;
  const bool behavior_protect =
      input.behavior.active_profile == ncos::core::contracts::BehaviorProfile::kEnergyProtect;
  out.energy_aligned = !critical_energy || behavior_protect || motion_safe;

  const bool attention_expected = input.perception.attention_active || input.voice.trigger_candidate;
  const bool attention_signaled = input.companion.attentional.lock_active ||
                                  input.companion.attentional.target !=
                                      ncos::core::contracts::AttentionTarget::kNone;
  out.attention_aligned = !attention_expected || attention_signaled;

  const int8_t face_x_sign = sign_i8(input.face_signal.gaze_x_percent);
  const int8_t motion_x_sign = sign_i8(input.motion.face_signal.gaze_x_percent);
  const int8_t face_y_sign = sign_i8(input.face_signal.gaze_y_percent);
  const int8_t motion_y_sign = sign_i8(input.motion.face_signal.gaze_y_percent);

  const bool face_horizontal_neutral = input.face_signal.gaze_x_percent > -12 &&
                                       input.face_signal.gaze_x_percent < 12;
  const bool face_vertical_neutral = input.face_signal.gaze_y_percent > -12 &&
                                     input.face_signal.gaze_y_percent < 12;

  const bool x_aligned = face_horizontal_neutral || (face_x_sign == motion_x_sign);
  const bool y_aligned = face_vertical_neutral || (face_y_sign == motion_y_sign);
  out.face_motion_aligned = x_aligned && y_aligned;

  const bool offline_first = input.companion.structural.offline_first;
  out.offline_authority_aligned = !offline_first || input.cloud.offline_authoritative;

  uint8_t score = 100;
  CrossSubsystemIssue issues = CrossSubsystemIssue::kNone;

  if (!out.safe_mode_aligned) {
    score = (score > kPenaltySafeMode) ? static_cast<uint8_t>(score - kPenaltySafeMode) : 0;
    issues = issues | CrossSubsystemIssue::kSafeModeDivergence;
  }

  if (!out.energy_aligned) {
    score = (score > kPenaltyEnergy) ? static_cast<uint8_t>(score - kPenaltyEnergy) : 0;
    issues = issues | CrossSubsystemIssue::kEnergyCriticalWithoutProtect;
  }

  if (!out.attention_aligned) {
    score = (score > kPenaltyAttention) ? static_cast<uint8_t>(score - kPenaltyAttention) : 0;
    issues = issues | CrossSubsystemIssue::kAttentionLockDivergence;
  }

  if (!out.face_motion_aligned) {
    score = (score > kPenaltyFaceMotion) ? static_cast<uint8_t>(score - kPenaltyFaceMotion) : 0;
    issues = issues | CrossSubsystemIssue::kFaceMotionDivergence;
  }

  if (!out.offline_authority_aligned) {
    score = (score > kPenaltyOffline) ? static_cast<uint8_t>(score - kPenaltyOffline) : 0;
    issues = issues | CrossSubsystemIssue::kOfflineAuthorityDivergence;
  }

  out.polish_score = score;
  out.issues = issues;
  out.coherent = (issues == CrossSubsystemIssue::kNone);
  return out;
}

size_t export_cross_subsystem_review_json(const CrossSubsystemReview& review,
                                          char* out_buffer,
                                          size_t out_buffer_size) {
  if (out_buffer == nullptr || out_buffer_size == 0) {
    return 0;
  }

  const int written = snprintf(
      out_buffer,
      out_buffer_size,
      "{\"coherent\":%s,\"polish_score\":%u,\"issues\":%u,\"safe_mode_aligned\":%s,"
      "\"energy_aligned\":%s,\"attention_aligned\":%s,\"face_motion_aligned\":%s,"
      "\"offline_authority_aligned\":%s,\"evaluated_at_ms\":%llu}",
      bool_text(review.coherent),
      review.polish_score,
      static_cast<unsigned>(review.issues),
      bool_text(review.safe_mode_aligned),
      bool_text(review.energy_aligned),
      bool_text(review.attention_aligned),
      bool_text(review.face_motion_aligned),
      bool_text(review.offline_authority_aligned),
      static_cast<unsigned long long>(review.evaluated_at_ms));

  if (written <= 0) {
    out_buffer[0] = '\0';
    return 0;
  }

  const size_t length = static_cast<size_t>(written);
  if (length >= out_buffer_size) {
    out_buffer[out_buffer_size - 1] = '\0';
    return out_buffer_size - 1;
  }

  return length;
}

}  // namespace ncos::services::observability

#include "services/emotion/emotion_service.hpp"

#include "core/contracts/companion_personality_contracts.hpp"
#include "models/emotion/emotion_model.hpp"

namespace {

int16_t clamp_i16(int16_t value, int16_t min_value, int16_t max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

}  // namespace

namespace ncos::services::emotion {

bool EmotionService::initialize(uint16_t service_id, uint64_t now_ms) {
  if (service_id == 0) {
    return false;
  }

  service_id_ = service_id;
  last_tick_ms_ = now_ms;
  return true;
}

bool EmotionService::tick(const ncos::core::contracts::CompanionSnapshot& snapshot,
                          const ncos::core::contracts::BehaviorRuntimeState& behavior_state,
                          const ncos::core::contracts::RoutineRuntimeState& routine_state,
                          uint64_t now_ms,
                          ncos::core::contracts::CompanionEmotionalSignal* out_signal) {
  if (service_id_ == 0 || out_signal == nullptr) {
    return false;
  }

  ncos::models::emotion::EmotionModelState model{};
  model.vector = snapshot.emotional.vector;
  model.intensity_percent = snapshot.emotional.intensity_percent;
  model.stability_percent = snapshot.emotional.stability_percent;

  switch (behavior_state.active_profile) {
    case ncos::core::contracts::BehaviorProfile::kEnergyProtect:
      model.vector.valence_percent = static_cast<int8_t>(
          clamp_i16(static_cast<int16_t>(model.vector.valence_percent) - 24, -100, 100));
      model.vector.arousal_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.vector.arousal_percent) + 12);
      model.vector.social_engagement_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.vector.social_engagement_percent) - 22);
      model.intensity_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.intensity_percent) + 20);
      break;
    case ncos::core::contracts::BehaviorProfile::kAlertScan:
      model.vector.arousal_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.vector.arousal_percent) + 24);
      model.intensity_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.intensity_percent) + 18);
      break;
    case ncos::core::contracts::BehaviorProfile::kAttendUser:
      model.vector.valence_percent = static_cast<int8_t>(
          clamp_i16(static_cast<int16_t>(model.vector.valence_percent) + 16, -100, 100));
      model.vector.social_engagement_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.vector.social_engagement_percent) + 26);
      model.intensity_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.intensity_percent) + 12);
      break;
    case ncos::core::contracts::BehaviorProfile::kIdleObserve:
    default:
      break;
  }

  switch (routine_state.attention_mode) {
    case ncos::core::contracts::AttentionMode::kUserEngaged:
      model.vector.social_engagement_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.vector.social_engagement_percent) + 10);
      break;
    case ncos::core::contracts::AttentionMode::kStimulusTracking:
      model.vector.arousal_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.vector.arousal_percent) + 8);
      break;
    case ncos::core::contracts::AttentionMode::kEnergyConserve:
      model.vector.arousal_percent = ncos::models::emotion::clamp_percent(
          static_cast<int16_t>(model.vector.arousal_percent) - 10);
      model.vector.valence_percent = static_cast<int8_t>(
          clamp_i16(static_cast<int16_t>(model.vector.valence_percent) - 5, -100, 100));
      break;
    case ncos::core::contracts::AttentionMode::kAmbient:
    default:
      break;
  }

  ncos::core::contracts::apply_personality_baseline_to_emotion(
      snapshot.personality, &model, behavior_state.active_profile, routine_state.attention_mode);
  model = ncos::models::emotion::normalize_model(model);

  ncos::core::contracts::CompanionEmotionalSignal signal{};
  signal.vector_authoritative = true;
  signal.vector = model.vector;
  signal.intensity_percent = model.intensity_percent;
  signal.stability_percent = model.stability_percent;
  signal.tone = ncos::core::contracts::tone_from_emotion_model(model);
  signal.arousal = ncos::core::contracts::arousal_from_emotion_model(model);

  *out_signal = signal;
  last_tick_ms_ = now_ms;
  return true;
}

}  // namespace ncos::services::emotion

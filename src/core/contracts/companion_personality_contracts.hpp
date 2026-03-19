#pragma once

#include <stdint.h>

#include "core/contracts/behavior_runtime_contracts.hpp"
#include "core/contracts/routine_runtime_contracts.hpp"
#include "models/emotion/emotion_model.hpp"

namespace ncos::core::contracts {

struct CompanionPersonalityBaseline {
  const char* profile_name = "companion_core";
  uint8_t warmth_percent = 68;
  uint8_t curiosity_percent = 58;
  uint8_t composure_percent = 72;
  uint8_t initiative_percent = 46;
  uint8_t assertiveness_percent = 52;
};

constexpr CompanionPersonalityBaseline active_companion_personality() {
  return CompanionPersonalityBaseline{};
}

constexpr uint32_t personality_behavior_ttl_ms(BehaviorProfile profile) {
  switch (profile) {
    case BehaviorProfile::kEnergyProtect:
      return 420;
    case BehaviorProfile::kAlertScan:
      return 220;
    case BehaviorProfile::kAttendUser:
      return 220;
    case BehaviorProfile::kIdleObserve:
    default:
      return 0;
  }
}

inline int16_t personality_clamp_i16(int16_t value, int16_t min_value, int16_t max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

inline uint8_t personality_clamp_percent(uint16_t value, uint8_t min_value, uint8_t max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return static_cast<uint8_t>(value);
}

inline void apply_personality_baseline_to_emotion(ncos::models::emotion::EmotionModelState* model,
                                                  BehaviorProfile behavior_profile,
                                                  AttentionMode attention_mode) {
  if (model == nullptr) {
    return;
  }

  const CompanionPersonalityBaseline baseline = active_companion_personality();

  int16_t valence_floor = 6;
  uint8_t arousal_ceiling = static_cast<uint8_t>(28 + baseline.composure_percent / 2);
  uint8_t social_floor = static_cast<uint8_t>(baseline.warmth_percent / 2);
  uint8_t intensity_ceiling = 62;

  if (behavior_profile == BehaviorProfile::kAttendUser ||
      attention_mode == AttentionMode::kUserEngaged) {
    valence_floor = 18;
    arousal_ceiling = 72;
    social_floor = static_cast<uint8_t>(44 + baseline.warmth_percent / 4);
    intensity_ceiling = 68;
  } else if (behavior_profile == BehaviorProfile::kAlertScan ||
             attention_mode == AttentionMode::kStimulusTracking) {
    valence_floor = -10;
    arousal_ceiling = 78;
    social_floor = static_cast<uint8_t>(20 + baseline.curiosity_percent / 10);
    intensity_ceiling = 76;
  } else if (behavior_profile == BehaviorProfile::kEnergyProtect ||
             attention_mode == AttentionMode::kEnergyConserve) {
    valence_floor = -8;
    arousal_ceiling = 58;
    social_floor = 18;
    intensity_ceiling = 58;
  }

  model->vector.valence_percent = static_cast<int8_t>(
      personality_clamp_i16(static_cast<int16_t>(model->vector.valence_percent), valence_floor, 100));
  model->vector.arousal_percent = personality_clamp_percent(model->vector.arousal_percent, 12, arousal_ceiling);
  model->vector.social_engagement_percent =
      personality_clamp_percent(model->vector.social_engagement_percent, social_floor, 100);
  model->intensity_percent = personality_clamp_percent(model->intensity_percent, 18, intensity_ceiling);
  if (model->stability_percent < baseline.composure_percent) {
    model->stability_percent = baseline.composure_percent;
  }
}

}  // namespace ncos::core::contracts

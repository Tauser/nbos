#pragma once

#include <stdint.h>

#include "core/contracts/behavior_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/routine_runtime_contracts.hpp"
#include "models/emotion/emotion_model.hpp"

namespace ncos::core::contracts {

enum class PersonalityContinuityKind : uint8_t {
  kUser = 0,
  kStimulus,
};

enum class PersonalityFaceMode : uint8_t {
  kBooting = 0,
  kIdleObserve,
  kWarmUser,
  kWarmStimulus,
  kAttendUser,
  kResponding,
  kAlertScan,
  kSleep,
  kEnergyProtect,
};

enum class PersonalityMotionMode : uint8_t {
  kRest = 0,
  kAttendUser,
  kResponding,
  kAlertScan,
  kWarmUser,
  kWarmStimulus,
};

struct PersonalityFaceProfile {
  uint8_t focus_percent = 48;
  uint8_t salience_percent = 30;
  uint16_t hold_ms = 420;
  uint16_t cadence_ms = 700;
  bool alternate_lateral = false;
};

struct PersonalityMotionProfile {
  int16_t pitch_permille = 0;
  uint16_t base_speed_percent = 24;
  uint16_t hold_ms = 180;
};

constexpr CompanionPersonalityState make_companion_personality_state() {
  return CompanionPersonalityState{};
}

constexpr int8_t personality_adaptive_social_warmth_bias_min_percent() {
  return -10;
}

constexpr int8_t personality_adaptive_social_warmth_bias_max_percent() {
  return 10;
}

constexpr int8_t personality_adaptive_response_energy_bias_min_percent() {
  return -8;
}

constexpr int8_t personality_adaptive_response_energy_bias_max_percent() {
  return 8;
}

constexpr int16_t personality_adaptive_continuity_window_bias_min_ms() {
  return -600;
}

constexpr int16_t personality_adaptive_continuity_window_bias_max_ms() {
  return 600;
}

constexpr int8_t personality_clamp_i8(int16_t value, int8_t min_value, int8_t max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return static_cast<int8_t>(value);
}

constexpr int16_t personality_clamp_i16(int32_t value, int16_t min_value, int16_t max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return static_cast<int16_t>(value);
}

constexpr int8_t personality_social_warmth_bias_percent(const CompanionPersonalityState& personality) {
  return personality_clamp_i8(personality.adaptive_social_warmth_bias_percent,
                              personality_adaptive_social_warmth_bias_min_percent(),
                              personality_adaptive_social_warmth_bias_max_percent());
}

constexpr int8_t personality_response_energy_bias_percent(const CompanionPersonalityState& personality) {
  return personality_clamp_i8(personality.adaptive_response_energy_bias_percent,
                              personality_adaptive_response_energy_bias_min_percent(),
                              personality_adaptive_response_energy_bias_max_percent());
}

constexpr int16_t personality_continuity_window_bias_ms(const CompanionPersonalityState& personality) {
  return personality_clamp_i16(personality.adaptive_continuity_window_bias_ms,
                               personality_adaptive_continuity_window_bias_min_ms(),
                               personality_adaptive_continuity_window_bias_max_ms());
}

constexpr uint8_t personality_behavior_priority(const CompanionPersonalityState& personality,
                                                BehaviorProfile profile,
                                                uint8_t base_priority) {
  const int8_t social_bias_percent = personality_social_warmth_bias_percent(personality);
  const int8_t response_bias_percent = personality_response_energy_bias_percent(personality);
  int16_t adjusted = base_priority;

  switch (profile) {
    case BehaviorProfile::kAttendUser:
      if (social_bias_percent >= 4) {
        ++adjusted;
      }
      if (response_bias_percent >= 6) {
        ++adjusted;
      }
      break;
    case BehaviorProfile::kAlertScan:
      if (response_bias_percent >= 4) {
        ++adjusted;
      }
      break;
    case BehaviorProfile::kEnergyProtect:
    case BehaviorProfile::kIdleObserve:
    default:
      break;
  }

  if (profile != BehaviorProfile::kEnergyProtect && adjusted > 9) {
    adjusted = 9;
  }
  if (adjusted < 1) {
    adjusted = 1;
  }
  if (adjusted > 10) {
    adjusted = 10;
  }
  return static_cast<uint8_t>(adjusted);
}

constexpr uint32_t personality_behavior_ttl_ms(const CompanionPersonalityState& personality,
                                               BehaviorProfile profile) {
  const int8_t social_bias_percent = personality_social_warmth_bias_percent(personality);
  const int8_t response_bias_percent = personality_response_energy_bias_percent(personality);
  const int16_t continuity_bias_ms = personality_continuity_window_bias_ms(personality);
  int32_t ttl_ms = 0;

  switch (profile) {
    case BehaviorProfile::kEnergyProtect:
      return personality.behavior_energy_protect_ttl_ms;
    case BehaviorProfile::kAlertScan:
      ttl_ms = static_cast<int32_t>(personality.behavior_alert_scan_ttl_ms) +
               response_bias_percent * 6 + continuity_bias_ms / 30;
      break;
    case BehaviorProfile::kAttendUser:
      ttl_ms = static_cast<int32_t>(personality.behavior_attend_user_ttl_ms) +
               social_bias_percent * 6 + continuity_bias_ms / 20;
      break;
    case BehaviorProfile::kIdleObserve:
    default:
      return 0;
  }

  if (ttl_ms < 120) {
    ttl_ms = 120;
  }
  if (ttl_ms > 520) {
    ttl_ms = 520;
  }
  return static_cast<uint32_t>(ttl_ms);
}

constexpr uint64_t personality_continuity_window_ms(const CompanionPersonalityState& personality,
                                                    PersonalityContinuityKind kind) {
  const int16_t continuity_bias_ms = personality_continuity_window_bias_ms(personality);
  switch (kind) {
    case PersonalityContinuityKind::kUser:
      return static_cast<uint64_t>(static_cast<int64_t>(personality.user_continuity_window_ms) +
                                   continuity_bias_ms);
    case PersonalityContinuityKind::kStimulus:
    default:
      return static_cast<uint64_t>(static_cast<int64_t>(personality.stimulus_continuity_window_ms) +
                                   continuity_bias_ms);
  }
}

constexpr uint8_t personality_continuity_engagement_threshold_percent(
    const CompanionPersonalityState& personality, PersonalityContinuityKind kind) {
  switch (kind) {
    case PersonalityContinuityKind::kUser:
      return static_cast<uint8_t>(50 + personality.warmth_percent / 10);
    case PersonalityContinuityKind::kStimulus:
    default:
      return static_cast<uint8_t>(42 + personality.curiosity_percent / 7);
  }
}

constexpr uint32_t personality_reengagement_ttl_ms(const CompanionPersonalityState& personality) {
  const int16_t continuity_bias_ms = personality_continuity_window_bias_ms(personality);
  const int8_t social_bias_percent = personality_social_warmth_bias_percent(personality);
  int32_t ttl_ms = static_cast<int32_t>(personality.reengagement_ttl_ms) + continuity_bias_ms / 20 +
                   social_bias_percent * 4;
  if (ttl_ms < 120) {
    ttl_ms = 120;
  }
  if (ttl_ms > 420) {
    ttl_ms = 420;
  }
  return static_cast<uint32_t>(ttl_ms);
}

constexpr PersonalityFaceProfile personality_face_profile(const CompanionPersonalityState& personality,
                                                          PersonalityFaceMode mode) {
  PersonalityFaceProfile profile{};
  const int8_t social_bias_percent = personality_social_warmth_bias_percent(personality);
  const int8_t response_bias_percent = personality_response_energy_bias_percent(personality);

  switch (mode) {
    case PersonalityFaceMode::kResponding:
      profile.focus_percent = static_cast<uint8_t>(62 + personality.assertiveness_percent / 8 +
                                                   response_bias_percent / 2);
      profile.salience_percent = static_cast<uint8_t>(56 + personality.assertiveness_percent / 8 +
                                                      response_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(520 + personality.assertiveness_percent +
                                              response_bias_percent * 4);
      profile.cadence_ms = static_cast<uint16_t>(340 + personality.initiative_percent -
                                                 response_bias_percent * 4);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kAttendUser:
      profile.focus_percent = static_cast<uint8_t>(58 + personality.warmth_percent / 10 +
                                                   social_bias_percent / 2);
      profile.salience_percent = static_cast<uint8_t>(46 + personality.warmth_percent / 8 +
                                                      social_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(480 + personality.assertiveness_percent +
                                              social_bias_percent * 4);
      profile.cadence_ms = static_cast<uint16_t>(360 + personality.initiative_percent -
                                                 social_bias_percent * 4);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kAlertScan:
      profile.focus_percent = static_cast<uint8_t>(50 + personality.curiosity_percent / 8 +
                                                   response_bias_percent / 3);
      profile.salience_percent = static_cast<uint8_t>(38 + personality.curiosity_percent / 10 +
                                                      response_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(380 + personality.curiosity_percent -
                                              response_bias_percent * 3);
      profile.cadence_ms = static_cast<uint16_t>(400 + personality.curiosity_percent -
                                                 response_bias_percent * 6);
      profile.alternate_lateral = true;
      break;
    case PersonalityFaceMode::kWarmUser:
      profile.focus_percent = static_cast<uint8_t>(52 + personality.warmth_percent / 8 +
                                                   social_bias_percent / 2);
      profile.salience_percent = static_cast<uint8_t>(38 + personality.warmth_percent / 10 +
                                                      social_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(460 + personality.warmth_percent +
                                              social_bias_percent * 5);
      profile.cadence_ms = static_cast<uint16_t>(480 + personality.initiative_percent / 2 -
                                                 social_bias_percent * 4);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kWarmStimulus:
      profile.focus_percent = static_cast<uint8_t>(44 + personality.curiosity_percent / 18 +
                                                   response_bias_percent / 3);
      profile.salience_percent = static_cast<uint8_t>(30 + personality.curiosity_percent / 6 +
                                                      response_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(400 + personality.curiosity_percent / 2 -
                                              response_bias_percent * 3);
      profile.cadence_ms = static_cast<uint16_t>(520 + personality.composure_percent / 4 -
                                                 response_bias_percent * 5);
      profile.alternate_lateral = true;
      break;
    case PersonalityFaceMode::kSleep:
      profile.focus_percent = static_cast<uint8_t>(20 + personality.initiative_percent / 20 +
                                                   response_bias_percent / 4);
      profile.salience_percent = static_cast<uint8_t>(10 + personality.warmth_percent / 16 +
                                                      social_bias_percent / 4);
      profile.hold_ms = static_cast<uint16_t>(900 + personality.composure_percent / 2 -
                                              response_bias_percent * 6 - social_bias_percent * 4);
      profile.cadence_ms = static_cast<uint16_t>(980 + personality.composure_percent / 2 -
                                                 response_bias_percent * 8 - social_bias_percent * 5);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kEnergyProtect:
      profile.focus_percent = static_cast<uint8_t>(24 + personality.composure_percent / 18 +
                                                   response_bias_percent / 4);
      profile.salience_percent = static_cast<uint8_t>(14 + personality.warmth_percent / 16 +
                                                      social_bias_percent / 4);
      profile.hold_ms = static_cast<uint16_t>(780 + personality.composure_percent -
                                              response_bias_percent * 6 - social_bias_percent * 4);
      profile.cadence_ms = static_cast<uint16_t>(860 + personality.composure_percent -
                                                 response_bias_percent * 8 - social_bias_percent * 4);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kBooting:
      profile.focus_percent = static_cast<uint8_t>(30 + personality.composure_percent / 16);
      profile.salience_percent = static_cast<uint8_t>(18 + personality.warmth_percent / 16);
      profile.hold_ms = static_cast<uint16_t>(660 + personality.composure_percent);
      profile.cadence_ms = static_cast<uint16_t>(720 + personality.curiosity_percent);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kIdleObserve:
    default:
      profile.focus_percent = static_cast<uint8_t>(42 + personality.curiosity_percent / 12 +
                                                   response_bias_percent / 4);
      profile.salience_percent = static_cast<uint8_t>(20 + personality.warmth_percent / 8 +
                                                      social_bias_percent / 3);
      profile.hold_ms = static_cast<uint16_t>(420 + personality.composure_percent / 3 -
                                              response_bias_percent * 2);
      profile.cadence_ms = static_cast<uint16_t>(700 + personality.composure_percent -
                                                 response_bias_percent * 3);
      profile.alternate_lateral = true;
      break;
  }

  return profile;
}

constexpr PersonalityMotionProfile personality_motion_profile(const CompanionPersonalityState& personality,
                                                              PersonalityMotionMode mode) {
  PersonalityMotionProfile profile{};
  const int8_t social_bias_percent = personality_social_warmth_bias_percent(personality);
  const int8_t response_bias_percent = personality_response_energy_bias_percent(personality);

  switch (mode) {
    case PersonalityMotionMode::kResponding:
      profile.pitch_permille = static_cast<int16_t>(60 + personality.assertiveness_percent / 6 +
                                                    response_bias_percent / 3);
      profile.base_speed_percent = static_cast<uint16_t>(28 + personality.assertiveness_percent / 8 +
                                                         response_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(220 + personality.assertiveness_percent / 2 +
                                              response_bias_percent * 4);
      break;
    case PersonalityMotionMode::kAttendUser:
      profile.pitch_permille = static_cast<int16_t>(42 + personality.warmth_percent / 10 +
                                                    social_bias_percent / 2);
      profile.base_speed_percent = static_cast<uint16_t>(24 + personality.assertiveness_percent / 10 +
                                                         social_bias_percent / 3);
      profile.hold_ms = static_cast<uint16_t>(190 + personality.warmth_percent / 3 +
                                              social_bias_percent * 3);
      break;
    case PersonalityMotionMode::kAlertScan:
      profile.pitch_permille = static_cast<int16_t>(20 + personality.curiosity_percent / 8 +
                                                    response_bias_percent / 3);
      profile.base_speed_percent = static_cast<uint16_t>(22 + personality.curiosity_percent / 9 +
                                                         response_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(180 + personality.curiosity_percent / 4 -
                                              response_bias_percent * 3);
      break;
    case PersonalityMotionMode::kWarmUser:
      profile.pitch_permille = static_cast<int16_t>(28 + personality.warmth_percent / 10 +
                                                    social_bias_percent / 2);
      profile.base_speed_percent = static_cast<uint16_t>(20 + personality.warmth_percent / 13 +
                                                         social_bias_percent / 3);
      profile.hold_ms = static_cast<uint16_t>(160 + personality.warmth_percent / 5 +
                                              social_bias_percent * 2);
      break;
    case PersonalityMotionMode::kWarmStimulus:
      profile.pitch_permille = static_cast<int16_t>(18 + personality.curiosity_percent / 14 +
                                                    response_bias_percent / 3);
      profile.base_speed_percent = static_cast<uint16_t>(18 + personality.curiosity_percent / 14 +
                                                         response_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(160 + personality.curiosity_percent / 6 -
                                              response_bias_percent * 3);
      break;
    case PersonalityMotionMode::kRest:
    default:
      profile.pitch_permille = static_cast<int16_t>(-36 - personality.composure_percent / 10 +
                                                    social_bias_percent / 4 + response_bias_percent / 4);
      profile.base_speed_percent = static_cast<uint16_t>(14 + personality.initiative_percent / 8 +
                                                         response_bias_percent / 2);
      profile.hold_ms = static_cast<uint16_t>(280 + personality.composure_percent / 2 -
                                              response_bias_percent * 8 - social_bias_percent * 4);
      break;
  }

  return profile;
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

inline void apply_personality_baseline_to_emotion(const CompanionPersonalityState& personality,
                                                  ncos::models::emotion::EmotionModelState* model,
                                                  BehaviorProfile behavior_profile,
                                                  AttentionMode attention_mode) {
  if (model == nullptr) {
    return;
  }

  int16_t valence_floor = 6;
  const int8_t social_bias_percent = personality_social_warmth_bias_percent(personality);
  const int8_t response_bias_percent = personality_response_energy_bias_percent(personality);
  uint8_t arousal_ceiling = static_cast<uint8_t>(28 + personality.composure_percent / 2 +
                                                 response_bias_percent);
  uint8_t social_floor = static_cast<uint8_t>(personality.warmth_percent / 2 + social_bias_percent);
  uint8_t intensity_ceiling = static_cast<uint8_t>(62 + response_bias_percent);

  if (behavior_profile == BehaviorProfile::kAttendUser ||
      attention_mode == AttentionMode::kUserEngaged) {
    valence_floor = 18;
    arousal_ceiling = static_cast<uint8_t>(72 + response_bias_percent);
    social_floor = static_cast<uint8_t>(44 + personality.warmth_percent / 4 + social_bias_percent);
    intensity_ceiling = static_cast<uint8_t>(68 + response_bias_percent);
  } else if (behavior_profile == BehaviorProfile::kAlertScan ||
             attention_mode == AttentionMode::kStimulusTracking) {
    valence_floor = -10;
    arousal_ceiling = static_cast<uint8_t>(78 + response_bias_percent);
    social_floor = static_cast<uint8_t>(20 + personality.curiosity_percent / 10 +
                                        social_bias_percent / 2);
    intensity_ceiling = static_cast<uint8_t>(76 + response_bias_percent);
  } else if (behavior_profile == BehaviorProfile::kEnergyProtect ||
             attention_mode == AttentionMode::kEnergyConserve) {
    valence_floor = -8;
    arousal_ceiling = static_cast<uint8_t>(58 + response_bias_percent / 2);
    social_floor = static_cast<uint8_t>(18 + social_bias_percent / 2);
    intensity_ceiling = static_cast<uint8_t>(58 + response_bias_percent / 2);
  }

  model->vector.valence_percent = static_cast<int8_t>(
      personality_clamp_i16(static_cast<int16_t>(model->vector.valence_percent), valence_floor, 100));
  model->vector.arousal_percent = personality_clamp_percent(model->vector.arousal_percent, 12, arousal_ceiling);
  model->vector.social_engagement_percent =
      personality_clamp_percent(model->vector.social_engagement_percent, social_floor, 100);
  model->intensity_percent = personality_clamp_percent(model->intensity_percent, 18, intensity_ceiling);
  if (model->stability_percent < personality.composure_percent) {
    model->stability_percent = personality.composure_percent;
  }
}

}  // namespace ncos::core::contracts



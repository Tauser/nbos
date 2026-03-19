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

constexpr uint32_t personality_behavior_ttl_ms(const CompanionPersonalityState& personality,
                                               BehaviorProfile profile) {
  switch (profile) {
    case BehaviorProfile::kEnergyProtect:
      return personality.behavior_energy_protect_ttl_ms;
    case BehaviorProfile::kAlertScan:
      return personality.behavior_alert_scan_ttl_ms;
    case BehaviorProfile::kAttendUser:
      return personality.behavior_attend_user_ttl_ms;
    case BehaviorProfile::kIdleObserve:
    default:
      return 0;
  }
}

constexpr uint64_t personality_continuity_window_ms(const CompanionPersonalityState& personality,
                                                    PersonalityContinuityKind kind) {
  switch (kind) {
    case PersonalityContinuityKind::kUser:
      return personality.user_continuity_window_ms;
    case PersonalityContinuityKind::kStimulus:
    default:
      return personality.stimulus_continuity_window_ms;
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
  return personality.reengagement_ttl_ms;
}

constexpr PersonalityFaceProfile personality_face_profile(const CompanionPersonalityState& personality,
                                                          PersonalityFaceMode mode) {
  PersonalityFaceProfile profile{};

  switch (mode) {
    case PersonalityFaceMode::kResponding:
      profile.focus_percent = static_cast<uint8_t>(62 + personality.assertiveness_percent / 8);
      profile.salience_percent = static_cast<uint8_t>(56 + personality.assertiveness_percent / 8);
      profile.hold_ms = static_cast<uint16_t>(520 + personality.assertiveness_percent);
      profile.cadence_ms = static_cast<uint16_t>(340 + personality.initiative_percent);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kAttendUser:
      profile.focus_percent = static_cast<uint8_t>(58 + personality.warmth_percent / 10);
      profile.salience_percent = static_cast<uint8_t>(46 + personality.warmth_percent / 8);
      profile.hold_ms = static_cast<uint16_t>(480 + personality.assertiveness_percent);
      profile.cadence_ms = static_cast<uint16_t>(360 + personality.initiative_percent);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kAlertScan:
      profile.focus_percent = static_cast<uint8_t>(50 + personality.curiosity_percent / 8);
      profile.salience_percent = static_cast<uint8_t>(38 + personality.curiosity_percent / 10);
      profile.hold_ms = static_cast<uint16_t>(380 + personality.curiosity_percent);
      profile.cadence_ms = static_cast<uint16_t>(400 + personality.curiosity_percent);
      profile.alternate_lateral = true;
      break;
    case PersonalityFaceMode::kWarmUser:
      profile.focus_percent = static_cast<uint8_t>(52 + personality.warmth_percent / 8);
      profile.salience_percent = static_cast<uint8_t>(38 + personality.warmth_percent / 10);
      profile.hold_ms = static_cast<uint16_t>(460 + personality.warmth_percent);
      profile.cadence_ms = static_cast<uint16_t>(480 + personality.initiative_percent / 2);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kWarmStimulus:
      profile.focus_percent = static_cast<uint8_t>(44 + personality.curiosity_percent / 18);
      profile.salience_percent = static_cast<uint8_t>(30 + personality.curiosity_percent / 6);
      profile.hold_ms = static_cast<uint16_t>(400 + personality.curiosity_percent / 2);
      profile.cadence_ms = static_cast<uint16_t>(520 + personality.composure_percent / 4);
      profile.alternate_lateral = true;
      break;
    case PersonalityFaceMode::kSleep:
      profile.focus_percent = static_cast<uint8_t>(20 + personality.initiative_percent / 20);
      profile.salience_percent = static_cast<uint8_t>(10 + personality.warmth_percent / 16);
      profile.hold_ms = static_cast<uint16_t>(900 + personality.composure_percent / 2);
      profile.cadence_ms = static_cast<uint16_t>(980 + personality.composure_percent / 2);
      profile.alternate_lateral = false;
      break;
    case PersonalityFaceMode::kEnergyProtect:
      profile.focus_percent = static_cast<uint8_t>(24 + personality.composure_percent / 18);
      profile.salience_percent = static_cast<uint8_t>(14 + personality.warmth_percent / 16);
      profile.hold_ms = static_cast<uint16_t>(780 + personality.composure_percent);
      profile.cadence_ms = static_cast<uint16_t>(860 + personality.composure_percent);
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
      profile.focus_percent = static_cast<uint8_t>(42 + personality.curiosity_percent / 12);
      profile.salience_percent = static_cast<uint8_t>(20 + personality.warmth_percent / 8);
      profile.hold_ms = static_cast<uint16_t>(420 + personality.composure_percent / 3);
      profile.cadence_ms = static_cast<uint16_t>(700 + personality.composure_percent);
      profile.alternate_lateral = true;
      break;
  }

  return profile;
}

constexpr PersonalityMotionProfile personality_motion_profile(const CompanionPersonalityState& personality,
                                                              PersonalityMotionMode mode) {
  PersonalityMotionProfile profile{};

  switch (mode) {
    case PersonalityMotionMode::kResponding:
      profile.pitch_permille = static_cast<int16_t>(60 + personality.assertiveness_percent / 6);
      profile.base_speed_percent = static_cast<uint16_t>(28 + personality.assertiveness_percent / 8);
      profile.hold_ms = static_cast<uint16_t>(220 + personality.assertiveness_percent / 2);
      break;
    case PersonalityMotionMode::kAttendUser:
      profile.pitch_permille = static_cast<int16_t>(42 + personality.warmth_percent / 10);
      profile.base_speed_percent = static_cast<uint16_t>(24 + personality.assertiveness_percent / 10);
      profile.hold_ms = static_cast<uint16_t>(190 + personality.warmth_percent / 3);
      break;
    case PersonalityMotionMode::kAlertScan:
      profile.pitch_permille = static_cast<int16_t>(20 + personality.curiosity_percent / 8);
      profile.base_speed_percent = static_cast<uint16_t>(22 + personality.curiosity_percent / 9);
      profile.hold_ms = static_cast<uint16_t>(180 + personality.curiosity_percent / 4);
      break;
    case PersonalityMotionMode::kWarmUser:
      profile.pitch_permille = static_cast<int16_t>(28 + personality.warmth_percent / 10);
      profile.base_speed_percent = static_cast<uint16_t>(20 + personality.warmth_percent / 13);
      profile.hold_ms = static_cast<uint16_t>(160 + personality.warmth_percent / 5);
      break;
    case PersonalityMotionMode::kWarmStimulus:
      profile.pitch_permille = static_cast<int16_t>(18 + personality.curiosity_percent / 14);
      profile.base_speed_percent = static_cast<uint16_t>(18 + personality.curiosity_percent / 14);
      profile.hold_ms = static_cast<uint16_t>(160 + personality.curiosity_percent / 6);
      break;
    case PersonalityMotionMode::kRest:
    default:
      profile.pitch_permille = static_cast<int16_t>(-36 - personality.composure_percent / 10);
      profile.base_speed_percent = static_cast<uint16_t>(14 + personality.initiative_percent / 8);
      profile.hold_ms = static_cast<uint16_t>(280 + personality.composure_percent / 2);
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
  uint8_t arousal_ceiling = static_cast<uint8_t>(28 + personality.composure_percent / 2);
  uint8_t social_floor = static_cast<uint8_t>(personality.warmth_percent / 2);
  uint8_t intensity_ceiling = 62;

  if (behavior_profile == BehaviorProfile::kAttendUser ||
      attention_mode == AttentionMode::kUserEngaged) {
    valence_floor = 18;
    arousal_ceiling = 72;
    social_floor = static_cast<uint8_t>(44 + personality.warmth_percent / 4);
    intensity_ceiling = 68;
  } else if (behavior_profile == BehaviorProfile::kAlertScan ||
             attention_mode == AttentionMode::kStimulusTracking) {
    valence_floor = -10;
    arousal_ceiling = 78;
    social_floor = static_cast<uint8_t>(20 + personality.curiosity_percent / 10);
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
  if (model->stability_percent < personality.composure_percent) {
    model->stability_percent = personality.composure_percent;
  }
}

}  // namespace ncos::core::contracts

#pragma once

#include <stdint.h>

namespace ncos::models::emotion {

enum class EmotionPhase : uint8_t {
  kBaseline = 1,
  kEngaged = 2,
  kAlerting = 3,
  kRecovering = 4,
};

struct EmotionVector {
  int8_t valence_percent = 0;              // -100..100
  uint8_t arousal_percent = 20;            // 0..100
  uint8_t social_engagement_percent = 45;  // 0..100
};

struct EmotionModelState {
  EmotionVector vector{};
  uint8_t intensity_percent = 0;
  uint8_t stability_percent = 100;
  EmotionPhase phase = EmotionPhase::kBaseline;
};

constexpr int8_t clamp_signed_percent(int16_t value) {
  return value < -100 ? static_cast<int8_t>(-100)
                      : (value > 100 ? static_cast<int8_t>(100) : static_cast<int8_t>(value));
}

constexpr uint8_t clamp_percent(int16_t value) {
  return value < 0 ? static_cast<uint8_t>(0)
                   : (value > 100 ? static_cast<uint8_t>(100) : static_cast<uint8_t>(value));
}

constexpr EmotionVector normalize_vector(const EmotionVector& vector) {
  EmotionVector out{};
  out.valence_percent = clamp_signed_percent(vector.valence_percent);
  out.arousal_percent = clamp_percent(vector.arousal_percent);
  out.social_engagement_percent = clamp_percent(vector.social_engagement_percent);
  return out;
}

constexpr EmotionPhase infer_phase(const EmotionModelState& state) {
  if (state.vector.arousal_percent >= 75 && state.intensity_percent >= 55) {
    return EmotionPhase::kAlerting;
  }

  if (state.vector.social_engagement_percent >= 60 && state.intensity_percent >= 30) {
    return EmotionPhase::kEngaged;
  }

  if (state.stability_percent < 45) {
    return EmotionPhase::kRecovering;
  }

  return EmotionPhase::kBaseline;
}

constexpr EmotionModelState normalize_model(const EmotionModelState& model) {
  EmotionModelState out{};
  out.vector = normalize_vector(model.vector);
  out.intensity_percent = clamp_percent(model.intensity_percent);
  out.stability_percent = clamp_percent(model.stability_percent);
  out.phase = infer_phase(out);
  return out;
}

}  // namespace ncos::models::emotion

#include "core/contracts/face_multimodal_contracts.hpp"

namespace {

uint8_t clamp_percent(uint32_t value) {
  return static_cast<uint8_t>(value > 100U ? 100U : value);
}

uint8_t audio_energy_percent(const ncos::core::contracts::AudioRuntimeState& audio) {
  if (!audio.last_capture_ok || audio.last_capture_samples == 0) {
    return 0;
  }

  const uint32_t peak = audio.last_peak_level < 0 ? static_cast<uint32_t>(-audio.last_peak_level)
                                                   : static_cast<uint32_t>(audio.last_peak_level);
  return clamp_percent((peak * 100U) / 32767U);
}

uint8_t touch_intensity_percent(const ncos::core::contracts::TouchRuntimeState& touch) {
  return clamp_percent((static_cast<uint32_t>(touch.normalized_level) * 100U) / 1000U);
}

uint8_t touch_social_engagement_percent(const ncos::core::contracts::TouchRuntimeState& touch) {
  if (!touch.initialized || !touch.trigger_active) {
    return 0;
  }

  return clamp_percent(40U + touch_intensity_percent(touch) / 2U);
}

uint8_t motion_intensity_percent(const ncos::core::contracts::ImuRuntimeState& imu) {
  const uint32_t gx = imu.gx_dps < 0 ? static_cast<uint32_t>(-imu.gx_dps) : static_cast<uint32_t>(imu.gx_dps);
  const uint32_t gy = imu.gy_dps < 0 ? static_cast<uint32_t>(-imu.gy_dps) : static_cast<uint32_t>(imu.gy_dps);
  const uint32_t gz = imu.gz_dps < 0 ? static_cast<uint32_t>(-imu.gz_dps) : static_cast<uint32_t>(imu.gz_dps);
  const uint32_t total = gx + gy + gz;
  return clamp_percent(total > 600U ? 100U : (total * 100U) / 600U);
}

uint8_t behavior_activation_percent(const ncos::core::contracts::BehaviorRuntimeState& behavior,
                                    uint64_t now_ms) {
  if (!behavior.initialized || behavior.active_profile == ncos::core::contracts::BehaviorProfile::kIdleObserve ||
      behavior.last_accept_ms == 0 || (now_ms - behavior.last_accept_ms) > 1400) {
    return 0;
  }

  switch (behavior.active_profile) {
    case ncos::core::contracts::BehaviorProfile::kEnergyProtect:
      return 82;
    case ncos::core::contracts::BehaviorProfile::kAlertScan:
      return 72;
    case ncos::core::contracts::BehaviorProfile::kAttendUser:
      return 58;
    case ncos::core::contracts::BehaviorProfile::kIdleObserve:
    default:
      return 0;
  }
}

}  // namespace

namespace ncos::core::contracts {

FaceMultimodalInput make_face_multimodal_input(const AudioRuntimeState& audio,
                                               const TouchRuntimeState& touch,
                                               const ImuRuntimeState& imu,
                                               const CompanionSnapshot& companion,
                                               const BehaviorRuntimeState& behavior,
                                               uint64_t now_ms) {
  FaceMultimodalInput input{};
  input.audio_ready = is_ready_for_local_audio(audio);
  input.touch_active = touch.initialized && touch.trigger_active;
  input.imu_ready = imu.initialized && imu.last_read_ok;

  input.audio_energy_percent = audio_energy_percent(audio);
  input.touch_intensity_percent = touch_intensity_percent(touch);
  input.motion_intensity_percent = motion_intensity_percent(imu);
  input.motion_active = input.motion_intensity_percent >= 15;
  input.session_warm = companion.session.warm;
  input.companion_product_state = companion.runtime.product_state;
  input.recent_stimulus_target = companion.session.recent_stimulus.target;
  input.recent_stimulus_channel = companion.session.recent_stimulus.channel;
  input.recent_interaction_phase = companion.session.recent_interaction.phase;
  input.recent_turn_owner = companion.session.last_turn_owner;

  input.emotional_arousal_percent = companion.emotional.vector.arousal_percent;
  input.social_engagement_percent = companion.emotional.vector.social_engagement_percent;
  input.recent_engagement_percent = companion.session.engagement_recent_percent;
  const uint8_t touch_social = touch_social_engagement_percent(touch);
  if (touch_social > input.social_engagement_percent) {
    input.social_engagement_percent = touch_social;
  }
  if (companion.session.warm) {
    const uint8_t continuity_social = companion.session.engagement_recent_percent > 12
                                          ? static_cast<uint8_t>(companion.session.engagement_recent_percent - 12)
                                          : companion.session.engagement_recent_percent;
    if (continuity_social > input.social_engagement_percent) {
      input.social_engagement_percent = continuity_social;
    }
  }
  input.behavior_activation_percent = behavior_activation_percent(behavior, now_ms);
  input.behavior_active = input.behavior_activation_percent > 0;

  input.observed_at_ms = now_ms;
  return input;
}

}  // namespace ncos::core::contracts

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

uint8_t motion_intensity_percent(const ncos::core::contracts::ImuRuntimeState& imu) {
  const uint32_t gx = imu.gx_dps < 0 ? static_cast<uint32_t>(-imu.gx_dps) : static_cast<uint32_t>(imu.gx_dps);
  const uint32_t gy = imu.gy_dps < 0 ? static_cast<uint32_t>(-imu.gy_dps) : static_cast<uint32_t>(imu.gy_dps);
  const uint32_t gz = imu.gz_dps < 0 ? static_cast<uint32_t>(-imu.gz_dps) : static_cast<uint32_t>(imu.gz_dps);
  const uint32_t total = gx + gy + gz;
  return clamp_percent(total > 600U ? 100U : (total * 100U) / 600U);
}

}  // namespace

namespace ncos::core::contracts {

FaceMultimodalInput make_face_multimodal_input(const AudioRuntimeState& audio,
                                               const TouchRuntimeState& touch,
                                               const ImuRuntimeState& imu,
                                               uint64_t now_ms) {
  FaceMultimodalInput input{};
  input.audio_ready = is_ready_for_local_audio(audio);
  input.touch_active = touch.initialized && touch.last_read_ok;
  input.imu_ready = imu.initialized && imu.last_read_ok;

  input.audio_energy_percent = audio_energy_percent(audio);
  input.touch_intensity_percent = touch_intensity_percent(touch);
  input.motion_intensity_percent = motion_intensity_percent(imu);
  input.motion_active = input.motion_intensity_percent >= 15;
  input.observed_at_ms = now_ms;
  return input;
}

}  // namespace ncos::core::contracts

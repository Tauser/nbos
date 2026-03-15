#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/audio_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

enum class VoiceStage : uint8_t {
  Dormant = 1,
  Listening = 2,
  TriggerCandidate = 3,
};

struct VoiceRuntimeState {
  bool initialized = false;
  bool input_available = false;
  bool speech_active = false;
  bool trigger_candidate = false;

  VoiceStage stage = VoiceStage::Dormant;
  uint8_t energy_percent = 0;
  uint16_t consecutive_speech_frames = 0;
  uint16_t silence_frames = 0;

  uint64_t last_trigger_ms = 0;
  uint64_t last_update_ms = 0;
  uint32_t speech_frames_total = 0;
  uint32_t trigger_candidates_total = 0;
};

constexpr VoiceRuntimeState make_voice_runtime_baseline() {
  return VoiceRuntimeState{};
}

inline uint8_t voice_energy_percent_from_audio(const AudioRuntimeState& audio) {
  if (!audio.last_capture_ok || audio.last_capture_samples == 0) {
    return 0;
  }

  const uint32_t peak = audio.last_peak_level < 0 ? static_cast<uint32_t>(-audio.last_peak_level)
                                                   : static_cast<uint32_t>(audio.last_peak_level);
  const uint32_t scaled = (peak * 100U) / 32767U;
  return static_cast<uint8_t>(scaled > 100U ? 100U : scaled);
}

}  // namespace ncos::core::contracts


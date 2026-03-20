#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/audio_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

enum class VoiceWakeMode : uint8_t {
  EnergyTrigger = 1,
  TouchAssisted = 2,
};

enum class VoiceAsrMode : uint8_t {
  ConstrainedIntent = 1,
  DeferredRichAsr = 2,
};

enum class VoiceTtsMode : uint8_t {
  EarconFirst = 1,
  DeferredNaturalTts = 2,
};

enum class VoiceLatencyPriority : uint8_t {
  TriggerResponsiveness = 1,
  ShortTurnUtility = 2,
  RichTranscriptFidelity = 3,
};

enum class VoiceStage : uint8_t {
  Dormant = 1,
  Listening = 2,
  TriggerCandidate = 3,
};

struct VoicePipelinePolicy {
  VoiceWakeMode wake_mode = VoiceWakeMode::EnergyTrigger;
  bool touch_assist_enabled = true;
  bool wake_word_required = false;
  VoiceAsrMode asr_mode = VoiceAsrMode::ConstrainedIntent;
  bool partial_transcript_required = false;
  bool cloud_asr_allowed = false;
  VoiceTtsMode tts_mode = VoiceTtsMode::EarconFirst;
  bool natural_tts_required = false;
  bool cloud_tts_allowed = false;
  VoiceLatencyPriority primary_priority = VoiceLatencyPriority::TriggerResponsiveness;
  VoiceLatencyPriority secondary_priority = VoiceLatencyPriority::ShortTurnUtility;
};

struct VoiceRuntimeState {
  bool initialized = false;
  bool input_available = false;
  bool speech_active = false;
  bool trigger_candidate = false;

  VoicePipelinePolicy policy{};
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
  VoiceRuntimeState state{};
  state.policy = VoicePipelinePolicy{};
  return state;
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

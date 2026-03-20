#pragma once

#include <stdint.h>

#include "core/contracts/audio_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/voice_runtime_contracts.hpp"

namespace ncos::services::voice {

class VoiceService final {
 public:
  bool initialize(uint16_t service_id, uint64_t now_ms);

  bool tick(const ncos::core::contracts::AudioRuntimeState& audio,
            const ncos::core::contracts::CompanionSnapshot& companion,
            uint64_t now_ms,
            ncos::core::contracts::CompanionAttentionalSignal* out_attention,
            ncos::core::contracts::CompanionInteractionSignal* out_interaction);

  bool take_response_plan(ncos::core::contracts::VoiceResponsePlan* out_plan);
  [[nodiscard]] const ncos::core::contracts::VoiceRuntimeState& state() const;

 private:
  static constexpr uint8_t SpeechThresholdPercent = 18;
  static constexpr uint8_t TriggerThresholdPercent = 28;
  static constexpr uint16_t TriggerSpeechFrames = 3;
  static constexpr uint16_t MinCaptureSamples = 128;
  static constexpr uint64_t CaptureFreshnessMs = 320;
  static constexpr uint64_t TriggerCooldownMs = 1400;

  uint16_t service_id_ = 0;
  ncos::core::contracts::VoiceRuntimeState state_ = ncos::core::contracts::make_voice_runtime_baseline();
  ncos::core::contracts::VoiceResponsePlan pending_response_plan_{};
};

}  // namespace ncos::services::voice

#pragma once

#include <stdint.h>

#include "core/contracts/audio_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/perception_runtime_contracts.hpp"
#include "core/contracts/sensing_runtime_contracts.hpp"
#include "core/contracts/vision_runtime_contracts.hpp"

namespace ncos::services::vision {

class PerceptionService final {
 public:
  bool initialize(uint16_t service_id, uint64_t now_ms);

  bool tick(const ncos::core::contracts::AudioRuntimeState& audio,
            const ncos::core::contracts::TouchRuntimeState& touch,
            const ncos::core::contracts::CameraRuntimeState& camera,
            const ncos::core::contracts::CompanionSnapshot& companion,
            uint64_t now_ms,
            ncos::core::contracts::CompanionAttentionalSignal* out_attention,
            ncos::core::contracts::CompanionInteractionSignal* out_interaction);

  [[nodiscard]] const ncos::core::contracts::PerceptionRuntimeState& state() const;

 private:
  static uint8_t visual_presence_confidence(const ncos::core::contracts::CameraRuntimeState& camera,
                                            uint64_t now_ms);
  static uint8_t auditory_presence_confidence(const ncos::core::contracts::AudioRuntimeState& audio);
  static uint8_t touch_presence_confidence(const ncos::core::contracts::TouchRuntimeState& touch);
  void update_visual_user_inference(uint8_t visual_confidence, uint64_t now_ms);
  void choose_attention_channel(uint8_t visual_confidence,
                                uint8_t auditory_confidence,
                                uint8_t touch_confidence,
                                ncos::core::contracts::CompanionAttentionalSignal* out_attention,
                                ncos::core::contracts::CompanionInteractionSignal* out_interaction);

  uint16_t service_id_ = 0;
  ncos::core::contracts::PerceptionRuntimeState state_ =
      ncos::core::contracts::make_perception_runtime_baseline();
};

}  // namespace ncos::services::vision

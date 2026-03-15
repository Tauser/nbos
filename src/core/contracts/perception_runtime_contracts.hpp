#pragma once

#include <stdint.h>

#include "core/contracts/audio_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/sensing_runtime_contracts.hpp"
#include "core/contracts/vision_runtime_contracts.hpp"

namespace ncos::core::contracts {

enum class PerceptionStage : uint8_t {
  Dormant = 1,
  PresenceDetected = 2,
  AttentionLocked = 3,
};

struct PerceptionRuntimeState {
  bool initialized = false;
  bool presence_active = false;
  bool attention_active = false;

  PerceptionStage stage = PerceptionStage::Dormant;
  AttentionTarget attention_target = AttentionTarget::kNone;
  AttentionChannel attention_channel = AttentionChannel::kVisual;

  uint8_t presence_confidence_percent = 0;
  uint8_t attention_confidence_percent = 0;

  uint64_t last_update_ms = 0;
  uint64_t last_presence_ms = 0;
  uint64_t last_attention_ms = 0;
  uint32_t updates_total = 0;
};

constexpr PerceptionRuntimeState make_perception_runtime_baseline() {
  return PerceptionRuntimeState{};
}

inline uint8_t clamp_percent_u8(uint16_t value) {
  return static_cast<uint8_t>(value > 100U ? 100U : value);
}

}  // namespace ncos::core::contracts

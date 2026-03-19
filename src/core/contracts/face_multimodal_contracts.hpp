#pragma once

#include <stdint.h>

#include "core/contracts/audio_runtime_contracts.hpp"
#include "core/contracts/behavior_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/sensing_runtime_contracts.hpp"

namespace ncos::core::contracts {

struct FaceMultimodalInput {
  bool audio_ready = false;
  bool touch_active = false;
  bool imu_ready = false;
  bool motion_active = false;
  bool behavior_active = false;
  bool session_warm = false;
  CompanionProductState companion_product_state = CompanionProductState::kBooting;
  AttentionTarget recent_stimulus_target = AttentionTarget::kNone;
  AttentionChannel recent_stimulus_channel = AttentionChannel::kVisual;
  InteractionPhase recent_interaction_phase = InteractionPhase::kIdle;
  TurnOwner recent_turn_owner = TurnOwner::kNone;

  uint8_t audio_energy_percent = 0;
  uint8_t touch_intensity_percent = 0;
  uint8_t motion_intensity_percent = 0;
  uint8_t emotional_arousal_percent = 0;
  uint8_t social_engagement_percent = 0;
  uint8_t behavior_activation_percent = 0;
  uint8_t recent_engagement_percent = 0;
  uint64_t observed_at_ms = 0;
};

FaceMultimodalInput make_face_multimodal_input(const AudioRuntimeState& audio,
                                               const TouchRuntimeState& touch,
                                               const ImuRuntimeState& imu,
                                               const CompanionSnapshot& companion,
                                               const BehaviorRuntimeState& behavior,
                                               uint64_t now_ms);

}  // namespace ncos::core::contracts

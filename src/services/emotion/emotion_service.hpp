#pragma once

#include <stdint.h>

#include "core/contracts/behavior_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/routine_runtime_contracts.hpp"

namespace ncos::services::emotion {

class EmotionService final {
 public:
  bool initialize(uint16_t service_id, uint64_t now_ms);

  bool tick(const ncos::core::contracts::CompanionSnapshot& snapshot,
            const ncos::core::contracts::BehaviorRuntimeState& behavior_state,
            const ncos::core::contracts::RoutineRuntimeState& routine_state,
            uint64_t now_ms,
            ncos::core::contracts::CompanionEmotionalSignal* out_signal);

 private:
  uint16_t service_id_ = 0;
  uint64_t last_tick_ms_ = 0;
};

}  // namespace ncos::services::emotion

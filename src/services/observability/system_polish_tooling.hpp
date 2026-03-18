#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/behavior_runtime_contracts.hpp"
#include "core/contracts/cloud_bridge_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/motion_runtime_contracts.hpp"
#include "core/contracts/perception_runtime_contracts.hpp"
#include "core/contracts/voice_runtime_contracts.hpp"

namespace ncos::services::observability {

enum class CrossSubsystemIssue : uint16_t {
  kNone = 0,
  kSafeModeDivergence = 1 << 0,
  kEnergyCriticalWithoutProtect = 1 << 1,
  kAttentionLockDivergence = 1 << 2,
  kFaceMotionDivergence = 1 << 3,
  kOfflineAuthorityDivergence = 1 << 4,
};

constexpr CrossSubsystemIssue operator|(CrossSubsystemIssue lhs, CrossSubsystemIssue rhs) {
  return static_cast<CrossSubsystemIssue>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
}

constexpr bool has_issue(CrossSubsystemIssue mask, CrossSubsystemIssue issue) {
  return (static_cast<uint16_t>(mask) & static_cast<uint16_t>(issue)) != 0;
}

struct CrossSubsystemInput {
  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::BehaviorRuntimeState behavior{};
  ncos::core::contracts::PerceptionRuntimeState perception{};
  ncos::core::contracts::VoiceRuntimeState voice{};
  ncos::core::contracts::MotionRuntimeState motion{};
  ncos::core::contracts::CloudBridgeRuntimeState cloud{};
  ncos::core::contracts::MotionFaceSignal face_signal{};
};

struct CrossSubsystemReview {
  bool coherent = true;
  uint8_t polish_score = 100;
  CrossSubsystemIssue issues = CrossSubsystemIssue::kNone;

  bool safe_mode_aligned = true;
  bool energy_aligned = true;
  bool attention_aligned = true;
  bool face_motion_aligned = true;
  bool offline_authority_aligned = true;
  uint64_t evaluated_at_ms = 0;
};

CrossSubsystemReview review_cross_subsystem_coherence(const CrossSubsystemInput& input,
                                                      uint64_t now_ms);

size_t export_cross_subsystem_review_json(const CrossSubsystemReview& review,
                                          char* out_buffer,
                                          size_t out_buffer_size);

}  // namespace ncos::services::observability

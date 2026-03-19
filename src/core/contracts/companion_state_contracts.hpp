#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "models/emotion/emotion_model.hpp"

namespace ncos::core::contracts {

enum class CompanionPresenceMode : uint8_t {
  kIdle = 1,
  kAttending = 2,
  kDormant = 3,
};

enum class CompanionProductState : uint8_t {
  kBooting = 1,
  kIdleObserve = 2,
  kSleep = 3,
  kAttendUser = 4,
  kAlertScan = 5,
  kResponding = 6,
  kEnergyProtect = 7,
};

enum class CompanionStateTransitionCause : uint8_t {
  kBootstrap = 1,
  kRuntimeStarted = 2,
  kRuntimeStopped = 3,
  kUserTrigger = 4,
  kUserRelease = 5,
  kStimulusObserved = 6,
  kCompanionResponding = 7,
  kEnergyGuard = 8,
  kRecoveryToIdle = 9,
  kIdleDecayToSleep = 10,
};

enum class GovernanceHealth : uint8_t {
  kUnknown = 1,
  kStable = 2,
  kContended = 3,
};

enum class EmotionalTone : uint8_t {
  kNeutral = 1,
  kCurious = 2,
  kAffiliative = 3,
  kAlert = 4,
};

enum class EmotionalArousal : uint8_t {
  kLow = 1,
  kMedium = 2,
  kHigh = 3,
};

enum class AttentionTarget : uint8_t {
  kNone = 1,
  kUser = 2,
  kStimulus = 3,
  kInternalTask = 4,
};

enum class AttentionChannel : uint8_t {
  kVisual = 1,
  kAuditory = 2,
  kTouch = 3,
  kMultimodal = 4,
};

enum class EnergyMode : uint8_t {
  kNominal = 1,
  kConstrained = 2,
  kCritical = 3,
  kCharging = 4,
};

enum class InteractionPhase : uint8_t {
  kIdle = 1,
  kListening = 2,
  kResponding = 3,
  kActing = 4,
};

enum class TurnOwner : uint8_t {
  kNone = 1,
  kUser = 2,
  kCompanion = 3,
};

enum class CompanionStateDomain : uint8_t {
  kStructural = 1,
  kPersonality = 2,
  kRuntime = 3,
  kGovernance = 4,
  kEmotional = 5,
  kAttentional = 6,
  kEnergetic = 7,
  kInteractional = 8,
  kTransient = 9,
  kSessionMemory = 10,
};

enum class CompanionStateWriter : uint8_t {
  kBootstrap = 1,
  kRuntimeCore = 2,
  kGovernanceCore = 3,
  kEmotionService = 4,
  kAttentionService = 5,
  kPowerService = 6,
  kInteractionService = 7,
};

enum class CompanionStateReader : uint8_t {
  kRuntimeCore = 1,
  kBehaviorService = 2,
  kFaceService = 3,
  kMotionService = 4,
  kVoiceService = 5,
  kPowerService = 6,
  kDiagnostics = 7,
  kCloudBridge = 8,
};

struct CompanionStructuralState {
  bool offline_first = true;
  uint16_t semantic_taxonomy_version = 0;
  const char* board_name = "unknown";
};

struct CompanionPersonalityState {
  const char* profile_name = "companion_core";
  uint8_t warmth_percent = 68;
  uint8_t curiosity_percent = 58;
  uint8_t composure_percent = 72;
  uint8_t initiative_percent = 46;
  uint8_t assertiveness_percent = 52;
  uint32_t behavior_energy_protect_ttl_ms = 420;
  uint32_t behavior_alert_scan_ttl_ms = 220;
  uint32_t behavior_attend_user_ttl_ms = 220;
  uint32_t reengagement_ttl_ms = 190;
  uint64_t user_continuity_window_ms = 3200;
  uint64_t stimulus_continuity_window_ms = 2400;
};

struct CompanionTransientState {
  bool has_active_trace = false;
  uint32_t active_trace_id = 0;
  ActionDomain active_domain = ActionDomain::kFace;
  uint16_t active_owner_service = 0;
  uint64_t last_transition_ms = 0;
};

struct CompanionRuntimeState {
  bool initialized = false;
  bool started = false;
  bool safe_mode = false;
  uint32_t scheduler_tasks = 0;
  uint32_t fault_count = 0;
  CompanionPresenceMode presence_mode = CompanionPresenceMode::kIdle;
  CompanionProductState product_state = CompanionProductState::kBooting;
  CompanionStateTransitionCause last_transition_cause =
      CompanionStateTransitionCause::kBootstrap;
  uint64_t last_state_change_ms = 0;
  uint64_t state_hold_until_ms = 0;
  uint64_t idle_since_ms = 0;
  uint64_t state_dwell_ms = 0;
  uint32_t state_transition_total = 0;
};

struct CompanionGovernanceState {
  uint32_t allowed_total = 0;
  uint32_t preempted_total = 0;
  uint32_t rejected_total = 0;
  GovernanceHealth health = GovernanceHealth::kUnknown;
};

struct CompanionEmotionalState {
  EmotionalTone tone = EmotionalTone::kNeutral;
  EmotionalArousal arousal = EmotionalArousal::kLow;
  ncos::models::emotion::EmotionVector vector{};
  ncos::models::emotion::EmotionPhase phase = ncos::models::emotion::EmotionPhase::kBaseline;
  uint8_t intensity_percent = 0;
  uint8_t stability_percent = 100;
};

struct CompanionAttentionalState {
  AttentionTarget target = AttentionTarget::kNone;
  AttentionChannel channel = AttentionChannel::kVisual;
  uint8_t focus_confidence_percent = 0;
  bool lock_active = false;
};

struct CompanionEnergeticState {
  EnergyMode mode = EnergyMode::kNominal;
  uint8_t battery_percent = 100;
  uint8_t thermal_load_percent = 0;
  bool external_power = false;
};

struct CompanionInteractionState {
  InteractionPhase phase = InteractionPhase::kIdle;
  TurnOwner turn_owner = TurnOwner::kNone;
  bool session_active = false;
  bool response_pending = false;
};

struct CompanionRecentStimulusState {
  AttentionTarget target = AttentionTarget::kNone;
  AttentionChannel channel = AttentionChannel::kVisual;
  uint8_t confidence_percent = 0;
  uint64_t observed_at_ms = 0;
};

struct CompanionRecentInteractionContextState {
  InteractionPhase phase = InteractionPhase::kIdle;
  TurnOwner turn_owner = TurnOwner::kNone;
  bool response_pending = false;
  uint64_t updated_at_ms = 0;
};

struct CompanionSessionMemoryState {
  bool warm = false;
  AttentionTarget anchor_target = AttentionTarget::kNone;
  AttentionChannel anchor_channel = AttentionChannel::kVisual;
  EmotionalTone anchor_tone = EmotionalTone::kNeutral;
  CompanionProductState anchor_state = CompanionProductState::kBooting;
  CompanionRecentStimulusState recent_stimulus{};
  CompanionRecentInteractionContextState recent_interaction{};
  TurnOwner last_turn_owner = TurnOwner::kNone;
  uint8_t engagement_recent_percent = 0;
  uint16_t user_trigger_count = 0;
  uint16_t companion_response_count = 0;
  uint64_t opened_at_ms = 0;
  uint64_t last_activity_ms = 0;
  uint64_t last_engagement_ms = 0;
  uint64_t last_user_trigger_ms = 0;
  uint64_t last_companion_response_ms = 0;
  uint64_t retention_until_ms = 0;
};

struct CompanionSnapshot {
  CompanionStructuralState structural{};
  CompanionPersonalityState personality{};
  CompanionRuntimeState runtime{};
  CompanionGovernanceState governance{};
  CompanionEmotionalState emotional{};
  CompanionAttentionalState attentional{};
  CompanionEnergeticState energetic{};
  CompanionInteractionState interactional{};
  CompanionTransientState transient{};
  CompanionSessionMemoryState session{};
  uint32_t revision = 0;
  uint64_t captured_at_ms = 0;
};

struct CompanionRuntimeSignal {
  bool initialized = false;
  bool started = false;
  bool safe_mode = false;
  uint32_t scheduler_tasks = 0;
  uint32_t fault_count = 0;
  uint32_t governance_allowed_total = 0;
  uint32_t governance_preempted_total = 0;
  uint32_t governance_rejected_total = 0;
};

struct CompanionEmotionalSignal {
  EmotionalTone tone = EmotionalTone::kNeutral;
  EmotionalArousal arousal = EmotionalArousal::kLow;
  ncos::models::emotion::EmotionVector vector{};
  bool vector_authoritative = false;
  uint8_t intensity_percent = 0;
  uint8_t stability_percent = 100;
};

struct CompanionAttentionalSignal {
  AttentionTarget target = AttentionTarget::kNone;
  AttentionChannel channel = AttentionChannel::kVisual;
  uint8_t focus_confidence_percent = 0;
  bool lock_active = false;
};

struct CompanionEnergeticSignal {
  EnergyMode mode = EnergyMode::kNominal;
  uint8_t battery_percent = 100;
  uint8_t thermal_load_percent = 0;
  bool external_power = false;
};

struct CompanionInteractionSignal {
  InteractionPhase phase = InteractionPhase::kIdle;
  TurnOwner turn_owner = TurnOwner::kNone;
  bool session_active = false;
  bool response_pending = false;
};

GovernanceHealth evaluate_governance_health(uint32_t allowed_total, uint32_t preempted_total,
                                            uint32_t rejected_total);
bool can_writer_mutate_domain(CompanionStateWriter writer, CompanionStateDomain domain);
bool can_reader_observe_domain(CompanionStateReader reader, CompanionStateDomain domain);
CompanionSnapshot redact_snapshot_for_reader(const CompanionSnapshot& source,
                                             CompanionStateReader reader);

CompanionEmotionalSignal normalize_emotional_signal(const CompanionEmotionalSignal& signal);
EmotionalTone tone_from_emotion_model(const ncos::models::emotion::EmotionModelState& model);
EmotionalArousal arousal_from_emotion_model(const ncos::models::emotion::EmotionModelState& model);

}  // namespace ncos::core::contracts

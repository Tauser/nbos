#pragma once

#include <stdint.h>

namespace ncos::core::contracts {

inline constexpr uint16_t kSemanticTaxonomyVersion = 1;

enum class SignalKind : uint8_t {
  kEvent = 1,
  kCommand = 2,
  kIntent = 3,
  kReaction = 4,
};

struct SignalHeader {
  SignalKind kind = SignalKind::kEvent;
  uint32_t trace_id = 0;
  uint64_t timestamp_ms = 0;
};

enum class EventTopic : uint8_t {
  kWorldObservation = 1,
  kSystemObservation = 2,
  kUserObservation = 3,
};

enum class CommandTopic : uint8_t {
  kMotionExecute = 1,
  kFaceRenderExecute = 2,
  kAudioOutputExecute = 3,
  kPowerModeSet = 4,
};

enum class IntentTopic : uint8_t {
  kAttendUser = 1,
  kAcknowledgeUser = 2,
  kInspectStimulus = 3,
  kPreserveEnergy = 4,
};

enum class ReactionTopic : uint8_t {
  kBlinkPulse = 1,
  kGazeMicroShift = 2,
  kEarconChirp = 3,
  kLedFeedbackPulse = 4,
};

struct EventMessage {
  SignalHeader header{};
  EventTopic topic = EventTopic::kWorldObservation;
  uint16_t source_service = 0;
};

struct CommandMessage {
  SignalHeader header{};
  CommandTopic topic = CommandTopic::kMotionExecute;
  uint16_t issuer_service = 0;
  uint8_t priority = 0;
  uint32_t ttl_ms = 0;
};

struct IntentMessage {
  SignalHeader header{};
  IntentTopic topic = IntentTopic::kAttendUser;
  uint32_t derived_from_trace_id = 0;
  uint8_t confidence_percent = 0;
};

struct ReactionMessage {
  SignalHeader header{};
  ReactionTopic topic = ReactionTopic::kBlinkPulse;
  uint32_t caused_by_trace_id = 0;
  uint32_t ttl_ms = 0;
};

constexpr bool is_signal_kind_consistent(const EventMessage& message) {
  return message.header.kind == SignalKind::kEvent;
}

constexpr bool is_signal_kind_consistent(const CommandMessage& message) {
  return message.header.kind == SignalKind::kCommand;
}

constexpr bool is_signal_kind_consistent(const IntentMessage& message) {
  return message.header.kind == SignalKind::kIntent;
}

constexpr bool is_signal_kind_consistent(const ReactionMessage& message) {
  return message.header.kind == SignalKind::kReaction;
}

const char* signal_kind_name(SignalKind kind);
const char* event_topic_name(EventTopic topic);
const char* command_topic_name(CommandTopic topic);
const char* intent_topic_name(IntentTopic topic);
const char* reaction_topic_name(ReactionTopic topic);

bool is_valid(const EventMessage& message);
bool is_valid(const CommandMessage& message);
bool is_valid(const IntentMessage& message);
bool is_valid(const ReactionMessage& message);

}  // namespace ncos::core::contracts

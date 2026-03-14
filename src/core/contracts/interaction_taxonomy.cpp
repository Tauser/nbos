#include "core/contracts/interaction_taxonomy.hpp"

namespace ncos::core::contracts {

const char* signal_kind_name(SignalKind kind) {
  switch (kind) {
    case SignalKind::kEvent:
      return "event";
    case SignalKind::kCommand:
      return "command";
    case SignalKind::kIntent:
      return "intent";
    case SignalKind::kReaction:
      return "reaction";
    default:
      return "unknown";
  }
}

const char* event_topic_name(EventTopic topic) {
  switch (topic) {
    case EventTopic::kWorldObservation:
      return "world_observation";
    case EventTopic::kSystemObservation:
      return "system_observation";
    case EventTopic::kUserObservation:
      return "user_observation";
    default:
      return "unknown";
  }
}

const char* command_topic_name(CommandTopic topic) {
  switch (topic) {
    case CommandTopic::kMotionExecute:
      return "motion_execute";
    case CommandTopic::kFaceRenderExecute:
      return "face_render_execute";
    case CommandTopic::kAudioOutputExecute:
      return "audio_output_execute";
    case CommandTopic::kPowerModeSet:
      return "power_mode_set";
    default:
      return "unknown";
  }
}

const char* intent_topic_name(IntentTopic topic) {
  switch (topic) {
    case IntentTopic::kAttendUser:
      return "attend_user";
    case IntentTopic::kAcknowledgeUser:
      return "acknowledge_user";
    case IntentTopic::kInspectStimulus:
      return "inspect_stimulus";
    case IntentTopic::kPreserveEnergy:
      return "preserve_energy";
    default:
      return "unknown";
  }
}

const char* reaction_topic_name(ReactionTopic topic) {
  switch (topic) {
    case ReactionTopic::kBlinkPulse:
      return "blink_pulse";
    case ReactionTopic::kGazeMicroShift:
      return "gaze_micro_shift";
    case ReactionTopic::kEarconChirp:
      return "earcon_chirp";
    case ReactionTopic::kLedFeedbackPulse:
      return "led_feedback_pulse";
    default:
      return "unknown";
  }
}

bool is_valid(const EventMessage& message) {
  return is_signal_kind_consistent(message) && message.header.timestamp_ms > 0;
}

bool is_valid(const CommandMessage& message) {
  return is_signal_kind_consistent(message) && message.header.timestamp_ms > 0 && message.ttl_ms > 0;
}

bool is_valid(const IntentMessage& message) {
  return is_signal_kind_consistent(message) && message.header.timestamp_ms > 0 &&
         message.confidence_percent <= 100;
}

bool is_valid(const ReactionMessage& message) {
  return is_signal_kind_consistent(message) && message.header.timestamp_ms > 0 && message.ttl_ms > 0;
}

}  // namespace ncos::core::contracts

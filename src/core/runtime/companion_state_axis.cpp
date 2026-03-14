#include "core/runtime/companion_state_axis.hpp"

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::runtime {

bool CompanionStateAxis::bind(ncos::interfaces::messaging::SemanticBusPort* bus,
                              ncos::interfaces::state::CompanionStatePort* state,
                              ncos::interfaces::governance::ActionGovernancePort* governance) {
  if (bus == nullptr || state == nullptr || governance == nullptr || bound_) {
    return false;
  }

  bus_ = bus;
  state_ = state;
  governance_ = governance;

  const bool events_ok = bus_->subscribe_events(&CompanionStateAxis::on_event, this);
  const bool commands_ok = bus_->subscribe_commands(&CompanionStateAxis::on_command, this);
  const bool intents_ok = bus_->subscribe_intents(&CompanionStateAxis::on_intent, this);
  const bool reactions_ok = bus_->subscribe_reactions(&CompanionStateAxis::on_reaction, this);

  bound_ = events_ok && commands_ok && intents_ok && reactions_ok;
  if (!bound_) {
    bus_ = nullptr;
    state_ = nullptr;
    governance_ = nullptr;
  }

  return bound_;
}

void CompanionStateAxis::on_event(const ncos::core::contracts::EventMessage& message, void* context) {
  if (context == nullptr) {
    return;
  }

  auto* self = static_cast<CompanionStateAxis*>(context);
  self->handle_event(message);
}

void CompanionStateAxis::on_command(const ncos::core::contracts::CommandMessage& message,
                                    void* context) {
  if (context == nullptr) {
    return;
  }

  auto* self = static_cast<CompanionStateAxis*>(context);
  self->handle_command(message);
}

void CompanionStateAxis::on_intent(const ncos::core::contracts::IntentMessage& message, void* context) {
  if (context == nullptr) {
    return;
  }

  auto* self = static_cast<CompanionStateAxis*>(context);
  self->handle_intent(message);
}

void CompanionStateAxis::on_reaction(const ncos::core::contracts::ReactionMessage& message,
                                     void* context) {
  if (context == nullptr) {
    return;
  }

  auto* self = static_cast<CompanionStateAxis*>(context);
  self->handle_reaction(message);
}

void CompanionStateAxis::handle_event(const ncos::core::contracts::EventMessage& message) {
  ncos::core::contracts::CompanionAttentionalSignal attentional{};

  switch (message.topic) {
    case ncos::core::contracts::EventTopic::kUserObservation:
      attentional.target = ncos::core::contracts::AttentionTarget::kUser;
      attentional.channel = ncos::core::contracts::AttentionChannel::kMultimodal;
      attentional.focus_confidence_percent = 70;
      break;
    case ncos::core::contracts::EventTopic::kWorldObservation:
      attentional.target = ncos::core::contracts::AttentionTarget::kStimulus;
      attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
      attentional.focus_confidence_percent = 50;
      break;
    case ncos::core::contracts::EventTopic::kSystemObservation:
    default:
      attentional.target = ncos::core::contracts::AttentionTarget::kInternalTask;
      attentional.channel = ncos::core::contracts::AttentionChannel::kMultimodal;
      attentional.focus_confidence_percent = 40;
      break;
  }

  (void)state_->ingest_attentional(attentional,
                                   ncos::core::contracts::CompanionStateWriter::kAttentionService,
                                   message.header.timestamp_ms);
}

void CompanionStateAxis::handle_command(const ncos::core::contracts::CommandMessage& message) {
  ncos::core::contracts::ActionProposal proposal{};
  proposal.origin = ncos::core::contracts::ProposalOrigin::kCommand;
  proposal.trace_id = message.header.trace_id;
  proposal.requester_service = message.issuer_service;
  proposal.domain = ncos::core::contracts::command_topic_to_domain(message.topic);
  proposal.action = message.topic;
  proposal.intent_context = ncos::core::contracts::IntentTopic::kAttendUser;
  proposal.priority = message.priority;
  proposal.ttl_ms = message.ttl_ms;
  proposal.preemption_policy = ncos::core::contracts::PreemptionPolicy::kAllowIfHigherPriority;

  const ncos::core::contracts::GovernanceDecision decision =
      governance_->evaluate(proposal, message.header.timestamp_ms);

  (void)state_->ingest_governance_decision(
      decision, ncos::core::contracts::CompanionStateWriter::kGovernanceCore,
      message.header.timestamp_ms);
}

void CompanionStateAxis::handle_intent(const ncos::core::contracts::IntentMessage& message) {
  if (message.topic == ncos::core::contracts::IntentTopic::kPreserveEnergy) {
    ncos::core::contracts::CompanionEnergeticSignal energetic{};
    energetic.mode = ncos::core::contracts::EnergyMode::kConstrained;
    energetic.battery_percent = 30;
    energetic.thermal_load_percent = 35;
    energetic.external_power = false;
    (void)state_->ingest_energetic(energetic, ncos::core::contracts::CompanionStateWriter::kPowerService,
                                   message.header.timestamp_ms);
    return;
  }

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.session_active = true;

  if (message.topic == ncos::core::contracts::IntentTopic::kAcknowledgeUser) {
    interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
    interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
    interaction.response_pending = true;
  } else {
    interaction.phase = ncos::core::contracts::InteractionPhase::kListening;
    interaction.turn_owner = ncos::core::contracts::TurnOwner::kUser;
    interaction.response_pending = false;
  }

  (void)state_->ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService,
      message.header.timestamp_ms);
}

void CompanionStateAxis::handle_reaction(const ncos::core::contracts::ReactionMessage& message) {
  ncos::core::contracts::CompanionEmotionalSignal emotional{};

  switch (message.topic) {
    case ncos::core::contracts::ReactionTopic::kLedFeedbackPulse:
      emotional.tone = ncos::core::contracts::EmotionalTone::kAlert;
      emotional.arousal = ncos::core::contracts::EmotionalArousal::kMedium;
      emotional.intensity_percent = 55;
      emotional.stability_percent = 70;
      break;
    case ncos::core::contracts::ReactionTopic::kEarconChirp:
      emotional.tone = ncos::core::contracts::EmotionalTone::kAffiliative;
      emotional.arousal = ncos::core::contracts::EmotionalArousal::kLow;
      emotional.intensity_percent = 35;
      emotional.stability_percent = 80;
      break;
    case ncos::core::contracts::ReactionTopic::kBlinkPulse:
    case ncos::core::contracts::ReactionTopic::kGazeMicroShift:
    default:
      emotional.tone = ncos::core::contracts::EmotionalTone::kCurious;
      emotional.arousal = ncos::core::contracts::EmotionalArousal::kLow;
      emotional.intensity_percent = 20;
      emotional.stability_percent = 85;
      break;
  }

  (void)state_->ingest_emotional(emotional, ncos::core::contracts::CompanionStateWriter::kEmotionService,
                                 message.header.timestamp_ms);
}

}  // namespace ncos::core::runtime

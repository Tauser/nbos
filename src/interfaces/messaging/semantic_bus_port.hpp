#pragma once

#include "core/contracts/interaction_taxonomy.hpp"

namespace ncos::interfaces::messaging {

using EventHandler = void (*)(const ncos::core::contracts::EventMessage& message, void* context);
using CommandHandler = void (*)(const ncos::core::contracts::CommandMessage& message, void* context);
using IntentHandler = void (*)(const ncos::core::contracts::IntentMessage& message, void* context);
using ReactionHandler = void (*)(const ncos::core::contracts::ReactionMessage& message, void* context);

class SemanticBusPort {
 public:
  virtual ~SemanticBusPort() = default;

  virtual bool publish_event(const ncos::core::contracts::EventMessage& message) = 0;
  virtual bool publish_command(const ncos::core::contracts::CommandMessage& message) = 0;
  virtual bool publish_intent(const ncos::core::contracts::IntentMessage& message) = 0;
  virtual bool publish_reaction(const ncos::core::contracts::ReactionMessage& message) = 0;

  virtual bool subscribe_events(EventHandler handler, void* context) = 0;
  virtual bool subscribe_commands(CommandHandler handler, void* context) = 0;
  virtual bool subscribe_intents(IntentHandler handler, void* context) = 0;
  virtual bool subscribe_reactions(ReactionHandler handler, void* context) = 0;
};

}  // namespace ncos::interfaces::messaging

#pragma once

#include <stdint.h>

#include "interfaces/governance/action_governance_port.hpp"
#include "interfaces/messaging/semantic_bus_port.hpp"
#include "interfaces/state/companion_state_port.hpp"

namespace ncos::core::runtime {

class CompanionStateAxis final {
 public:
  bool bind(ncos::interfaces::messaging::SemanticBusPort* bus,
            ncos::interfaces::state::CompanionStatePort* state,
            ncos::interfaces::governance::ActionGovernancePort* governance);

 private:
  static void on_event(const ncos::core::contracts::EventMessage& message, void* context);
  static void on_command(const ncos::core::contracts::CommandMessage& message, void* context);
  static void on_intent(const ncos::core::contracts::IntentMessage& message, void* context);
  static void on_reaction(const ncos::core::contracts::ReactionMessage& message, void* context);

  void handle_event(const ncos::core::contracts::EventMessage& message);
  void handle_command(const ncos::core::contracts::CommandMessage& message);
  void handle_intent(const ncos::core::contracts::IntentMessage& message);
  void handle_reaction(const ncos::core::contracts::ReactionMessage& message);

  ncos::interfaces::messaging::SemanticBusPort* bus_ = nullptr;
  ncos::interfaces::state::CompanionStatePort* state_ = nullptr;
  ncos::interfaces::governance::ActionGovernancePort* governance_ = nullptr;
  bool bound_ = false;
};

}  // namespace ncos::core::runtime

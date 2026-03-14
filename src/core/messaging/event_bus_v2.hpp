#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/interaction_taxonomy.hpp"
#include "interfaces/messaging/semantic_bus_port.hpp"

namespace ncos::core::messaging {

struct EventBusStats {
  uint32_t published_events = 0;
  uint32_t published_commands = 0;
  uint32_t published_intents = 0;
  uint32_t published_reactions = 0;

  uint32_t dropped_events = 0;
  uint32_t dropped_commands = 0;
  uint32_t dropped_intents = 0;
  uint32_t dropped_reactions = 0;

  uint32_t dispatched_events = 0;
  uint32_t dispatched_commands = 0;
  uint32_t dispatched_intents = 0;
  uint32_t dispatched_reactions = 0;
};

class EventBusV2 final : public ncos::interfaces::messaging::SemanticBusPort {
 public:
  static constexpr size_t kMaxSubscribersPerLane = 8;
  static constexpr size_t kEventQueueCapacity = 16;
  static constexpr size_t kCommandQueueCapacity = 16;
  static constexpr size_t kIntentQueueCapacity = 16;
  static constexpr size_t kReactionQueueCapacity = 16;

  bool publish_event(const ncos::core::contracts::EventMessage& message) override;
  bool publish_command(const ncos::core::contracts::CommandMessage& message) override;
  bool publish_intent(const ncos::core::contracts::IntentMessage& message) override;
  bool publish_reaction(const ncos::core::contracts::ReactionMessage& message) override;

  bool subscribe_events(ncos::interfaces::messaging::EventHandler handler, void* context) override;
  bool subscribe_commands(ncos::interfaces::messaging::CommandHandler handler, void* context) override;
  bool subscribe_intents(ncos::interfaces::messaging::IntentHandler handler, void* context) override;
  bool subscribe_reactions(ncos::interfaces::messaging::ReactionHandler handler, void* context) override;

  void drain(uint16_t max_per_lane);
  EventBusStats stats() const;

 private:
  struct EventSlot {
    ncos::interfaces::messaging::EventHandler handler = nullptr;
    void* context = nullptr;
    bool used = false;
  };

  struct CommandSlot {
    ncos::interfaces::messaging::CommandHandler handler = nullptr;
    void* context = nullptr;
    bool used = false;
  };

  struct IntentSlot {
    ncos::interfaces::messaging::IntentHandler handler = nullptr;
    void* context = nullptr;
    bool used = false;
  };

  struct ReactionSlot {
    ncos::interfaces::messaging::ReactionHandler handler = nullptr;
    void* context = nullptr;
    bool used = false;
  };

  struct EventQueue {
    ncos::core::contracts::EventMessage items[kEventQueueCapacity] = {};
    size_t head = 0;
    size_t size = 0;
  };

  struct IntentQueue {
    ncos::core::contracts::IntentMessage items[kIntentQueueCapacity] = {};
    size_t head = 0;
    size_t size = 0;
  };

  struct ReactionQueue {
    ncos::core::contracts::ReactionMessage items[kReactionQueueCapacity] = {};
    size_t head = 0;
    size_t size = 0;
  };

  struct CommandQueue {
    ncos::core::contracts::CommandMessage items[kCommandQueueCapacity] = {};
    size_t size = 0;
  };

  bool enqueue_event(const ncos::core::contracts::EventMessage& message);
  bool enqueue_intent(const ncos::core::contracts::IntentMessage& message);
  bool enqueue_reaction(const ncos::core::contracts::ReactionMessage& message);
  bool enqueue_command(const ncos::core::contracts::CommandMessage& message);

  bool dequeue_event(ncos::core::contracts::EventMessage* out_message);
  bool dequeue_intent(ncos::core::contracts::IntentMessage* out_message);
  bool dequeue_reaction(ncos::core::contracts::ReactionMessage* out_message);
  bool dequeue_command(ncos::core::contracts::CommandMessage* out_message);

  bool register_event_subscriber(ncos::interfaces::messaging::EventHandler handler, void* context);
  bool register_command_subscriber(ncos::interfaces::messaging::CommandHandler handler, void* context);
  bool register_intent_subscriber(ncos::interfaces::messaging::IntentHandler handler, void* context);
  bool register_reaction_subscriber(ncos::interfaces::messaging::ReactionHandler handler, void* context);

  EventSlot event_subscribers_[kMaxSubscribersPerLane] = {};
  CommandSlot command_subscribers_[kMaxSubscribersPerLane] = {};
  IntentSlot intent_subscribers_[kMaxSubscribersPerLane] = {};
  ReactionSlot reaction_subscribers_[kMaxSubscribersPerLane] = {};

  EventQueue event_queue_{};
  CommandQueue command_queue_{};
  IntentQueue intent_queue_{};
  ReactionQueue reaction_queue_{};

  EventBusStats stats_{};
};

}  // namespace ncos::core::messaging

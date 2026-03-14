#include "core/messaging/event_bus.hpp"

namespace {

size_t queue_tail_index(size_t head, size_t size, size_t capacity) {
  return (head + size) % capacity;
}

}  // namespace

namespace ncos::core::messaging {

bool EventBusV2::publish_event(const ncos::core::contracts::EventMessage& message) {
  if (!ncos::core::contracts::is_valid(message)) {
    return false;
  }

  if (!enqueue_event(message)) {
    ++stats_.dropped_events;
    return false;
  }

  ++stats_.published_events;
  return true;
}

bool EventBusV2::publish_command(const ncos::core::contracts::CommandMessage& message) {
  if (!ncos::core::contracts::is_valid(message)) {
    return false;
  }

  if (!enqueue_command(message)) {
    ++stats_.dropped_commands;
    return false;
  }

  ++stats_.published_commands;
  return true;
}

bool EventBusV2::publish_intent(const ncos::core::contracts::IntentMessage& message) {
  if (!ncos::core::contracts::is_valid(message)) {
    return false;
  }

  if (!enqueue_intent(message)) {
    ++stats_.dropped_intents;
    return false;
  }

  ++stats_.published_intents;
  return true;
}

bool EventBusV2::publish_reaction(const ncos::core::contracts::ReactionMessage& message) {
  if (!ncos::core::contracts::is_valid(message)) {
    return false;
  }

  if (!enqueue_reaction(message)) {
    ++stats_.dropped_reactions;
    return false;
  }

  ++stats_.published_reactions;
  return true;
}

bool EventBusV2::subscribe_events(ncos::interfaces::messaging::EventHandler handler, void* context) {
  return register_event_subscriber(handler, context);
}

bool EventBusV2::subscribe_commands(ncos::interfaces::messaging::CommandHandler handler, void* context) {
  return register_command_subscriber(handler, context);
}

bool EventBusV2::subscribe_intents(ncos::interfaces::messaging::IntentHandler handler, void* context) {
  return register_intent_subscriber(handler, context);
}

bool EventBusV2::subscribe_reactions(ncos::interfaces::messaging::ReactionHandler handler, void* context) {
  return register_reaction_subscriber(handler, context);
}

void EventBusV2::drain(uint16_t max_per_lane) {
  ncos::core::contracts::EventMessage event_message{};
  for (uint16_t i = 0; i < max_per_lane && dequeue_event(&event_message); ++i) {
    for (const EventSlot& slot : event_subscribers_) {
      if (!slot.used || slot.handler == nullptr) {
        continue;
      }
      slot.handler(event_message, slot.context);
    }
    ++stats_.dispatched_events;
  }

  ncos::core::contracts::IntentMessage intent_message{};
  for (uint16_t i = 0; i < max_per_lane && dequeue_intent(&intent_message); ++i) {
    for (const IntentSlot& slot : intent_subscribers_) {
      if (!slot.used || slot.handler == nullptr) {
        continue;
      }
      slot.handler(intent_message, slot.context);
    }
    ++stats_.dispatched_intents;
  }

  ncos::core::contracts::CommandMessage command_message{};
  for (uint16_t i = 0; i < max_per_lane && dequeue_command(&command_message); ++i) {
    for (const CommandSlot& slot : command_subscribers_) {
      if (!slot.used || slot.handler == nullptr) {
        continue;
      }
      slot.handler(command_message, slot.context);
    }
    ++stats_.dispatched_commands;
  }

  ncos::core::contracts::ReactionMessage reaction_message{};
  for (uint16_t i = 0; i < max_per_lane && dequeue_reaction(&reaction_message); ++i) {
    for (const ReactionSlot& slot : reaction_subscribers_) {
      if (!slot.used || slot.handler == nullptr) {
        continue;
      }
      slot.handler(reaction_message, slot.context);
    }
    ++stats_.dispatched_reactions;
  }
}

EventBusStats EventBusV2::stats() const {
  return stats_;
}

bool EventBusV2::enqueue_event(const ncos::core::contracts::EventMessage& message) {
  if (event_queue_.size >= kEventQueueCapacity) {
    return false;
  }

  const size_t tail = queue_tail_index(event_queue_.head, event_queue_.size, kEventQueueCapacity);
  event_queue_.items[tail] = message;
  ++event_queue_.size;
  return true;
}

bool EventBusV2::enqueue_intent(const ncos::core::contracts::IntentMessage& message) {
  if (intent_queue_.size >= kIntentQueueCapacity) {
    return false;
  }

  const size_t tail = queue_tail_index(intent_queue_.head, intent_queue_.size, kIntentQueueCapacity);
  intent_queue_.items[tail] = message;
  ++intent_queue_.size;
  return true;
}

bool EventBusV2::enqueue_reaction(const ncos::core::contracts::ReactionMessage& message) {
  if (reaction_queue_.size >= kReactionQueueCapacity) {
    return false;
  }

  const size_t tail = queue_tail_index(reaction_queue_.head, reaction_queue_.size, kReactionQueueCapacity);
  reaction_queue_.items[tail] = message;
  ++reaction_queue_.size;
  return true;
}

bool EventBusV2::enqueue_command(const ncos::core::contracts::CommandMessage& message) {
  if (command_queue_.size >= kCommandQueueCapacity) {
    return false;
  }

  size_t insert_at = command_queue_.size;
  while (insert_at > 0) {
    const ncos::core::contracts::CommandMessage& current = command_queue_.items[insert_at - 1];
    if (current.priority >= message.priority) {
      break;
    }
    command_queue_.items[insert_at] = current;
    --insert_at;
  }

  command_queue_.items[insert_at] = message;
  ++command_queue_.size;
  return true;
}

bool EventBusV2::dequeue_event(ncos::core::contracts::EventMessage* out_message) {
  if (out_message == nullptr || event_queue_.size == 0) {
    return false;
  }

  *out_message = event_queue_.items[event_queue_.head];
  event_queue_.head = (event_queue_.head + 1U) % kEventQueueCapacity;
  --event_queue_.size;
  return true;
}

bool EventBusV2::dequeue_intent(ncos::core::contracts::IntentMessage* out_message) {
  if (out_message == nullptr || intent_queue_.size == 0) {
    return false;
  }

  *out_message = intent_queue_.items[intent_queue_.head];
  intent_queue_.head = (intent_queue_.head + 1U) % kIntentQueueCapacity;
  --intent_queue_.size;
  return true;
}

bool EventBusV2::dequeue_reaction(ncos::core::contracts::ReactionMessage* out_message) {
  if (out_message == nullptr || reaction_queue_.size == 0) {
    return false;
  }

  *out_message = reaction_queue_.items[reaction_queue_.head];
  reaction_queue_.head = (reaction_queue_.head + 1U) % kReactionQueueCapacity;
  --reaction_queue_.size;
  return true;
}

bool EventBusV2::dequeue_command(ncos::core::contracts::CommandMessage* out_message) {
  if (out_message == nullptr || command_queue_.size == 0) {
    return false;
  }

  *out_message = command_queue_.items[0];
  for (size_t i = 1; i < command_queue_.size; ++i) {
    command_queue_.items[i - 1] = command_queue_.items[i];
  }
  --command_queue_.size;
  return true;
}

bool EventBusV2::register_event_subscriber(ncos::interfaces::messaging::EventHandler handler,
                                           void* context) {
  if (handler == nullptr) {
    return false;
  }

  for (EventSlot& slot : event_subscribers_) {
    if (slot.used) {
      continue;
    }
    slot.used = true;
    slot.handler = handler;
    slot.context = context;
    return true;
  }

  return false;
}

bool EventBusV2::register_command_subscriber(ncos::interfaces::messaging::CommandHandler handler,
                                             void* context) {
  if (handler == nullptr) {
    return false;
  }

  for (CommandSlot& slot : command_subscribers_) {
    if (slot.used) {
      continue;
    }
    slot.used = true;
    slot.handler = handler;
    slot.context = context;
    return true;
  }

  return false;
}

bool EventBusV2::register_intent_subscriber(ncos::interfaces::messaging::IntentHandler handler,
                                            void* context) {
  if (handler == nullptr) {
    return false;
  }

  for (IntentSlot& slot : intent_subscribers_) {
    if (slot.used) {
      continue;
    }
    slot.used = true;
    slot.handler = handler;
    slot.context = context;
    return true;
  }

  return false;
}

bool EventBusV2::register_reaction_subscriber(ncos::interfaces::messaging::ReactionHandler handler,
                                              void* context) {
  if (handler == nullptr) {
    return false;
  }

  for (ReactionSlot& slot : reaction_subscribers_) {
    if (slot.used) {
      continue;
    }
    slot.used = true;
    slot.handler = handler;
    slot.context = context;
    return true;
  }

  return false;
}

}  // namespace ncos::core::messaging


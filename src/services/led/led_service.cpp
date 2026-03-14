#include "services/led/led_service.hpp"

namespace ncos::services::led {

void LedService::bind_port(ncos::interfaces::led::LedPort* port) {
  port_ = port;
}

bool LedService::initialize(uint64_t now_ms) {
  state_ = ncos::core::contracts::make_led_runtime_baseline();
  next_refresh_ms_ = now_ms;

  for (auto& slot : requests_) {
    slot = ncos::core::contracts::LedRequest{};
  }

  if (port_ == nullptr || !port_->ensure_ready()) {
    return false;
  }

  state_.initialized = true;

  ncos::core::contracts::LedRequest off{};
  off.active = true;
  off.owner_service = 0;
  off.priority = ncos::core::contracts::LedPriority::kLow;
  off.state = ncos::core::contracts::make_led_off_state();
  off.expires_at_ms = now_ms + 1000;
  return apply_selected(off, now_ms);
}

bool LedService::submit_request(const ncos::core::contracts::LedRequest& request, uint64_t now_ms) {
  if (!state_.initialized || !ncos::core::contracts::is_valid(request, now_ms)) {
    return false;
  }

  for (auto& slot : requests_) {
    if (slot.active && slot.owner_service == request.owner_service) {
      slot = request;
      return true;
    }
  }

  for (auto& slot : requests_) {
    if (!slot.active) {
      slot = request;
      return true;
    }
  }

  return false;
}

void LedService::clear_owner(uint16_t owner_service) {
  if (owner_service == 0) {
    return;
  }

  for (auto& slot : requests_) {
    if (slot.active && slot.owner_service == owner_service) {
      slot = ncos::core::contracts::LedRequest{};
    }
  }
}

void LedService::tick(uint64_t now_ms, uint32_t refresh_interval_ms) {
  if (!state_.initialized || now_ms < next_refresh_ms_) {
    return;
  }

  expire_requests(now_ms);

  ncos::core::contracts::LedRequest selected{};
  if (select_top_request(&selected)) {
    (void)apply_selected(selected, now_ms);
  } else {
    ncos::core::contracts::LedRequest off{};
    off.active = true;
    off.owner_service = 0;
    off.priority = ncos::core::contracts::LedPriority::kLow;
    off.state = ncos::core::contracts::make_led_off_state();
    off.expires_at_ms = now_ms + refresh_interval_ms;
    (void)apply_selected(off, now_ms);
  }

  next_refresh_ms_ = now_ms + refresh_interval_ms;
}

const ncos::core::contracts::LedRuntimeState& LedService::state() const {
  return state_;
}

void LedService::expire_requests(uint64_t now_ms) {
  for (auto& slot : requests_) {
    if (slot.active && slot.expires_at_ms <= now_ms) {
      slot = ncos::core::contracts::LedRequest{};
    }
  }
}

bool LedService::select_top_request(ncos::core::contracts::LedRequest* out_request) const {
  if (out_request == nullptr) {
    return false;
  }

  bool found = false;
  ncos::core::contracts::LedRequest best{};

  for (const auto& slot : requests_) {
    if (!slot.active) {
      continue;
    }

    if (!found) {
      best = slot;
      found = true;
      continue;
    }

    const uint8_t best_priority = static_cast<uint8_t>(best.priority);
    const uint8_t slot_priority = static_cast<uint8_t>(slot.priority);
    if (slot_priority > best_priority ||
        (slot_priority == best_priority && slot.owner_service < best.owner_service)) {
      best = slot;
    }
  }

  if (!found) {
    return false;
  }

  *out_request = best;
  return true;
}

bool LedService::apply_selected(const ncos::core::contracts::LedRequest& request, uint64_t now_ms) {
  if (port_ == nullptr) {
    return false;
  }

  const bool ok = port_->apply_state(request.state);
  state_.last_apply_ok = ok;
  state_.applied_state = request.state;
  state_.applied_priority = request.priority;
  state_.applied_owner = request.owner_service;
  state_.last_update_ms = now_ms;
  ++state_.arbitration_total;

  if (ok) {
    ++state_.apply_success_total;
  } else {
    ++state_.apply_failure_total;
  }

  return ok;
}

}  // namespace ncos::services::led

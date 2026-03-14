#include "app/lifecycle/system_lifecycle.hpp"

namespace ncos::app::lifecycle {

void SystemLifecycle::start_boot() {
  state_ = SystemState::kBooting;
}

void SystemLifecycle::finish_boot(bool has_required_failures, bool has_warnings) {
  if (has_required_failures) {
    state_ = SystemState::kFaulted;
    return;
  }

  if (has_warnings) {
    state_ = SystemState::kDegraded;
    return;
  }

  state_ = SystemState::kRunning;
}

void SystemLifecycle::mark_fault() {
  state_ = SystemState::kFaulted;
}

SystemState SystemLifecycle::state() const {
  return state_;
}

const char* SystemLifecycle::state_name() const {
  switch (state_) {
    case SystemState::kPowerOn:
      return "power_on";
    case SystemState::kBooting:
      return "booting";
    case SystemState::kRunning:
      return "running";
    case SystemState::kDegraded:
      return "degraded";
    case SystemState::kFaulted:
      return "faulted";
    default:
      return "unknown";
  }
}

}  // namespace ncos::app::lifecycle

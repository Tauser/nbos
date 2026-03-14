#include "core/runtime/safe_mode.hpp"

namespace ncos::core::runtime {

void SafeModeController::enter(const char* reason) {
  active_ = true;
  reason_ = reason == nullptr ? "unknown" : reason;
}

bool SafeModeController::active() const {
  return active_;
}

const char* SafeModeController::reason() const {
  return reason_;
}

}  // namespace ncos::core::runtime

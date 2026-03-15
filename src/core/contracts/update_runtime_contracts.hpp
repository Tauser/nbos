#pragma once

#include <stdint.h>

namespace ncos::core::contracts {

enum class OtaRunningSlot : uint8_t {
  kUnknown = 0,
  kFactory = 1,
  kSlotA = 2,
  kSlotB = 3,
};

enum class OtaHealth : uint8_t {
  kDisabled = 1,
  kNominal = 2,
  kAwaitingConfirm = 3,
  kBlockedUnsafe = 4,
  kFallbackOperational = 5,
};

struct UpdateRuntimeState {
  bool initialized = false;
  bool ota_enabled = false;
  bool remote_allowed = false;
  bool rollback_supported = false;
  bool pending_confirm = false;

  OtaRunningSlot running_slot = OtaRunningSlot::kUnknown;
  OtaHealth health = OtaHealth::kDisabled;

  uint64_t started_ms = 0;
  uint64_t confirm_deadline_ms = 0;
  uint64_t last_update_ms = 0;

  uint32_t checks_total = 0;
  uint32_t blocked_total = 0;
  uint32_t fallback_total = 0;
  uint32_t confirm_success_total = 0;

  const char* last_reason = "none";
};

struct UpdateDecision {
  bool valid = false;
  bool request_safe_fallback = false;
  bool request_rollback = false;
  bool request_confirm = false;
  const char* reason = "none";
};

constexpr UpdateRuntimeState make_update_runtime_baseline() {
  return UpdateRuntimeState{};
}

}  // namespace ncos::core::contracts

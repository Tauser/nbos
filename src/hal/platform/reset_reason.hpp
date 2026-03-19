#pragma once

#include <stdint.h>

namespace ncos::hal::platform {

enum class ResetReasonKind : uint8_t {
  kUnknown = 0,
  kPowerOn,
  kSoftware,
  kDeepSleep,
  kBrownout,
  kWatchdog,
  kPanic,
  kOther,
};

struct ResetReasonStatus {
  ResetReasonKind kind = ResetReasonKind::kUnknown;
  const char* name = "unknown";
  bool unstable_boot_context = false;
};

const ResetReasonStatus& active_reset_reason();

}  // namespace ncos::hal::platform

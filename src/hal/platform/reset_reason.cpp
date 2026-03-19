#include "hal/platform/reset_reason.hpp"

#if defined(ESP_PLATFORM)
#include "esp_system.h"
#endif

namespace {

ncos::hal::platform::ResetReasonStatus make_reset_reason_status() {
#if defined(ESP_PLATFORM)
  const auto reason = esp_reset_reason();
  using ncos::hal::platform::ResetReasonKind;
  using ncos::hal::platform::ResetReasonStatus;

  switch (reason) {
    case ESP_RST_POWERON:
      return {ResetReasonKind::kPowerOn, "power_on", false};
    case ESP_RST_SW:
      return {ResetReasonKind::kSoftware, "software", false};
    case ESP_RST_DEEPSLEEP:
      return {ResetReasonKind::kDeepSleep, "deep_sleep", false};
    case ESP_RST_BROWNOUT:
      return {ResetReasonKind::kBrownout, "brownout", true};
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
      return {ResetReasonKind::kWatchdog, "watchdog", true};
    case ESP_RST_PANIC:
      return {ResetReasonKind::kPanic, "panic", true};
    case ESP_RST_UNKNOWN:
      return {ResetReasonKind::kUnknown, "unknown", false};
    default:
      return {ResetReasonKind::kOther, "other", false};
  }
#else
  return {};
#endif
}

}  // namespace

namespace ncos::hal::platform {

const ResetReasonStatus& active_reset_reason() {
  static const ResetReasonStatus status = make_reset_reason_status();
  return status;
}

}  // namespace ncos::hal::platform

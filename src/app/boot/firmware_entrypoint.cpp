#include "app/boot/firmware_entrypoint.hpp"

#include "app/boot/boot_flow.hpp"

#include "esp_log.h"

namespace {
constexpr const char* kTag = "NCOS_ENTRY";
}

namespace ncos::app::boot {

void FirmwareEntrypoint::run() {
  ESP_LOGI(kTag, "Entrypoint iniciado");
  lifecycle_.start_boot();

  BootFlow boot_flow;
  const BootReport report = boot_flow.execute();

  lifecycle_.finish_boot(report.has_required_failures, report.has_warnings);
  ESP_LOGI(kTag, "Lifecycle apos boot: %s", lifecycle_.state_name());
}

void FirmwareEntrypoint::tick() {
  if (lifecycle_.state() == ncos::app::lifecycle::SystemState::kFaulted) {
    ESP_LOGW(kTag, "Sistema em faulted; aguardando recuperacao manual");
  }
}

const ncos::app::lifecycle::SystemLifecycle& FirmwareEntrypoint::lifecycle() const {
  return lifecycle_;
}

}  // namespace ncos::app::boot

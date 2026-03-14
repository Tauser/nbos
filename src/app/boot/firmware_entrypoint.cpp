#include "app/boot/firmware_entrypoint.hpp"

#include <stdint.h>

#include "app/boot/boot_flow.hpp"

#include "esp_log.h"
#include "esp_timer.h"

namespace {
constexpr const char* kTag = "NCOS_ENTRY";

uint64_t monotonic_ms() {
  return static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
}
}

namespace ncos::app::boot {

void FirmwareEntrypoint::run() {
  ESP_LOGI(kTag, "Entrypoint iniciado");
  lifecycle_.start_boot();

  BootFlow boot_flow;
  const BootReport report = boot_flow.execute();

  lifecycle_.finish_boot(report.has_required_failures, report.has_warnings);
  ESP_LOGI(kTag, "Lifecycle apos boot: %s", lifecycle_.state_name());

  if (!system_manager_.initialize(&lifecycle_)) {
    ESP_LOGE(kTag, "Falha ao inicializar SystemManager");
    lifecycle_.mark_fault();
    return;
  }

  system_manager_.start(monotonic_ms());
}

void FirmwareEntrypoint::tick() {
  system_manager_.tick(monotonic_ms());
}

const ncos::app::lifecycle::SystemLifecycle& FirmwareEntrypoint::lifecycle() const {
  return lifecycle_;
}

}  // namespace ncos::app::boot

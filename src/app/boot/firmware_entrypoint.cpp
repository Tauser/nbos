#include "app/boot/firmware_entrypoint.hpp"

#include <stdint.h>

#include "app/boot/boot_flow.hpp"
#include "config/system_config.hpp"

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

  if (!ncos::config::kConfigReady) {
    ESP_LOGE(kTag, "Config centralizada invalida: build_profile=%s",
             ncos::config::build_profile_name());
    lifecycle_.mark_fault();
    return;
  }

  ESP_LOGI(kTag, "Config ativa: profile=%s board=%s touch=%d imu=(%d,%d)",
           ncos::config::build_profile_name(), ncos::config::kGlobalConfig.board.board_name,
           ncos::config::kGlobalConfig.board.touch, ncos::config::kGlobalConfig.board.imu_sda,
           ncos::config::kGlobalConfig.board.imu_scl);

  lifecycle_.start_boot();

  BootFlow boot_flow;
  const BootReport report = boot_flow.execute();

  lifecycle_.finish_boot(report.has_required_failures, report.has_warnings);
  ESP_LOGI(kTag, "Lifecycle apos boot: %s", lifecycle_.state_name());

  if (!system_manager_.initialize(&lifecycle_, &ncos::config::kGlobalConfig)) {
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

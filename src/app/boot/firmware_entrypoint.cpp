#include "app/boot/firmware_entrypoint.hpp"

#include <stdint.h>

#include "app/boot/boot_flow.hpp"
#include "config/system_config.hpp"
#include "core/contracts/interaction_taxonomy.hpp"
#include "core/runtime/runtime_readiness.hpp"
#include "drivers/audio/audio_local_port.hpp"
#include "drivers/imu/imu_local_port.hpp"
#include "drivers/led/led_local_port.hpp"
#include "drivers/touch/touch_local_port.hpp"
#include "drivers/ttlinker/ttlinker_motion_port.hpp"

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
  ESP_LOGI(kTag, "Taxonomia semantica v%u", ncos::core::contracts::kSemanticTaxonomyVersion);

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

  const uint64_t now = monotonic_ms();
  system_manager_.start(now);

  audio_service_.bind_port(ncos::drivers::audio::acquire_shared_local_audio_port());
  if (!audio_service_.initialize(now)) {
    ESP_LOGW(kTag, "AudioService iniciou em estado degradado");
  }

  touch_service_.bind_port(ncos::drivers::touch::acquire_shared_touch_port());
  if (!touch_service_.initialize(now)) {
    ESP_LOGW(kTag, "TouchService iniciou em estado degradado");
  }

  imu_service_.bind_port(ncos::drivers::imu::acquire_shared_imu_port());
  if (!imu_service_.initialize(now)) {
    ESP_LOGW(kTag, "ImuService iniciou em estado degradado");
  }

  motion_service_.bind_port(ncos::drivers::ttlinker::acquire_shared_motion_port());
  if (!motion_service_.initialize(now)) {
    ESP_LOGW(kTag, "MotionService iniciou em estado degradado");
  }

  led_service_.bind_port(ncos::drivers::led::acquire_shared_led_port());
  if (!led_service_.initialize(now)) {
    ESP_LOGW(kTag, "LedService iniciou em estado degradado");
  }

  if (!face_service_.initialize(now)) {
    ESP_LOGW(kTag, "Pipeline grafico base nao inicializado");
  }

  const ncos::core::runtime::RuntimeReadinessReport readiness =
      ncos::core::runtime::evaluate_runtime_readiness(ncos::config::kGlobalConfig, lifecycle_,
                                                      system_manager_.status());

  ESP_LOGI(kTag,
           "Runtime readiness=%s cfg=%d board=%d lifecycle=%d init=%d started=%d tasks=%d safe=%d faults=%d",
           readiness.level_name(), readiness.config_valid ? 1 : 0,
           readiness.board_profile_bound ? 1 : 0, readiness.lifecycle_allows_runtime ? 1 : 0,
           readiness.runtime_initialized ? 1 : 0, readiness.runtime_started ? 1 : 0,
           readiness.scheduler_has_minimum_tasks ? 1 : 0, readiness.safe_mode_inactive ? 1 : 0,
           readiness.no_faults_recorded ? 1 : 0);
}

void FirmwareEntrypoint::tick() {
  const uint64_t now = monotonic_ms();
  system_manager_.tick(now);
  audio_service_.tick(now);
  touch_service_.tick(now);
  imu_service_.tick(now);
  motion_service_.tick(now);
  led_service_.tick(now, ncos::config::kGlobalConfig.runtime.led_refresh_interval_ms);
  face_service_.tick(now);
}

const ncos::app::lifecycle::SystemLifecycle& FirmwareEntrypoint::lifecycle() const {
  return lifecycle_;
}

}  // namespace ncos::app::boot

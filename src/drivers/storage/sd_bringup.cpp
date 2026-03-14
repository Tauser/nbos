#include "drivers/storage/sd_bringup.hpp"

#include "driver/sdmmc_host.h"
#include "esp_log.h"

namespace {
constexpr const char* kTag = "NCOS_SD";
}

namespace ncos::drivers::storage {

bool SdBringup::run_probe(SdProbeResult* out_result) {
  if (out_result == nullptr) {
    return false;
  }

  SdProbeResult result{};

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_cfg = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_cfg.width = 1;
  slot_cfg.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  const esp_err_t host_err = sdmmc_host_init();
  if (host_err != ESP_OK) {
    ESP_LOGE(kTag, "sdmmc_host_init failed: %s", esp_err_to_name(host_err));
    *out_result = result;
    return true;
  }

  const esp_err_t slot_err = sdmmc_host_init_slot(host.slot, &slot_cfg);
  if (slot_err != ESP_OK) {
    ESP_LOGE(kTag, "sdmmc_host_init_slot failed: %s", esp_err_to_name(slot_err));
    sdmmc_host_deinit();
    *out_result = result;
    return true;
  }

  result.interface_ok = true;
  result.card_init_ok = false;  // Deliberadamente adiado para evitar bloqueio no bring-up inicial.
  result.card_size_mb = 0;

  ESP_LOGW(kTag, "Card init/teste de filesystem adiado para etapa dedicada de storage");

  sdmmc_host_deinit();
  *out_result = result;
  return true;
}

}  // namespace ncos::drivers::storage

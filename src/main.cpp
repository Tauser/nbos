#include "config/system_config.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr const char* kTag = "NCOS";
}

extern "C" void app_main(void) {
  ESP_LOGI(kTag, "Booting NC-OS");
  ESP_LOGI(kTag, "Board profile: %s", ncos::config::kBoardName);
  ESP_LOGI(kTag, "Build profile: %s", ncos::config::build_profile_name());

  if (!ncos::config::kConfigReady) {
    ESP_LOGE(kTag, "Invalid build profile configuration");
  }

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

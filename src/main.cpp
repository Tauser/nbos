#include "config/system_config.hpp"
#include "drivers/display/display_driver.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr const char* kTag = "NCOS";

void run_display_smoke_test(ncos::drivers::display::DisplayDriver& display) {
  if (!display.init()) {
    ESP_LOGE(kTag, "ST7789 init failed");
    return;
  }

  display.setRotation(0);
  display.setTextSize(2);
  display.setTextColor(TFT_WHITE, TFT_BLACK);

  display.fillScreen(TFT_RED);
  vTaskDelay(pdMS_TO_TICKS(400));
  display.fillScreen(TFT_GREEN);
  vTaskDelay(pdMS_TO_TICKS(400));
  display.fillScreen(TFT_BLUE);
  vTaskDelay(pdMS_TO_TICKS(400));

  display.fillScreen(TFT_BLACK);
  display.setCursor(12, 20);
  display.printf("NC-OS ST7789");
  display.setCursor(12, 52);
  display.printf("SMOKE OK");

  ESP_LOGI(kTag, "Display smoke test completed");
}
}  // namespace

extern "C" void app_main(void) {
  ESP_LOGI(kTag, "Booting NC-OS");
  ESP_LOGI(kTag, "Board profile: %s", ncos::config::kBoardName);
  ESP_LOGI(kTag, "Build profile: %s", ncos::config::build_profile_name());

  if (!ncos::config::kConfigReady) {
    ESP_LOGE(kTag, "Invalid build profile configuration");
  }

  ncos::drivers::display::DisplayDriver display;
  run_display_smoke_test(display);

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


#include "config/system_config.hpp"
#include "drivers/audio/audio_bringup.hpp"
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

void run_audio_smoke_test(ncos::drivers::audio::AudioBringup& audio) {
  const bool tx_ok = audio.init_output();
  const bool rx_ok = audio.init_input();

  ESP_LOGI(kTag, "Audio TX init: %s", tx_ok ? "OK" : "FAIL");
  ESP_LOGI(kTag, "Audio RX init: %s", rx_ok ? "OK" : "FAIL");

  if (tx_ok) {
    const bool tone_a = audio.play_tone(440.0F, 350);
    vTaskDelay(pdMS_TO_TICKS(120));
    const bool tone_b = audio.play_tone(880.0F, 350);
    ESP_LOGI(kTag, "Audio TX tones: %s", (tone_a && tone_b) ? "OK" : "FAIL");
  }

  if (rx_ok) {
    int32_t peak = 0;
    size_t samples = 0;
    const bool capture_ok = audio.capture_peak_level(1000, &peak, &samples);
    ESP_LOGI(kTag, "Audio RX capture: %s (samples=%u peak=%ld)",
             capture_ok ? "OK" : "FAIL", static_cast<unsigned>(samples), static_cast<long>(peak));
  }
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

  ncos::drivers::audio::AudioBringup audio;
  run_audio_smoke_test(audio);

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

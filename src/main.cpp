#include "config/system_config.hpp"
#include "drivers/audio/audio_bringup.hpp"
#include "drivers/camera/camera_bringup.hpp"
#include "drivers/display/display_driver.hpp"
#include "drivers/imu/mpu6050_bringup.hpp"
#include "drivers/storage/sd_bringup.hpp"
#include "drivers/touch/touch_bringup.hpp"
#include "drivers/ttlinker/ttlinker_bringup.hpp"

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

void run_touch_smoke_test(ncos::drivers::touch::TouchBringup& touch) {
  const bool init_ok = touch.init();
  ESP_LOGI(kTag, "Touch init: %s", init_ok ? "OK" : "FAIL");
  if (!init_ok) {
    return;
  }

  uint32_t raw = 0;
  if (touch.read_raw(&raw)) {
    ESP_LOGI(kTag, "Touch raw (single): %lu", static_cast<unsigned long>(raw));
  }

  ncos::drivers::touch::TouchStats stats{};
  const bool stats_ok = touch.sample_idle_stats(30, 20, &stats);
  ESP_LOGI(kTag,
           "Touch idle stats: %s (min=%lu max=%lu avg=%lu noise=%lu suggest_delta=%lu)",
           stats_ok ? "OK" : "FAIL",
           static_cast<unsigned long>(stats.min_raw),
           static_cast<unsigned long>(stats.max_raw),
           static_cast<unsigned long>(stats.avg_raw),
           static_cast<unsigned long>(stats.noise_span),
           static_cast<unsigned long>(stats.suggested_delta));
}

void run_imu_smoke_test(ncos::drivers::imu::Mpu6050Bringup& imu) {
  const bool init_ok = imu.init();
  ESP_LOGI(kTag, "IMU init: %s", init_ok ? "OK" : "FAIL");
  if (!init_ok) {
    return;
  }

  ncos::drivers::imu::ImuSample sample{};
  if (imu.read_sample(&sample)) {
    ESP_LOGI(kTag, "IMU sample ax=%d ay=%d az=%d gx=%d gy=%d gz=%d",
             sample.ax, sample.ay, sample.az, sample.gx, sample.gy, sample.gz);
  }

  ncos::drivers::imu::ImuWindowStats stats{};
  const bool noise_ok = imu.sample_window(40, 20, &stats);
  const int ax_span = static_cast<int>(stats.ax_max) - static_cast<int>(stats.ax_min);
  const int ay_span = static_cast<int>(stats.ay_max) - static_cast<int>(stats.ay_min);
  const int az_span = static_cast<int>(stats.az_max) - static_cast<int>(stats.az_min);
  const int gx_span = static_cast<int>(stats.gx_max) - static_cast<int>(stats.gx_min);
  const int gy_span = static_cast<int>(stats.gy_max) - static_cast<int>(stats.gy_min);
  const int gz_span = static_cast<int>(stats.gz_max) - static_cast<int>(stats.gz_min);
  ESP_LOGI(kTag,
           "IMU noise window: %s (ax[%d,%d] ay[%d,%d] az[%d,%d] gx[%d,%d] gy[%d,%d] gz[%d,%d])",
           noise_ok ? "OK" : "FAIL",
           stats.ax_min, stats.ax_max,
           stats.ay_min, stats.ay_max,
           stats.az_min, stats.az_max,
           stats.gx_min, stats.gx_max,
           stats.gy_min, stats.gy_max,
           stats.gz_min, stats.gz_max);
  ESP_LOGI(kTag,
           "IMU noise spans: ax=%d ay=%d az=%d gx=%d gy=%d gz=%d; avg ax=%ld ay=%ld az=%ld gx=%ld gy=%ld gz=%ld",
           ax_span, ay_span, az_span, gx_span, gy_span, gz_span,
           static_cast<long>(stats.ax_avg), static_cast<long>(stats.ay_avg), static_cast<long>(stats.az_avg),
           static_cast<long>(stats.gx_avg), static_cast<long>(stats.gy_avg), static_cast<long>(stats.gz_avg));

  int16_t peak_delta = 0;
  const bool resp_ok = imu.measure_response_peak(40, 20, &peak_delta);
  ESP_LOGI(kTag, "IMU response peak delta: %s (%d)", resp_ok ? "OK" : "FAIL", peak_delta);

  imu.deinit();
}

void run_ttlinker_smoke_test(ncos::drivers::ttlinker::TtlinkerBringup& ttlinker) {
  (void)ttlinker;
  ESP_LOGW(kTag,
           "TTLinker probe adiado: pinos 43/44 compartilhados com console na configuracao atual");
}

void run_camera_smoke_test(ncos::drivers::camera::CameraBringup& camera) {
  ncos::drivers::camera::CameraProbeResult probe{};
  const bool probe_ok = camera.run_probe(&probe);
  ESP_LOGI(kTag,
           "Camera probe: %s (component=%s init=%s frame=%s size=%dx%d len=%d)",
           probe_ok ? "OK" : "FAIL",
           probe.camera_component_available ? "true" : "false",
           probe.init_ok ? "true" : "false",
           probe.frame_ok ? "true" : "false",
           probe.frame_width,
           probe.frame_height,
           probe.frame_len);
}

void run_sd_smoke_test(ncos::drivers::storage::SdBringup& sd) {
  ncos::drivers::storage::SdProbeResult probe{};
  const bool probe_ok = sd.run_probe(&probe);
  ESP_LOGI(kTag,
           "SD probe: %s (interface=%s card_init=%s card_mb=%d)",
           probe_ok ? "OK" : "FAIL",
           probe.interface_ok ? "true" : "false",
           probe.card_init_ok ? "true" : "false",
           probe.card_size_mb);
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

  ncos::drivers::touch::TouchBringup touch;
  run_touch_smoke_test(touch);

  ncos::drivers::imu::Mpu6050Bringup imu;
  run_imu_smoke_test(imu);

  ncos::drivers::camera::CameraBringup camera;
  run_camera_smoke_test(camera);

  ncos::drivers::storage::SdBringup sd;
  run_sd_smoke_test(sd);

  // TTLinker usa pinos sensiveis que tambem podem ser usados pelo console em bring-up.
  // Rodamos por ultimo para nao perder logs dos outros subsistemas.
  ncos::drivers::ttlinker::TtlinkerBringup ttlinker;
  run_ttlinker_smoke_test(ttlinker);

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

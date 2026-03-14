#include "drivers/camera/camera_bringup.hpp"

#include "esp_log.h"

#if __has_include("esp_camera.h")
#include "esp_camera.h"
#define NCOS_HAS_ESP_CAMERA 1
#else
#define NCOS_HAS_ESP_CAMERA 0
#endif

namespace {
constexpr const char* kTag = "NCOS_CAMERA";
}

namespace ncos::drivers::camera {

bool CameraBringup::run_probe(CameraProbeResult* out_result) {
  if (out_result == nullptr) {
    return false;
  }

  CameraProbeResult result{};
  result.camera_component_available = NCOS_HAS_ESP_CAMERA == 1;

#if NCOS_HAS_ESP_CAMERA
  // Pin mapping based on Freenove ESP32-S3-WROOM CAM onboard reference.
  camera_config_t cfg{};
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer = LEDC_TIMER_0;
  cfg.pin_d0 = 11;
  cfg.pin_d1 = 9;
  cfg.pin_d2 = 8;
  cfg.pin_d3 = 10;
  cfg.pin_d4 = 12;
  cfg.pin_d5 = 18;
  cfg.pin_d6 = 17;
  cfg.pin_d7 = 16;
  cfg.pin_xclk = 15;
  cfg.pin_pclk = 13;
  cfg.pin_vsync = 6;
  cfg.pin_href = 7;
  cfg.pin_sccb_sda = 4;
  cfg.pin_sccb_scl = 5;
  cfg.pin_pwdn = -1;
  cfg.pin_reset = -1;
  cfg.xclk_freq_hz = 20000000;
  cfg.frame_size = FRAMESIZE_QVGA;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  cfg.fb_location = CAMERA_FB_IN_PSRAM;
  cfg.jpeg_quality = 15;
  cfg.fb_count = 1;

  if (esp_camera_init(&cfg) != ESP_OK) {
    ESP_LOGE(kTag, "esp_camera_init failed");
    *out_result = result;
    return true;
  }

  result.init_ok = true;

  camera_fb_t* fb = esp_camera_fb_get();
  if (fb != nullptr) {
    result.frame_ok = true;
    result.frame_len = static_cast<int>(fb->len);
    result.frame_width = fb->width;
    result.frame_height = fb->height;
    esp_camera_fb_return(fb);
  }

  esp_camera_deinit();
#else
  ESP_LOGW(kTag, "esp_camera.h not available in current build; probe skipped");
#endif

  *out_result = result;
  return true;
}

}  // namespace ncos::drivers::camera

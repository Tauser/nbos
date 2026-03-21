#include "drivers/camera/camera_bringup.hpp"

#include "config/pins/board_pins.hpp"
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

bool CameraBringup::initialize() {
  if (initialized_) {
    return true;
  }

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
  cfg.pixel_format = PIXFORMAT_GRAYSCALE; // Extração de Luma nativa
  cfg.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  cfg.fb_location = CAMERA_FB_IN_PSRAM;
  cfg.jpeg_quality = 15;
  cfg.fb_count = 1;

  if (esp_camera_init(&cfg) != ESP_OK) {
    ESP_LOGE(kTag, "esp_camera_init failed");
    return false;
  }

  initialized_ = true;
  return true;
#else
  ESP_LOGW(kTag, "esp_camera.h not available in current build; init failed");
  return false;
#endif
}

bool CameraBringup::capture_frame(CameraProbeResult* out_result) {
  if (out_result == nullptr) {
    return false;
  }

  CameraProbeResult result{};
  result.camera_component_available = NCOS_HAS_ESP_CAMERA == 1;

#if NCOS_HAS_ESP_CAMERA
  if (!initialized_) {
    *out_result = result;
    return false;
  }

  result.init_ok = true;

  camera_fb_t* fb = esp_camera_fb_get();
  if (fb != nullptr) {
    result.frame_ok = true;
    result.frame_len = static_cast<int>(fb->len);
    result.frame_width = fb->width;
    result.frame_height = fb->height;

    if (fb->width == 320 && fb->height == 240 && fb->format == PIXFORMAT_GRAYSCALE) {
      uint32_t diff_sum = 0;
      for (int y = 0; y < 24; ++y) {
        for (int x = 0; x < 32; ++x) {
          uint32_t block_sum = 0;
          for (int dy = 0; dy < 10; ++dy) {
            for (int dx = 0; dx < 10; ++dx) {
              block_sum += fb->buf[(y * 10 + dy) * 320 + (x * 10 + dx)];
            }
          }
          const uint8_t block_avg = static_cast<uint8_t>(block_sum / 100);
          const int idx = y * 32 + x;

          if (has_prev_frame_) {
            const int diff = static_cast<int>(block_avg) - static_cast<int>(prev_frame_downsampled_[idx]);
            diff_sum += (diff > 0) ? diff : -diff;
          }
          prev_frame_downsampled_[idx] = block_avg;
        }
      }

      if (has_prev_frame_) {
        // max possible diff = 32 * 24 * 255 = 195840
        result.motion_delta_percent = static_cast<uint8_t>((diff_sum * 100) / 195840);
      }
      has_prev_frame_ = true;
    }

    esp_camera_fb_return(fb);
  }
#else
  ESP_LOGW(kTag, "esp_camera.h not available; frame dropped");
#endif

  *out_result = result;
  return true;
}

void CameraBringup::deinit() {
#if NCOS_HAS_ESP_CAMERA
  if (initialized_) {
    esp_camera_deinit();
    initialized_ = false;
    has_prev_frame_ = false;
  }
#endif
}

}  // namespace ncos::drivers::camera

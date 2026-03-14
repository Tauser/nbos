#include "drivers/touch/touch_bringup.hpp"

#include "config/pins/board_freenove_esp32s3_cam.hpp"
#include "driver/touch_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr const char* kTag = "NCOS_TOUCH";

// GPIO2 maps directly to touch channel 2 on ESP32-S3.
constexpr touch_pad_t kTouchChannel = static_cast<touch_pad_t>(ncos::config::pins::kTouch);
}

namespace ncos::drivers::touch {

bool TouchBringup::init() {
  if (ready_) {
    return true;
  }

  if (touch_pad_init() != ESP_OK) {
    ESP_LOGE(kTag, "touch_pad_init failed");
    return false;
  }

  if (touch_pad_config(kTouchChannel) != ESP_OK) {
    ESP_LOGE(kTag, "touch_pad_config failed");
    touch_pad_deinit();
    return false;
  }

  if (touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER) != ESP_OK) {
    ESP_LOGE(kTag, "touch_pad_set_fsm_mode failed");
    touch_pad_deinit();
    return false;
  }

  if (touch_pad_fsm_start() != ESP_OK) {
    ESP_LOGE(kTag, "touch_pad_fsm_start failed");
    touch_pad_deinit();
    return false;
  }

  ready_ = true;
  return true;
}

bool TouchBringup::read_raw(uint32_t* out_raw) const {
  if (!ready_ || out_raw == nullptr) {
    return false;
  }

  uint32_t raw = 0;
  if (touch_pad_read_raw_data(kTouchChannel, &raw) != ESP_OK) {
    return false;
  }

  *out_raw = raw;
  return true;
}

bool TouchBringup::sample_idle_stats(size_t samples, int delay_ms, TouchStats* out_stats) const {
  if (!ready_ || out_stats == nullptr || samples == 0 || delay_ms < 0) {
    return false;
  }

  // Warm-up reads to let the touch FSM settle after initialization.
  for (int i = 0; i < 5; ++i) {
    uint32_t discard = 0;
    if (!read_raw(&discard)) {
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  uint64_t sum = 0;
  uint32_t min_raw = UINT32_MAX;
  uint32_t max_raw = 0;

  for (size_t i = 0; i < samples; ++i) {
    uint32_t raw = 0;
    if (!read_raw(&raw)) {
      return false;
    }

    if (raw < min_raw) {
      min_raw = raw;
    }
    if (raw > max_raw) {
      max_raw = raw;
    }
    sum += raw;

    if (delay_ms > 0) {
      vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
  }

  const uint32_t avg_raw = static_cast<uint32_t>(sum / samples);
  const uint32_t noise_span = max_raw - min_raw;
  const uint32_t delta_noise = noise_span * 3U;
  const uint32_t delta_percent = avg_raw / 20U;  // ~5% baseline delta.

  out_stats->min_raw = min_raw;
  out_stats->max_raw = max_raw;
  out_stats->avg_raw = avg_raw;
  out_stats->noise_span = noise_span;
  out_stats->suggested_delta = (delta_noise > delta_percent) ? delta_noise : delta_percent;

  return true;
}

void TouchBringup::deinit() {
  if (!ready_) {
    return;
  }

  touch_pad_deinit();
  ready_ = false;
}

}  // namespace ncos::drivers::touch


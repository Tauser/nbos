#include "drivers/audio/audio_bringup.hpp"

#include <math.h>

#include "config/pins/board_pins.hpp"
#include "driver/i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

namespace {
constexpr const char* kTag = "NCOS_AUDIO";
constexpr i2s_port_t kTxPort = I2S_NUM_0;
constexpr i2s_port_t kRxPort = I2S_NUM_1;
constexpr int kSampleRateHz = 16000;
constexpr float kPi = 3.14159265358979323846F;
}  // namespace

namespace ncos::drivers::audio {

bool AudioBringup::init_output() {
  if (output_ready_) {
    return true;
  }

  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = kSampleRateHz,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0,
  };

  const i2s_pin_config_t pins = {
      .bck_io_num = ncos::config::pins::kAudioBclk,
      .ws_io_num = ncos::config::pins::kAudioLrc,
      .data_out_num = ncos::config::pins::kAudioDin,
      .data_in_num = I2S_PIN_NO_CHANGE,
  };

  if (i2s_driver_install(kTxPort, &config, 0, nullptr) != ESP_OK) {
    ESP_LOGE(kTag, "i2s_driver_install TX failed");
    return false;
  }

  if (i2s_set_pin(kTxPort, &pins) != ESP_OK) {
    ESP_LOGE(kTag, "i2s_set_pin TX failed");
    i2s_driver_uninstall(kTxPort);
    return false;
  }

  i2s_zero_dma_buffer(kTxPort);
  output_ready_ = true;
  return true;
}

bool AudioBringup::init_input() {
  if (input_ready_) {
    return true;
  }

  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_SLAVE | I2S_MODE_RX),
      .sample_rate = kSampleRateHz,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0,
  };

  const i2s_pin_config_t pins = {
      .bck_io_num = ncos::config::pins::kMicSck,
      .ws_io_num = ncos::config::pins::kMicWs,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = ncos::config::pins::kMicSd,
  };

  if (i2s_driver_install(kRxPort, &config, 0, nullptr) != ESP_OK) {
    ESP_LOGE(kTag, "i2s_driver_install RX failed");
    return false;
  }

  if (i2s_set_pin(kRxPort, &pins) != ESP_OK) {
    ESP_LOGE(kTag, "i2s_set_pin RX failed");
    i2s_driver_uninstall(kRxPort);
    return false;
  }

  input_ready_ = true;
  return true;
}

bool AudioBringup::play_tone(float frequency_hz, int duration_ms) {
  if (!output_ready_ || duration_ms <= 0 || frequency_hz <= 0.0F) {
    return false;
  }

  constexpr int kFramesPerChunk = 256;
  int32_t samples[kFramesPerChunk * 2];

  const int total_frames = (kSampleRateHz * duration_ms) / 1000;
  const float phase_step = 2.0F * kPi * frequency_hz / static_cast<float>(kSampleRateHz);
  float phase = 0.0F;

  int frames_written = 0;
  while (frames_written < total_frames) {
    const int chunk_frames = (total_frames - frames_written > kFramesPerChunk) ? kFramesPerChunk : (total_frames - frames_written);

    for (int i = 0; i < chunk_frames; ++i) {
      const int32_t value = static_cast<int32_t>(sinf(phase) * 6000.0F) << 16;
      samples[(i * 2)] = value;
      samples[(i * 2) + 1] = value;
      phase += phase_step;
      if (phase > 2.0F * kPi) {
        phase -= 2.0F * kPi;
      }
    }

    size_t bytes_written = 0;
    const size_t bytes_to_write = static_cast<size_t>(chunk_frames * 2 * sizeof(int32_t));
    if (i2s_write(kTxPort, samples, bytes_to_write, &bytes_written, pdMS_TO_TICKS(200)) != ESP_OK || bytes_written != bytes_to_write) {
      ESP_LOGE(kTag, "i2s_write failed");
      return false;
    }

    frames_written += chunk_frames;
  }

  return true;
}

bool AudioBringup::capture_peak_level(int duration_ms, int32_t* out_peak, size_t* out_samples) {
  if (!input_ready_ || duration_ms <= 0 || out_peak == nullptr || out_samples == nullptr) {
    return false;
  }

  int32_t peak = 0;
  size_t total_samples = 0;

  constexpr int kChunkSamples = 256;
  int32_t buffer[kChunkSamples];

  const int loops = duration_ms / 50;
  for (int i = 0; i < loops; ++i) {
    size_t bytes_read = 0;
    if (i2s_read(kRxPort, buffer, sizeof(buffer), &bytes_read, pdMS_TO_TICKS(80)) != ESP_OK) {
      ESP_LOGE(kTag, "i2s_read failed");
      return false;
    }

    const size_t sample_count = bytes_read / sizeof(int32_t);
    total_samples += sample_count;

    for (size_t s = 0; s < sample_count; ++s) {
      const int32_t value = buffer[s];
      const int32_t abs_value = value < 0 ? -value : value;
      if (abs_value > peak) {
        peak = abs_value;
      }
    }
  }

  *out_peak = peak;
  *out_samples = total_samples;
  return true;
}

void AudioBringup::deinit() {
  if (output_ready_) {
    i2s_driver_uninstall(kTxPort);
    output_ready_ = false;
  }
  if (input_ready_) {
    i2s_driver_uninstall(kRxPort);
    input_ready_ = false;
  }
}

}  // namespace ncos::drivers::audio



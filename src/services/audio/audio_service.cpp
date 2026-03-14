#include "services/audio/audio_service.hpp"

#include "config/system_config.hpp"

#ifndef NCOS_NATIVE_TESTS
#include "esp_log.h"
#else
#define ESP_LOGI(tag, fmt, ...) ((void)0)
#define ESP_LOGW(tag, fmt, ...) ((void)0)
#endif

namespace {
constexpr const char* kTag = "NCOS_AUDIO_SVC";
constexpr int kCaptureWindowMs = 60;
}  // namespace

namespace ncos::services::audio {

void AudioService::bind_port(ncos::interfaces::audio::LocalAudioPort* port) {
  port_ = port;
}

bool AudioService::initialize(uint64_t now_ms) {
  if (port_ == nullptr) {
    ESP_LOGW(kTag, "Porta de audio nao conectada ao runtime");
    return false;
  }

  state_.output_ready = port_->ensure_output();
  state_.input_ready = port_->ensure_input();
  state_.initialized = state_.output_ready || state_.input_ready;
  state_.last_capture_ok = false;
  state_.last_peak_level = 0;
  state_.last_capture_samples = 0;
  state_.capture_success_total = 0;
  state_.capture_failure_total = 0;
  state_.last_capture_ms = now_ms;
  next_probe_ms_ = now_ms;

  if (!state_.initialized) {
    ESP_LOGW(kTag, "Falha ao inicializar audio local (tx=%d rx=%d)", state_.output_ready ? 1 : 0,
             state_.input_ready ? 1 : 0);
    return false;
  }

  ESP_LOGI(kTag, "Audio runtime inicializado (tx=%d rx=%d)", state_.output_ready ? 1 : 0,
           state_.input_ready ? 1 : 0);
  return true;
}

void AudioService::tick(uint64_t now_ms) {
  if (!state_.initialized || !state_.input_ready) {
    return;
  }

  const uint32_t probe_interval = ncos::config::kGlobalConfig.runtime.audio_probe_interval_ms;
  if (now_ms < next_probe_ms_) {
    return;
  }

  int32_t peak = 0;
  size_t samples = 0;
  const bool ok = port_->capture_peak_level(kCaptureWindowMs, &peak, &samples);

  state_.last_capture_ok = ok;
  state_.last_capture_ms = now_ms;
  state_.last_capture_samples = samples;

  if (ok) {
    state_.last_peak_level = peak;
    ++state_.capture_success_total;
  } else {
    ++state_.capture_failure_total;
  }

  next_probe_ms_ = now_ms + probe_interval;
}

const ncos::core::contracts::AudioRuntimeState& AudioService::state() const {
  return state_;
}

}  // namespace ncos::services::audio

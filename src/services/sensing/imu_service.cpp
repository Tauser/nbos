#include "services/sensing/imu_service.hpp"

#include <limits.h>

#include "config/system_config.hpp"

#ifndef NCOS_NATIVE_TESTS
#include "esp_log.h"
#else
#define ESP_LOGW(tag, fmt, ...) ((void)0)
#endif

namespace {
constexpr const char* kTag = "NCOS_IMU_SVC";

int16_t clamp_i32_to_i16(int32_t value) {
  if (value > SHRT_MAX) {
    return SHRT_MAX;
  }
  if (value < SHRT_MIN) {
    return SHRT_MIN;
  }
  return static_cast<int16_t>(value);
}
}  // namespace

namespace ncos::services::sensing {

void ImuService::bind_port(ncos::interfaces::sensing::ImuPort* port) {
  port_ = port;
}

bool ImuService::initialize(uint64_t now_ms) {
  if (port_ == nullptr) {
    ESP_LOGW(kTag, "Porta de IMU nao conectada");
    return false;
  }

  state_.initialized = port_->ensure_ready();
  state_.last_read_ok = false;
  state_.last_read_ms = now_ms;
  state_.read_success_total = 0;
  state_.read_failure_total = 0;
  next_probe_ms_ = now_ms;
  return state_.initialized;
}

void ImuService::tick(uint64_t now_ms) {
  if (!state_.initialized || port_ == nullptr) {
    return;
  }

  if (now_ms < next_probe_ms_) {
    return;
  }

  ncos::interfaces::sensing::ImuSampleRaw sample{};
  const bool ok = port_->read_sample(&sample);
  state_.last_read_ok = ok;
  state_.last_read_ms = now_ms;

  if (ok) {
    state_.ax_raw = sample.ax;
    state_.ay_raw = sample.ay;
    state_.az_raw = sample.az;
    state_.gx_raw = sample.gx;
    state_.gy_raw = sample.gy;
    state_.gz_raw = sample.gz;

    state_.ax_mg = accel_raw_to_mg(sample.ax);
    state_.ay_mg = accel_raw_to_mg(sample.ay);
    state_.az_mg = accel_raw_to_mg(sample.az);
    state_.gx_dps = gyro_raw_to_dps(sample.gx);
    state_.gy_dps = gyro_raw_to_dps(sample.gy);
    state_.gz_dps = gyro_raw_to_dps(sample.gz);
    ++state_.read_success_total;
  } else {
    ++state_.read_failure_total;
  }

  next_probe_ms_ = now_ms + ncos::config::kGlobalConfig.runtime.imu_probe_interval_ms;
}

const ncos::core::contracts::ImuRuntimeState& ImuService::state() const {
  return state_;
}

int16_t ImuService::accel_raw_to_mg(int16_t raw) {
  const int32_t mg = (static_cast<int32_t>(raw) * 1000) / 16384;
  return clamp_i32_to_i16(mg);
}

int16_t ImuService::gyro_raw_to_dps(int16_t raw) {
  const int32_t dps = static_cast<int32_t>(raw) / 131;
  return clamp_i32_to_i16(dps);
}

}  // namespace ncos::services::sensing


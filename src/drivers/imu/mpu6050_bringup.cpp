#include "drivers/imu/mpu6050_bringup.hpp"

#include <limits.h>

#include "config/pins/board_freenove_esp32s3_cam.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr const char* kTag = "NCOS_IMU";

constexpr i2c_port_num_t kI2cPort = I2C_NUM_0;
constexpr uint8_t kMpuAddrPrimary = 0x68;
constexpr uint8_t kMpuAddrSecondary = 0x69;
constexpr uint32_t kI2cHz = 100000;

constexpr uint8_t kRegSmplrtDiv = 0x19;
constexpr uint8_t kRegConfig = 0x1A;
constexpr uint8_t kRegGyroConfig = 0x1B;
constexpr uint8_t kRegAccelConfig = 0x1C;
constexpr uint8_t kRegAccelXoutH = 0x3B;
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
constexpr uint8_t kRegWhoAmI = 0x75;
}

namespace ncos::drivers::imu {

bool Mpu6050Bringup::init() {
  if (ready_) {
    return true;
  }

  i2c_master_bus_config_t bus_cfg{};
  bus_cfg.i2c_port = kI2cPort;
  bus_cfg.sda_io_num = static_cast<gpio_num_t>(ncos::config::pins::kImuSda);
  bus_cfg.scl_io_num = static_cast<gpio_num_t>(ncos::config::pins::kImuScl);
  bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
  bus_cfg.glitch_ignore_cnt = 7;
  bus_cfg.intr_priority = 0;
  bus_cfg.trans_queue_depth = 0;
  bus_cfg.flags.enable_internal_pullup = true;

  if (i2c_new_master_bus(&bus_cfg, &bus_) != ESP_OK) {
    ESP_LOGE(kTag, "i2c_new_master_bus failed");
    return false;
  }

  uint8_t selected_addr = 0;
  if (i2c_master_probe(bus_, kMpuAddrPrimary, 100) == ESP_OK) {
    selected_addr = kMpuAddrPrimary;
  } else if (i2c_master_probe(bus_, kMpuAddrSecondary, 100) == ESP_OK) {
    selected_addr = kMpuAddrSecondary;
  } else {
    ESP_LOGE(kTag,
             "MPU6050 nao encontrado no I2C (SDA=%d SCL=%d, candidatos: 0x%02X/0x%02X)",
             ncos::config::pins::kImuSda,
             ncos::config::pins::kImuScl,
             kMpuAddrPrimary,
             kMpuAddrSecondary);
    deinit();
    return false;
  }

  i2c_device_config_t dev_cfg{};
  dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_cfg.device_address = selected_addr;
  dev_cfg.scl_speed_hz = kI2cHz;
  dev_cfg.scl_wait_us = 0;

  if (i2c_master_bus_add_device(bus_, &dev_cfg, &dev_) != ESP_OK) {
    ESP_LOGE(kTag, "i2c_master_bus_add_device failed");
    i2c_del_master_bus(bus_);
    bus_ = nullptr;
    return false;
  }

  uint8_t who = 0;
  if (!read_regs(kRegWhoAmI, &who, 1)) {
    ESP_LOGE(kTag, "MPU6050 WHO_AM_I read failed (addr=0x%02X)", selected_addr);
    deinit();
    return false;
  }

  if (who != 0x68 && who != 0x69) {
    ESP_LOGE(kTag, "Unexpected WHO_AM_I: 0x%02X (addr=0x%02X)", who, selected_addr);
    deinit();
    return false;
  }

  ESP_LOGI(kTag, "MPU6050 detectado addr=0x%02X who=0x%02X", selected_addr, who);

  if (!write_reg(kRegPwrMgmt1, 0x00) || !write_reg(kRegSmplrtDiv, 0x07) ||
      !write_reg(kRegConfig, 0x03) || !write_reg(kRegGyroConfig, 0x00) ||
      !write_reg(kRegAccelConfig, 0x00)) {
    ESP_LOGE(kTag, "MPU6050 basic config failed");
    deinit();
    return false;
  }

  vTaskDelay(pdMS_TO_TICKS(50));
  ready_ = true;
  return true;
}
bool Mpu6050Bringup::read_sample(ImuSample* out_sample) const {
  if (!ready_ || out_sample == nullptr) {
    return false;
  }

  uint8_t raw[14] = {0};
  if (!read_regs(kRegAccelXoutH, raw, sizeof(raw))) {
    return false;
  }

  auto be_to_i16 = [](uint8_t hi, uint8_t lo) -> int16_t {
    return static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
  };

  out_sample->ax = be_to_i16(raw[0], raw[1]);
  out_sample->ay = be_to_i16(raw[2], raw[3]);
  out_sample->az = be_to_i16(raw[4], raw[5]);
  out_sample->temp_raw = be_to_i16(raw[6], raw[7]);
  out_sample->gx = be_to_i16(raw[8], raw[9]);
  out_sample->gy = be_to_i16(raw[10], raw[11]);
  out_sample->gz = be_to_i16(raw[12], raw[13]);

  return true;
}

bool Mpu6050Bringup::sample_window(size_t samples, int delay_ms, ImuWindowStats* out_stats) const {
  if (!ready_ || out_stats == nullptr || samples == 0 || delay_ms < 0) {
    return false;
  }

  ImuWindowStats stats{};
  stats.ax_min = stats.ay_min = stats.az_min = SHRT_MAX;
  stats.gx_min = stats.gy_min = stats.gz_min = SHRT_MAX;
  stats.ax_max = stats.ay_max = stats.az_max = SHRT_MIN;
  stats.gx_max = stats.gy_max = stats.gz_max = SHRT_MIN;

  int64_t ax_sum = 0;
  int64_t ay_sum = 0;
  int64_t az_sum = 0;
  int64_t gx_sum = 0;
  int64_t gy_sum = 0;
  int64_t gz_sum = 0;

  for (size_t i = 0; i < samples; ++i) {
    ImuSample s{};
    if (!read_sample(&s)) {
      return false;
    }

    if (s.ax < stats.ax_min) stats.ax_min = s.ax;
    if (s.ax > stats.ax_max) stats.ax_max = s.ax;
    if (s.ay < stats.ay_min) stats.ay_min = s.ay;
    if (s.ay > stats.ay_max) stats.ay_max = s.ay;
    if (s.az < stats.az_min) stats.az_min = s.az;
    if (s.az > stats.az_max) stats.az_max = s.az;
    if (s.gx < stats.gx_min) stats.gx_min = s.gx;
    if (s.gx > stats.gx_max) stats.gx_max = s.gx;
    if (s.gy < stats.gy_min) stats.gy_min = s.gy;
    if (s.gy > stats.gy_max) stats.gy_max = s.gy;
    if (s.gz < stats.gz_min) stats.gz_min = s.gz;
    if (s.gz > stats.gz_max) stats.gz_max = s.gz;

    ax_sum += s.ax;
    ay_sum += s.ay;
    az_sum += s.az;
    gx_sum += s.gx;
    gy_sum += s.gy;
    gz_sum += s.gz;

    if (delay_ms > 0) {
      vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
  }

  stats.samples = samples;
  stats.ax_avg = static_cast<int32_t>(ax_sum / static_cast<int64_t>(samples));
  stats.ay_avg = static_cast<int32_t>(ay_sum / static_cast<int64_t>(samples));
  stats.az_avg = static_cast<int32_t>(az_sum / static_cast<int64_t>(samples));
  stats.gx_avg = static_cast<int32_t>(gx_sum / static_cast<int64_t>(samples));
  stats.gy_avg = static_cast<int32_t>(gy_sum / static_cast<int64_t>(samples));
  stats.gz_avg = static_cast<int32_t>(gz_sum / static_cast<int64_t>(samples));

  *out_stats = stats;
  return true;
}

bool Mpu6050Bringup::measure_response_peak(size_t samples, int delay_ms, int16_t* out_peak_delta) const {
  if (!ready_ || out_peak_delta == nullptr || samples < 2 || delay_ms < 0) {
    return false;
  }

  ImuSample prev{};
  if (!read_sample(&prev)) {
    return false;
  }

  int16_t peak = 0;
  for (size_t i = 1; i < samples; ++i) {
    if (delay_ms > 0) {
      vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    ImuSample cur{};
    if (!read_sample(&cur)) {
      return false;
    }

    auto abs_i16 = [](int32_t v) -> int16_t {
      const int32_t abs_v = v < 0 ? -v : v;
      return static_cast<int16_t>(abs_v > SHRT_MAX ? SHRT_MAX : abs_v);
    };

    const int16_t d_ax = abs_i16(static_cast<int32_t>(cur.ax) - prev.ax);
    const int16_t d_ay = abs_i16(static_cast<int32_t>(cur.ay) - prev.ay);
    const int16_t d_az = abs_i16(static_cast<int32_t>(cur.az) - prev.az);
    const int16_t d_gx = abs_i16(static_cast<int32_t>(cur.gx) - prev.gx);
    const int16_t d_gy = abs_i16(static_cast<int32_t>(cur.gy) - prev.gy);
    const int16_t d_gz = abs_i16(static_cast<int32_t>(cur.gz) - prev.gz);

    const int16_t local_peak = (d_ax > d_ay ? d_ax : d_ay) > d_az ? (d_ax > d_ay ? d_ax : d_ay) : d_az;
    const int16_t local_peak_g = (d_gx > d_gy ? d_gx : d_gy) > d_gz ? (d_gx > d_gy ? d_gx : d_gy) : d_gz;
    const int16_t total_peak = local_peak > local_peak_g ? local_peak : local_peak_g;

    if (total_peak > peak) {
      peak = total_peak;
    }

    prev = cur;
  }

  *out_peak_delta = peak;
  return true;
}

void Mpu6050Bringup::deinit() {
  if (dev_ != nullptr) {
    i2c_master_bus_rm_device(dev_);
    dev_ = nullptr;
  }

  if (bus_ != nullptr) {
    i2c_del_master_bus(bus_);
    bus_ = nullptr;
  }

  ready_ = false;
}

bool Mpu6050Bringup::write_reg(uint8_t reg, uint8_t value) const {
  if (dev_ == nullptr) {
    return false;
  }

  const uint8_t data[2] = {reg, value};
  return i2c_master_transmit(dev_, data, sizeof(data), 100) == ESP_OK;
}

bool Mpu6050Bringup::read_regs(uint8_t start_reg, uint8_t* out_data, size_t len) const {
  if (dev_ == nullptr || out_data == nullptr || len == 0) {
    return false;
  }

  return i2c_master_transmit_receive(dev_, &start_reg, 1, out_data, len, 100) == ESP_OK;
}

}  // namespace ncos::drivers::imu

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"

namespace ncos::drivers::imu {

struct ImuSample {
  int16_t ax = 0;
  int16_t ay = 0;
  int16_t az = 0;
  int16_t temp_raw = 0;
  int16_t gx = 0;
  int16_t gy = 0;
  int16_t gz = 0;
};

struct ImuWindowStats {
  int16_t ax_min = 0;
  int16_t ax_max = 0;
  int16_t ay_min = 0;
  int16_t ay_max = 0;
  int16_t az_min = 0;
  int16_t az_max = 0;
  int16_t gx_min = 0;
  int16_t gx_max = 0;
  int16_t gy_min = 0;
  int16_t gy_max = 0;
  int16_t gz_min = 0;
  int16_t gz_max = 0;
  int32_t ax_avg = 0;
  int32_t ay_avg = 0;
  int32_t az_avg = 0;
  int32_t gx_avg = 0;
  int32_t gy_avg = 0;
  int32_t gz_avg = 0;
  size_t samples = 0;
};

class Mpu6050Bringup final {
 public:
  bool init();
  bool read_sample(ImuSample* out_sample) const;
  bool sample_window(size_t samples, int delay_ms, ImuWindowStats* out_stats) const;
  bool measure_response_peak(size_t samples, int delay_ms, int16_t* out_peak_delta) const;
  void deinit();

 private:
  bool write_reg(uint8_t reg, uint8_t value) const;
  bool read_regs(uint8_t start_reg, uint8_t* out_data, size_t len) const;

  i2c_master_bus_handle_t bus_ = nullptr;
  i2c_master_dev_handle_t dev_ = nullptr;
  bool ready_ = false;
};

}  // namespace ncos::drivers::imu

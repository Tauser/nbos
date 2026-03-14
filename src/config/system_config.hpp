#pragma once

#include <stdint.h>

#include "config/build_profile.hpp"
#include "config/pins/board_freenove_esp32s3_cam.hpp"

namespace ncos::config {

struct RuntimeConfig {
  uint32_t lifecycle_watchdog_ms = 1000;
  uint32_t diagnostics_heartbeat_ms = 3000;
  bool diagnostics_enabled = true;
};

struct BoardProfile {
  const char* board_name = "unknown";
  int display_mosi = -1;
  int display_sck = -1;
  int display_dc = -1;
  int display_rst = -1;
  int display_cs = -1;
  int touch = -1;
  int imu_sda = -1;
  int imu_scl = -1;
  bool gpio0_boot_sensitive = false;
  bool gpio46_input_only = false;
};

struct GlobalConfig {
  BuildProfile build_profile = BuildProfile::kUnknown;
  bool config_ready = false;
  RuntimeConfig runtime{};
  BoardProfile board{};
};

constexpr RuntimeConfig make_runtime_config() {
  RuntimeConfig cfg{};

  if (detect_build_profile() == BuildProfile::kProd) {
    cfg.diagnostics_heartbeat_ms = 5000;
  }

  return cfg;
}

constexpr BoardProfile make_board_profile() {
  BoardProfile board{};
  board.board_name = kBoardName;
  board.display_mosi = pins::kDisplayMosi;
  board.display_sck = pins::kDisplaySck;
  board.display_dc = pins::kDisplayDc;
  board.display_rst = pins::kDisplayRst;
  board.display_cs = pins::kDisplayCs;
  board.touch = pins::kTouch;
  board.imu_sda = pins::kImuSda;
  board.imu_scl = pins::kImuScl;
  board.gpio0_boot_sensitive = pin_flags::kGpio0BootSensitive;
  board.gpio46_input_only = pin_flags::kGpio46InputOnly;
  return board;
}

constexpr GlobalConfig make_global_config() {
  GlobalConfig cfg{};
  cfg.build_profile = detect_build_profile();
  cfg.config_ready = is_build_profile_valid();
  cfg.runtime = make_runtime_config();
  cfg.board = make_board_profile();
  return cfg;
}

inline constexpr GlobalConfig kGlobalConfig = make_global_config();
inline constexpr bool kConfigReady = kGlobalConfig.config_ready;

}  // namespace ncos::config

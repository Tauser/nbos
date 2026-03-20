#pragma once

#include <stdint.h>

#include "config/build_profile.hpp"
#include "config/pins/board_pins.hpp"

namespace ncos::config {

enum class DisplayDiagnosticsMode : uint8_t {
  kOff = 0,
  kStaticPrimaries,
  kStaticClipGrid,
  kHorizontalSweepFullRedraw,
  kEyeTrailFullRedraw,
  kEyeTrailDirtyRect,
  kEyeTrailBandComposite,
  kEyeTrailBandRedraw,
  kSpriteWindowTrail,
  kPanelPolarityFlip,
  kFaceVisualDebug,
};

struct RuntimeConfig {
  uint32_t lifecycle_watchdog_ms = 1000;
  uint32_t runtime_tick_watchdog_ms = 2600;
  uint32_t diagnostics_heartbeat_ms = 3000;
  bool diagnostics_enabled = true;
  uint8_t boot_display_attempts = 2;
  DisplayDiagnosticsMode display_diagnostics_mode = DisplayDiagnosticsMode::kOff;
  uint32_t face_frame_budget_us = 14000;
  uint32_t display_diagnostics_spi_write_hz = 40000000;
  bool display_diagnostics_conservative_pacing = false;
  uint16_t display_diagnostics_conservative_period_ms = 135;

  bool ota_enabled = false;
  bool ota_remote_allowed = false;
  uint32_t ota_confirm_uptime_ms = 12000;

  bool cloud_sync_enabled = false;
  bool cloud_bridge_enabled = false;
  bool cloud_extension_enabled = false;
  uint32_t cloud_sync_interval_ms = 2500;

  bool telemetry_enabled = false;
  bool telemetry_export_off_device = false;
  uint32_t telemetry_interval_ms = 8000;
  bool telemetry_collect_interactional = false;
  bool telemetry_collect_emotional = false;
  bool telemetry_collect_transient = false;
  uint32_t cloud_sync_retry_backoff_ms = 6000;
  uint8_t cloud_sync_failure_threshold = 3;

  uint32_t audio_probe_interval_ms = 120;
  uint32_t touch_probe_interval_ms = 120;
  uint32_t imu_probe_interval_ms = 100;
  uint32_t camera_probe_interval_ms = 700;
  uint32_t power_probe_interval_ms = 1800;
  uint16_t power_guard_brownout_mv = 3380;
  uint8_t power_guard_thermal_constrained_percent = 75;
  uint8_t power_guard_thermal_critical_percent = 90;
  uint8_t power_guard_sample_failure_limit = 3;
  uint32_t led_refresh_interval_ms = 120;
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
    cfg.runtime_tick_watchdog_ms = 3200;
    cfg.diagnostics_heartbeat_ms = 5000;
    cfg.boot_display_attempts = 3;

    cfg.ota_enabled = true;
    cfg.ota_remote_allowed = false;
    cfg.ota_confirm_uptime_ms = 18000;

    cfg.cloud_sync_enabled = false;
    cfg.cloud_bridge_enabled = false;
    cfg.cloud_extension_enabled = false;
    cfg.cloud_sync_interval_ms = 3500;

    cfg.telemetry_enabled = false;
    cfg.telemetry_export_off_device = false;
    cfg.telemetry_interval_ms = 12000;
    cfg.telemetry_collect_interactional = false;
    cfg.telemetry_collect_emotional = false;
    cfg.telemetry_collect_transient = false;
    cfg.cloud_sync_retry_backoff_ms = 8000;
    cfg.cloud_sync_failure_threshold = 3;

    cfg.audio_probe_interval_ms = 160;
    cfg.touch_probe_interval_ms = 160;
    cfg.imu_probe_interval_ms = 140;
    cfg.camera_probe_interval_ms = 1000;
    cfg.power_probe_interval_ms = 2600;
    cfg.power_guard_brownout_mv = 3360;
    cfg.power_guard_thermal_constrained_percent = 78;
    cfg.power_guard_thermal_critical_percent = 92;
    cfg.power_guard_sample_failure_limit = 4;
    cfg.led_refresh_interval_ms = 180;
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



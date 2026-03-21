#pragma once

#include <stdint.h>

namespace ncos::config {

inline constexpr const char* kBoardName = "freenove_esp32s3_wroom_cam_n16r8";

namespace pins {

// ST7789 display (CS is hardwired to GND in the current prototype).
inline constexpr int kDisplayMosi = 21;
inline constexpr int kDisplaySck = 47;
inline constexpr int kDisplayDc = 45;
inline constexpr int kDisplayRst = -1;
inline constexpr int kDisplayCs = -1;  // Not connected to MCU.

// INMP441 microphone
inline constexpr int kMicWs = 41;
inline constexpr int kMicSck = 42;
inline constexpr int kMicSd = 14;

// MAX98357A amplifier
inline constexpr int kAudioLrc = 41;
inline constexpr int kAudioBclk = 42;
inline constexpr int kAudioDin = 1;

// Capacitive touch
inline constexpr int kTouch = 2;

// TTLinker mini / servos
inline constexpr int kServoTx = 43;
inline constexpr int kServoRx = 44;

// MPU6050 (provisional baseline; must stay under architectural control)
inline constexpr int kImuSda = 0;
inline constexpr int kImuScl = 19;

}  // namespace pins

namespace power_rails {

// Logical power domains documented for board-level integration.
inline constexpr float kBatteryNominalVolts = 3.7F;
inline constexpr float kBatteryMaxVolts = 4.2F;
inline constexpr float kMain5vRailVolts = 5.0F;
inline constexpr float kLogic3v3RailVolts = 3.3F;

}  // namespace power_rails

namespace pin_flags {

// Sensitive pins: require explicit justification before remap.
inline constexpr bool kGpio0BootSensitive = true;   // IMU SDA (provisional)
inline constexpr bool kGpio3UartSensitive = true;   // Shared debug implications
inline constexpr bool kGpio46InputOnly = true;      // Mic SD line

}  // namespace pin_flags

}  // namespace ncos::config

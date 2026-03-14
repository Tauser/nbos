#pragma once

#include <stdint.h>

namespace ncos::config {

inline constexpr const char* kBoardName = "freenove_esp32s3_wroom_cam_n16r8";

namespace pins {

// ST7789 display
inline constexpr int kDisplayMosi = 21;
inline constexpr int kDisplaySck = 47;
inline constexpr int kDisplayDc = 45;
inline constexpr int kDisplayRst = 20;

// INMP441 microphone
inline constexpr int kMicWs = 14;
inline constexpr int kMicSck = 3;
inline constexpr int kMicSd = 46;

// MAX98357A amplifier
inline constexpr int kAudioLrc = 41;
inline constexpr int kAudioBclk = 42;
inline constexpr int kAudioDin = 1;

// Capacitive touch
inline constexpr int kTouch = 2;

// TTLinker mini / servos
inline constexpr int kServoTx = 43;
inline constexpr int kServoRx = 44;

// MPU6050 (provisional baseline; keep under architectural control)
inline constexpr int kImuSda = 0;
inline constexpr int kImuScl = 19;

}  // namespace pins

}  // namespace ncos::config

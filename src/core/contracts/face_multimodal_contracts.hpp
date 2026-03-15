#pragma once

#include <stdint.h>

#include "core/contracts/audio_runtime_contracts.hpp"
#include "core/contracts/sensing_runtime_contracts.hpp"

namespace ncos::core::contracts {

struct FaceMultimodalInput {
  bool audio_ready = false;
  bool touch_active = false;
  bool imu_ready = false;
  bool motion_active = false;

  uint8_t audio_energy_percent = 0;
  uint8_t touch_intensity_percent = 0;
  uint8_t motion_intensity_percent = 0;
  uint64_t observed_at_ms = 0;
};

FaceMultimodalInput make_face_multimodal_input(const AudioRuntimeState& audio,
                                               const TouchRuntimeState& touch,
                                               const ImuRuntimeState& imu,
                                               uint64_t now_ms);

}  // namespace ncos::core::contracts

#pragma once

#include "app/lifecycle/system_lifecycle.hpp"
#include "core/runtime/system_manager.hpp"
#include "services/audio/audio_service.hpp"
#include "services/face/face_service.hpp"
#include "services/led/led_service.hpp"
#include "services/motion/motion_service.hpp"
#include "services/sensing/imu_service.hpp"
#include "services/sensing/touch_service.hpp"

namespace ncos::app::boot {

class FirmwareEntrypoint final {
 public:
  FirmwareEntrypoint() = default;

  void run();
  void tick();
  [[nodiscard]] const ncos::app::lifecycle::SystemLifecycle& lifecycle() const;

 private:
  ncos::app::lifecycle::SystemLifecycle lifecycle_{};
  ncos::core::runtime::SystemManager system_manager_{};
  ncos::services::audio::AudioService audio_service_{};
  ncos::services::sensing::TouchService touch_service_{};
  ncos::services::sensing::ImuService imu_service_{};
  ncos::services::motion::MotionService motion_service_{};
  ncos::services::led::LedService led_service_{};
  ncos::services::face::FaceService face_service_{};
};

}  // namespace ncos::app::boot

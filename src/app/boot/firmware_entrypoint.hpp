#pragma once

#include "app/lifecycle/system_lifecycle.hpp"
#include "core/runtime/system_manager.hpp"
#include "services/audio/audio_service.hpp"
#include "services/behavior/behavior_service.hpp"
#include "services/cloud/cloud_sync_service.hpp"
#include "services/emotion/emotion_service.hpp"
#include "services/face/face_service.hpp"
#include "services/led/led_service.hpp"
#include "services/motion/motion_service.hpp"
#include "services/power/power_service.hpp"
#include "services/routine/routine_service.hpp"
#include "services/sensing/imu_service.hpp"
#include "services/sensing/touch_service.hpp"
#include "services/update/update_service.hpp"
#include "services/voice/voice_service.hpp"
#include "services/vision/camera_service.hpp"
#include "services/vision/perception_service.hpp"

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
  ncos::services::behavior::BehaviorService behavior_service_{};
  ncos::services::routine::RoutineService routine_service_{};
  ncos::services::emotion::EmotionService emotion_service_{};
  ncos::services::voice::VoiceService voice_service_{};
  ncos::services::sensing::TouchService touch_service_{};
  ncos::services::sensing::ImuService imu_service_{};
  ncos::services::vision::CameraService camera_service_{};
  ncos::services::vision::PerceptionService perception_service_{};
  ncos::services::power::PowerService power_service_{};
  ncos::services::update::UpdateService update_service_{};
  ncos::services::cloud::CloudSyncService cloud_sync_service_{};
  ncos::services::motion::MotionService motion_service_{};
  ncos::services::led::LedService led_service_{};
  ncos::services::face::FaceService face_service_{};

  bool ota_fault_reported_ = false;
  bool ota_fallback_reported_ = false;
  bool power_electrical_fault_reported_ = false;
  bool power_thermal_fault_reported_ = false;
};

}  // namespace ncos::app::boot


#include "app/boot/boot_flow.hpp"

#include "config/system_config.hpp"
#include "drivers/audio/audio_local_port.hpp"
#include "drivers/camera/camera_bringup.hpp"
#include "drivers/display/display_runtime.hpp"
#include "drivers/imu/imu_local_port.hpp"
#include "drivers/storage/sd_bringup.hpp"
#include "drivers/touch/touch_local_port.hpp"
#include "drivers/ttlinker/ttlinker_motion_port.hpp"
#include "hal/platform/reset_reason.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr const char* kTag = "NCOS_BOOT";

void merge_result(ncos::app::boot::BootReport* report, bool ok, bool required) {
  if (ok) {
    return;
  }

  report->has_warnings = true;
  if (required) {
    report->has_required_failures = true;
  }
}

bool step_config_gate() {
  ESP_LOGI(kTag, "[1/8] Config gate");
  if (!ncos::config::kConfigReady) {
    ESP_LOGE(kTag, "Build profile invalido");
    return false;
  }

  const auto& reset = ncos::hal::platform::active_reset_reason();
  ESP_LOGI(kTag, "Board=%s Profile=%s Reset=%s unstable=%d", ncos::config::kBoardName,
           ncos::config::build_profile_name(), reset.name, reset.unstable_boot_context ? 1 : 0);
  return true;
}

bool try_display_once() {
  struct DisplayTaskContext {
    TaskHandle_t caller;
    bool ok;
  };

  auto display_task = [](void* arg) {
    auto* ctx = static_cast<DisplayTaskContext*>(arg);

    ncos::drivers::display::DisplayDriver* display = ncos::drivers::display::acquire_shared_display();
    ctx->ok = display != nullptr;
    if (ctx->ok) {
      display->setRotation(0);
      display->setTextSize(2);
      display->setTextColor(TFT_WHITE, TFT_BLACK);
      display->fillScreen(TFT_BLACK);
      display->setCursor(12, 20);
      display->printf("NC-OS");
      display->setCursor(12, 52);
      display->printf("BOOT");
    }

    xTaskNotifyGive(ctx->caller);
    vTaskDelete(nullptr);
  };

  DisplayTaskContext ctx{
      .caller = xTaskGetCurrentTaskHandle(),
      .ok = false,
  };

  constexpr uint32_t kDisplayTaskStackBytes = 12 * 1024;
  const BaseType_t created = xTaskCreate(display_task,
                                         "ncos_disp_boot",
                                         kDisplayTaskStackBytes,
                                         &ctx,
                                         tskIDLE_PRIORITY + 1,
                                         nullptr);
  if (created != pdPASS) {
    ESP_LOGE(kTag, "Falha ao criar task de bring-up do display");
    return false;
  }

  const uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(4000));
  if (notified == 0) {
    ESP_LOGE(kTag, "Timeout no bring-up do display");
    return false;
  }

  return ctx.ok;
}

bool step_display() {
  ESP_LOGI(kTag, "[2/8] Display bring-up");

  uint8_t attempts = ncos::config::kGlobalConfig.runtime.boot_display_attempts;
  if (attempts < 1) {
    attempts = 1;
  }
  if (attempts > 3) {
    attempts = 3;
  }

  for (uint8_t attempt = 1; attempt <= attempts; ++attempt) {
    const bool ok = try_display_once();
    if (ok) {
      if (attempt > 1) {
        ESP_LOGW(kTag, "Display estabilizou apos retry %u/%u", static_cast<unsigned>(attempt),
                 static_cast<unsigned>(attempts));
      }
      return true;
    }

    ESP_LOGW(kTag, "Display falhou na tentativa %u/%u", static_cast<unsigned>(attempt),
             static_cast<unsigned>(attempts));
    if (attempt < attempts) {
      vTaskDelay(pdMS_TO_TICKS(120));
    }
  }

  ESP_LOGE(kTag, "Display esgotou retries de boot");
  return false;
}

bool step_audio() {
  ESP_LOGI(kTag, "[3/8] Audio bring-up");
  ncos::interfaces::audio::LocalAudioPort* audio = ncos::drivers::audio::acquire_shared_local_audio_port();
  if (audio == nullptr) {
    ESP_LOGE(kTag, "Audio backend nao disponivel");
    return false;
  }

  const bool tx_ok = audio->ensure_output();
  const bool rx_ok = audio->ensure_input();
  ESP_LOGI(kTag, "Audio tx=%s rx=%s", tx_ok ? "OK" : "FAIL", rx_ok ? "OK" : "FAIL");
  return tx_ok && rx_ok;
}

bool step_touch() {
  ESP_LOGI(kTag, "[4/8] Touch bring-up");
  ncos::interfaces::sensing::TouchPort* touch = ncos::drivers::touch::acquire_shared_touch_port();
  if (touch == nullptr || !touch->ensure_ready()) {
    ESP_LOGE(kTag, "Touch init falhou");
    return false;
  }

  (void)touch->calibrate_idle();

  uint32_t raw = 0;
  if (touch->read_raw(&raw)) {
    ESP_LOGI(kTag, "Touch raw=%lu", static_cast<unsigned long>(raw));
  }
  return true;
}

bool step_imu() {
  ESP_LOGI(kTag, "[5/8] IMU bring-up");
  ncos::interfaces::sensing::ImuPort* imu = ncos::drivers::imu::acquire_shared_imu_port();
  if (imu == nullptr || !imu->ensure_ready()) {
    ESP_LOGW(kTag, "IMU init falhou");
    return false;
  }

  ncos::interfaces::sensing::ImuSampleRaw sample{};
  const bool sample_ok = imu->read_sample(&sample);
  if (sample_ok) {
    ESP_LOGI(kTag, "IMU sample ax=%d ay=%d az=%d", sample.ax, sample.ay, sample.az);
  }
  return sample_ok;
}

bool step_camera() {
  ESP_LOGI(kTag, "[6/8] Camera bring-up");
  ncos::drivers::camera::CameraBringup camera;
  ncos::drivers::camera::CameraProbeResult probe{};
  if (!camera.run_probe(&probe)) {
    ESP_LOGW(kTag, "Camera probe falhou");
    return false;
  }

  ESP_LOGI(kTag, "Camera component=%s init=%s frame=%s",
           probe.camera_component_available ? "true" : "false",
           probe.init_ok ? "true" : "false",
           probe.frame_ok ? "true" : "false");
  return probe.camera_component_available && probe.init_ok;
}

bool step_sd() {
  ESP_LOGI(kTag, "[7/8] SD bring-up");
  ncos::drivers::storage::SdBringup sd;
  ncos::drivers::storage::SdProbeResult probe{};
  if (!sd.run_probe(&probe)) {
    ESP_LOGW(kTag, "SD probe falhou");
    return false;
  }

  ESP_LOGI(kTag, "SD interface=%s card_init=%s card_mb=%d",
           probe.interface_ok ? "true" : "false",
           probe.card_init_ok ? "true" : "false",
           probe.card_size_mb);
  return probe.interface_ok;
}

bool step_ttlinker() {
  ESP_LOGI(kTag, "[8/8] TTLinker bring-up");
  ncos::interfaces::motion::MotionPort* motion = ncos::drivers::ttlinker::acquire_shared_motion_port();
  if (motion == nullptr) {
    ESP_LOGW(kTag, "TTLinker backend indisponivel");
    return false;
  }  if (motion->has_transport_conflict()) {
    ESP_LOGW(kTag, "TTLinker adiado: transporte indisponivel no perfil atual");
    return false;
  }

  if (!motion->ensure_ready()) {
    ESP_LOGW(kTag, "TTLinker init falhou");
    return false;
  }

  const ncos::core::contracts::MotionPoseCommand neutral = ncos::core::contracts::make_neutral_pose();
  size_t bytes = 0;
  const bool applied = motion->apply_pose(neutral, &bytes);
  ESP_LOGI(kTag, "TTLinker neutral_pose=%s tx_bytes=%u", applied ? "OK" : "FAIL",
           static_cast<unsigned>(bytes));
  return applied;
}

}  // namespace

namespace ncos::app::boot {

BootReport BootFlow::execute() {
  BootReport report{};
  const bool is_dev_profile = ncos::config::detect_build_profile() == ncos::config::BuildProfile::kDev;

  merge_result(&report, step_config_gate(), true);
  merge_result(&report, step_display(), true);
  merge_result(&report, step_audio(), false);
  merge_result(&report, step_touch(), false);
  merge_result(&report, step_imu(), false);
  merge_result(&report, step_camera(), false);
  if (is_dev_profile) {
    ESP_LOGW(kTag, "[7/8] SD bring-up adiado no profile dev para destravar runtime/diagnostico");
  } else {
    merge_result(&report, step_sd(), false);
  }
  merge_result(&report, step_ttlinker(), false);

  return report;
}

}  // namespace ncos::app::boot






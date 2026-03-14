#include "app/boot/boot_flow.hpp"

#include "config/system_config.hpp"
#include "drivers/audio/audio_local_port.hpp"
#include "drivers/camera/camera_bringup.hpp"
#include "drivers/display/display_runtime.hpp"
#include "drivers/imu/mpu6050_bringup.hpp"
#include "drivers/storage/sd_bringup.hpp"
#include "drivers/touch/touch_bringup.hpp"
#include "drivers/ttlinker/ttlinker_bringup.hpp"

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
  ESP_LOGI(kTag, "Board=%s Profile=%s", ncos::config::kBoardName, ncos::config::build_profile_name());
  return true;
}

bool step_display() {
  ESP_LOGI(kTag, "[2/8] Display bring-up");

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

  if (!ctx.ok) {
    ESP_LOGE(kTag, "Display init falhou");
  }
  return ctx.ok;
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
  ncos::drivers::touch::TouchBringup touch;
  if (!touch.init()) {
    ESP_LOGE(kTag, "Touch init falhou");
    return false;
  }

  uint32_t raw = 0;
  if (touch.read_raw(&raw)) {
    ESP_LOGI(kTag, "Touch raw=%lu", static_cast<unsigned long>(raw));
  }
  return true;
}

bool step_imu() {
  ESP_LOGI(kTag, "[5/8] IMU bring-up");
  ncos::drivers::imu::Mpu6050Bringup imu;
  const bool init_ok = imu.init();
  if (!init_ok) {
    ESP_LOGW(kTag, "IMU init falhou");
    return false;
  }

  ncos::drivers::imu::ImuSample sample{};
  const bool sample_ok = imu.read_sample(&sample);
  if (sample_ok) {
    ESP_LOGI(kTag, "IMU sample ax=%d ay=%d az=%d", sample.ax, sample.ay, sample.az);
  }
  imu.deinit();
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
  ncos::drivers::ttlinker::TtlinkerBringup ttlinker;
  ESP_LOGW(kTag, "TTLinker adiado: pinos 43/44 compartilhados com console no perfil atual");
  (void)ttlinker;
  return false;
}

}  // namespace

namespace ncos::app::boot {

BootReport BootFlow::execute() {
  BootReport report{};

  merge_result(&report, step_config_gate(), true);
  merge_result(&report, step_display(), true);
  merge_result(&report, step_audio(), false);
  merge_result(&report, step_touch(), false);
  merge_result(&report, step_imu(), false);
  merge_result(&report, step_camera(), false);
  merge_result(&report, step_sd(), false);
  merge_result(&report, step_ttlinker(), false);

  return report;
}

}  // namespace ncos::app::boot

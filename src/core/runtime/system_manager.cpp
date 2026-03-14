#include "core/runtime/system_manager.hpp"

#include "esp_log.h"

namespace {
constexpr const char* kTag = "NCOS_SYSMGR";
constexpr uint32_t kLifecycleWatchdogMs = 1000;
}

namespace ncos::core::runtime {

bool SystemManager::initialize(const ncos::app::lifecycle::SystemLifecycle* lifecycle) {
  if (lifecycle == nullptr) {
    return false;
  }

  lifecycle_ = lifecycle;
  initialized_ = true;
  return true;
}

void SystemManager::start(uint64_t now_ms) {
  if (!initialized_ || started_) {
    return;
  }

  const bool added = scheduler_.register_task(
      "lifecycle_watchdog", kLifecycleWatchdogMs, &SystemManager::lifecycle_watchdog_task, this, now_ms);
  if (!added) {
    ESP_LOGE(kTag, "Falha ao registrar tarefa lifecycle_watchdog");
    return;
  }

  started_ = true;
  ESP_LOGI(kTag, "SystemManager iniciado com %u tarefa(s)", static_cast<unsigned>(scheduler_.task_count()));
}

void SystemManager::tick(uint64_t now_ms) {
  if (!started_) {
    return;
  }

  scheduler_.tick(now_ms);
}

void SystemManager::lifecycle_watchdog_task(void* context) {
  if (context == nullptr) {
    return;
  }

  auto* self = static_cast<SystemManager*>(context);
  if (self->lifecycle_ == nullptr) {
    return;
  }

  if (self->lifecycle_->state() == ncos::app::lifecycle::SystemState::kFaulted) {
    ESP_LOGW(kTag, "Lifecycle em faulted");
  }
}

}  // namespace ncos::core::runtime

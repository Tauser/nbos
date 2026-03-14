#include "core/runtime/system_manager.hpp"

#include "esp_log.h"

namespace {
constexpr const char* kTag = "NCOS_SYSMGR";
constexpr uint32_t kLifecycleWatchdogMs = 1000;
constexpr uint32_t kDiagnosticsHeartbeatMs = 3000;
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

  last_tick_ms_ = now_ms;
  health_.reset(now_ms);
  refresh_health(now_ms);

  const bool lifecycle_added = scheduler_.register_task("lifecycle_watchdog", kLifecycleWatchdogMs,
                                                        &SystemManager::lifecycle_watchdog_task, this, now_ms);
  if (!lifecycle_added) {
    fault_history_.push(FaultCode::kLifecycleWatchdogRegistrationFailed, now_ms,
                        "watchdog task registration failed");
    safe_mode_.enter("watchdog_task_registration_failed");
    refresh_health(now_ms);
    ESP_LOGE(kTag, "Falha ao registrar tarefa lifecycle_watchdog");
    return;
  }

  const bool diagnostics_added = scheduler_.register_task(
      "diagnostics_heartbeat", kDiagnosticsHeartbeatMs, &SystemManager::diagnostics_heartbeat_task, this, now_ms);
  if (!diagnostics_added) {
    fault_history_.push(FaultCode::kDiagnosticsTaskRegistrationFailed, now_ms,
                        "diagnostics task registration failed");
    safe_mode_.enter("diagnostics_task_registration_failed");
    refresh_health(now_ms);
    ESP_LOGW(kTag, "Falha ao registrar diagnostics_heartbeat; runtime segue em modo seguro");
  }

  started_ = true;
  refresh_health(now_ms);
  ESP_LOGI(kTag, "SystemManager iniciado com %u tarefa(s)", static_cast<unsigned>(scheduler_.task_count()));
}

void SystemManager::tick(uint64_t now_ms) {
  if (!started_) {
    return;
  }

  last_tick_ms_ = now_ms;
  scheduler_.tick(now_ms);
  refresh_health(now_ms);
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
    self->handle_lifecycle_fault();
  }
}

void SystemManager::diagnostics_heartbeat_task(void* context) {
  if (context == nullptr) {
    return;
  }

  auto* self = static_cast<SystemManager*>(context);
  const DiagnosticsSnapshot snapshot =
      self->diagnostics_.snapshot(self->health_, self->safe_mode_, self->fault_history_);
  self->diagnostics_.log_heartbeat(snapshot);
}

void SystemManager::handle_lifecycle_fault() {
  if (lifecycle_fault_latched_) {
    return;
  }

  lifecycle_fault_latched_ = true;
  fault_history_.push(FaultCode::kLifecycleFaulted, last_tick_ms_, "lifecycle entered faulted state");
  safe_mode_.enter("lifecycle_faulted");
  refresh_health(last_tick_ms_);
  ESP_LOGW(kTag, "Lifecycle em faulted; safe mode ativado");
}

void SystemManager::refresh_health(uint64_t now_ms) {
  health_.update(now_ms, static_cast<uint32_t>(scheduler_.task_count()), fault_history_.count(),
                 safe_mode_.active());
}

}  // namespace ncos::core::runtime

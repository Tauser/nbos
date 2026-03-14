#include "core/runtime/system_manager.hpp"

#include "core/contracts/interaction_taxonomy.hpp"
#include "esp_log.h"

namespace {
constexpr const char* kTag = "NCOS_SYSMGR";
constexpr uint16_t kBusDrainPerLane = 4;
}

namespace ncos::core::runtime {

bool SystemManager::initialize(const ncos::app::lifecycle::SystemLifecycle* lifecycle,
                               const ncos::config::GlobalConfig* config) {
  if (lifecycle == nullptr || config == nullptr || !config->config_ready) {
    return false;
  }

  lifecycle_ = lifecycle;
  config_ = config;
  initialized_ = true;
  return true;
}

void SystemManager::start(uint64_t now_ms) {
  if (!initialized_ || started_ || config_ == nullptr) {
    return;
  }

  last_tick_ms_ = now_ms;
  health_.reset(now_ms);
  refresh_health(now_ms);

  const bool lifecycle_added = scheduler_.register_task(
      "lifecycle_watchdog", config_->runtime.lifecycle_watchdog_ms,
      &SystemManager::lifecycle_watchdog_task, this, now_ms);
  if (!lifecycle_added) {
    fault_history_.push(FaultCode::kLifecycleWatchdogRegistrationFailed, now_ms,
                        "watchdog task registration failed");
    safe_mode_.enter("watchdog_task_registration_failed");
    refresh_health(now_ms);
    ESP_LOGE(kTag, "Falha ao registrar tarefa lifecycle_watchdog");
    return;
  }

  if (config_->runtime.diagnostics_enabled) {
    const bool diagnostics_added = scheduler_.register_task(
        "diagnostics_heartbeat", config_->runtime.diagnostics_heartbeat_ms,
        &SystemManager::diagnostics_heartbeat_task, this, now_ms);
    if (!diagnostics_added) {
      fault_history_.push(FaultCode::kDiagnosticsTaskRegistrationFailed, now_ms,
                          "diagnostics task registration failed");
      safe_mode_.enter("diagnostics_task_registration_failed");
      refresh_health(now_ms);
      ESP_LOGW(kTag, "Falha ao registrar diagnostics_heartbeat; runtime segue em modo seguro");
    }
  }

  ncos::core::contracts::EventMessage runtime_started{};
  runtime_started.header.kind = ncos::core::contracts::SignalKind::kEvent;
  runtime_started.header.trace_id = static_cast<uint32_t>(now_ms & 0xFFFFFFFFULL);
  runtime_started.header.timestamp_ms = now_ms;
  runtime_started.topic = ncos::core::contracts::EventTopic::kSystemObservation;
  runtime_started.source_service = 1;
  (void)event_bus_.publish_event(runtime_started);
  event_bus_.drain(kBusDrainPerLane);

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
  event_bus_.drain(kBusDrainPerLane);
  refresh_health(now_ms);
}

RuntimeStatus SystemManager::status() const {
  RuntimeStatus out{};
  out.initialized = initialized_;
  out.started = started_;
  out.scheduler_tasks = static_cast<uint32_t>(scheduler_.task_count());
  out.safe_mode = safe_mode_.active();
  out.fault_count = fault_history_.count();
  out.last_tick_ms = last_tick_ms_;

  const ncos::core::messaging::EventBusStats bus_stats = event_bus_.stats();
  out.bus_published_total = bus_stats.published_events + bus_stats.published_commands +
                            bus_stats.published_intents + bus_stats.published_reactions;
  out.bus_dispatched_total = bus_stats.dispatched_events + bus_stats.dispatched_commands +
                             bus_stats.dispatched_intents + bus_stats.dispatched_reactions;
  out.bus_dropped_total = bus_stats.dropped_events + bus_stats.dropped_commands +
                          bus_stats.dropped_intents + bus_stats.dropped_reactions;

  const ncos::core::governance::ActionGovernanceStats governance_stats = action_governor_.stats();
  out.governance_allowed_total = governance_stats.allowed;
  out.governance_preempted_total = governance_stats.preempted;
  out.governance_rejected_total = governance_stats.rejected;
  return out;
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

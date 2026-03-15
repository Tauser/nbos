#include "app/boot/firmware_entrypoint.hpp"

#include <stdint.h>

#include "app/boot/boot_flow.hpp"
#include "config/system_config.hpp"
#include "core/contracts/behavior_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/face_multimodal_contracts.hpp"
#include "core/contracts/interaction_taxonomy.hpp"
#include "core/contracts/motion_runtime_contracts.hpp"
#include "core/contracts/routine_runtime_contracts.hpp"
#include "core/runtime/runtime_readiness.hpp"
#include "drivers/audio/audio_local_port.hpp"
#include "drivers/imu/imu_local_port.hpp"
#include "drivers/led/led_local_port.hpp"
#include "drivers/touch/touch_local_port.hpp"
#include "drivers/ttlinker/ttlinker_motion_port.hpp"

#include "esp_log.h"
#include "esp_timer.h"

namespace {
constexpr uint16_t kBehaviorServiceId = 61;
constexpr uint16_t kRoutineServiceId = 62;
constexpr const char* kTag = "NCOS_ENTRY";

uint64_t monotonic_ms() {
  return static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
}

uint8_t map_arousal_percent(const ncos::core::contracts::CompanionEmotionalState& emotional) {
  uint8_t base = 30;
  switch (emotional.arousal) {
    case ncos::core::contracts::EmotionalArousal::kHigh:
      base = 85;
      break;
    case ncos::core::contracts::EmotionalArousal::kMedium:
      base = 60;
      break;
    case ncos::core::contracts::EmotionalArousal::kLow:
    default:
      base = 30;
      break;
  }

  return static_cast<uint8_t>((static_cast<uint16_t>(base) + emotional.intensity_percent) / 2U);
}

ncos::core::contracts::MotionCompanionSignal make_motion_companion_signal(
    const ncos::core::contracts::CompanionSnapshot& snapshot) {
  ncos::core::contracts::MotionCompanionSignal signal{};
  signal.safe_mode = snapshot.runtime.safe_mode ||
                     snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kCritical;
  signal.attention_lock = snapshot.attentional.lock_active ||
                          snapshot.attentional.focus_confidence_percent >= 70;
  signal.emotional_arousal_percent = map_arousal_percent(snapshot.emotional);
  return signal;
}
}  // namespace

namespace ncos::app::boot {

void FirmwareEntrypoint::run() {
  ESP_LOGI(kTag, "Entrypoint iniciado");

  if (!ncos::config::kConfigReady) {
    ESP_LOGE(kTag, "Config centralizada invalida: build_profile=%s",
             ncos::config::build_profile_name());
    lifecycle_.mark_fault();
    return;
  }

  ESP_LOGI(kTag, "Config ativa: profile=%s board=%s touch=%d imu=(%d,%d)",
           ncos::config::build_profile_name(), ncos::config::kGlobalConfig.board.board_name,
           ncos::config::kGlobalConfig.board.touch, ncos::config::kGlobalConfig.board.imu_sda,
           ncos::config::kGlobalConfig.board.imu_scl);
  ESP_LOGI(kTag, "Taxonomia semantica v%u", ncos::core::contracts::kSemanticTaxonomyVersion);

  lifecycle_.start_boot();

  BootFlow boot_flow;
  const BootReport report = boot_flow.execute();

  lifecycle_.finish_boot(report.has_required_failures, report.has_warnings);
  ESP_LOGI(kTag, "Lifecycle apos boot: %s", lifecycle_.state_name());

  if (!system_manager_.initialize(&lifecycle_, &ncos::config::kGlobalConfig)) {
    ESP_LOGE(kTag, "Falha ao inicializar SystemManager");
    lifecycle_.mark_fault();
    return;
  }

  const uint64_t now = monotonic_ms();
  system_manager_.start(now);

  audio_service_.bind_port(ncos::drivers::audio::acquire_shared_local_audio_port());
  if (!audio_service_.initialize(now)) {
    ESP_LOGW(kTag, "AudioService iniciou em estado degradado");
  }

  if (!behavior_service_.initialize(kBehaviorServiceId, now)) {
    ESP_LOGW(kTag, "BehaviorService iniciou em estado degradado");
  }

  if (!routine_service_.initialize(kRoutineServiceId, now)) {
    ESP_LOGW(kTag, "RoutineService iniciou em estado degradado");
  }

  touch_service_.bind_port(ncos::drivers::touch::acquire_shared_touch_port());
  if (!touch_service_.initialize(now)) {
    ESP_LOGW(kTag, "TouchService iniciou em estado degradado");
  }

  imu_service_.bind_port(ncos::drivers::imu::acquire_shared_imu_port());
  if (!imu_service_.initialize(now)) {
    ESP_LOGW(kTag, "ImuService iniciou em estado degradado");
  }

  motion_service_.bind_port(ncos::drivers::ttlinker::acquire_shared_motion_port());
  if (!motion_service_.initialize(now)) {
    ESP_LOGW(kTag, "MotionService iniciou em estado degradado");
  }

  led_service_.bind_port(ncos::drivers::led::acquire_shared_led_port());
  if (!led_service_.initialize(now)) {
    ESP_LOGW(kTag, "LedService iniciou em estado degradado");
  }

  if (!face_service_.initialize(now)) {
    ESP_LOGW(kTag, "Pipeline grafico base nao inicializado");
  }

  const ncos::core::runtime::RuntimeReadinessReport readiness =
      ncos::core::runtime::evaluate_runtime_readiness(ncos::config::kGlobalConfig, lifecycle_,
                                                      system_manager_.status());

  ESP_LOGI(kTag,
           "Runtime readiness=%s cfg=%d board=%d lifecycle=%d init=%d started=%d tasks=%d safe=%d faults=%d",
           readiness.level_name(), readiness.config_valid ? 1 : 0,
           readiness.board_profile_bound ? 1 : 0, readiness.lifecycle_allows_runtime ? 1 : 0,
           readiness.runtime_initialized ? 1 : 0, readiness.runtime_started ? 1 : 0,
           readiness.scheduler_has_minimum_tasks ? 1 : 0, readiness.safe_mode_inactive ? 1 : 0,
           readiness.no_faults_recorded ? 1 : 0);
}

void FirmwareEntrypoint::tick() {
  const uint64_t now = monotonic_ms();
  system_manager_.tick(now);
  audio_service_.tick(now);
  touch_service_.tick(now);
  imu_service_.tick(now);

  const ncos::core::contracts::FaceMultimodalInput face_multimodal =
      ncos::core::contracts::make_face_multimodal_input(audio_service_.state(), touch_service_.state(),
                                                        imu_service_.state(), now);
  face_service_.tick(now, face_multimodal);

  const ncos::core::contracts::CompanionSnapshot behavior_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kBehaviorService);
  ncos::core::contracts::BehaviorProposal behavior_proposal{};
  if (behavior_service_.tick(behavior_snapshot, now, &behavior_proposal) && behavior_proposal.valid) {
    const ncos::core::contracts::GovernanceDecision behavior_decision =
        system_manager_.govern_action(behavior_proposal.proposal, now);
    behavior_service_.on_governance_decision(behavior_decision, now);
  }

  ncos::core::contracts::RoutineProposal routine_proposal{};
  if (routine_service_.tick(behavior_snapshot, behavior_service_.state(), now, &routine_proposal) &&
      routine_proposal.valid) {
    const ncos::core::contracts::GovernanceDecision routine_decision =
        system_manager_.govern_action(routine_proposal.proposal, now);
    routine_service_.on_governance_decision(routine_decision, now);
  }

  const ncos::core::contracts::CompanionSnapshot companion_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kMotionService);
  motion_service_.update_companion_signal(make_motion_companion_signal(companion_snapshot), now);
  motion_service_.update_face_signal(face_service_.motion_signal(), now);
  motion_service_.tick(now);

  led_service_.tick(now, ncos::config::kGlobalConfig.runtime.led_refresh_interval_ms);
}

const ncos::app::lifecycle::SystemLifecycle& FirmwareEntrypoint::lifecycle() const {
  return lifecycle_;
}

}  // namespace ncos::app::boot

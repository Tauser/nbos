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
#include "drivers/camera/camera_local_port.hpp"
#include "drivers/imu/imu_local_port.hpp"
#include "drivers/led/led_local_port.hpp"
#include "drivers/power/power_local_port.hpp"
#include "drivers/touch/touch_local_port.hpp"
#include "drivers/ttlinker/ttlinker_motion_port.hpp"

#include "esp_log.h"
#include "esp_timer.h"

namespace {
constexpr uint16_t BehaviorServiceId = 61;
constexpr uint16_t RoutineServiceId = 62;
constexpr uint16_t EmotionServiceId = 63;
constexpr uint16_t VoiceServiceId = 64;
constexpr uint16_t PerceptionServiceId = 65;
constexpr uint16_t PowerServiceId = 66;
constexpr const char* Tag = "NCOS_ENTRY";

uint64_t monotonic_ms() {
  return static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
}

bool decision_allows(const ncos::core::contracts::GovernanceDecision& decision) {
  return decision.kind == ncos::core::contracts::GovernanceDecisionKind::kAllow ||
         decision.kind == ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow;
}

void ingest_behavior_companion_signals(ncos::core::runtime::SystemManager* manager,
                                       ncos::core::contracts::BehaviorProfile profile,
                                       uint64_t now_ms) {
  if (manager == nullptr) {
    return;
  }

  switch (profile) {
    case ncos::core::contracts::BehaviorProfile::kAlertScan: {
      ncos::core::contracts::CompanionAttentionalSignal attentional{};
      attentional.target = ncos::core::contracts::AttentionTarget::kStimulus;
      attentional.channel = ncos::core::contracts::AttentionChannel::kMultimodal;
      attentional.focus_confidence_percent = 78;
      attentional.lock_active = true;
      (void)manager->ingest_attentional_signal(attentional, now_ms);

      ncos::core::contracts::CompanionInteractionSignal interaction{};
      interaction.phase = ncos::core::contracts::InteractionPhase::kActing;
      interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
      interaction.session_active = true;
      interaction.response_pending = false;
      (void)manager->ingest_interactional_signal(interaction, now_ms);
      break;
    }

    case ncos::core::contracts::BehaviorProfile::kAttendUser: {
      ncos::core::contracts::CompanionAttentionalSignal attentional{};
      attentional.target = ncos::core::contracts::AttentionTarget::kUser;
      attentional.channel = ncos::core::contracts::AttentionChannel::kMultimodal;
      attentional.focus_confidence_percent = 86;
      attentional.lock_active = true;
      (void)manager->ingest_attentional_signal(attentional, now_ms);

      ncos::core::contracts::CompanionInteractionSignal interaction{};
      interaction.phase = ncos::core::contracts::InteractionPhase::kListening;
      interaction.turn_owner = ncos::core::contracts::TurnOwner::kUser;
      interaction.session_active = true;
      interaction.response_pending = false;
      (void)manager->ingest_interactional_signal(interaction, now_ms);
      break;
    }

    case ncos::core::contracts::BehaviorProfile::kIdleObserve:
    default:
      break;
  }
}

uint8_t map_arousal_percent(const ncos::core::contracts::CompanionSnapshot& snapshot,
                            const ncos::core::contracts::BehaviorRuntimeState& behavior_state,
                            uint64_t now_ms) {
  uint16_t base = snapshot.emotional.vector.arousal_percent;

  if (snapshot.emotional.arousal == ncos::core::contracts::EmotionalArousal::kHigh) {
    base = static_cast<uint16_t>(base + 10);
  }

  if (behavior_state.active_profile == ncos::core::contracts::BehaviorProfile::kAlertScan &&
      behavior_state.last_accept_ms > 0 && (now_ms - behavior_state.last_accept_ms) < 1400) {
    base = static_cast<uint16_t>(base + 12);
  }

  if (base > 100U) {
    base = 100U;
  }

  return static_cast<uint8_t>(base);
}

ncos::core::contracts::MotionCompanionSignal make_motion_companion_signal(
    const ncos::core::contracts::CompanionSnapshot& snapshot,
    const ncos::core::contracts::BehaviorRuntimeState& behavior_state,
    uint64_t now_ms) {
  ncos::core::contracts::MotionCompanionSignal signal{};
  signal.safe_mode = snapshot.runtime.safe_mode ||
                     snapshot.energetic.mode == ncos::core::contracts::EnergyMode::kCritical;

  const bool behavior_attention_lock =
      (behavior_state.active_profile == ncos::core::contracts::BehaviorProfile::kAlertScan ||
       behavior_state.active_profile == ncos::core::contracts::BehaviorProfile::kAttendUser) &&
      behavior_state.last_accept_ms > 0 && (now_ms - behavior_state.last_accept_ms) < 1600;

  signal.attention_lock = snapshot.attentional.lock_active ||
                          snapshot.attentional.focus_confidence_percent >= 70 ||
                          behavior_attention_lock;
  signal.emotional_arousal_percent = map_arousal_percent(snapshot, behavior_state, now_ms);
  return signal;
}
}  // namespace

namespace ncos::app::boot {

void FirmwareEntrypoint::run() {
  ESP_LOGI(Tag, "Entrypoint iniciado");

  if (!ncos::config::kConfigReady) {
    ESP_LOGE(Tag, "Config centralizada invalida: build_profile=%s",
             ncos::config::build_profile_name());
    lifecycle_.mark_fault();
    return;
  }

  ESP_LOGI(Tag, "Config ativa: profile=%s board=%s touch=%d imu=(%d,%d)",
           ncos::config::build_profile_name(), ncos::config::kGlobalConfig.board.board_name,
           ncos::config::kGlobalConfig.board.touch, ncos::config::kGlobalConfig.board.imu_sda,
           ncos::config::kGlobalConfig.board.imu_scl);
  ESP_LOGI(Tag, "Taxonomia semantica v%u", ncos::core::contracts::kSemanticTaxonomyVersion);

  lifecycle_.start_boot();

  BootFlow boot_flow;
  const BootReport report = boot_flow.execute();

  lifecycle_.finish_boot(report.has_required_failures, report.has_warnings);
  ESP_LOGI(Tag, "Lifecycle apos boot: %s", lifecycle_.state_name());

  if (report.has_required_failures) {
    ESP_LOGE(Tag, "Boot com falha obrigatoria; runtime nao sera iniciado");
    return;
  }

  if (!system_manager_.initialize(&lifecycle_, &ncos::config::kGlobalConfig)) {
    ESP_LOGE(Tag, "Falha ao inicializar SystemManager");
    lifecycle_.mark_fault();
    return;
  }

  const uint64_t now = monotonic_ms();
  system_manager_.start(now);

  audio_service_.bind_port(ncos::drivers::audio::acquire_shared_local_audio_port());
  if (!audio_service_.initialize(now)) {
    ESP_LOGW(Tag, "AudioService iniciou em estado degradado");
  }

  if (!behavior_service_.initialize(BehaviorServiceId, now)) {
    ESP_LOGW(Tag, "BehaviorService iniciou em estado degradado");
  }

  if (!routine_service_.initialize(RoutineServiceId, now)) {
    ESP_LOGW(Tag, "RoutineService iniciou em estado degradado");
  }

  if (!emotion_service_.initialize(EmotionServiceId, now)) {
    ESP_LOGW(Tag, "EmotionService iniciou em estado degradado");
  }

  if (!voice_service_.initialize(VoiceServiceId, now)) {
    ESP_LOGW(Tag, "VoiceService iniciou em estado degradado");
  }

  touch_service_.bind_port(ncos::drivers::touch::acquire_shared_touch_port());
  if (!touch_service_.initialize(now)) {
    ESP_LOGW(Tag, "TouchService iniciou em estado degradado");
  }

  imu_service_.bind_port(ncos::drivers::imu::acquire_shared_imu_port());
  if (!imu_service_.initialize(now)) {
    ESP_LOGW(Tag, "ImuService iniciou em estado degradado");
  }

  camera_service_.bind_port(ncos::drivers::camera::acquire_shared_camera_port());
  if (!camera_service_.initialize(now)) {
    ESP_LOGW(Tag, "CameraService iniciou em estado degradado");
  }

  if (!perception_service_.initialize(PerceptionServiceId, now)) {
    ESP_LOGW(Tag, "PerceptionService iniciou em estado degradado");
  }

  power_service_.bind_port(ncos::drivers::power::acquire_shared_power_port());
  if (!power_service_.initialize(PowerServiceId, now)) {
    ESP_LOGW(Tag, "PowerService iniciou em estado degradado");
  }

  motion_service_.bind_port(ncos::drivers::ttlinker::acquire_shared_motion_port());
  if (!motion_service_.initialize(now)) {
    ESP_LOGW(Tag, "MotionService iniciou em estado degradado");
  }

  led_service_.bind_port(ncos::drivers::led::acquire_shared_led_port());
  if (!led_service_.initialize(now)) {
    ESP_LOGW(Tag, "LedService iniciou em estado degradado");
  }

  if (!face_service_.initialize(now)) {
    ESP_LOGW(Tag, "Pipeline grafico base nao inicializado");
  }

  const ncos::core::runtime::RuntimeReadinessReport readiness =
      ncos::core::runtime::evaluate_runtime_readiness(ncos::config::kGlobalConfig, lifecycle_,
                                                      system_manager_.status());

  ESP_LOGI(Tag,
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
  camera_service_.tick(now);

  const ncos::core::contracts::CompanionSnapshot perception_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kBehaviorService);
  ncos::core::contracts::CompanionAttentionalSignal perception_attention{};
  ncos::core::contracts::CompanionInteractionSignal perception_interaction{};
  if (perception_service_.tick(audio_service_.state(), touch_service_.state(), camera_service_.state(),
                               perception_snapshot, now, &perception_attention,
                               &perception_interaction)) {
    (void)system_manager_.ingest_attentional_signal(perception_attention, now);
    (void)system_manager_.ingest_interactional_signal(perception_interaction, now);
  }

  const ncos::core::contracts::CompanionSnapshot voice_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kVoiceService);
  ncos::core::contracts::CompanionAttentionalSignal voice_attention{};
  ncos::core::contracts::CompanionInteractionSignal voice_interaction{};
  if (voice_service_.tick(audio_service_.state(), voice_snapshot, now, &voice_attention,
                          &voice_interaction)) {
    (void)system_manager_.ingest_attentional_signal(voice_attention, now);
    (void)system_manager_.ingest_interactional_signal(voice_interaction, now);
  }

  const ncos::core::contracts::CompanionSnapshot behavior_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kBehaviorService);

  ncos::core::contracts::BehaviorProposal behavior_proposal{};
  if (behavior_service_.tick(behavior_snapshot, now, &behavior_proposal) && behavior_proposal.valid) {
    const ncos::core::contracts::GovernanceDecision behavior_decision =
        system_manager_.govern_action(behavior_proposal.proposal, now);
    behavior_service_.on_governance_decision(behavior_decision, now);
    if (decision_allows(behavior_decision)) {
      ingest_behavior_companion_signals(&system_manager_, behavior_proposal.profile, now);
    }
  }

  const ncos::core::contracts::CompanionSnapshot routine_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kBehaviorService);

  ncos::core::contracts::RoutineProposal routine_proposal{};
  if (routine_service_.tick(routine_snapshot, behavior_service_.state(), now, &routine_proposal) &&
      routine_proposal.valid) {
    const ncos::core::contracts::GovernanceDecision routine_decision =
        system_manager_.govern_action(routine_proposal.proposal, now);
    routine_service_.on_governance_decision(routine_decision, now);
  }

  const ncos::core::contracts::CompanionSnapshot emotion_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kBehaviorService);
  ncos::core::contracts::CompanionEmotionalSignal emotional_signal{};
  if (emotion_service_.tick(emotion_snapshot, behavior_service_.state(), routine_service_.state(), now,
                            &emotional_signal)) {
    (void)system_manager_.ingest_emotional_signal(emotional_signal, now);
  }

  const ncos::core::contracts::CompanionSnapshot power_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kPowerService);
  ncos::core::contracts::CompanionEnergeticSignal energetic_signal{};
  if (power_service_.tick(power_snapshot, now, &energetic_signal)) {
    (void)system_manager_.ingest_energetic_signal(energetic_signal, now);
  }

  const ncos::core::contracts::PowerRuntimeState& power_state = power_service_.state();
  if (power_state.electrical_guard_latched && !power_electrical_fault_reported_) {
    power_electrical_fault_reported_ = true;
    system_manager_.report_runtime_fault(ncos::core::runtime::FaultCode::kPowerElectricalGuardTripped,
                                         "power_electrical_guard_tripped",
                                         now,
                                         true);
  }

  if (power_state.thermal_guard_latched && !power_thermal_fault_reported_) {
    power_thermal_fault_reported_ = true;
    const bool thermal_is_critical =
        power_state.mode == ncos::core::contracts::EnergyMode::kCritical;
    system_manager_.report_runtime_fault(ncos::core::runtime::FaultCode::kPowerThermalGuardTripped,
                                         "power_thermal_guard_tripped",
                                         now,
                                         thermal_is_critical);
  }

  const ncos::core::contracts::CompanionSnapshot face_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kFaceService);
  const ncos::core::contracts::FaceMultimodalInput face_multimodal =
      ncos::core::contracts::make_face_multimodal_input(audio_service_.state(), touch_service_.state(),
                                                        imu_service_.state(), face_snapshot,
                                                        behavior_service_.state(), now);
  face_service_.tick(now, face_multimodal);

  const ncos::core::contracts::CompanionSnapshot companion_snapshot =
      system_manager_.companion_snapshot_for(ncos::core::contracts::CompanionStateReader::kMotionService);
  motion_service_.update_companion_signal(
      make_motion_companion_signal(companion_snapshot, behavior_service_.state(), now), now);
  motion_service_.update_face_signal(face_service_.motion_signal(), now);
  motion_service_.tick(now);

  led_service_.tick(now, ncos::config::kGlobalConfig.runtime.led_refresh_interval_ms);
}

const ncos::app::lifecycle::SystemLifecycle& FirmwareEntrypoint::lifecycle() const {
  return lifecycle_;
}

}  // namespace ncos::app::boot

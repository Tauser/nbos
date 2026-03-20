#include "services/motion/motion_service.hpp"

#include "core/contracts/companion_personality_contracts.hpp"
#include "hal/platform/runtime_logging.hpp"

namespace {
constexpr const char* kTag = "NCOS_MOTION_SVC";
constexpr uint64_t kFaceSignalStaleMs = 1400;
constexpr uint64_t kNeutralGuardMinIntervalMs = 650;
constexpr uint64_t kEmbodimentTickIntervalMs = 260;
constexpr int16_t kMaxStepYawPermille = 140;
constexpr int16_t kMaxStepPitchPermille = 120;
constexpr uint32_t kMaxHoldMs = 900;

int16_t abs_i16(int16_t v) {
  return v < 0 ? static_cast<int16_t>(-v) : v;
}

int16_t clamp_step_i16(int16_t last, int16_t target, int16_t max_step) {
  if (target > static_cast<int16_t>(last + max_step)) {
    return static_cast<int16_t>(last + max_step);
  }
  if (target < static_cast<int16_t>(last - max_step)) {
    return static_cast<int16_t>(last - max_step);
  }
  return target;
}

bool is_guard_allowed_command(const ncos::core::contracts::MotionCommand& command) {
  return command.intent == ncos::core::contracts::MotionIntent::kRecovery ||
         command.intent == ncos::core::contracts::MotionIntent::kNeutralHold ||
         command.priority == ncos::core::contracts::MotionPriority::kCritical;
}

uint64_t session_context_age_ms(const ncos::core::contracts::MotionCompanionSignal& signal, uint64_t now_ms) {
  if (signal.session_last_activity_ms == 0 || now_ms < signal.session_last_activity_ms) {
    return UINT64_MAX;
  }

  return now_ms - signal.session_last_activity_ms;
}

bool has_fresh_warm_continuity(const ncos::core::contracts::MotionCompanionSignal& signal,
                               uint64_t now_ms, uint64_t window_ms) {
  return signal.session_warm && session_context_age_ms(signal, now_ms) <= window_ms;
}

}  // namespace

namespace ncos::services::motion {

void MotionService::bind_port(ncos::interfaces::motion::MotionPort* port) {
  port_ = port;
}

bool MotionService::initialize(uint64_t now_ms) {
  state_ = ncos::core::contracts::make_motion_runtime_baseline();
  state_.last_update_ms = now_ms;
  state_.last_companion_signal_ms = now_ms;
  state_.last_face_signal_ms = now_ms;

  if (port_ == nullptr) {
    NCOS_LOGW(kTag, "Porta de motion nao conectada");
    return false;
  }

  state_.transport_conflict = port_->has_transport_conflict();
  if (state_.transport_conflict) {
    NCOS_LOGW(kTag, "Motion bloqueado por conflito de transporte");
    return false;
  }

  state_.initialized = port_->ensure_ready();
  if (!state_.initialized) {
    return false;
  }

  next_embodiment_ms_ = now_ms;
  return apply_neutral_pose(now_ms);
}

bool MotionService::apply_neutral_pose(uint64_t now_ms) {
  return request_motion(ncos::core::contracts::make_neutral_hold_command(), now_ms);
}

bool MotionService::recover_to_neutral(uint64_t now_ms) {
  return request_motion(ncos::core::contracts::make_recovery_command(), now_ms);
}

bool MotionService::apply_pose(const ncos::core::contracts::MotionPoseCommand& pose, uint64_t now_ms) {
  ncos::core::contracts::MotionCommand command{};
  command.intent = ncos::core::contracts::MotionIntent::kDirectPose;
  command.origin = ncos::core::contracts::MotionCommandOrigin::kRuntime;
  command.priority = ncos::core::contracts::MotionPriority::kNormal;
  command.pose = pose;
  command.hold_ms = 120;
  return request_motion(command, now_ms);
}

bool MotionService::request_motion(const ncos::core::contracts::MotionCommand& command,
                                   uint64_t now_ms) {
  if (!state_.initialized || port_ == nullptr || !ncos::core::contracts::is_motion_command_valid(command) ||
      !ncos::core::contracts::are_safety_limits_valid(state_.safety_limits)) {
    state_.last_apply_ok = false;
    ++state_.apply_failure_total;
    ++state_.plan_rejected_total;
    state_.last_update_ms = now_ms;
    return false;
  }

  const ncos::core::contracts::MotionCommand safe_command = sanitize_command(command, now_ms);

  if (state_.fail_safe_guard_active && !is_guard_allowed_command(safe_command)) {
    state_.rejected_for_priority = true;
    ++state_.plan_rejected_total;
    ++state_.guard_interventions_total;
    state_.last_update_ms = now_ms;
    return false;
  }

  if (state_.has_active_plan &&
      !ncos::core::contracts::should_preempt_plan(state_.active_plan, safe_command, now_ms)) {
    state_.rejected_for_priority = true;
    ++state_.plan_rejected_total;
    state_.last_update_ms = now_ms;
    return false;
  }

  state_.rejected_for_priority = false;
  const ncos::core::contracts::MotionExecutionPlan plan =
      ncos::core::contracts::make_execution_plan(safe_command, now_ms);
  return apply_plan(plan, now_ms);
}

void MotionService::update_companion_signal(const ncos::core::contracts::MotionCompanionSignal& signal,
                                            uint64_t now_ms) {
  state_.companion_signal = signal;
  state_.last_companion_signal_ms = now_ms;
  state_.last_update_ms = now_ms;
}

void MotionService::update_face_signal(const ncos::core::contracts::MotionFaceSignal& signal,
                                       uint64_t now_ms) {
  state_.face_signal = signal;
  state_.last_face_signal_ms = now_ms;
  state_.last_update_ms = now_ms;
}

void MotionService::tick(uint64_t now_ms) {
  if (state_.has_active_plan && now_ms >= state_.active_plan.hold_until_ms) {
    state_.has_active_plan = false;
  }

  if (!state_.initialized || now_ms < next_embodiment_ms_) {
    return;
  }

  if (state_.companion_signal.safe_mode || state_.fail_safe_guard_active) {
    const bool recovery = state_.fail_safe_guard_active;
    (void)enforce_neutral_guard(now_ms, recovery);
    next_embodiment_ms_ = now_ms + kNeutralGuardMinIntervalMs;
    return;
  }

  const bool face_signal_stale =
      state_.last_face_signal_ms > 0 && (now_ms - state_.last_face_signal_ms) > kFaceSignalStaleMs;
  if (face_signal_stale) {
    state_.stale_face_guard_active = true;
    ++state_.guard_interventions_total;
    (void)enforce_neutral_guard(now_ms, false);
    next_embodiment_ms_ = now_ms + kNeutralGuardMinIntervalMs;
    return;
  }
  state_.stale_face_guard_active = false;

  const bool gaze_has_shift = abs_i16(state_.face_signal.gaze_x_percent) >= 12 ||
                              abs_i16(state_.face_signal.gaze_y_percent) >= 10;
  const bool active_face_expression = gaze_has_shift || state_.face_signal.clip_active;
  const auto product_state = state_.companion_signal.product_state;
  const bool restful_state = product_state == ncos::core::contracts::CompanionProductState::kSleep ||
                             product_state == ncos::core::contracts::CompanionProductState::kEnergyProtect;
  const bool attentive_state = product_state == ncos::core::contracts::CompanionProductState::kAttendUser ||
                               product_state == ncos::core::contracts::CompanionProductState::kResponding;
  const bool alert_state = product_state == ncos::core::contracts::CompanionProductState::kAlertScan;
  const bool warm_user_context =
      !restful_state &&
      has_fresh_warm_continuity(
          state_.companion_signal, now_ms,
          ncos::core::contracts::personality_continuity_window_ms(
              state_.companion_signal.personality,
              ncos::core::contracts::PersonalityContinuityKind::kUser)) &&
      (state_.companion_signal.recent_stimulus_target == ncos::core::contracts::AttentionTarget::kUser ||
       state_.companion_signal.recent_interaction_phase == ncos::core::contracts::InteractionPhase::kResponding ||
       state_.companion_signal.recent_turn_owner != ncos::core::contracts::TurnOwner::kNone) &&
      state_.companion_signal.recent_engagement_percent >=
          ncos::core::contracts::personality_continuity_engagement_threshold_percent(
              state_.companion_signal.personality,
              ncos::core::contracts::PersonalityContinuityKind::kUser);
  const bool warm_stimulus_context =
      !restful_state &&
      has_fresh_warm_continuity(
          state_.companion_signal, now_ms,
          ncos::core::contracts::personality_continuity_window_ms(
              state_.companion_signal.personality,
              ncos::core::contracts::PersonalityContinuityKind::kStimulus)) &&
      state_.companion_signal.recent_stimulus_target == ncos::core::contracts::AttentionTarget::kStimulus &&
      state_.companion_signal.recent_engagement_percent >=
          ncos::core::contracts::personality_continuity_engagement_threshold_percent(
              state_.companion_signal.personality,
              ncos::core::contracts::PersonalityContinuityKind::kStimulus);
  const bool historical_user_expression =
      product_state == ncos::core::contracts::CompanionProductState::kIdleObserve && !restful_state &&
      !warm_user_context && !warm_stimulus_context &&
      ncos::core::contracts::personality_historical_user_affinity(state_.companion_signal.personality);
  const bool historical_stimulus_expression =
      product_state == ncos::core::contracts::CompanionProductState::kIdleObserve && !restful_state &&
      !warm_user_context && !warm_stimulus_context && !historical_user_expression &&
      ncos::core::contracts::personality_historical_stimulus_affinity(state_.companion_signal.personality);
  const bool attentive_hold_requested = !active_face_expression &&
                                        (state_.companion_signal.attention_lock || attentive_state || alert_state ||
                                         warm_user_context || warm_stimulus_context);

  if (!active_face_expression && restful_state) {
    const auto rest_profile =
        ncos::core::contracts::personality_motion_profile(
            state_.companion_signal.personality, ncos::core::contracts::PersonalityMotionMode::kRest);

    ncos::core::contracts::MotionCommand rest{};
    rest.intent = ncos::core::contracts::MotionIntent::kPowerSave;
    rest.origin = ncos::core::contracts::MotionCommandOrigin::kCompanionState;
    rest.priority = ncos::core::contracts::MotionPriority::kLow;
    rest.pose = ncos::core::contracts::make_neutral_pose();
    rest.pose.pitch_permille = rest_profile.pitch_permille;
    rest.pose.speed_percent =
        static_cast<uint16_t>(rest_profile.base_speed_percent + state_.companion_signal.emotional_arousal_percent / 12);
    rest.hold_ms = rest_profile.hold_ms;
    (void)request_motion(rest, now_ms);
    next_embodiment_ms_ = now_ms + rest_profile.hold_ms;
    return;
  }

  if (attentive_hold_requested) {
    const bool warm_only = !state_.companion_signal.attention_lock && !attentive_state && !alert_state &&
                           (warm_user_context || warm_stimulus_context);
    const auto profile = ncos::core::contracts::personality_motion_profile(
        state_.companion_signal.personality,
        product_state == ncos::core::contracts::CompanionProductState::kResponding
            ? ncos::core::contracts::PersonalityMotionMode::kResponding
            : (alert_state
                   ? ncos::core::contracts::PersonalityMotionMode::kAlertScan
                   : (warm_stimulus_context
                          ? ncos::core::contracts::PersonalityMotionMode::kWarmStimulus
                          : (warm_user_context ? ncos::core::contracts::PersonalityMotionMode::kWarmUser
                                               : ncos::core::contracts::PersonalityMotionMode::kAttendUser))));

    ncos::core::contracts::MotionCommand attend{};
    attend.intent = (alert_state || warm_stimulus_context)
                        ? ncos::core::contracts::MotionIntent::kObserveStimulus
                        : ncos::core::contracts::MotionIntent::kAttendUser;
    attend.origin = ncos::core::contracts::MotionCommandOrigin::kCompanionState;
    attend.priority = ncos::core::contracts::MotionPriority::kLow;
    attend.pose = ncos::core::contracts::make_neutral_pose();
    attend.pose.pitch_permille = profile.pitch_permille;
    attend.pose.speed_percent = warm_only
                                    ? static_cast<uint16_t>(profile.base_speed_percent +
                                                            state_.companion_signal.recent_engagement_percent / 8)
                                    : static_cast<uint16_t>(profile.base_speed_percent +
                                                            state_.companion_signal.emotional_arousal_percent / 6);
    attend.hold_ms = profile.hold_ms;
    (void)request_motion(attend, now_ms);
    next_embodiment_ms_ = now_ms + kEmbodimentTickIntervalMs;
    return;
  }

  if (!active_face_expression) {
    if (historical_user_expression || historical_stimulus_expression) {
      const auto profile = ncos::core::contracts::personality_motion_profile(
          state_.companion_signal.personality,
          historical_stimulus_expression ? ncos::core::contracts::PersonalityMotionMode::kWarmStimulus
                                         : ncos::core::contracts::PersonalityMotionMode::kWarmUser);
      const uint8_t boost = historical_stimulus_expression
                                ? ncos::core::contracts::personality_historical_stimulus_expression_boost_percent(
                                      state_.companion_signal.personality)
                                : ncos::core::contracts::personality_historical_user_expression_boost_percent(
                                      state_.companion_signal.personality);

      ncos::core::contracts::MotionCommand historical{};
      historical.intent = historical_stimulus_expression
                              ? ncos::core::contracts::MotionIntent::kObserveStimulus
                              : ncos::core::contracts::MotionIntent::kAttendUser;
      historical.origin = ncos::core::contracts::MotionCommandOrigin::kCompanionState;
      historical.priority = ncos::core::contracts::MotionPriority::kLow;
      historical.pose = ncos::core::contracts::make_neutral_pose();
      historical.pose.pitch_permille = static_cast<int16_t>(profile.pitch_permille + boost / 2);
      historical.pose.speed_percent = static_cast<uint16_t>(profile.base_speed_percent + boost / 3);
      historical.hold_ms = static_cast<uint16_t>(profile.hold_ms + boost * 5);
      (void)request_motion(historical, now_ms);
      next_embodiment_ms_ = now_ms + 240;
      return;
    }

    if ((state_.last_pose.yaw_permille != 0 || state_.last_pose.pitch_permille != 0) &&
        !state_.companion_signal.attention_lock) {
      (void)enforce_neutral_guard(now_ms, false);
    }
    next_embodiment_ms_ = now_ms + 220;
    return;
  }

  ncos::core::contracts::MotionCommand follow{};
  follow.intent = state_.face_signal.clip_active ? ncos::core::contracts::MotionIntent::kAttendUser
                                                 : (state_.companion_signal.attention_lock
                                                        ? ncos::core::contracts::MotionIntent::kAttendUser
                                                        : ncos::core::contracts::MotionIntent::kObserveStimulus);
  follow.origin = ncos::core::contracts::MotionCommandOrigin::kFace;
  follow.priority = ncos::core::contracts::MotionPriority::kLow;
  follow.pose.yaw_permille = static_cast<int16_t>(state_.face_signal.gaze_x_percent * 6);
  follow.pose.pitch_permille = static_cast<int16_t>(state_.face_signal.gaze_y_percent * 5);
  follow.pose.speed_percent = static_cast<uint16_t>(28 + state_.companion_signal.emotional_arousal_percent / 4);
  follow.hold_ms = 140;

  (void)request_motion(follow, now_ms);
  next_embodiment_ms_ = now_ms + kEmbodimentTickIntervalMs;
}

const ncos::core::contracts::MotionRuntimeState& MotionService::state() const {
  return state_;
}

ncos::core::contracts::MotionCommand MotionService::sanitize_command(
    const ncos::core::contracts::MotionCommand& command, uint64_t now_ms) {
  ncos::core::contracts::MotionCommand safe = command;

  if (safe.hold_ms > kMaxHoldMs) {
    safe.hold_ms = kMaxHoldMs;
    ++state_.guard_interventions_total;
  }

  if (state_.companion_signal.safe_mode && safe.priority != ncos::core::contracts::MotionPriority::kCritical) {
    safe.pose.speed_percent = safe.pose.speed_percent > 45 ? 45 : safe.pose.speed_percent;
    safe.hold_ms = safe.hold_ms > 180 ? 180 : safe.hold_ms;
  }

  if (safe.intent == ncos::core::contracts::MotionIntent::kRecovery) {
    safe.pose = ncos::core::contracts::make_neutral_pose();
    safe.pose.speed_percent = state_.safety_limits.recovery_speed_percent;
  }

  bool slew_guard_applied = false;
  if (state_.apply_success_total > 0 && safe.priority != ncos::core::contracts::MotionPriority::kCritical &&
      safe.intent != ncos::core::contracts::MotionIntent::kRecovery) {
    const int16_t guarded_yaw =
        clamp_step_i16(state_.last_pose.yaw_permille, safe.pose.yaw_permille, kMaxStepYawPermille);
    const int16_t guarded_pitch =
        clamp_step_i16(state_.last_pose.pitch_permille, safe.pose.pitch_permille, kMaxStepPitchPermille);
    slew_guard_applied = guarded_yaw != safe.pose.yaw_permille || guarded_pitch != safe.pose.pitch_permille;
    safe.pose.yaw_permille = guarded_yaw;
    safe.pose.pitch_permille = guarded_pitch;
  }

  ncos::core::contracts::MotionSafetyReport report{};
  safe.pose = ncos::core::contracts::clamp_pose_to_safety(safe.pose, state_.safety_limits, &report);

  state_.safety_clamp_applied =
      report.clamped_yaw || report.clamped_pitch || report.clamped_speed || slew_guard_applied;
  if (state_.safety_clamp_applied) {
    ++state_.safety_clamp_total;
  }

  if (slew_guard_applied) {
    ++state_.guard_interventions_total;
  }

  state_.last_update_ms = now_ms;
  return safe;
}

bool MotionService::apply_plan(const ncos::core::contracts::MotionExecutionPlan& plan, uint64_t now_ms) {
  size_t tx_bytes = 0;
  const bool ok = port_->apply_pose(plan.target_pose, &tx_bytes);

  state_.last_apply_ok = ok;
  state_.last_tx_bytes = tx_bytes;
  state_.last_pose = plan.target_pose;
  state_.last_update_ms = now_ms;

  if (!ok) {
    ++state_.apply_failure_total;
    ++state_.plan_rejected_total;
    ++state_.consecutive_apply_failures;
    if (!state_.fail_safe_guard_active && state_.consecutive_apply_failures >= 3) {
      state_.fail_safe_guard_active = true;
      ++state_.guard_interventions_total;
    }
    return false;
  }

  state_.consecutive_apply_failures = 0;
  state_.active_plan = plan;
  state_.has_active_plan = true;
  state_.neutral_applied =
      (plan.target_pose.yaw_permille == 0 && plan.target_pose.pitch_permille == 0);
  if (state_.neutral_applied) {
    state_.last_neutral_command_ms = now_ms;
    state_.fail_safe_guard_active = false;
  }
  ++state_.apply_success_total;
  ++state_.plan_applied_total;
  return true;
}

bool MotionService::enforce_neutral_guard(uint64_t now_ms, bool recovery_command) {
  if (state_.last_neutral_command_ms > 0 && (now_ms - state_.last_neutral_command_ms) < kNeutralGuardMinIntervalMs) {
    return false;
  }

  return recovery_command ? recover_to_neutral(now_ms) : apply_neutral_pose(now_ms);
}

}  // namespace ncos::services::motion

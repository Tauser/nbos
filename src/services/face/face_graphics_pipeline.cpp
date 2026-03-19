#include "services/face/face_graphics_pipeline.hpp"

#include "config/system_config.hpp"
#include "core/contracts/companion_personality_contracts.hpp"
#include "drivers/display/display_runtime.hpp"
#include "hal/platform/monotonic_clock.hpp"

namespace {

using ncos::core::contracts::AttentionTarget;
using ncos::core::contracts::CompanionProductState;
using ncos::core::contracts::InteractionPhase;
using ncos::core::contracts::TurnOwner;

uint64_t session_context_age_ms(const ncos::core::contracts::FaceMultimodalInput& input) {
  if (input.session_last_activity_ms == 0 || input.observed_at_ms < input.session_last_activity_ms) {
    return UINT64_MAX;
  }

  return input.observed_at_ms - input.session_last_activity_ms;
}

bool has_warm_continuity(const ncos::core::contracts::FaceMultimodalInput& input, uint64_t window_ms) {
  if (!input.session_warm || input.companion_product_state != CompanionProductState::kIdleObserve ||
      input.recent_engagement_percent < 48) {
    return false;
  }

  return session_context_age_ms(input) <= window_ms;
}

bool has_warm_user_continuity(const ncos::core::contracts::FaceMultimodalInput& input) {
  return has_warm_continuity(
           input,
           ncos::core::contracts::personality_continuity_window_ms(
               input.personality, ncos::core::contracts::PersonalityContinuityKind::kUser)) &&
         (input.recent_stimulus_target == AttentionTarget::kUser ||
          input.recent_interaction_phase == InteractionPhase::kResponding ||
          input.recent_turn_owner != TurnOwner::kNone) &&
         input.recent_engagement_percent >= 58;
}

bool has_warm_stimulus_continuity(const ncos::core::contracts::FaceMultimodalInput& input) {
  return has_warm_continuity(
           input,
           ncos::core::contracts::personality_continuity_window_ms(
               input.personality, ncos::core::contracts::PersonalityContinuityKind::kStimulus)) &&
         input.recent_stimulus_target == AttentionTarget::kStimulus;
}

bool has_warm_continuity(const ncos::core::contracts::FaceMultimodalInput& input) {
  return has_warm_user_continuity(input) || has_warm_stimulus_continuity(input);
}

bool is_diagonal_direction(ncos::models::face::GazeDirection direction) {
  using ncos::models::face::GazeDirection;
  return direction == GazeDirection::kUpLeft || direction == GazeDirection::kUpRight ||
         direction == GazeDirection::kDownLeft || direction == GazeDirection::kDownRight;
}

constexpr ncos::models::face::FaceClipKeyframe SignatureClipFrames[] = {
    {0, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kCenter, 54, 96}},
    {180, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kUpRight, 74, 92}},
    {360, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kUpLeft, 70, 88}},
    {540, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kCenter, 50, 96}},
};

constexpr ncos::models::face::FaceClip SignatureClip = {
    1001,
    "signature_glance_arc",
    SignatureClipFrames,
    sizeof(SignatureClipFrames) / sizeof(SignatureClipFrames[0]),
    620,
    220,
    8,
    760,
    280,
};

struct FaceAutonomyGazeProfile {
  ncos::models::face::GazeDirection direction = ncos::models::face::GazeDirection::kCenter;
  uint8_t focus_percent = 48;
  uint8_t salience_percent = 30;
  uint16_t hold_ms = 420;
  uint16_t cadence_ms = 700;
  bool alternate_lateral = false;
};

ncos::services::face::FaceOfficialPresetId select_official_preset_for_input(
    const ncos::core::contracts::FaceMultimodalInput& input) {
  switch (input.companion_product_state) {
    case CompanionProductState::kResponding:
      return ncos::services::face::FaceOfficialPresetId::kCoreLock;
    case CompanionProductState::kAttendUser:
      return ncos::services::face::FaceOfficialPresetId::kCoreAttend;
    case CompanionProductState::kAlertScan:
      return ncos::services::face::FaceOfficialPresetId::kCoreCurious;
    case CompanionProductState::kSleep:
    case CompanionProductState::kEnergyProtect:
      return ncos::services::face::FaceOfficialPresetId::kCoreCalm;
    case CompanionProductState::kBooting:
    case CompanionProductState::kIdleObserve:
    default:
      break;
  }

  if (input.behavior_activation_percent >= 75 || input.emotional_arousal_percent >= 80) {
    return ncos::services::face::FaceOfficialPresetId::kCoreLock;
  }

  if (has_warm_user_continuity(input)) {
    return ncos::services::face::FaceOfficialPresetId::kCoreAttend;
  }

  if (has_warm_stimulus_continuity(input)) {
    return ncos::services::face::FaceOfficialPresetId::kCoreCurious;
  }

  if (input.behavior_activation_percent >= 55 || input.social_engagement_percent >= 70) {
    return ncos::services::face::FaceOfficialPresetId::kCoreAttend;
  }

  if (input.emotional_arousal_percent >= 58) {
    return ncos::services::face::FaceOfficialPresetId::kCoreCurious;
  }

  if (input.social_engagement_percent <= 30) {
    return ncos::services::face::FaceOfficialPresetId::kCoreCalm;
  }

  return ncos::services::face::FaceOfficialPresetId::kCoreNeutral;
}

bool is_attending_user_input(const ncos::core::contracts::FaceMultimodalInput& input) {
  if (input.companion_product_state == CompanionProductState::kSleep ||
      input.companion_product_state == CompanionProductState::kEnergyProtect) {
    return false;
  }

  return input.touch_active || input.behavior_active || input.social_engagement_percent >= 70 ||
         has_warm_user_continuity(input) ||
         input.companion_product_state == CompanionProductState::kAttendUser ||
         input.companion_product_state == CompanionProductState::kResponding;
}

bool should_run_idle_signature_clip(const ncos::core::contracts::FaceMultimodalInput& input) {
  return input.companion_product_state == CompanionProductState::kIdleObserve && !has_warm_continuity(input);
}

FaceAutonomyGazeProfile select_autonomy_gaze_profile(const ncos::core::contracts::FaceMultimodalInput& input,
                                                     bool gaze_left) {
  using ncos::core::contracts::PersonalityFaceMode;
  using ncos::core::contracts::personality_face_profile;

  PersonalityFaceMode mode = PersonalityFaceMode::kIdleObserve;
  switch (input.companion_product_state) {
    case CompanionProductState::kResponding:
      mode = PersonalityFaceMode::kResponding;
      break;
    case CompanionProductState::kAttendUser:
      mode = PersonalityFaceMode::kAttendUser;
      break;
    case CompanionProductState::kAlertScan:
      mode = PersonalityFaceMode::kAlertScan;
      break;
    case CompanionProductState::kSleep:
      mode = PersonalityFaceMode::kSleep;
      break;
    case CompanionProductState::kEnergyProtect:
      mode = PersonalityFaceMode::kEnergyProtect;
      break;
    case CompanionProductState::kBooting:
      mode = PersonalityFaceMode::kBooting;
      break;
    case CompanionProductState::kIdleObserve:
    default:
      if (has_warm_user_continuity(input)) {
        mode = PersonalityFaceMode::kWarmUser;
      } else if (has_warm_stimulus_continuity(input)) {
        mode = PersonalityFaceMode::kWarmStimulus;
      }
      break;
  }

  const auto envelope = personality_face_profile(input.personality, mode);
  FaceAutonomyGazeProfile profile{};
  profile.focus_percent = envelope.focus_percent;
  profile.salience_percent = envelope.salience_percent;
  profile.hold_ms = envelope.hold_ms;
  profile.cadence_ms = envelope.cadence_ms;
  profile.alternate_lateral = envelope.alternate_lateral;

  switch (mode) {
    case PersonalityFaceMode::kResponding:
    case PersonalityFaceMode::kAttendUser:
    case PersonalityFaceMode::kSleep:
    case PersonalityFaceMode::kEnergyProtect:
    case PersonalityFaceMode::kBooting:
    case PersonalityFaceMode::kWarmUser:
      profile.direction = ncos::models::face::GazeDirection::kCenter;
      break;
    case PersonalityFaceMode::kAlertScan:
    case PersonalityFaceMode::kWarmStimulus:
    case PersonalityFaceMode::kIdleObserve:
    default:
      profile.direction = gaze_left ? ncos::models::face::GazeDirection::kLeft
                                    : ncos::models::face::GazeDirection::kRight;
      break;
  }

  if (mode == PersonalityFaceMode::kWarmUser &&
      input.recent_interaction_phase == InteractionPhase::kResponding) {
    profile.focus_percent = static_cast<uint8_t>(profile.focus_percent + 4);
    profile.salience_percent = static_cast<uint8_t>(profile.salience_percent + 4);
    profile.hold_ms = static_cast<uint16_t>(profile.hold_ms + 32);
  }

  return profile;
}

ncos::core::contracts::MotionFaceSignal face_direction_to_motion_signal(
    ncos::models::face::GazeDirection direction, bool clip_active) {
  ncos::core::contracts::MotionFaceSignal signal{};
  signal.clip_active = clip_active;

  switch (direction) {
    case ncos::models::face::GazeDirection::kLeft:
      signal.gaze_x_percent = -55;
      break;
    case ncos::models::face::GazeDirection::kRight:
      signal.gaze_x_percent = 55;
      break;
    case ncos::models::face::GazeDirection::kUp:
      signal.gaze_y_percent = 45;
      break;
    case ncos::models::face::GazeDirection::kDown:
      signal.gaze_y_percent = -45;
      break;
    case ncos::models::face::GazeDirection::kUpLeft:
      signal.gaze_x_percent = -45;
      signal.gaze_y_percent = 35;
      break;
    case ncos::models::face::GazeDirection::kUpRight:
      signal.gaze_x_percent = 45;
      signal.gaze_y_percent = 35;
      break;
    case ncos::models::face::GazeDirection::kDownLeft:
      signal.gaze_x_percent = -45;
      signal.gaze_y_percent = -35;
      break;
    case ncos::models::face::GazeDirection::kDownRight:
      signal.gaze_x_percent = 45;
      signal.gaze_y_percent = -35;
      break;
    case ncos::models::face::GazeDirection::kCenter:
    default:
      break;
  }

  return signal;
}

}  // namespace

namespace ncos::services::face {


void FaceGraphicsPipeline::schedule_next_render(uint64_t now_ms) {
  if (next_render_ms_ == 0) {
    next_render_ms_ = now_ms + RenderPeriodMs;
    return;
  }

  const uint64_t next_slot_ms = next_render_ms_ + RenderPeriodMs;
  next_render_ms_ = next_slot_ms > now_ms ? next_slot_ms : now_ms + RenderPeriodMs;
}

bool FaceGraphicsPipeline::initialize(uint64_t now_ms) {
  if (initialized_) {
    return true;
  }

  auto* display = ncos::drivers::display::acquire_shared_display();
  if (display == nullptr || !renderer_.bind(display)) {
    return false;
  }

  diagnostics_mode_ = ncos::config::kGlobalConfig.runtime.display_diagnostics_mode;
  (void)diagnostics_runner_.bind(display, &renderer_);
  diagnostics_runner_.set_mode(diagnostics_mode_);

  state_ = ncos::core::contracts::make_face_render_state_baseline();
  state_.safety_mode = ncos::core::contracts::FaceRenderSafetyMode::kNominal;
  motion_safety_result_ = FaceMotionSafetyResult{};
  reset_face_motion_safety_status(&motion_safety_status_, state_, now_ms);
  fallback_status_ = FaceVisualFallbackStatus{};
  tuning_ = FaceTuningTelemetry{};
  tuning_.frame_budget_us = ncos::config::kGlobalConfig.runtime.face_frame_budget_us;
  preview_snapshot_ = make_face_preview_snapshot(state_, false, now_ms, tuning_);
  motion_signal_ = ncos::core::contracts::MotionFaceSignal{};

  if (diagnostics_mode_ != ncos::config::DisplayDiagnosticsMode::kOff &&
      diagnostics_mode_ != ncos::config::DisplayDiagnosticsMode::kFaceVisualDebug) {
    next_render_ms_ = now_ms;
    initialized_ = true;
    return true;
  }

  if (!compositor_.bind_state(&state_)) {
    return false;
  }

  FaceLayerRequest base_request{};
  base_request.layer = ncos::models::face::FaceLayer::kBase;
  base_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
  base_request.requester_service = PresetOwnerServiceId;
  base_request.priority = 3;

  if (!compositor_.request_layer(base_request, now_ms).granted) {
    return false;
  }

  if (!apply_official_face_preset(official_preset_, &state_, now_ms)) {
    return false;
  }

  FaceLayerRequest gaze_request{};
  gaze_request.layer = ncos::models::face::FaceLayer::kGaze;
  gaze_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  gaze_request.requester_service = GazeOwnerServiceId;
  gaze_request.priority = 5;

  if (!compositor_.request_layer(gaze_request, now_ms).granted) {
    return false;
  }

  FaceLayerRequest modulation_request{};
  modulation_request.layer = ncos::models::face::FaceLayer::kModulation;
  modulation_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kModulationOwner;
  modulation_request.requester_service = ModulationOwnerServiceId;
  modulation_request.priority = 4;

  if (!compositor_.request_layer(modulation_request, now_ms).granted) {
    return false;
  }

  preview_snapshot_ = make_face_preview_snapshot(state_, false, now_ms, tuning_);
  motion_signal_ = face_direction_to_motion_signal(state_.eyes.direction, false);
  next_render_ms_ = now_ms;
  next_gaze_target_ms_ = now_ms;
  next_clip_start_ms_ = now_ms + 3800;
  initialized_ = true;
  return true;
}

void FaceGraphicsPipeline::tick(uint64_t now_ms,
                                const ncos::core::contracts::FaceMultimodalInput& multimodal) {
  if (!initialized_ || now_ms < next_render_ms_) {
    return;
  }

  if (diagnostics_mode_ != ncos::config::DisplayDiagnosticsMode::kOff &&
      diagnostics_mode_ != ncos::config::DisplayDiagnosticsMode::kFaceVisualDebug) {
    diagnostics_runner_.tick(now_ms);
    schedule_next_render(now_ms);
    return;
  }

  tuning_.frame_budget_us = ncos::config::kGlobalConfig.runtime.face_frame_budget_us;
  tuning_.degradation = FaceVisualDegradationFlag::kNone;
  const uint64_t total_start_us = ncos::hal::platform::monotonic_time_us();

  const uint64_t compositor_start_us = ncos::hal::platform::monotonic_time_us();
  compositor_.tick(now_ms);
  tuning_.stages.compositor_us = static_cast<uint32_t>(ncos::hal::platform::monotonic_time_us() - compositor_start_us);

  const FaceOfficialPresetId target_preset = select_official_preset_for_input(multimodal);
  const bool attending_user = is_attending_user_input(multimodal);
  if (target_preset != official_preset_ && !clip_player_.active()) {
    FaceLayerRequest base_request{};
    base_request.layer = ncos::models::face::FaceLayer::kBase;
    base_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
    base_request.requester_service = PresetOwnerServiceId;
    base_request.priority = 3;
    base_request.hold_ms = 260;
    base_request.cooldown_ms = 120;

    if (compositor_.request_layer(base_request, now_ms).granted &&
        compositor_.can_write(ncos::models::face::FaceLayer::kBase, PresetOwnerServiceId) &&
        apply_official_face_preset(target_preset, &state_, now_ms)) {
      official_preset_ = target_preset;
    }
  }

  if (!fallback_status_.active && !clip_player_.active() && should_run_idle_signature_clip(multimodal) &&
      !attending_user && now_ms >= next_clip_start_ms_) {
    (void)clip_player_.play(SignatureClip, &compositor_, &state_, now_ms);
    next_clip_start_ms_ = now_ms + 6200;
  }

  const uint64_t clip_start_us = ncos::hal::platform::monotonic_time_us();
  const bool clip_updated = clip_player_.tick(now_ms, &compositor_, &state_);
  tuning_.stages.clip_us = static_cast<uint32_t>(ncos::hal::platform::monotonic_time_us() - clip_start_us);

  const uint64_t gaze_start_us = ncos::hal::platform::monotonic_time_us();
  if (!clip_player_.active()) {
    if (now_ms >= next_gaze_target_ms_) {
      FaceLayerRequest gaze_request{};
      gaze_request.layer = ncos::models::face::FaceLayer::kGaze;
      gaze_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
      gaze_request.requester_service = GazeOwnerServiceId;
      gaze_request.priority = 5;

      if (compositor_.request_layer(gaze_request, now_ms).granted) {
        const FaceAutonomyGazeProfile profile = select_autonomy_gaze_profile(multimodal, gaze_left_);

        ncos::models::face::FaceGazeTarget target{};
        target.anchor = ncos::models::face::GazeAnchor::kUser;
        target.direction = profile.direction;
        target.focus_percent = profile.focus_percent;
        target.salience_percent = profile.salience_percent;
        target.hold_ms = profile.hold_ms;
        target.origin = ncos::models::face::FaceGazeTargetOrigin::kSystem;

        (void)gaze_controller_.set_target(target, now_ms);
        if (profile.alternate_lateral) {
          gaze_left_ = !gaze_left_;
        }

        next_gaze_target_ms_ = now_ms + profile.cadence_ms;
      } else {
        next_gaze_target_ms_ = now_ms + (attending_user ? 420 : 700);
      }
    }

    (void)gaze_controller_.tick(now_ms, &state_);
  } else if (!clip_updated) {
    (void)clip_player_.tick(now_ms, &compositor_, &state_);
  }
  tuning_.stages.gaze_us = static_cast<uint32_t>(ncos::hal::platform::monotonic_time_us() - gaze_start_us);

  const uint64_t modulation_start_us = ncos::hal::platform::monotonic_time_us();
  (void)multimodal_sync_.apply(multimodal, &compositor_, &state_, ModulationOwnerServiceId, now_ms);
  tuning_.stages.modulation_us = static_cast<uint32_t>(ncos::hal::platform::monotonic_time_us() - modulation_start_us);

  motion_safety_result_ = FaceMotionSafetyResult{};
  (void)apply_face_motion_safety(&state_, &motion_safety_status_, now_ms, &motion_safety_result_);

  FaceFrame frame{};
  const uint64_t compose_start_us = ncos::hal::platform::monotonic_time_us();
  const bool composed = composer_.compose(state_, &frame);
  tuning_.stages.compose_us = static_cast<uint32_t>(ncos::hal::platform::monotonic_time_us() - compose_start_us);

  if (composed) {
    const uint64_t render_start_us = ncos::hal::platform::monotonic_time_us();
    (void)renderer_.render(frame);
    tuning_.stages.render_us = static_cast<uint32_t>(ncos::hal::platform::monotonic_time_us() - render_start_us);
  } else {
    tuning_.stages.render_us = 0;
  }

  tuning_.stages.total_us = static_cast<uint32_t>(ncos::hal::platform::monotonic_time_us() - total_start_us);

  const auto render_stats = renderer_.render_stats();
  const auto render_plan = renderer_.last_render_plan();
  tuning_.dirty_area_px = render_stats.last_dirty_area_px;
  tuning_.avg_frame_time_us = render_stats.avg_frame_time_us;
  tuning_.peak_frame_time_us = render_stats.peak_frame_time_us;
  tuning_.rendered_frames = render_stats.rendered_frames;
  tuning_.skipped_duplicate_frames = render_stats.skipped_duplicate_frames;
  tuning_.frame_skipped = render_stats.last_frame_skipped;
  tuning_.full_redraw = render_stats.last_frame_full_redraw;
  tuning_.high_contrast_motion = render_plan.high_contrast_motion;

  if (tuning_.stages.total_us > tuning_.frame_budget_us) {
    tuning_.degradation = tuning_.degradation | FaceVisualDegradationFlag::kFrameOverBudget;
  }
  if (tuning_.full_redraw) {
    tuning_.degradation = tuning_.degradation | FaceVisualDegradationFlag::kFullRedraw;
  }
  if (tuning_.high_contrast_motion) {
    tuning_.degradation = tuning_.degradation | FaceVisualDegradationFlag::kHighContrastMotion;
  }
  const auto* shared_display = ncos::drivers::display::acquire_shared_display();
  const uint32_t large_dirty_budget_px =
      shared_display != nullptr ? static_cast<uint32_t>(shared_display->width()) * 24u : 5760u;
  if (tuning_.dirty_area_px > large_dirty_budget_px) {
    tuning_.degradation = tuning_.degradation | FaceVisualDegradationFlag::kLargeDirtyRect;
  }
  if (is_diagonal_direction(state_.eyes.direction)) {
    tuning_.degradation = tuning_.degradation | FaceVisualDegradationFlag::kDiagonalMotion;
  }

  if (!fallback_status_.active && should_enter_face_visual_fallback(tuning_)) {
    if (set_face_visual_fallback_active(&fallback_status_, true, now_ms, tuning_.degradation) &&
        clip_player_.active()) {
      (void)clip_player_.cancel(now_ms, &state_);
    }
  } else if (fallback_status_.active && should_exit_face_visual_fallback(tuning_)) {
    (void)set_face_visual_fallback_active(&fallback_status_, false, now_ms, FaceVisualDegradationFlag::kNone);
  }

  if (fallback_status_.active) {
    apply_face_visual_fallback(&state_);
  } else {
    state_.safety_mode = motion_safety_result_.active
                             ? ncos::core::contracts::FaceRenderSafetyMode::kSafeFallback
                             : ncos::core::contracts::FaceRenderSafetyMode::kNominal;
  }

  tuning_.safe_visual_mode = fallback_status_.active;
  tuning_.fallback_entries = fallback_status_.entry_count;
  tuning_.fallback_exits = fallback_status_.exit_count;
  tuning_.fallback_last_change_ms = fallback_status_.last_change_ms;
  tuning_.fallback_trigger = fallback_status_.last_trigger;

  preview_snapshot_ = make_face_preview_snapshot(state_, clip_player_.active(), now_ms, tuning_);
  if (diagnostics_mode_ == ncos::config::DisplayDiagnosticsMode::kFaceVisualDebug) {
    diagnostics_runner_.render_face_visual_debug(preview_snapshot_);
  }
  motion_signal_ = face_direction_to_motion_signal(state_.eyes.direction, clip_player_.active());
  schedule_next_render(now_ms);
}

bool FaceGraphicsPipeline::initialized() const {
  return initialized_;
}

size_t FaceGraphicsPipeline::export_preview_json(char* out_buffer, size_t out_buffer_size) const {
  return export_face_preview_json(preview_snapshot_, out_buffer, out_buffer_size);
}

ncos::core::contracts::MotionFaceSignal FaceGraphicsPipeline::motion_signal() const {
  return motion_signal_;
}

FaceRenderStats FaceGraphicsPipeline::render_stats() const {
  return renderer_.render_stats();
}

}  // namespace ncos::services::face







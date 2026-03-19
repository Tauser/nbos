#include "services/face/face_gaze_controller.hpp"

namespace {

struct DirVec {
  int8_t x = 0;
  int8_t y = 0;
};

uint8_t clamp_percent(uint8_t value) {
  return value > 100 ? 100 : value;
}

uint8_t lerp_u8(uint8_t from, uint8_t to, uint64_t elapsed, uint64_t duration) {
  if (duration == 0 || elapsed >= duration) {
    return to;
  }

  const int32_t delta = static_cast<int32_t>(to) - static_cast<int32_t>(from);
  return clamp_percent(static_cast<uint8_t>(static_cast<int32_t>(from) +
                                            static_cast<int32_t>((delta * static_cast<int64_t>(elapsed)) /
                                                                 static_cast<int64_t>(duration))));
}

DirVec direction_to_vec(ncos::models::face::GazeDirection direction) {
  switch (direction) {
    case ncos::models::face::GazeDirection::kLeft:
      return {-1, 0};
    case ncos::models::face::GazeDirection::kRight:
      return {1, 0};
    case ncos::models::face::GazeDirection::kUp:
      return {0, -1};
    case ncos::models::face::GazeDirection::kDown:
      return {0, 1};
    case ncos::models::face::GazeDirection::kUpLeft:
      return {-1, -1};
    case ncos::models::face::GazeDirection::kUpRight:
      return {1, -1};
    case ncos::models::face::GazeDirection::kDownLeft:
      return {-1, 1};
    case ncos::models::face::GazeDirection::kDownRight:
      return {1, 1};
    default:
      return {0, 0};
  }
}

bool is_diagonal(ncos::models::face::GazeDirection direction) {
  const DirVec vec = direction_to_vec(direction);
  return vec.x != 0 && vec.y != 0;
}

int transition_distance(ncos::models::face::GazeDirection from,
                        ncos::models::face::GazeDirection to) {
  const DirVec a = direction_to_vec(from);
  const DirVec b = direction_to_vec(to);
  const int dx = static_cast<int>(a.x) - static_cast<int>(b.x);
  const int dy = static_cast<int>(a.y) - static_cast<int>(b.y);
  return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

uint16_t saccade_duration_ms(int distance) {
  if (distance <= 1) {
    return 60;
  }
  if (distance <= 2) {
    return 90;
  }
  return 120;
}

ncos::models::face::GazeDirection overshoot_for_target(
    ncos::models::face::GazeDirection target) {
  switch (target) {
    case ncos::models::face::GazeDirection::kRight:
      return ncos::models::face::GazeDirection::kUpRight;
    case ncos::models::face::GazeDirection::kLeft:
      return ncos::models::face::GazeDirection::kUpLeft;
    case ncos::models::face::GazeDirection::kUp:
      return ncos::models::face::GazeDirection::kUpRight;
    case ncos::models::face::GazeDirection::kDown:
      return ncos::models::face::GazeDirection::kDownRight;
    case ncos::models::face::GazeDirection::kUpRight:
      return ncos::models::face::GazeDirection::kRight;
    case ncos::models::face::GazeDirection::kUpLeft:
      return ncos::models::face::GazeDirection::kLeft;
    case ncos::models::face::GazeDirection::kDownRight:
      return ncos::models::face::GazeDirection::kRight;
    case ncos::models::face::GazeDirection::kDownLeft:
      return ncos::models::face::GazeDirection::kLeft;
    default:
      return ncos::models::face::GazeDirection::kCenter;
  }
}

uint8_t saccade_entry_focus(uint8_t target_focus_percent) {
  return clamp_percent(static_cast<uint8_t>(target_focus_percent / 6 + 4));
}

uint8_t overshoot_focus(uint8_t target_focus_percent) {
  return clamp_percent(static_cast<uint8_t>(target_focus_percent + 14));
}

}  // namespace

namespace ncos::services::face {

FaceGazeController::FaceGazeController(uint16_t owner_service) : owner_service_(owner_service) {}

void FaceGazeController::bind_owner(uint16_t owner_service) {
  owner_service_ = owner_service;
}

bool FaceGazeController::set_target(const ncos::models::face::FaceGazeTarget& target,
                                    uint64_t now_ms) {
  if (target.hold_ms == 0) {
    return false;
  }

  target_ = target;
  target_.focus_percent = clamp_percent(target.focus_percent);
  target_.salience_percent = clamp_percent(target.salience_percent);

  start_direction_ = current_direction_;
  overshoot_direction_ = overshoot_for_target(target_.direction);

  const int distance = transition_distance(start_direction_, target_.direction);
  const uint16_t saccade_ms = saccade_duration_ms(distance);
  const bool allow_overshoot = distance != 0 && !is_diagonal(target_.direction);

  saccade_start_ms_ = now_ms;
  saccade_end_ms_ = now_ms + saccade_ms;
  overshoot_end_ms_ = saccade_end_ms_ + (allow_overshoot ? 45 : 0);
  settle_end_ms_ = overshoot_end_ms_ + (allow_overshoot ? 70 : 0);
  fixation_end_ms_ = settle_end_ms_ + target_.hold_ms;

  phase_ = distance == 0 ? FaceGazeMotionPhase::kFixation : FaceGazeMotionPhase::kSaccade;
  active_target_ = true;
  return true;
}

void FaceGazeController::clear_target() {
  active_target_ = false;
  phase_ = FaceGazeMotionPhase::kIdle;
  saccade_start_ms_ = 0;
  saccade_end_ms_ = 0;
  overshoot_end_ms_ = 0;
  settle_end_ms_ = 0;
  fixation_end_ms_ = 0;
}

bool FaceGazeController::owner_can_write(const ncos::core::contracts::FaceRenderState& state) const {
  if (owner_service_ == 0) {
    return false;
  }

  const size_t idx = ncos::models::face::face_layer_index(ncos::models::face::FaceLayer::kGaze);
  const auto& gaze_layer = state.composition.layers[idx];

  return gaze_layer.owner_role == ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner &&
         gaze_layer.owner_service == owner_service_;
}

bool FaceGazeController::tick(uint64_t now_ms, ncos::core::contracts::FaceRenderState* state) {
  if (state == nullptr || !ncos::core::contracts::is_valid(*state) || !owner_can_write(*state) ||
      !active_target_) {
    return false;
  }

  if (now_ms > fixation_end_ms_) {
    active_target_ = false;
    phase_ = FaceGazeMotionPhase::kIdle;
    return false;
  }

  ncos::models::face::GazeDirection direction = target_.direction;
  uint8_t focus = target_.focus_percent;

  if (now_ms < saccade_end_ms_) {
    phase_ = FaceGazeMotionPhase::kSaccade;
    direction = target_.direction;
    focus = lerp_u8(saccade_entry_focus(target_.focus_percent), target_.focus_percent,
                    now_ms - saccade_start_ms_, saccade_end_ms_ - saccade_start_ms_);
  } else if (now_ms < overshoot_end_ms_) {
    phase_ = FaceGazeMotionPhase::kOvershoot;
    direction = overshoot_direction_;
    focus = overshoot_focus(target_.focus_percent);
  } else if (now_ms < settle_end_ms_) {
    phase_ = FaceGazeMotionPhase::kSettle;
    direction = target_.direction;
    focus = lerp_u8(overshoot_focus(target_.focus_percent), target_.focus_percent,
                    now_ms - overshoot_end_ms_, settle_end_ms_ - overshoot_end_ms_);
  } else {
    phase_ = FaceGazeMotionPhase::kFixation;
    direction = target_.direction;
    focus = target_.focus_percent;
  }

  const bool changed = state->eyes.anchor != target_.anchor || state->eyes.direction != direction ||
                       state->eyes.focus_percent != focus;

  if (!changed) {
    return false;
  }

  state->eyes.anchor = target_.anchor;
  state->eyes.direction = direction;
  state->eyes.focus_percent = focus;
  state->updated_at_ms = now_ms;
  state->owner_service = owner_service_;
  ++state->revision;
  current_direction_ = direction;
  return true;
}

}  // namespace ncos::services::face

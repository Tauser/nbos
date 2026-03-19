#include "services/face/face_motion_safety.hpp"

namespace {

using ncos::models::face::BlinkPhase;
using ncos::models::face::GazeDirection;

struct DirVec {
  int8_t x = 0;
  int8_t y = 0;
};

uint8_t clamp_percent(uint8_t value) {
  return value > 100 ? 100 : value;
}

uint8_t min_u8(uint8_t a, uint8_t b) {
  return a < b ? a : b;
}

uint8_t max_u8(uint8_t a, uint8_t b) {
  return a > b ? a : b;
}

uint8_t abs_diff_u8(uint8_t a, uint8_t b) {
  return a > b ? static_cast<uint8_t>(a - b) : static_cast<uint8_t>(b - a);
}

DirVec direction_to_vec(GazeDirection direction) {
  switch (direction) {
    case GazeDirection::kLeft:
      return {-1, 0};
    case GazeDirection::kRight:
      return {1, 0};
    case GazeDirection::kUp:
      return {0, -1};
    case GazeDirection::kDown:
      return {0, 1};
    case GazeDirection::kUpLeft:
      return {-1, -1};
    case GazeDirection::kUpRight:
      return {1, -1};
    case GazeDirection::kDownLeft:
      return {-1, 1};
    case GazeDirection::kDownRight:
      return {1, 1};
    case GazeDirection::kCenter:
    default:
      return {0, 0};
  }
}

bool is_diagonal(GazeDirection direction) {
  const DirVec vec = direction_to_vec(direction);
  return vec.x != 0 && vec.y != 0;
}

bool is_vertical(GazeDirection direction) {
  const DirVec vec = direction_to_vec(direction);
  return vec.x == 0 && vec.y != 0;
}

int transition_distance(GazeDirection from, GazeDirection to) {
  const DirVec a = direction_to_vec(from);
  const DirVec b = direction_to_vec(to);
  const int dx = static_cast<int>(a.x) - static_cast<int>(b.x);
  const int dy = static_cast<int>(a.y) - static_cast<int>(b.y);
  const int abs_dx = dx < 0 ? -dx : dx;
  const int abs_dy = dy < 0 ? -dy : dy;
  return abs_dx + abs_dy;
}

uint8_t limit_focus_step(uint8_t previous_focus, uint8_t target_focus, uint8_t max_step) {
  if (target_focus > previous_focus) {
    const uint8_t delta = static_cast<uint8_t>(target_focus - previous_focus);
    return delta > max_step ? static_cast<uint8_t>(previous_focus + max_step) : target_focus;
  }

  const uint8_t delta = static_cast<uint8_t>(previous_focus - target_focus);
  return delta > max_step ? static_cast<uint8_t>(previous_focus - max_step) : target_focus;
}

uint8_t safe_focus_cap_for_direction(GazeDirection direction) {
  if (is_diagonal(direction)) {
    return 44;
  }
  if (is_vertical(direction)) {
    return 54;
  }
  if (direction == GazeDirection::kCenter) {
    return 100;
  }
  return 62;
}

uint8_t safe_focus_cap_for_blink(BlinkPhase phase, uint8_t openness_percent) {
  switch (phase) {
    case BlinkPhase::kClosing:
      return openness_percent > 38 ? 28 : 20;
    case BlinkPhase::kClosed:
      return 0;
    case BlinkPhase::kOpening:
      return openness_percent > 45 ? 26 : 18;
    case BlinkPhase::kOpen:
    default:
      break;
  }

  if (openness_percent < 55) {
    return 24;
  }
  if (openness_percent < 72) {
    return 34;
  }
  return 100;
}

uint8_t safe_focus_cap_for_transition(int distance, uint64_t elapsed_ms, bool diagonal) {
  if (distance <= 1) {
    return 100;
  }

  const bool very_abrupt = distance >= 3;
  const uint8_t start_cap = diagonal ? (very_abrupt ? 30 : 38) : (very_abrupt ? 36 : 44);
  const uint8_t end_cap = diagonal ? 46 : 60;
  if (elapsed_ms >= 160) {
    return end_cap;
  }

  const uint8_t ramp =
      static_cast<uint8_t>((static_cast<uint32_t>(elapsed_ms) * (end_cap - start_cap)) / 160U);
  return static_cast<uint8_t>(start_cap + ramp);
}

uint8_t safe_step_cap(bool diagonal, bool blink_active, bool abrupt_transition, uint8_t focus_delta) {
  uint8_t step = diagonal ? 16 : 20;
  if (abrupt_transition) {
    step = diagonal ? 14 : 18;
  }
  if (blink_active) {
    step = min_u8(step, diagonal ? 10 : 12);
  }
  if (focus_delta >= 24) {
    step = min_u8(step, static_cast<uint8_t>(step > 2 ? step - 2 : step));
  }
  return max_u8(step, static_cast<uint8_t>(8));
}

}  // namespace

namespace ncos::services::face {

void reset_face_motion_safety_status(FaceMotionSafetyStatus* status,
                                     const ncos::core::contracts::FaceRenderState& state,
                                     uint64_t now_ms) {
  if (status == nullptr) {
    return;
  }

  status->last_direction = state.eyes.direction;
  status->last_focus_percent = clamp_percent(state.eyes.focus_percent);
  status->last_update_ms = now_ms;
}

bool apply_face_motion_safety(ncos::core::contracts::FaceRenderState* state,
                              FaceMotionSafetyStatus* status,
                              uint64_t now_ms,
                              FaceMotionSafetyResult* out_result) {
  if (state == nullptr || status == nullptr || !ncos::core::contracts::is_valid(*state)) {
    return false;
  }

  FaceMotionSafetyResult result{};
  const GazeDirection direction = state->eyes.direction;
  const uint8_t original_focus = clamp_percent(state->eyes.focus_percent);
  uint8_t safe_focus = original_focus;
  const bool diagonal = is_diagonal(direction);
  const bool blink_active = state->lids.phase != BlinkPhase::kOpen || state->lids.openness_percent < 85;

  const int distance = transition_distance(status->last_direction, direction);
  const uint8_t focus_delta = abs_diff_u8(original_focus, status->last_focus_percent);
  const uint64_t elapsed_ms = status->last_update_ms == 0 || now_ms <= status->last_update_ms
                                  ? 160
                                  : now_ms - status->last_update_ms;
  const bool abrupt_transition = distance >= 2 || focus_delta >= 18;

  const uint8_t direction_cap = safe_focus_cap_for_direction(direction);
  if (direction_cap < safe_focus) {
    safe_focus = direction_cap;
    result.limited_amplitude = true;
  }

  const uint8_t blink_cap = safe_focus_cap_for_blink(state->lids.phase, state->lids.openness_percent);
  if (blink_cap < safe_focus) {
    safe_focus = blink_cap;
    result.limited_blink_interaction = true;
  }

  const uint8_t transition_cap = safe_focus_cap_for_transition(distance, elapsed_ms, diagonal);
  if (transition_cap < safe_focus) {
    safe_focus = transition_cap;
    result.limited_velocity = true;
  }

  if (state->lids.phase != BlinkPhase::kClosed) {
    const uint8_t step_cap = safe_step_cap(diagonal, blink_active, abrupt_transition, focus_delta);
    const uint8_t stepped_focus = limit_focus_step(status->last_focus_percent, safe_focus, step_cap);
    if (stepped_focus != safe_focus) {
      safe_focus = stepped_focus;
      result.limited_velocity = true;
    }
  }

  if (diagonal && safe_focus < original_focus) {
    result.limited_diagonal = true;
  }

  result.effective_focus_percent = safe_focus;
  result.active = result.limited_amplitude || result.limited_velocity || result.limited_diagonal ||
                  result.limited_blink_interaction;

  const bool changed = state->eyes.focus_percent != safe_focus;
  if (changed) {
    state->eyes.focus_percent = safe_focus;
    state->updated_at_ms = now_ms;
    ++state->revision;
  }

  status->last_direction = direction;
  status->last_focus_percent = safe_focus;
  status->last_update_ms = now_ms;

  if (out_result != nullptr) {
    *out_result = result;
  }

  return changed;
}

}  // namespace ncos::services::face

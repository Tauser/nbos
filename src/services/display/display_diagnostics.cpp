#include "services/display/display_diagnostics.hpp"

#include <stdio.h>

namespace {

constexpr uint16_t BackgroundColor = 0x0000;
constexpr uint16_t FaceColor = 0x1082;
constexpr uint16_t EyeColor = 0xFFFF;
constexpr uint16_t AccentColor = 0xF800;
constexpr uint16_t ClipGridColor = 0x07E0;
constexpr uint16_t DebugPanelColor = 0x0000;
constexpr uint16_t DebugFrameColor = 0x39E7;
constexpr uint16_t DebugWarnColor = 0xFD20;
constexpr uint16_t DebugHotColor = 0xF800;
constexpr uint16_t DebugCoolColor = 0x07E0;
constexpr uint16_t DebugFallbackColor = 0xFFE0;
constexpr int16_t SweepWidth = 24;

uint16_t bar_color_for_ratio(uint32_t value, uint32_t budget) {
  if (budget == 0) {
    return DebugCoolColor;
  }
  if (value >= budget) {
    return DebugHotColor;
  }
  if (value * 5 >= budget * 4) {
    return DebugWarnColor;
  }
  return DebugCoolColor;
}

int16_t clamp_bar_width(uint32_t value, uint32_t budget, int16_t max_width) {
  if (budget == 0 || max_width <= 0) {
    return 0;
  }
  const uint32_t scaled = (value >= budget ? budget : value) * static_cast<uint32_t>(max_width);
  return static_cast<int16_t>(scaled / budget);
}

uint16_t panel_flip_color(uint8_t phase) {
  switch (phase % 4) {
    case 0:
      return 0x0000;
    case 1:
      return 0xFFFF;
    case 2:
      return 0xF800;
    default:
      return 0x001F;
  }
}

uint16_t full_redraw_period_ms(const ncos::drivers::display::DisplayDriver* display) {
  return display->capability_profile().timing.recommended_full_redraw_period_ms;
}

uint16_t partial_redraw_period_ms(const ncos::drivers::display::DisplayDriver* display) {
  return display->capability_profile().timing.recommended_partial_redraw_period_ms;
}

uint16_t sprite_period_ms(const ncos::drivers::display::DisplayDriver* display) {
  return display->capability_profile().timing.recommended_sprite_window_period_ms;
}

uint16_t abs_i16(int16_t value) {
  return value < 0 ? static_cast<uint16_t>(-value) : static_cast<uint16_t>(value);
}

bool eye_trail_in_bad_motion_zone(const ncos::services::face::FaceFrame& previous,
                                  const ncos::services::face::FaceFrame& current) {
  const int16_t left_dx = static_cast<int16_t>(current.left_eye_x - previous.left_eye_x);
  const int16_t right_dx = static_cast<int16_t>(current.right_eye_x - previous.right_eye_x);
  const int16_t left_dy = static_cast<int16_t>(current.left_eye_y - previous.left_eye_y);
  const int16_t right_dy = static_cast<int16_t>(current.right_eye_y - previous.right_eye_y);

  const uint16_t max_dx = abs_i16(abs_i16(left_dx) > abs_i16(right_dx) ? left_dx : right_dx);
  const uint16_t max_dy = abs_i16(abs_i16(left_dy) > abs_i16(right_dy) ? left_dy : right_dy);
  const bool abrupt_motion = max_dx >= 8 || max_dy >= 6;
  const bool diagonal_motion = max_dx > 0 && max_dy > 0;
  const bool high_visual_activity = abrupt_motion && current.eye_color != current.face_color;
  return abrupt_motion || diagonal_motion || high_visual_activity;
}

}  // namespace

namespace ncos::services::display {

bool DisplayDiagnosticsRunner::bind(ncos::drivers::display::DisplayDriver* display,
                                    ncos::services::face::FaceDisplayRenderer* renderer) {
  display_ = display;
  renderer_ = renderer;
  last_step_ms_ = 0;
  last_sweep_x_ = -1;
  last_sprite_x_ = -1;
  panel_flip_phase_ = 0;
  has_previous_eye_trail_frame_ = false;
  previous_eye_trail_frame_ = ncos::services::face::FaceFrame{};
  return display_ != nullptr && renderer_ != nullptr;
}

void DisplayDiagnosticsRunner::set_mode(ncos::config::DisplayDiagnosticsMode mode) {
  mode_ = mode;
  last_step_ms_ = 0;
  last_sweep_x_ = -1;
  last_sprite_x_ = -1;
  panel_flip_phase_ = 0;
  has_previous_eye_trail_frame_ = false;
  previous_eye_trail_frame_ = ncos::services::face::FaceFrame{};
}

ncos::config::DisplayDiagnosticsMode DisplayDiagnosticsRunner::mode() const {
  return mode_;
}

void DisplayDiagnosticsRunner::tick(uint64_t now_ms) {
  if (display_ == nullptr || renderer_ == nullptr || mode_ == ncos::config::DisplayDiagnosticsMode::kOff) {
    return;
  }

  switch (mode_) {
    case ncos::config::DisplayDiagnosticsMode::kStaticPrimaries:
      render_static_primaries();
      break;
    case ncos::config::DisplayDiagnosticsMode::kStaticClipGrid:
      render_static_clip_grid();
      break;
    case ncos::config::DisplayDiagnosticsMode::kHorizontalSweepFullRedraw:
      render_horizontal_sweep(now_ms);
      break;
    case ncos::config::DisplayDiagnosticsMode::kEyeTrailFullRedraw:
      render_eye_trail(now_ms, DisplayRenderMode::kForceFullRedraw);
      break;
    case ncos::config::DisplayDiagnosticsMode::kEyeTrailDirtyRect:
      render_eye_trail(now_ms, DisplayRenderMode::kForceDirtyRect);
      break;
    case ncos::config::DisplayDiagnosticsMode::kSpriteWindowTrail:
      render_sprite_window_trail(now_ms);
      break;
    case ncos::config::DisplayDiagnosticsMode::kPanelPolarityFlip:
      render_panel_polarity_flip(now_ms);
      break;
    case ncos::config::DisplayDiagnosticsMode::kFaceVisualDebug:
    case ncos::config::DisplayDiagnosticsMode::kOff:
    default:
      break;
  }
}

void DisplayDiagnosticsRunner::render_face_visual_debug(
    const ncos::services::face::FacePreviewSnapshot& snapshot) {
  if (display_ == nullptr) {
    return;
  }

  constexpr int16_t panel_height = 44;
  const int16_t panel_y = static_cast<int16_t>(display_->height() - panel_height);
  const uint32_t dirty_budget_px = static_cast<uint32_t>(display_->width()) * 24u;

  display_->startWrite();
  display_->fillRect(0, panel_y, display_->width(), panel_height, DebugPanelColor);
  display_->drawFastHLine(0, panel_y, display_->width(), DebugFrameColor);

  const int16_t bar_x = 6;
  const int16_t bar_w = 78;
  const int16_t total_y = static_cast<int16_t>(panel_y + 6);
  const int16_t render_y = static_cast<int16_t>(panel_y + 18);
  const int16_t dirty_y = static_cast<int16_t>(panel_y + 30);

  display_->drawRect(bar_x, total_y, bar_w, 7, DebugFrameColor);
  display_->fillRect(bar_x + 1, total_y + 1,
                     clamp_bar_width(snapshot.tuning.stages.total_us, snapshot.tuning.frame_budget_us, bar_w - 2), 5,
                     bar_color_for_ratio(snapshot.tuning.stages.total_us, snapshot.tuning.frame_budget_us));

  display_->drawRect(bar_x, render_y, bar_w, 7, DebugFrameColor);
  display_->fillRect(bar_x + 1, render_y + 1,
                     clamp_bar_width(snapshot.tuning.stages.render_us, snapshot.tuning.frame_budget_us, bar_w - 2), 5,
                     bar_color_for_ratio(snapshot.tuning.stages.render_us, snapshot.tuning.frame_budget_us));

  display_->drawRect(bar_x, dirty_y, bar_w, 7, DebugFrameColor);
  display_->fillRect(bar_x + 1, dirty_y + 1,
                     clamp_bar_width(snapshot.tuning.dirty_area_px, dirty_budget_px, bar_w - 2), 5,
                     bar_color_for_ratio(snapshot.tuning.dirty_area_px, dirty_budget_px));

  display_->setTextColor(EyeColor, DebugPanelColor);
  display_->setTextSize(1);
  display_->setCursor(92, total_y - 1);
  display_->printf("T%lu/%lu", static_cast<unsigned long>(snapshot.tuning.stages.total_us),
                   static_cast<unsigned long>(snapshot.tuning.frame_budget_us));
  display_->setCursor(92, render_y - 1);
  display_->printf("R%lu A%lu", static_cast<unsigned long>(snapshot.tuning.stages.render_us),
                   static_cast<unsigned long>(snapshot.tuning.avg_frame_time_us));
  display_->setCursor(92, dirty_y - 1);
  display_->printf("D%lu S%lu", static_cast<unsigned long>(snapshot.tuning.dirty_area_px),
                   static_cast<unsigned long>(snapshot.tuning.skipped_duplicate_frames));

  const struct {
    bool active;
    uint16_t color;
  } flags[] = {
      {snapshot.tuning.safe_visual_mode, DebugFallbackColor},
      {snapshot.clip_active, AccentColor},
      {snapshot.tuning.full_redraw, DebugWarnColor},
      {snapshot.tuning.high_contrast_motion, DebugHotColor},
      {ncos::services::face::has_face_visual_degradation(
           snapshot.tuning.degradation, ncos::services::face::FaceVisualDegradationFlag::kDiagonalMotion),
       0x001F},
      {ncos::services::face::has_face_visual_degradation(
           snapshot.tuning.degradation, ncos::services::face::FaceVisualDegradationFlag::kFrameOverBudget),
       DebugHotColor},
  };

  int16_t flag_x = static_cast<int16_t>(display_->width() - 45);
  const int16_t flag_y = static_cast<int16_t>(panel_y + 14);
  for (const auto& flag : flags) {
    display_->drawRect(flag_x, flag_y, 6, 6, DebugFrameColor);
    if (flag.active) {
      display_->fillRect(flag_x + 1, flag_y + 1, 4, 4, flag.color);
    }
    flag_x = static_cast<int16_t>(flag_x + 7);
  }

  display_->endWrite();
}

void DisplayDiagnosticsRunner::render_static_primaries() {
  display_->startWrite();
  display_->fillScreen(BackgroundColor);
  display_->fillRect(0, 0, display_->width() / 3, display_->height(), 0xF800);
  display_->fillRect(display_->width() / 3, 0, display_->width() / 3, display_->height(), 0x07E0);
  display_->fillRect((display_->width() / 3) * 2, 0, display_->width() / 3, display_->height(), 0x001F);
  display_->drawRect(0, 0, display_->width(), display_->height(), EyeColor);
  display_->endWrite();
}

void DisplayDiagnosticsRunner::render_static_clip_grid() {
  display_->startWrite();
  display_->fillScreen(BackgroundColor);
  for (int16_t x = -20; x < display_->width(); x += 24) {
    display_->drawFastVLine(x, 0, display_->height(), ClipGridColor);
  }
  for (int16_t y = -20; y < display_->height(); y += 24) {
    display_->drawFastHLine(0, y, display_->width(), ClipGridColor);
  }
  display_->drawRoundRect(-6, -6, 72, 52, 10, AccentColor);
  display_->drawRoundRect(display_->width() - 54, display_->height() - 40, 64, 50, 10, AccentColor);
  display_->endWrite();
}

void DisplayDiagnosticsRunner::render_horizontal_sweep(uint64_t now_ms) {
  if (last_step_ms_ != 0 && (now_ms - last_step_ms_) < full_redraw_period_ms(display_)) {
    return;
  }

  last_step_ms_ = now_ms;
  int16_t x = static_cast<int16_t>((now_ms / full_redraw_period_ms(display_)) %
                                   (display_->width() + SweepWidth)) - SweepWidth;

  display_->startWrite();
  display_->fillScreen(BackgroundColor);
  if (x < display_->width()) {
    display_->fillRect(x, 0, SweepWidth, display_->height(), EyeColor);
  }
  display_->endWrite();
  last_sweep_x_ = x;
}

ncos::services::face::FaceFrame DisplayDiagnosticsRunner::make_eye_trail_frame(uint64_t now_ms) const {
  ncos::services::face::FaceFrame frame{};
  frame.background = BackgroundColor;
  frame.face_color = FaceColor;
  frame.eye_color = EyeColor;
  frame.head_x = 28;
  frame.head_y = 42;
  frame.head_w = 184;
  frame.head_h = 184;
  frame.head_radius = 32;
  frame.eye_w = 34;
  frame.eye_h = 54;
  frame.eye_corner = 14;

  const uint16_t period_ms = partial_redraw_period_ms(display_);
  const int16_t phase = static_cast<int16_t>((now_ms / period_ms) % 10);
  const int16_t offset = static_cast<int16_t>((phase - 5) * 8);
  frame.left_eye_x = static_cast<int16_t>(84 + offset);
  frame.right_eye_x = static_cast<int16_t>(156 + offset);
  frame.left_eye_y = 124;
  frame.right_eye_y = 124;
  return frame;
}

void DisplayDiagnosticsRunner::render_eye_trail(uint64_t now_ms,
                                                ncos::services::display::DisplayRenderMode render_mode) {
  auto frame = make_eye_trail_frame(now_ms);
  uint16_t period_ms = render_mode == DisplayRenderMode::kForceFullRedraw
                           ? full_redraw_period_ms(display_)
                           : partial_redraw_period_ms(display_);

  if (ncos::config::kGlobalConfig.runtime.display_diagnostics_conservative_pacing &&
      has_previous_eye_trail_frame_ && eye_trail_in_bad_motion_zone(previous_eye_trail_frame_, frame)) {
    period_ms = static_cast<uint16_t>(ncos::config::kGlobalConfig.runtime.display_diagnostics_conservative_period_ms > period_ms
                                          ? ncos::config::kGlobalConfig.runtime.display_diagnostics_conservative_period_ms
                                          : period_ms);
  }

  if (last_step_ms_ != 0 && (now_ms - last_step_ms_) < period_ms) {
    return;
  }

  last_step_ms_ = now_ms;
  display_->set_write_clock_hz(ncos::config::kGlobalConfig.runtime.display_diagnostics_spi_write_hz);
  renderer_->set_render_mode(render_mode);
  (void)renderer_->render(frame);
  previous_eye_trail_frame_ = frame;
  has_previous_eye_trail_frame_ = true;
}

void DisplayDiagnosticsRunner::render_sprite_window_trail(uint64_t now_ms) {
  if (last_step_ms_ != 0 && (now_ms - last_step_ms_) < sprite_period_ms(display_)) {
    return;
  }

  last_step_ms_ = now_ms;
  const int16_t sprite_w = 92;
  const int16_t sprite_h = 72;
  const int16_t x = static_cast<int16_t>((now_ms / sprite_period_ms(display_)) %
                                         (display_->width() + sprite_w)) - sprite_w;
  const int16_t y = static_cast<int16_t>(display_->height() / 2 - sprite_h / 2);

  if (last_sprite_x_ < 0) {
    display_->fillScreen(BackgroundColor);
  }

  if (last_sprite_x_ >= 0) {
    display_->fillRect(last_sprite_x_, y, sprite_w, sprite_h, BackgroundColor);
  }

  lgfx::LGFX_Sprite sprite(display_);
  sprite.setColorDepth(16);
  sprite.createSprite(sprite_w, sprite_h);
  sprite.fillSprite(BackgroundColor);
  sprite.fillRoundRect(6, 8, sprite_w - 12, sprite_h - 16, 14, FaceColor);
  sprite.fillRoundRect(18, 22, 18, 28, 8, EyeColor);
  sprite.fillRoundRect(sprite_w - 36, 22, 18, 28, 8, EyeColor);
  sprite.drawRect(0, 0, sprite_w, sprite_h, AccentColor);
  sprite.pushSprite(x, y);
  sprite.deleteSprite();
  last_sprite_x_ = x;
}

void DisplayDiagnosticsRunner::render_panel_polarity_flip(uint64_t now_ms) {
  if (last_step_ms_ != 0 && (now_ms - last_step_ms_) < 240) {
    return;
  }

  last_step_ms_ = now_ms;
  display_->fillScreen(panel_flip_color(panel_flip_phase_));
  ++panel_flip_phase_;
}

}  // namespace ncos::services::display

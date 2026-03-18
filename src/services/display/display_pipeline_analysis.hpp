#pragma once

#include <stdint.h>

#include "drivers/display/panel_capability_profile.hpp"
#include "services/face/face_frame.hpp"

namespace ncos::services::display {

enum class DisplayRenderMode : uint8_t {
  kAuto = 0,
  kForceFullRedraw,
  kForceDirtyRect,
};

enum class FullRedrawReason : uint8_t {
  kNone = 0,
  kNoPreviousFrame = 1 << 0,
  kBackgroundChanged = 1 << 1,
  kFaceColorChanged = 1 << 2,
  kHeadGeometryChanged = 1 << 3,
  kForced = 1 << 4,
};

constexpr FullRedrawReason operator|(FullRedrawReason lhs, FullRedrawReason rhs) {
  return static_cast<FullRedrawReason>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr bool has_full_redraw_reason(FullRedrawReason mask, FullRedrawReason reason) {
  return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(reason)) != 0;
}

struct DirtyRect {
  int16_t x = 0;
  int16_t y = 0;
  int16_t w = 0;
  int16_t h = 0;
  bool valid = false;
  bool clipped = false;
};

struct DisplayRenderPlan {
  DisplayRenderMode requested_mode = DisplayRenderMode::kAuto;
  DisplayRenderMode effective_mode = DisplayRenderMode::kAuto;
  bool full_redraw = true;
  bool high_contrast_motion = false;
  FullRedrawReason full_redraw_reason = FullRedrawReason::kNone;
  ncos::drivers::display::DisplayFlushPath recommended_flush_path =
      ncos::drivers::display::DisplayFlushPath::kDirectPrimitives;
  DirtyRect dirty_rect{};
};

DirtyRect compute_eye_dirty_rect(const ncos::services::face::FaceFrame& previous,
                                 const ncos::services::face::FaceFrame& current,
                                 int16_t display_width,
                                 int16_t display_height);

DisplayRenderPlan analyze_render_plan(const ncos::services::face::FaceFrame* previous,
                                      const ncos::services::face::FaceFrame& current,
                                      int16_t display_width,
                                      int16_t display_height,
                                      DisplayRenderMode requested_mode,
                                      const ncos::drivers::display::DisplayPanelCapabilityProfile& panel_profile);

}  // namespace ncos::services::display

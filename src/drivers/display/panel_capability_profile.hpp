#pragma once

#include <stdint.h>

namespace ncos::drivers::display {

enum class DisplayPanelId : uint8_t {
  kUnknown = 0,
  kSt7789_240x320,
};

enum class DisplayFlushPath : uint8_t {
  kDirectPrimitives = 0,
  kSpriteWindow,
};

struct DisplayTimingLimits {
  uint32_t spi_write_hz = 0;
  uint32_t spi_read_hz = 0;
  uint16_t recommended_full_redraw_period_ms = 0;
  uint16_t recommended_partial_redraw_period_ms = 0;
  uint16_t recommended_sprite_window_period_ms = 0;
};

struct DisplayObservedArtifacts {
  bool panel_polarity_flip_clean = false;
  bool full_redraw_motion_artifacts = false;
  bool dirty_rect_motion_artifacts = false;
  bool sprite_window_flicker = false;
};

struct DisplayWorkarounds {
  bool keep_panel_inverted = false;
  bool prefer_direct_primitives = false;
  bool allow_dirty_rect_for_small_eye_motion = false;
  bool avoid_sprite_window_in_product_path = false;
  bool avoid_large_high_contrast_full_redraw_when_possible = false;
};

struct DisplayPanelCapabilityProfile {
  DisplayPanelId panel_id = DisplayPanelId::kUnknown;
  const char* panel_name = "unknown";
  uint16_t width = 0;
  uint16_t height = 0;
  bool readable = false;
  bool reset_shared_with_enable = false;
  DisplayTimingLimits timing{};
  DisplayObservedArtifacts observed{};
  DisplayWorkarounds workarounds{};
};

const DisplayPanelCapabilityProfile& active_panel_capability_profile();

bool flush_path_recommended(const DisplayPanelCapabilityProfile& profile,
                            DisplayFlushPath path,
                            bool high_contrast_motion);

}  // namespace ncos::drivers::display

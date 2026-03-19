#pragma once

#include <array>

#include "drivers/display/display_driver.hpp"
#include "services/display/display_pipeline_analysis.hpp"
#include "services/face/face_frame.hpp"

namespace ncos::services::face {

struct FaceRenderStats {
  uint32_t last_frame_time_us = 0;
  uint32_t avg_frame_time_us = 0;
  uint32_t peak_frame_time_us = 0;
  uint32_t last_dirty_area_px = 0;
  uint32_t avg_dirty_area_px = 0;
  uint32_t rendered_frames = 0;
  uint32_t skipped_duplicate_frames = 0;
  uint16_t last_frame_flushes = 0;
  uint16_t composite_buffer_bytes = 0;
  bool last_frame_skipped = false;
  bool last_frame_full_redraw = false;
};

class FaceDisplayRenderer final {
 public:
  bool bind(ncos::drivers::display::DisplayDriver* display);
  bool render(const FaceFrame& frame);
  void set_render_mode(ncos::services::display::DisplayRenderMode mode);
  ncos::services::display::DisplayRenderPlan last_render_plan() const;
  FaceRenderStats render_stats() const;

 private:
  ncos::drivers::display::DisplayDriver* display_ = nullptr;
  bool has_previous_frame_ = false;
  FaceFrame previous_frame_{};
  ncos::services::display::DisplayRenderMode render_mode_ =
      ncos::services::display::DisplayRenderMode::kAuto;
  ncos::services::display::DisplayRenderPlan last_render_plan_{};
  FaceRenderStats render_stats_{};
  static constexpr int16_t CompositeRegionMaxWidth = 64;
  static constexpr int16_t CompositeRegionMaxHeight = 72;
  static constexpr size_t CompositeRegionMaxPixels =
      static_cast<size_t>(CompositeRegionMaxWidth) * static_cast<size_t>(CompositeRegionMaxHeight);
  std::array<uint16_t, CompositeRegionMaxPixels> composite_region_buffer_{};
};

}  // namespace ncos::services::face

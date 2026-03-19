#pragma once

#include "drivers/display/display_driver.hpp"
#include "services/display/display_pipeline_analysis.hpp"
#include "services/face/face_frame.hpp"

namespace ncos::services::face {

struct FaceRenderStats {
  uint32_t last_frame_time_us = 0;
  uint32_t avg_frame_time_us = 0;
  uint32_t peak_frame_time_us = 0;
  uint32_t last_dirty_area_px = 0;
  uint32_t rendered_frames = 0;
  uint32_t skipped_duplicate_frames = 0;
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
};

}  // namespace ncos::services::face

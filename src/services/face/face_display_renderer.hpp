#pragma once

#include "drivers/display/display_driver.hpp"
#include "services/display/display_pipeline_analysis.hpp"
#include "services/face/face_frame.hpp"

namespace ncos::services::face {

class FaceDisplayRenderer final {
 public:
  bool bind(ncos::drivers::display::DisplayDriver* display);
  bool render(const FaceFrame& frame);
  void set_render_mode(ncos::services::display::DisplayRenderMode mode);
  ncos::services::display::DisplayRenderPlan last_render_plan() const;

 private:
  ncos::drivers::display::DisplayDriver* display_ = nullptr;
  bool has_previous_frame_ = false;
  FaceFrame previous_frame_{};
  ncos::services::display::DisplayRenderMode render_mode_ =
      ncos::services::display::DisplayRenderMode::kAuto;
  ncos::services::display::DisplayRenderPlan last_render_plan_{};
};

}  // namespace ncos::services::face

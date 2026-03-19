#pragma once

#include <stdint.h>

#include "config/system_config.hpp"
#include "drivers/display/display_driver.hpp"
#include "services/face/face_display_renderer.hpp"
#include "services/face/face_tooling.hpp"
#include "services/face/face_frame.hpp"

namespace ncos::services::display {

class DisplayDiagnosticsRunner final {
 public:
  bool bind(ncos::drivers::display::DisplayDriver* display,
            ncos::services::face::FaceDisplayRenderer* renderer);
  void set_mode(ncos::config::DisplayDiagnosticsMode mode);
  void tick(uint64_t now_ms);
  void render_face_visual_debug(const ncos::services::face::FacePreviewSnapshot& snapshot);
  ncos::config::DisplayDiagnosticsMode mode() const;

 private:
  void render_static_primaries();
  void render_static_clip_grid();
  void render_horizontal_sweep(uint64_t now_ms);
  void render_eye_trail(uint64_t now_ms, ncos::services::display::DisplayRenderMode render_mode);
  void render_sprite_window_trail(uint64_t now_ms);
  void render_panel_polarity_flip(uint64_t now_ms);

  ncos::services::face::FaceFrame make_eye_trail_frame(uint64_t now_ms) const;

  ncos::drivers::display::DisplayDriver* display_ = nullptr;
  ncos::services::face::FaceDisplayRenderer* renderer_ = nullptr;
  ncos::config::DisplayDiagnosticsMode mode_ = ncos::config::DisplayDiagnosticsMode::kOff;
  uint64_t last_step_ms_ = 0;
  int16_t last_sweep_x_ = -1;
  int16_t last_sprite_x_ = -1;
  uint8_t panel_flip_phase_ = 0;
};

}  // namespace ncos::services::display

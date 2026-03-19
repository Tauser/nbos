#include "services/face/face_display_renderer.hpp"

#include <algorithm>
#include <chrono>

namespace {

constexpr int16_t BandMarginX = 8;
constexpr int16_t BandHeight = 72;
constexpr int16_t BandMinWidth = 124;

uint64_t monotonic_time_us() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

int16_t min_i16(int16_t a, int16_t b) {
  return a < b ? a : b;
}

int16_t max_i16(int16_t a, int16_t b) {
  return a > b ? a : b;
}

int16_t left_eye_w(const ncos::services::face::FaceFrame& frame) {
  return frame.left_eye_w > 0 ? frame.left_eye_w : frame.eye_w;
}

int16_t left_eye_h(const ncos::services::face::FaceFrame& frame) {
  return frame.left_eye_h > 0 ? frame.left_eye_h : frame.eye_h;
}

int16_t left_eye_corner(const ncos::services::face::FaceFrame& frame) {
  return frame.left_eye_corner > 0 ? frame.left_eye_corner : frame.eye_corner;
}

int16_t right_eye_w(const ncos::services::face::FaceFrame& frame) {
  return frame.right_eye_w > 0 ? frame.right_eye_w : frame.eye_w;
}

int16_t right_eye_h(const ncos::services::face::FaceFrame& frame) {
  return frame.right_eye_h > 0 ? frame.right_eye_h : frame.eye_h;
}

int16_t right_eye_corner(const ncos::services::face::FaceFrame& frame) {
  return frame.right_eye_corner > 0 ? frame.right_eye_corner : frame.eye_corner;
}

bool frames_equal(const ncos::services::face::FaceFrame& lhs, const ncos::services::face::FaceFrame& rhs) {
  return lhs.background == rhs.background && lhs.face_color == rhs.face_color &&
         lhs.eye_color == rhs.eye_color && lhs.pupil_color == rhs.pupil_color &&
         lhs.mouth_color == rhs.mouth_color && lhs.head_x == rhs.head_x && lhs.head_y == rhs.head_y &&
         lhs.head_w == rhs.head_w && lhs.head_h == rhs.head_h && lhs.head_radius == rhs.head_radius &&
         lhs.left_eye_x == rhs.left_eye_x && lhs.left_eye_y == rhs.left_eye_y &&
         lhs.right_eye_x == rhs.right_eye_x && lhs.right_eye_y == rhs.right_eye_y &&
         lhs.eye_radius == rhs.eye_radius && lhs.eye_w == rhs.eye_w && lhs.eye_h == rhs.eye_h &&
         lhs.eye_corner == rhs.eye_corner && lhs.left_eye_radius == rhs.left_eye_radius &&
         lhs.left_eye_w == rhs.left_eye_w && lhs.left_eye_h == rhs.left_eye_h &&
         lhs.left_eye_corner == rhs.left_eye_corner && lhs.right_eye_radius == rhs.right_eye_radius &&
         lhs.right_eye_w == rhs.right_eye_w && lhs.right_eye_h == rhs.right_eye_h &&
         lhs.right_eye_corner == rhs.right_eye_corner && lhs.left_pupil_x == rhs.left_pupil_x &&
         lhs.left_pupil_y == rhs.left_pupil_y && lhs.right_pupil_x == rhs.right_pupil_x &&
         lhs.right_pupil_y == rhs.right_pupil_y && lhs.pupil_radius == rhs.pupil_radius &&
         lhs.mouth_x == rhs.mouth_x && lhs.mouth_y == rhs.mouth_y && lhs.mouth_w == rhs.mouth_w &&
         lhs.mouth_h == rhs.mouth_h && lhs.mouth_corner == rhs.mouth_corner;
}

struct EyeGeometry {
  int16_t x = 0;
  int16_t y = 0;
  int16_t w = 0;
  int16_t h = 0;
  int16_t corner = 0;
  uint16_t color = 0;
};

EyeGeometry resolve_eye_geometry(const ncos::services::face::FaceFrame& frame, bool left_eye) {
  const int16_t resolved_eye_w = left_eye ? left_eye_w(frame) : right_eye_w(frame);
  const int16_t resolved_eye_h = left_eye ? left_eye_h(frame) : right_eye_h(frame);

  EyeGeometry geometry{};
  geometry.w = resolved_eye_w;
  geometry.h = resolved_eye_h;
  geometry.corner = left_eye ? left_eye_corner(frame) : right_eye_corner(frame);
  geometry.color = frame.eye_color;
  geometry.x = static_cast<int16_t>((left_eye ? frame.left_eye_x : frame.right_eye_x) - resolved_eye_w / 2);
  geometry.y = static_cast<int16_t>((left_eye ? frame.left_eye_y : frame.right_eye_y) - resolved_eye_h / 2);
  return geometry;
}

ncos::services::display::DirtyRect clip_rect(int16_t x,
                                             int16_t y,
                                             int16_t w,
                                             int16_t h,
                                             int16_t display_width,
                                             int16_t display_height) {
  ncos::services::display::DirtyRect clipped{};
  int16_t x0 = x;
  int16_t y0 = y;
  int16_t x1 = static_cast<int16_t>(x + w);
  int16_t y1 = static_cast<int16_t>(y + h);

  if (x0 < 0) {
    x0 = 0;
  }
  if (y0 < 0) {
    y0 = 0;
  }
  if (x1 > display_width) {
    x1 = display_width;
  }
  if (y1 > display_height) {
    y1 = display_height;
  }

  clipped.valid = x1 > x0 && y1 > y0;
  clipped.clipped = x0 != x || y0 != y || x1 != x + w || y1 != y + h;
  clipped.x = x0;
  clipped.y = y0;
  clipped.w = clipped.valid ? static_cast<int16_t>(x1 - x0) : 0;
  clipped.h = clipped.valid ? static_cast<int16_t>(y1 - y0) : 0;
  return clipped;
}

ncos::services::display::DirtyRect compute_fixed_band_rect(const EyeGeometry& left_eye,
                                                           const EyeGeometry& right_eye,
                                                           int16_t display_width,
                                                           int16_t display_height) {
  const int16_t left_center_x = static_cast<int16_t>(left_eye.x + left_eye.w / 2);
  const int16_t right_center_x = static_cast<int16_t>(right_eye.x + right_eye.w / 2);
  const int16_t center_x = static_cast<int16_t>((left_center_x + right_center_x) / 2);
  const int16_t center_y = static_cast<int16_t>(((left_eye.y + left_eye.h / 2) + (right_eye.y + right_eye.h / 2)) / 2);
  const int16_t natural_width = static_cast<int16_t>((right_center_x - left_center_x) + left_eye.w / 2 +
                                                     right_eye.w / 2 + BandMarginX * 2);
  const int16_t band_width = max_i16(BandMinWidth, natural_width);
  return clip_rect(static_cast<int16_t>(center_x - band_width / 2),
                   static_cast<int16_t>(center_y - BandHeight / 2),
                   band_width, BandHeight, display_width, display_height);
}

bool rects_intersect(const ncos::services::display::DirtyRect& rect, const EyeGeometry& eye) {
  if (!rect.valid || eye.w <= 0 || eye.h <= 0) {
    return false;
  }

  const int16_t rect_x1 = static_cast<int16_t>(rect.x + rect.w);
  const int16_t rect_y1 = static_cast<int16_t>(rect.y + rect.h);
  const int16_t eye_x1 = static_cast<int16_t>(eye.x + eye.w);
  const int16_t eye_y1 = static_cast<int16_t>(eye.y + eye.h);
  return !(rect_x1 <= eye.x || eye_x1 <= rect.x || rect_y1 <= eye.y || eye_y1 <= rect.y);
}

bool point_inside_round_rect(int16_t px, int16_t py, const EyeGeometry& eye) {
  if (px < eye.x || py < eye.y || px >= eye.x + eye.w || py >= eye.y + eye.h) {
    return false;
  }

  const int16_t radius_limit = std::min(static_cast<int16_t>(eye.w / 2), static_cast<int16_t>(eye.h / 2));
  const int16_t radius = std::min(eye.corner, radius_limit);
  if (radius <= 0) {
    return true;
  }

  if (px >= eye.x + radius && px < eye.x + eye.w - radius) {
    return true;
  }
  if (py >= eye.y + radius && py < eye.y + eye.h - radius) {
    return true;
  }

  const int16_t center_left = static_cast<int16_t>(eye.x + radius);
  const int16_t center_right = static_cast<int16_t>(eye.x + eye.w - radius - 1);
  const int16_t center_top = static_cast<int16_t>(eye.y + radius);
  const int16_t center_bottom = static_cast<int16_t>(eye.y + eye.h - radius - 1);
  const int16_t circle_x = px < eye.x + radius ? center_left : center_right;
  const int16_t circle_y = py < eye.y + radius ? center_top : center_bottom;
  const int32_t dx = static_cast<int32_t>(px - circle_x);
  const int32_t dy = static_cast<int32_t>(py - circle_y);
  return dx * dx + dy * dy <= static_cast<int32_t>(radius) * static_cast<int32_t>(radius);
}

uint32_t rect_area(const ncos::services::display::DirtyRect& rect) {
  if (!rect.valid) {
    return 0;
  }
  return static_cast<uint32_t>(rect.w) * static_cast<uint32_t>(rect.h);
}

}  // namespace

namespace ncos::services::face {

bool FaceDisplayRenderer::bind(ncos::drivers::display::DisplayDriver* display) {
  display_ = display;
  has_previous_frame_ = false;
  previous_frame_ = FaceFrame{};
  last_render_plan_ = ncos::services::display::DisplayRenderPlan{};
  render_stats_ = FaceRenderStats{};
  render_stats_.composite_buffer_bytes = static_cast<uint16_t>(sizeof(composite_region_buffer_));
  return display_ != nullptr;
}

bool FaceDisplayRenderer::render(const FaceFrame& frame) {
  if (display_ == nullptr) {
    return false;
  }

  const FaceFrame* previous = has_previous_frame_ ? &previous_frame_ : nullptr;
  last_render_plan_ = ncos::services::display::analyze_render_plan(
      previous, frame, static_cast<int16_t>(display_->width()), static_cast<int16_t>(display_->height()),
      render_mode_, display_->capability_profile());

  if (previous != nullptr && frames_equal(*previous, frame)) {
    render_stats_.last_frame_time_us = 0;
    render_stats_.last_dirty_area_px = 0;
    render_stats_.last_frame_flushes = 0;
    render_stats_.last_frame_skipped = true;
    render_stats_.last_frame_full_redraw = last_render_plan_.full_redraw;
    ++render_stats_.skipped_duplicate_frames;
    return true;
  }

  const EyeGeometry left_eye = resolve_eye_geometry(frame, true);
  const EyeGeometry right_eye = resolve_eye_geometry(frame, false);
  const auto ocular_band = compute_fixed_band_rect(left_eye, right_eye, static_cast<int16_t>(display_->width()),
                                                   static_cast<int16_t>(display_->height()));
  const uint64_t frame_start_us = monotonic_time_us();
  uint16_t frame_flushes = 0;
  uint32_t updated_area_px = 0;

  display_->setRotation(1);
  display_->startWrite();

  bool used_partial_strategy = false;
  if (!last_render_plan_.full_redraw) {
    switch (ocular_update_pattern_) {
      case OcularUpdatePattern::kDirtyPerEye: {
        const auto flush_region = [this, &frame, &left_eye, &right_eye, &frame_flushes](
                                      const ncos::services::display::DirtyRect& region) -> bool {
          if (!region.valid || region.w <= 0 || region.h <= 0) {
            return true;
          }
          if (region.w > CompositeRegionMaxWidth || region.h > CompositeRegionMaxHeight) {
            return false;
          }

          const size_t pixel_count = static_cast<size_t>(region.w) * static_cast<size_t>(region.h);
          std::fill_n(composite_region_buffer_.data(), pixel_count, frame.face_color);

          const EyeGeometry eyes[] = {left_eye, right_eye};
          for (const auto& eye : eyes) {
            if (!rects_intersect(region, eye)) {
              continue;
            }

            const int16_t x0 = max_i16(region.x, eye.x);
            const int16_t y0 = max_i16(region.y, eye.y);
            const int16_t x1 = min_i16(static_cast<int16_t>(region.x + region.w),
                                       static_cast<int16_t>(eye.x + eye.w));
            const int16_t y1 = min_i16(static_cast<int16_t>(region.y + region.h),
                                       static_cast<int16_t>(eye.y + eye.h));

            for (int16_t y = y0; y < y1; ++y) {
              const size_t row_offset = static_cast<size_t>(y - region.y) * static_cast<size_t>(region.w);
              for (int16_t x = x0; x < x1; ++x) {
                if (!point_inside_round_rect(x, y, eye)) {
                  continue;
                }
                composite_region_buffer_[row_offset + static_cast<size_t>(x - region.x)] = eye.color;
              }
            }
          }

          display_->pushImage(region.x, region.y, region.w, region.h, composite_region_buffer_.data());
          ++frame_flushes;
          return true;
        };

        used_partial_strategy = flush_region(last_render_plan_.dirty_rect) &&
                                flush_region(last_render_plan_.dirty_rect_secondary);
        if (used_partial_strategy) {
          updated_area_px = rect_area(last_render_plan_.dirty_rect) + rect_area(last_render_plan_.dirty_rect_secondary);
        }
        break;
      }

      case OcularUpdatePattern::kFixedBandComposite: {
        if (ocular_band.valid && ocular_band.w <= CompositeRegionMaxWidth && ocular_band.h <= CompositeRegionMaxHeight) {
          const size_t pixel_count = static_cast<size_t>(ocular_band.w) * static_cast<size_t>(ocular_band.h);
          std::fill_n(composite_region_buffer_.data(), pixel_count, frame.face_color);

          const EyeGeometry eyes[] = {left_eye, right_eye};
          for (const auto& eye : eyes) {
            if (!rects_intersect(ocular_band, eye)) {
              continue;
            }

            const int16_t x0 = max_i16(ocular_band.x, eye.x);
            const int16_t y0 = max_i16(ocular_band.y, eye.y);
            const int16_t x1 = min_i16(static_cast<int16_t>(ocular_band.x + ocular_band.w),
                                       static_cast<int16_t>(eye.x + eye.w));
            const int16_t y1 = min_i16(static_cast<int16_t>(ocular_band.y + ocular_band.h),
                                       static_cast<int16_t>(eye.y + eye.h));

            for (int16_t y = y0; y < y1; ++y) {
              const size_t row_offset = static_cast<size_t>(y - ocular_band.y) * static_cast<size_t>(ocular_band.w);
              for (int16_t x = x0; x < x1; ++x) {
                if (!point_inside_round_rect(x, y, eye)) {
                  continue;
                }
                composite_region_buffer_[row_offset + static_cast<size_t>(x - ocular_band.x)] = eye.color;
              }
            }
          }

          display_->pushImage(ocular_band.x, ocular_band.y, ocular_band.w, ocular_band.h, composite_region_buffer_.data());
          ++frame_flushes;
          updated_area_px = rect_area(ocular_band);
          used_partial_strategy = true;
        }
        break;
      }

      case OcularUpdatePattern::kFixedBandRedraw:
        if (ocular_band.valid) {
          display_->fillRect(ocular_band.x, ocular_band.y, ocular_band.w, ocular_band.h, frame.face_color);
          ++frame_flushes;
          display_->fillRoundRect(left_eye.x, left_eye.y, left_eye.w, left_eye.h, left_eye.corner, frame.eye_color);
          ++frame_flushes;
          display_->fillRoundRect(right_eye.x, right_eye.y, right_eye.w, right_eye.h, right_eye.corner, frame.eye_color);
          ++frame_flushes;
          updated_area_px = rect_area(ocular_band);
          used_partial_strategy = true;
        }
        break;

      default:
        break;
    }
  }

  if (!used_partial_strategy) {
    if (last_render_plan_.full_redraw) {
      display_->fillScreen(frame.background);
      ++frame_flushes;

      if (frame.face_color != frame.background && frame.head_w > 0 && frame.head_h > 0) {
        display_->fillRoundRect(frame.head_x, frame.head_y, frame.head_w, frame.head_h,
                                frame.head_radius, frame.face_color);
        ++frame_flushes;
      }
    } else {
      if (last_render_plan_.dirty_rect.valid) {
        display_->fillRect(last_render_plan_.dirty_rect.x, last_render_plan_.dirty_rect.y,
                           last_render_plan_.dirty_rect.w, last_render_plan_.dirty_rect.h,
                           frame.face_color);
        ++frame_flushes;
      }
      if (last_render_plan_.dirty_rect_secondary.valid) {
        display_->fillRect(last_render_plan_.dirty_rect_secondary.x,
                           last_render_plan_.dirty_rect_secondary.y,
                           last_render_plan_.dirty_rect_secondary.w,
                           last_render_plan_.dirty_rect_secondary.h,
                           frame.face_color);
        ++frame_flushes;
      }
      updated_area_px = rect_area(last_render_plan_.dirty_rect) + rect_area(last_render_plan_.dirty_rect_secondary);
    }

    display_->fillRoundRect(left_eye.x, left_eye.y, left_eye.w, left_eye.h, left_eye.corner, frame.eye_color);
    ++frame_flushes;
    display_->fillRoundRect(right_eye.x, right_eye.y, right_eye.w, right_eye.h, right_eye.corner, frame.eye_color);
    ++frame_flushes;
  }

  display_->endWrite();

  const uint64_t frame_end_us = monotonic_time_us();
  const uint32_t frame_time_us = static_cast<uint32_t>(frame_end_us - frame_start_us);

  render_stats_.last_frame_time_us = frame_time_us;
  render_stats_.last_dirty_area_px = updated_area_px;
  render_stats_.avg_dirty_area_px = render_stats_.rendered_frames == 0
                                        ? updated_area_px
                                        : static_cast<uint32_t>((render_stats_.avg_dirty_area_px * 7u + updated_area_px) / 8u);
  render_stats_.last_frame_flushes = frame_flushes;
  render_stats_.last_frame_skipped = false;
  render_stats_.last_frame_full_redraw = last_render_plan_.full_redraw;
  render_stats_.peak_frame_time_us =
      frame_time_us > render_stats_.peak_frame_time_us ? frame_time_us : render_stats_.peak_frame_time_us;
  render_stats_.avg_frame_time_us = render_stats_.rendered_frames == 0
                                        ? frame_time_us
                                        : static_cast<uint32_t>((render_stats_.avg_frame_time_us * 7u + frame_time_us) / 8u);
  ++render_stats_.rendered_frames;

  previous_frame_ = frame;
  has_previous_frame_ = true;
  return true;
}

void FaceDisplayRenderer::set_render_mode(ncos::services::display::DisplayRenderMode mode) {
  render_mode_ = mode;
}

void FaceDisplayRenderer::set_ocular_update_pattern(OcularUpdatePattern pattern) {
  ocular_update_pattern_ = pattern;
}

OcularUpdatePattern FaceDisplayRenderer::ocular_update_pattern() const {
  return ocular_update_pattern_;
}

ncos::services::display::DisplayRenderPlan FaceDisplayRenderer::last_render_plan() const {
  return last_render_plan_;
}

FaceRenderStats FaceDisplayRenderer::render_stats() const {
  return render_stats_;
}

}  // namespace ncos::services::face

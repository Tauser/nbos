#pragma once

#include "drivers/display/display_driver.hpp"
#include "services/face/face_frame.hpp"

namespace ncos::services::face {

class FaceDisplayRenderer final {
 public:
  bool bind(ncos::drivers::display::DisplayDriver* display);
  bool render(const FaceFrame& frame) const;

 private:
  ncos::drivers::display::DisplayDriver* display_ = nullptr;
};

}  // namespace ncos::services::face

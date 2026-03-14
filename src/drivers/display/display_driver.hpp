#pragma once

#include <LovyanGFX.hpp>

namespace ncos::drivers::display {

class DisplayDriver final : public lgfx::LGFX_Device {
 public:
  DisplayDriver();

 private:
  lgfx::Panel_ST7789 panel_;
  lgfx::Bus_SPI bus_;
};

}  // namespace ncos::drivers::display

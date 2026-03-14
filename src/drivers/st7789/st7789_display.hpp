#pragma once

#include <LovyanGFX.hpp>

namespace ncos::drivers::display {

class St7789Display final : public lgfx::LGFX_Device {
 public:
  St7789Display();

 private:
  lgfx::Panel_ST7789 panel_;
  lgfx::Bus_SPI bus_;
};

}  // namespace ncos::drivers::display

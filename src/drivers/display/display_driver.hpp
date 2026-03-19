#pragma once

#include <LovyanGFX.hpp>

#include "drivers/display/panel_capability_profile.hpp"

namespace ncos::drivers::display {

class DisplayDriver final : public lgfx::LGFX_Device {
 public:
  DisplayDriver();
  const DisplayPanelCapabilityProfile& capability_profile() const;
  void set_write_clock_hz(uint32_t hz);
  uint32_t write_clock_hz() const;

 private:
  lgfx::Panel_ST7789 panel_;
  lgfx::Bus_SPI bus_;
};

}  // namespace ncos::drivers::display

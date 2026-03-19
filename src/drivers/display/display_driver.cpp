#include "drivers/display/display_driver.hpp"

namespace ncos::drivers::display {

DisplayDriver::DisplayDriver() {
  const auto& bsp = active_display_platform_bsp();
  const auto& profile = *bsp.profile;

  {
    auto cfg = bus_.config();
    cfg.spi_host = bsp.bus_flags.spi_host;
    cfg.spi_mode = bsp.bus_flags.spi_mode;
    cfg.freq_write = profile.timing.spi_write_hz;
    cfg.freq_read = profile.timing.spi_read_hz;
    cfg.spi_3wire = bsp.bus_flags.spi_3wire;
    cfg.use_lock = bsp.bus_flags.use_lock;
    cfg.dma_channel = bsp.bus_flags.dma_channel;
    cfg.pin_sclk = bsp.bus_wiring.sclk;
    cfg.pin_mosi = bsp.bus_wiring.mosi;
    cfg.pin_miso = bsp.bus_wiring.miso;
    cfg.pin_dc = bsp.bus_wiring.dc;
    bus_.config(cfg);
    panel_.setBus(&bus_);
  }

  {
    auto cfg = panel_.config();
    cfg.pin_cs = bsp.panel_wiring.cs;
    cfg.pin_rst = bsp.panel_flags.reset_shared_with_enable ? -1 : bsp.panel_wiring.rst;
    cfg.pin_busy = bsp.panel_wiring.busy;

    cfg.memory_width = profile.width;
    cfg.memory_height = profile.height;
    cfg.panel_width = profile.width;
    cfg.panel_height = profile.height;

    cfg.offset_x = bsp.panel_flags.offset_x;
    cfg.offset_y = bsp.panel_flags.offset_y;
    cfg.offset_rotation = bsp.panel_flags.offset_rotation;

    cfg.dummy_read_pixel = bsp.panel_flags.dummy_read_pixel;
    cfg.dummy_read_bits = bsp.panel_flags.dummy_read_bits;
    cfg.readable = bsp.panel_flags.readable;
    cfg.invert = bsp.panel_flags.keep_panel_inverted;
    cfg.rgb_order = bsp.panel_flags.rgb_order;
    cfg.dlen_16bit = bsp.panel_flags.dlen_16bit;
    cfg.bus_shared = bsp.panel_flags.bus_shared;

    panel_.config(cfg);
  }

  setPanel(&panel_);
}

const DisplayPanelCapabilityProfile& DisplayDriver::capability_profile() const {
  return *active_display_platform_bsp().profile;
}

const DisplayPlatformBsp& DisplayDriver::platform_bsp() const {
  return active_display_platform_bsp();
}

void DisplayDriver::set_write_clock_hz(uint32_t hz) {
  bus_.setClock(hz);
}

uint32_t DisplayDriver::write_clock_hz() const {
  return bus_.getClock();
}

}  // namespace ncos::drivers::display

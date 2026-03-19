#include "drivers/display/display_driver.hpp"

#include "config/pins/board_pins.hpp"

namespace ncos::drivers::display {

DisplayDriver::DisplayDriver() {
  const auto& profile = active_panel_capability_profile();

  {
    auto cfg = bus_.config();
    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = profile.timing.spi_write_hz;
    cfg.freq_read = profile.timing.spi_read_hz;
    cfg.spi_3wire = false;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = ncos::config::pins::kDisplaySck;
    cfg.pin_mosi = ncos::config::pins::kDisplayMosi;
    cfg.pin_miso = -1;
    cfg.pin_dc = ncos::config::pins::kDisplayDc;
    bus_.config(cfg);
    panel_.setBus(&bus_);
  }

  {
    auto cfg = panel_.config();
    cfg.pin_cs = ncos::config::pins::kDisplayCs;
    cfg.pin_rst = profile.reset_shared_with_enable ? -1 : ncos::config::pins::kDisplayRst;
    cfg.pin_busy = -1;

    cfg.memory_width = profile.width;
    cfg.memory_height = profile.height;
    cfg.panel_width = profile.width;
    cfg.panel_height = profile.height;

    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;

    cfg.dummy_read_pixel = 8;
    cfg.dummy_read_bits = 1;
    cfg.readable = profile.readable;
    cfg.invert = profile.workarounds.keep_panel_inverted;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;

    panel_.config(cfg);
  }

  setPanel(&panel_);
}

const DisplayPanelCapabilityProfile& DisplayDriver::capability_profile() const {
  return active_panel_capability_profile();
}

void DisplayDriver::set_write_clock_hz(uint32_t hz) {
  bus_.setClock(hz);
}

uint32_t DisplayDriver::write_clock_hz() const {
  return bus_.getClock();
}

}  // namespace ncos::drivers::display

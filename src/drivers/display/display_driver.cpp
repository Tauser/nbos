#include "drivers/display/display_driver.hpp"

#include "config/pins/board_pins.hpp"

namespace ncos::drivers::display {

DisplayDriver::DisplayDriver() {
  {
    auto cfg = bus_.config();
    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 40000000;
    cfg.freq_read = 16000000;
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
    cfg.pin_rst = -1;
    cfg.pin_busy = -1;

    cfg.memory_width = 240;
    cfg.memory_height = 320;
    cfg.panel_width = 240;
    cfg.panel_height = 320;

    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;

    cfg.dummy_read_pixel = 8;
    cfg.dummy_read_bits = 1;
    cfg.readable = false;
    cfg.invert = true;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;

    panel_.config(cfg);
  }

  setPanel(&panel_);
}

}  // namespace ncos::drivers::display


#include "drivers/display/display_platform_bsp.hpp"

#include "config/pins/board_pins.hpp"

namespace {

ncos::drivers::display::DisplayPlatformBsp make_bsp() {
  const auto& profile = ncos::drivers::display::active_panel_capability_profile();

  ncos::drivers::display::DisplayPlatformBsp bsp{};
  bsp.board_name = ncos::config::kBoardName;
  bsp.profile = &profile;

  bsp.bus_wiring.mosi = static_cast<int8_t>(ncos::config::pins::kDisplayMosi);
  bsp.bus_wiring.miso = -1;
  bsp.bus_wiring.sclk = static_cast<int8_t>(ncos::config::pins::kDisplaySck);
  bsp.bus_wiring.dc = static_cast<int8_t>(ncos::config::pins::kDisplayDc);

  bsp.panel_wiring.cs = static_cast<int8_t>(ncos::config::pins::kDisplayCs);
  bsp.panel_wiring.rst = static_cast<int8_t>(ncos::config::pins::kDisplayRst);
  bsp.panel_wiring.busy = -1;

#ifdef SPI2_HOST
  bsp.bus_flags.spi_host = SPI2_HOST;
#else
  bsp.bus_flags.spi_host = 1;
#endif
  bsp.bus_flags.spi_mode = 0;
  bsp.bus_flags.spi_3wire = false;
  bsp.bus_flags.use_lock = true;
#ifdef SPI_DMA_CH_AUTO
  bsp.bus_flags.dma_channel = SPI_DMA_CH_AUTO;
#else
  bsp.bus_flags.dma_channel = 0;
#endif

  bsp.panel_flags.readable = profile.readable;
  bsp.panel_flags.keep_panel_inverted = profile.workarounds.keep_panel_inverted;
  bsp.panel_flags.rgb_order = false;
  bsp.panel_flags.dlen_16bit = false;
  bsp.panel_flags.bus_shared = false;
  bsp.panel_flags.dummy_read_pixel = 8;
  bsp.panel_flags.dummy_read_bits = 1;
  bsp.panel_flags.offset_x = 0;
  bsp.panel_flags.offset_y = 0;
  bsp.panel_flags.offset_rotation = 0;
  bsp.panel_flags.reset_shared_with_enable = profile.reset_shared_with_enable;

  return bsp;
}

}  // namespace

namespace ncos::drivers::display {

const DisplayPlatformBsp& active_display_platform_bsp() {
  static const DisplayPlatformBsp display_bsp = make_bsp();
  return display_bsp;
}

}  // namespace ncos::drivers::display

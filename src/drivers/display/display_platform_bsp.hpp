#pragma once

#include <stdint.h>

#include "drivers/display/panel_capability_profile.hpp"

namespace ncos::drivers::display {

struct DisplayBusWiring {
  int8_t mosi = -1;
  int8_t miso = -1;
  int8_t sclk = -1;
  int8_t dc = -1;
};

struct DisplayPanelWiring {
  int8_t cs = -1;
  int8_t rst = -1;
  int8_t busy = -1;
};

struct DisplayBusInitFlags {
  int spi_host = 0;
  uint8_t spi_mode = 0;
  bool spi_3wire = false;
  bool use_lock = true;
  int dma_channel = 0;
};

struct DisplayPanelInitFlags {
  bool readable = false;
  bool keep_panel_inverted = false;
  bool rgb_order = false;
  bool dlen_16bit = false;
  bool bus_shared = false;
  uint8_t dummy_read_pixel = 0;
  uint8_t dummy_read_bits = 0;
  int16_t offset_x = 0;
  int16_t offset_y = 0;
  int8_t offset_rotation = 0;
  bool reset_shared_with_enable = false;
};

struct DisplayPlatformBsp {
  const char* board_name = "unknown";
  const DisplayPanelCapabilityProfile* profile = nullptr;
  DisplayBusWiring bus_wiring{};
  DisplayPanelWiring panel_wiring{};
  DisplayBusInitFlags bus_flags{};
  DisplayPanelInitFlags panel_flags{};
};

const DisplayPlatformBsp& active_display_platform_bsp();

}  // namespace ncos::drivers::display

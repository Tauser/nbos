#pragma once

#include <stdint.h>

namespace ncos::drivers::ttlinker {

struct TtlinkerUartWiring {
  int tx_gpio = -1;
  int rx_gpio = -1;
};

struct TtlinkerUartConfig {
  int port = 0;
  int baud_rate = 115200;
  int rx_buffer_size = 512;
  int tx_buffer_size = 512;
  uint8_t data_bits = 8;
  uint8_t stop_bits = 1;
};

struct TtlinkerTransportFlags {
  bool console_conflict = false;
  bool probe_allowed = true;
};

struct TtlinkerTransportBsp {
  const char* board_name = "unknown";
  TtlinkerUartWiring wiring{};
  TtlinkerUartConfig uart{};
  TtlinkerTransportFlags flags{};
};

const TtlinkerTransportBsp& active_ttlinker_transport_bsp();

}  // namespace ncos::drivers::ttlinker

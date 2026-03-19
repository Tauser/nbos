#include "drivers/ttlinker/ttlinker_transport_bsp.hpp"

#include "config/pins/board_pins.hpp"

#if defined(ESP_PLATFORM)
#include "driver/uart.h"
#include "sdkconfig.h"
#endif

namespace {

bool console_conflicts_with_ttlinker() {
#if defined(ESP_PLATFORM)
#if defined(CONFIG_ESP_CONSOLE_UART) && defined(CONFIG_ESP_CONSOLE_UART_NUM)
  if (CONFIG_ESP_CONSOLE_UART_NUM == 1) {
#if defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    constexpr int tx_gpio = CONFIG_ESP_CONSOLE_UART_TX_GPIO;
    constexpr int rx_gpio = CONFIG_ESP_CONSOLE_UART_RX_GPIO;
    return tx_gpio == ncos::config::pins::kServoTx || tx_gpio == ncos::config::pins::kServoRx ||
           rx_gpio == ncos::config::pins::kServoTx || rx_gpio == ncos::config::pins::kServoRx;
#else
    // Conservative guard: this board profile already documents UART1 console/TTLinker risk.
    return true;
#endif
  }
#endif
#endif
  return false;
}

ncos::drivers::ttlinker::TtlinkerTransportBsp make_transport_bsp() {
  ncos::drivers::ttlinker::TtlinkerTransportBsp bsp{};
  bsp.board_name = ncos::config::kBoardName;
  bsp.wiring.tx_gpio = ncos::config::pins::kServoTx;
  bsp.wiring.rx_gpio = ncos::config::pins::kServoRx;
#if defined(ESP_PLATFORM)
  bsp.uart.port = static_cast<int>(UART_NUM_1);
#else
  bsp.uart.port = 1;
#endif
  bsp.uart.baud_rate = 115200;
  bsp.uart.rx_buffer_size = 512;
  bsp.uart.tx_buffer_size = 512;
  bsp.uart.data_bits = 8;
  bsp.uart.stop_bits = 1;
  bsp.flags.console_conflict = console_conflicts_with_ttlinker();
  bsp.flags.probe_allowed = !bsp.flags.console_conflict;
  return bsp;
}

const ncos::drivers::ttlinker::TtlinkerTransportBsp TtlinkerBsp = make_transport_bsp();

}  // namespace

namespace ncos::drivers::ttlinker {

const TtlinkerTransportBsp& active_ttlinker_transport_bsp() {
  return TtlinkerBsp;
}

}  // namespace ncos::drivers::ttlinker

#include "drivers/ttlinker/ttlinker_bringup.hpp"

#include "config/pins/board_pins.hpp"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace {
constexpr const char* kTag = "NCOS_TTLINKER";
constexpr uart_port_t kUartPort = UART_NUM_1;
constexpr int kBaudRate = 115200;
constexpr int kRxBufferSize = 512;
constexpr int kTxBufferSize = 512;
}

namespace ncos::drivers::ttlinker {

bool TtlinkerBringup::can_run_probe_with_console() const {
#if defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
  constexpr int tx_gpio = CONFIG_ESP_CONSOLE_UART_TX_GPIO;
  constexpr int rx_gpio = CONFIG_ESP_CONSOLE_UART_RX_GPIO;
  if (tx_gpio == ncos::config::pins::kServoTx || tx_gpio == ncos::config::pins::kServoRx ||
      rx_gpio == ncos::config::pins::kServoTx || rx_gpio == ncos::config::pins::kServoRx) {
    return false;
  }
#endif
  return true;
}

bool TtlinkerBringup::init() {
  if (ready_) {
    return true;
  }

  uart_config_t cfg{};
  cfg.baud_rate = kBaudRate;
  cfg.data_bits = UART_DATA_8_BITS;
  cfg.parity = UART_PARITY_DISABLE;
  cfg.stop_bits = UART_STOP_BITS_1;
  cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  cfg.source_clk = UART_SCLK_DEFAULT;

  if (uart_driver_install(kUartPort, kRxBufferSize, kTxBufferSize, 0, nullptr, 0) != ESP_OK) {
    ESP_LOGE(kTag, "uart_driver_install failed");
    return false;
  }

  if (uart_param_config(kUartPort, &cfg) != ESP_OK) {
    ESP_LOGE(kTag, "uart_param_config failed");
    uart_driver_delete(kUartPort);
    return false;
  }

  if (uart_set_pin(kUartPort, ncos::config::pins::kServoTx, ncos::config::pins::kServoRx,
                   UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
    ESP_LOGE(kTag, "uart_set_pin failed");
    uart_driver_delete(kUartPort);
    return false;
  }

  uart_flush_input(kUartPort);
  ready_ = true;
  return true;
}

bool TtlinkerBringup::send_frame(const uint8_t* frame, size_t len, size_t* out_written) const {
  if (!ready_ || frame == nullptr || len == 0) {
    return false;
  }

  const int written = uart_write_bytes(kUartPort, frame, len);
  if (out_written != nullptr) {
    *out_written = written > 0 ? static_cast<size_t>(written) : 0;
  }
  return written == static_cast<int>(len);
}

bool TtlinkerBringup::run_probe(int read_window_ms, TtlinkerProbeResult* out_result) const {
  if (!ready_ || out_result == nullptr || read_window_ms < 0) {
    return false;
  }

  TtlinkerProbeResult result{};
  const uint8_t probe_frame[] = {0xAA, 0x55, 0x00, 0xFF};
  (void)send_frame(probe_frame, sizeof(probe_frame), &result.bytes_written);
  result.tx_ok = result.bytes_written == sizeof(probe_frame);

  if (read_window_ms > 0) {
    vTaskDelay(pdMS_TO_TICKS(read_window_ms));
  }

  uint8_t rx_buf[32] = {0};
  const int read = uart_read_bytes(kUartPort, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(20));
  if (read > 0) {
    result.bytes_read = static_cast<size_t>(read);
    result.first_byte = rx_buf[0];
  }

  *out_result = result;
  return true;
}

void TtlinkerBringup::deinit() {
  if (!ready_) {
    return;
  }

  uart_driver_delete(kUartPort);
  ready_ = false;
}

}  // namespace ncos::drivers::ttlinker


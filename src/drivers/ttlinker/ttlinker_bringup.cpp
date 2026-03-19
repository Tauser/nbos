#include "drivers/ttlinker/ttlinker_bringup.hpp"

#include "drivers/ttlinker/ttlinker_transport_bsp.hpp"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace {
constexpr const char* kTag = "NCOS_TTLINKER";
}

namespace ncos::drivers::ttlinker {

bool TtlinkerBringup::can_run_probe_with_console() const {
  return active_ttlinker_transport_bsp().flags.probe_allowed;
}

bool TtlinkerBringup::init() {
  if (ready_) {
    return true;
  }

  const auto& transport = active_ttlinker_transport_bsp();
  const auto uart_port = static_cast<uart_port_t>(transport.uart.port);

  uart_config_t cfg{};
  cfg.baud_rate = transport.uart.baud_rate;
  cfg.data_bits = UART_DATA_8_BITS;
  cfg.parity = UART_PARITY_DISABLE;
  cfg.stop_bits = UART_STOP_BITS_1;
  cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  cfg.source_clk = UART_SCLK_DEFAULT;

  if (uart_driver_install(uart_port, transport.uart.rx_buffer_size, transport.uart.tx_buffer_size, 0, nullptr, 0) !=
      ESP_OK) {
    ESP_LOGE(kTag, "uart_driver_install failed");
    return false;
  }

  if (uart_param_config(uart_port, &cfg) != ESP_OK) {
    ESP_LOGE(kTag, "uart_param_config failed");
    uart_driver_delete(uart_port);
    return false;
  }

  if (uart_set_pin(uart_port, transport.wiring.tx_gpio, transport.wiring.rx_gpio, UART_PIN_NO_CHANGE,
                   UART_PIN_NO_CHANGE) != ESP_OK) {
    ESP_LOGE(kTag, "uart_set_pin failed");
    uart_driver_delete(uart_port);
    return false;
  }

  uart_flush_input(uart_port);
  ready_ = true;
  return true;
}

bool TtlinkerBringup::send_frame(const uint8_t* frame, size_t len, size_t* out_written) const {
  if (!ready_ || frame == nullptr || len == 0) {
    return false;
  }

  const auto uart_port = static_cast<uart_port_t>(active_ttlinker_transport_bsp().uart.port);
  const int written = uart_write_bytes(uart_port, frame, len);
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
  const auto uart_port = static_cast<uart_port_t>(active_ttlinker_transport_bsp().uart.port);
  const int read = uart_read_bytes(uart_port, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(20));
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

  uart_driver_delete(static_cast<uart_port_t>(active_ttlinker_transport_bsp().uart.port));
  ready_ = false;
}

}  // namespace ncos::drivers::ttlinker

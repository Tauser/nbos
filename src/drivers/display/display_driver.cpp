#include "drivers/display/display_driver.hpp"

#include "config/pins/board_freenove_esp32s3_cam.hpp"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
    cfg.pin_rst = ncos::config::pins::kDisplayRst;
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

bool DisplayDriver::begin() {
  if (ncos::config::pins::kDisplayRst >= 0) {
    const gpio_num_t rst_pin = static_cast<gpio_num_t>(ncos::config::pins::kDisplayRst);

    gpio_config_t io_cfg{};
    io_cfg.pin_bit_mask = (1ULL << static_cast<uint32_t>(rst_pin));
    io_cfg.mode = GPIO_MODE_OUTPUT;
    io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_cfg);

    // Pulso de reset fisico para reiniciar o ST7789 mesmo sem corte de energia.
    gpio_set_level(rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
  }

  return init();
}

}  // namespace ncos::drivers::display

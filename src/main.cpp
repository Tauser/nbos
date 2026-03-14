#include "app/boot/firmware_entrypoint.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
  ncos::app::boot::FirmwareEntrypoint entrypoint;
  entrypoint.run();

  while (true) {
    entrypoint.tick();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

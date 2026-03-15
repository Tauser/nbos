#include "app/boot/firmware_entrypoint.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
  ncos::app::boot::FirmwareEntrypoint entrypoint;
  entrypoint.run();

  constexpr uint32_t kMainTickMs = 20;

  while (true) {
    entrypoint.tick();
    vTaskDelay(pdMS_TO_TICKS(kMainTickMs));
  }
}

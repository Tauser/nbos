#include "app/boot/firmware_entrypoint.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
ncos::app::boot::FirmwareEntrypoint g_entrypoint;
}

extern "C" void app_main(void) {
  g_entrypoint.run();

  constexpr uint32_t kMainTickMs = 20;

  while (true) {
    g_entrypoint.tick();
    vTaskDelay(pdMS_TO_TICKS(kMainTickMs));
  }
}

#pragma once

#include "hal/platform/runtime_build_target.hpp"

#if !defined(NCOS_NATIVE_TESTS)
#include "esp_log.h"
#endif

#if defined(NCOS_NATIVE_TESTS)
#define NCOS_LOGE(...) ((void)0)
#define NCOS_LOGW(...) ((void)0)
#define NCOS_LOGI(...) ((void)0)
#else
#define NCOS_LOGE(...) ESP_LOGE(__VA_ARGS__)
#define NCOS_LOGW(...) ESP_LOGW(__VA_ARGS__)
#define NCOS_LOGI(...) ESP_LOGI(__VA_ARGS__)
#endif

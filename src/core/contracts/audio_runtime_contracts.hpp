#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::core::contracts {

struct AudioRuntimeState {
  bool initialized = false;
  bool output_ready = false;
  bool input_ready = false;
  bool last_capture_ok = false;
  int32_t last_peak_level = 0;
  size_t last_capture_samples = 0;
  uint32_t capture_success_total = 0;
  uint32_t capture_failure_total = 0;
  uint64_t last_capture_ms = 0;
};

constexpr AudioRuntimeState make_audio_runtime_baseline() {
  return AudioRuntimeState{};
}

constexpr bool is_ready_for_local_audio(const AudioRuntimeState& state) {
  return state.initialized && state.output_ready && state.input_ready;
}

constexpr bool is_degraded_audio(const AudioRuntimeState& state) {
  return state.initialized && (!state.output_ready || !state.input_ready);
}

}  // namespace ncos::core::contracts

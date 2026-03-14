#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::interfaces::audio {

class LocalAudioPort {
 public:
  virtual ~LocalAudioPort() = default;

  virtual bool ensure_output() = 0;
  virtual bool ensure_input() = 0;
  virtual bool play_tone(float frequency_hz, int duration_ms) = 0;
  virtual bool capture_peak_level(int duration_ms, int32_t* out_peak, size_t* out_samples) = 0;
};

}  // namespace ncos::interfaces::audio

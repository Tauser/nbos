#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::drivers::audio {

class AudioBringup final {
 public:
  bool init_output();
  bool init_input();

  bool play_tone(float frequency_hz, int duration_ms);
  bool capture_peak_level(int duration_ms, int32_t* out_peak, size_t* out_samples);

  void deinit();

 private:
  bool output_ready_ = false;
  bool input_ready_ = false;
};

}  // namespace ncos::drivers::audio

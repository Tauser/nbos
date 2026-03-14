#include "drivers/audio/audio_local_port.hpp"

namespace ncos::drivers::audio {

bool AudioLocalPort::ensure_output() {
  return bringup_.init_output();
}

bool AudioLocalPort::ensure_input() {
  return bringup_.init_input();
}

bool AudioLocalPort::play_tone(float frequency_hz, int duration_ms) {
  return bringup_.play_tone(frequency_hz, duration_ms);
}

bool AudioLocalPort::capture_peak_level(int duration_ms, int32_t* out_peak, size_t* out_samples) {
  return bringup_.capture_peak_level(duration_ms, out_peak, out_samples);
}

ncos::interfaces::audio::LocalAudioPort* acquire_shared_local_audio_port() {
  static AudioLocalPort shared{};
  return &shared;
}

}  // namespace ncos::drivers::audio

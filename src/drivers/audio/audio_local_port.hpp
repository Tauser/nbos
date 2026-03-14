#pragma once

#include "drivers/audio/audio_bringup.hpp"
#include "interfaces/audio/local_audio_port.hpp"

namespace ncos::drivers::audio {

class AudioLocalPort final : public ncos::interfaces::audio::LocalAudioPort {
 public:
  bool ensure_output() override;
  bool ensure_input() override;
  bool play_tone(float frequency_hz, int duration_ms) override;
  bool capture_peak_level(int duration_ms, int32_t* out_peak, size_t* out_samples) override;

 private:
  AudioBringup bringup_{};
};

ncos::interfaces::audio::LocalAudioPort* acquire_shared_local_audio_port();

}  // namespace ncos::drivers::audio

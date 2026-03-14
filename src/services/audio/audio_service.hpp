#pragma once

#include <stdint.h>

#include "core/contracts/audio_runtime_contracts.hpp"
#include "interfaces/audio/local_audio_port.hpp"

namespace ncos::services::audio {

class AudioService final {
 public:
  AudioService() = default;

  void bind_port(ncos::interfaces::audio::LocalAudioPort* port);
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms);
  [[nodiscard]] const ncos::core::contracts::AudioRuntimeState& state() const;

 private:
  ncos::interfaces::audio::LocalAudioPort* port_ = nullptr;
  ncos::core::contracts::AudioRuntimeState state_ =
      ncos::core::contracts::make_audio_runtime_baseline();
  uint64_t next_probe_ms_ = 0;
};

}  // namespace ncos::services::audio

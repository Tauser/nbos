#include <unity.h>

#include "services/audio/audio_service.hpp"

// Native tests run with test_build_src = no.
#include "services/audio/audio_service.cpp"

namespace {

class FakeAudioPort final : public ncos::interfaces::audio::LocalAudioPort {
 public:
  bool ensure_output() override {
    return output_ready;
  }

  bool ensure_input() override {
    return input_ready;
  }

  bool play_tone(float frequency_hz, int duration_ms) override {
    ++play_calls;
    last_frequency_hz = frequency_hz;
    last_duration_ms = duration_ms;
    return play_ok;
  }

  bool capture_peak_level(int, int32_t* out_peak, size_t* out_samples) override {
    ++capture_calls;
    if (!capture_ok) {
      return false;
    }

    if (out_peak != nullptr) {
      *out_peak = peak_value;
    }
    if (out_samples != nullptr) {
      *out_samples = sample_count;
    }
    return true;
  }

  bool output_ready = true;
  bool input_ready = true;
  bool play_ok = true;
  bool capture_ok = true;
  int32_t peak_value = 1234;
  size_t sample_count = 512;
  uint32_t capture_calls = 0;
  uint32_t play_calls = 0;
  float last_frequency_hz = 0.0F;
  int last_duration_ms = 0;
};

}  // namespace

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_audio_service_initializes_and_marks_degraded_when_input_missing() {
  FakeAudioPort fake{};
  fake.input_ready = false;

  ncos::services::audio::AudioService service;
  service.bind_port(&fake);

  TEST_ASSERT_TRUE(service.initialize(100));
  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.initialized);
  TEST_ASSERT_TRUE(state.output_ready);
  TEST_ASSERT_FALSE(state.input_ready);
  TEST_ASSERT_TRUE(ncos::core::contracts::is_degraded_audio(state));
}

void test_audio_service_tick_captures_peak_on_probe_window() {
  FakeAudioPort fake{};

  ncos::services::audio::AudioService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(100));

  service.tick(100);
  const auto& first = service.state();
  TEST_ASSERT_TRUE(first.last_capture_ok);
  TEST_ASSERT_EQUAL_INT32(1234, first.last_peak_level);
  TEST_ASSERT_EQUAL_UINT32(1, first.capture_success_total);

  service.tick(200);
  TEST_ASSERT_EQUAL_UINT32(1, fake.capture_calls);

  service.tick(2000);
  TEST_ASSERT_EQUAL_UINT32(2, fake.capture_calls);
}

void test_audio_service_play_tone_uses_bound_output_port() {
  FakeAudioPort fake{};

  ncos::services::audio::AudioService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(100));

  TEST_ASSERT_TRUE(service.play_tone(880.0F, 90));
  TEST_ASSERT_EQUAL_UINT32(1, fake.play_calls);
  TEST_ASSERT_FLOAT_WITHIN(0.1F, 880.0F, fake.last_frequency_hz);
  TEST_ASSERT_EQUAL_INT(90, fake.last_duration_ms);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_audio_service_initializes_and_marks_degraded_when_input_missing);
  RUN_TEST(test_audio_service_tick_captures_peak_on_probe_window);
  RUN_TEST(test_audio_service_play_tone_uses_bound_output_port);
  return UNITY_END();
}

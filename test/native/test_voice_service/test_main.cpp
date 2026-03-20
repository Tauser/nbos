#include <unity.h>

#include "services/voice/voice_service.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "services/voice/voice_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_voice_runtime_baseline_uses_mvp_local_pipeline_policy() {
  const auto state = ncos::core::contracts::make_voice_runtime_baseline();

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceWakeMode::EnergyTrigger),
                        static_cast<int>(state.policy.wake_mode));
  TEST_ASSERT_TRUE(state.policy.touch_assist_enabled);
  TEST_ASSERT_FALSE(state.policy.wake_word_required);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceAsrMode::ConstrainedIntent),
                        static_cast<int>(state.policy.asr_mode));
  TEST_ASSERT_FALSE(state.policy.partial_transcript_required);
  TEST_ASSERT_FALSE(state.policy.cloud_asr_allowed);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceTtsMode::EarconFirst),
                        static_cast<int>(state.policy.tts_mode));
  TEST_ASSERT_FALSE(state.policy.natural_tts_required);
  TEST_ASSERT_FALSE(state.policy.cloud_tts_allowed);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(ncos::core::contracts::VoiceLatencyPriority::TriggerResponsiveness),
      static_cast<int>(state.policy.primary_priority));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(ncos::core::contracts::VoiceLatencyPriority::ShortTurnUtility),
      static_cast<int>(state.policy.secondary_priority));
}

void test_voice_service_enters_listening_with_speech_energy() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 1000));

  ncos::core::contracts::AudioRuntimeState audio{};
  audio.initialized = true;
  audio.input_ready = true;
  audio.last_capture_ok = true;
  audio.last_capture_samples = 512;
  audio.last_peak_level = 9000;

  ncos::core::contracts::CompanionSnapshot snapshot{};

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};
  TEST_ASSERT_TRUE(service.tick(audio, snapshot, 1100, &attention, &interaction));

  const auto& state = service.state();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::Listening),
                        static_cast<int>(state.stage));
  TEST_ASSERT_TRUE(state.speech_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kAuditory),
                        static_cast<int>(attention.channel));
}

void test_voice_service_raises_trigger_candidate_after_consecutive_frames() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 2000));

  ncos::core::contracts::AudioRuntimeState audio{};
  audio.initialized = true;
  audio.input_ready = true;
  audio.last_capture_ok = true;
  audio.last_capture_samples = 512;
  audio.last_peak_level = 13000;

  ncos::core::contracts::CompanionSnapshot snapshot{};

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_TRUE(service.tick(audio, snapshot, 2100, &attention, &interaction));
  TEST_ASSERT_TRUE(service.tick(audio, snapshot, 2200, &attention, &interaction));
  TEST_ASSERT_TRUE(service.tick(audio, snapshot, 2300, &attention, &interaction));

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.trigger_candidate);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::TriggerCandidate),
                        static_cast<int>(state.stage));
  TEST_ASSERT_TRUE(attention.lock_active);
  TEST_ASSERT_TRUE(interaction.response_pending);
}

void test_voice_service_dormant_in_safe_mode() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 3000));

  ncos::core::contracts::AudioRuntimeState audio{};
  audio.initialized = true;
  audio.input_ready = true;
  audio.last_capture_ok = true;
  audio.last_capture_samples = 512;
  audio.last_peak_level = 15000;

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.runtime.safe_mode = true;

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};
  TEST_ASSERT_FALSE(service.tick(audio, snapshot, 3100, &attention, &interaction));

  const auto& state = service.state();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::Dormant),
                        static_cast<int>(state.stage));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_voice_runtime_baseline_uses_mvp_local_pipeline_policy);
  RUN_TEST(test_voice_service_enters_listening_with_speech_energy);
  RUN_TEST(test_voice_service_raises_trigger_candidate_after_consecutive_frames);
  RUN_TEST(test_voice_service_dormant_in_safe_mode);
  return UNITY_END();
}

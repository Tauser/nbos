#include <unity.h>

#include "services/voice/voice_service.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "core/contracts/interaction_taxonomy.cpp"
#include "services/voice/voice_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

ncos::core::contracts::AudioRuntimeState make_voice_ready_audio(uint64_t capture_ms, int32_t peak = 9000,
                                                                size_t samples = 512) {
  ncos::core::contracts::AudioRuntimeState audio{};
  audio.initialized = true;
  audio.input_ready = true;
  audio.last_capture_ok = true;
  audio.last_capture_ms = capture_ms;
  audio.last_capture_samples = samples;
  audio.last_peak_level = peak;
  return audio;
}

void drive_trigger_candidate(ncos::services::voice::VoiceService* service,
                             const ncos::core::contracts::CompanionSnapshot& snapshot,
                             uint64_t start_ms, int32_t peak = 13000) {
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  auto audio = make_voice_ready_audio(start_ms - 10, peak, 512);
  TEST_ASSERT_TRUE(service->tick(audio, snapshot, start_ms, &attention, &interaction));
  audio.last_capture_ms = start_ms + 90;
  TEST_ASSERT_TRUE(service->tick(audio, snapshot, start_ms + 100, &attention, &interaction));
}

}  // namespace

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
}

void test_voice_service_enters_listening_with_fresh_speech_capture() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 1000));

  const auto audio = make_voice_ready_audio(1080, 9000, 512);
  ncos::core::contracts::CompanionSnapshot snapshot{};

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};
  TEST_ASSERT_TRUE(service.tick(audio, snapshot, 1100, &attention, &interaction));

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.input_available);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::Listening),
                        static_cast<int>(state.stage));
  TEST_ASSERT_TRUE(state.speech_active);
  TEST_ASSERT_EQUAL_UINT16(20, state.last_capture_age_ms);
}

void test_voice_service_raises_trigger_candidate_with_shorter_latency_budget() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 2000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  drive_trigger_candidate(&service, snapshot, 2100);

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.trigger_candidate);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::TriggerCandidate),
                        static_cast<int>(state.stage));
  TEST_ASSERT_TRUE(state.last_trigger_latency_ms <= 100);
  TEST_ASSERT_EQUAL_UINT16(state.last_trigger_latency_ms, state.last_response_ready_latency_ms);
}

void test_voice_service_requires_stronger_energy_for_fast_trigger() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 3000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  auto audio = make_voice_ready_audio(3090, 9300, 512);
  TEST_ASSERT_TRUE(service.tick(audio, snapshot, 3100, &attention, &interaction));
  audio.last_capture_ms = 3190;
  TEST_ASSERT_TRUE(service.tick(audio, snapshot, 3200, &attention, &interaction));

  const auto& state = service.state();
  TEST_ASSERT_FALSE(state.trigger_candidate);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::Listening),
                        static_cast<int>(state.stage));
}

void test_voice_service_maps_cold_trigger_to_attend_user_response() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 4000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  drive_trigger_candidate(&service, snapshot, 4100);

  ncos::core::contracts::VoiceResponsePlan response{};
  TEST_ASSERT_TRUE(service.take_response_plan(&response));
  TEST_ASSERT_TRUE(response.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IntentTopic::kAttendUser),
                        static_cast<int>(response.intent));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceResponseCue::WakeChirp),
                        static_cast<int>(response.cue));
  TEST_ASSERT_FLOAT_WITHIN(0.1F, 760.0F, response.tone_frequency_hz);
  TEST_ASSERT_EQUAL_INT(60, response.tone_duration_ms);
}

void test_voice_service_maps_warm_session_to_acknowledge_response() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 5000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.interactional.session_active = true;
  snapshot.session.warm = true;
  snapshot.session.last_turn_owner = ncos::core::contracts::TurnOwner::kUser;
  drive_trigger_candidate(&service, snapshot, 5100);

  ncos::core::contracts::VoiceResponsePlan response{};
  TEST_ASSERT_TRUE(service.take_response_plan(&response));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IntentTopic::kAcknowledgeUser),
                        static_cast<int>(response.intent));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceResponseCue::AcknowledgeChirp),
                        static_cast<int>(response.cue));
  TEST_ASSERT_FLOAT_WITHIN(0.1F, 980.0F, response.tone_frequency_hz);
  TEST_ASSERT_EQUAL_INT(75, response.tone_duration_ms);
}

void test_voice_service_maps_stimulus_context_to_inspect_response() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 6000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.session.recent_stimulus.target = ncos::core::contracts::AttentionTarget::kStimulus;
  drive_trigger_candidate(&service, snapshot, 6100);

  ncos::core::contracts::VoiceResponsePlan response{};
  TEST_ASSERT_TRUE(service.take_response_plan(&response));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IntentTopic::kInspectStimulus),
                        static_cast<int>(response.intent));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceResponseCue::StimulusChirp),
                        static_cast<int>(response.cue));
  TEST_ASSERT_FLOAT_WITHIN(0.1F, 560.0F, response.tone_frequency_hz);
  TEST_ASSERT_EQUAL_INT(105, response.tone_duration_ms);
}

void test_voice_service_maps_constrained_energy_to_soft_energy_response() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 7000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.energetic.mode = ncos::core::contracts::EnergyMode::kConstrained;
  drive_trigger_candidate(&service, snapshot, 7100);

  ncos::core::contracts::VoiceResponsePlan response{};
  TEST_ASSERT_TRUE(service.take_response_plan(&response));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IntentTopic::kPreserveEnergy),
                        static_cast<int>(response.intent));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceResponseCue::EnergySoftChirp),
                        static_cast<int>(response.cue));
  TEST_ASSERT_FLOAT_WITHIN(0.1F, 392.0F, response.tone_frequency_hz);
  TEST_ASSERT_EQUAL_INT(85, response.tone_duration_ms);
}

void test_voice_response_cues_stay_distinct_for_clarity() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 8000));

  ncos::core::contracts::CompanionSnapshot cold{};
  drive_trigger_candidate(&service, cold, 8100);
  ncos::core::contracts::VoiceResponsePlan wake{};
  TEST_ASSERT_TRUE(service.take_response_plan(&wake));

  ncos::core::contracts::CompanionSnapshot warm{};
  warm.session.warm = true;
  warm.session.last_turn_owner = ncos::core::contracts::TurnOwner::kUser;
  drive_trigger_candidate(&service, warm, 9600);
  ncos::core::contracts::VoiceResponsePlan ack{};
  TEST_ASSERT_TRUE(service.take_response_plan(&ack));

  ncos::core::contracts::CompanionSnapshot stimulus{};
  stimulus.session.recent_stimulus.target = ncos::core::contracts::AttentionTarget::kStimulus;
  drive_trigger_candidate(&service, stimulus, 11100);
  ncos::core::contracts::VoiceResponsePlan inspect{};
  TEST_ASSERT_TRUE(service.take_response_plan(&inspect));

  TEST_ASSERT_TRUE(static_cast<int>(ack.tone_frequency_hz - wake.tone_frequency_hz) >= 180);
  TEST_ASSERT_TRUE(static_cast<int>(wake.tone_frequency_hz - inspect.tone_frequency_hz) >= 180);
}

void test_voice_service_response_plan_is_one_shot() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 12000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  drive_trigger_candidate(&service, snapshot, 12100);

  ncos::core::contracts::VoiceResponsePlan response{};
  TEST_ASSERT_TRUE(service.take_response_plan(&response));
  TEST_ASSERT_FALSE(service.take_response_plan(&response));
}

void test_voice_service_ignores_stale_capture_snapshots() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 13000));

  const auto audio = make_voice_ready_audio(13000, 15000, 512);
  ncos::core::contracts::CompanionSnapshot snapshot{};

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};
  TEST_ASSERT_FALSE(service.tick(audio, snapshot, 13300, &attention, &interaction));

  const auto& state = service.state();
  TEST_ASSERT_FALSE(state.input_available);
  TEST_ASSERT_FALSE(state.trigger_candidate);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::Dormant),
                        static_cast<int>(state.stage));
}

void test_voice_service_ignores_too_small_capture_windows() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 14000));

  const auto audio = make_voice_ready_audio(14080, 15000, 48);
  ncos::core::contracts::CompanionSnapshot snapshot{};

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};
  TEST_ASSERT_FALSE(service.tick(audio, snapshot, 14100, &attention, &interaction));

  const auto& state = service.state();
  TEST_ASSERT_FALSE(state.input_available);
  TEST_ASSERT_FALSE(state.speech_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::Dormant),
                        static_cast<int>(state.stage));
}

void test_voice_service_dormant_in_safe_mode() {
  ncos::services::voice::VoiceService service;
  TEST_ASSERT_TRUE(service.initialize(64, 15000));

  const auto audio = make_voice_ready_audio(15080, 15000, 512);

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.runtime.safe_mode = true;

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};
  TEST_ASSERT_FALSE(service.tick(audio, snapshot, 15100, &attention, &interaction));

  const auto& state = service.state();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::VoiceStage::Dormant),
                        static_cast<int>(state.stage));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_voice_runtime_baseline_uses_mvp_local_pipeline_policy);
  RUN_TEST(test_voice_service_enters_listening_with_fresh_speech_capture);
  RUN_TEST(test_voice_service_raises_trigger_candidate_with_shorter_latency_budget);
  RUN_TEST(test_voice_service_requires_stronger_energy_for_fast_trigger);
  RUN_TEST(test_voice_service_maps_cold_trigger_to_attend_user_response);
  RUN_TEST(test_voice_service_maps_warm_session_to_acknowledge_response);
  RUN_TEST(test_voice_service_maps_stimulus_context_to_inspect_response);
  RUN_TEST(test_voice_service_maps_constrained_energy_to_soft_energy_response);
  RUN_TEST(test_voice_response_cues_stay_distinct_for_clarity);
  RUN_TEST(test_voice_service_response_plan_is_one_shot);
  RUN_TEST(test_voice_service_ignores_stale_capture_snapshots);
  RUN_TEST(test_voice_service_ignores_too_small_capture_windows);
  RUN_TEST(test_voice_service_dormant_in_safe_mode);
  return UNITY_END();
}

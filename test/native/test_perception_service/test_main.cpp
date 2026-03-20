#include <unity.h>

#include "services/vision/perception_service.hpp"

// Native tests run with test_build_src = no.
#include "services/vision/perception_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

static ncos::core::contracts::CameraRuntimeState make_visual_camera(uint64_t capture_ms) {
  ncos::core::contracts::CameraRuntimeState camera{};
  camera.initialized = true;
  camera.capture_ready = true;
  camera.last_capture_ok = true;
  camera.last_capture_ms = capture_ms;
  camera.last_frame_width = 320;
  camera.last_frame_height = 240;
  camera.last_frame_bytes = 1600;
  return camera;
}

void test_perception_detects_visual_presence_as_stimulus_attention_only() {
  ncos::services::vision::PerceptionService service;
  TEST_ASSERT_TRUE(service.initialize(65, 1000));

  ncos::core::contracts::AudioRuntimeState audio{};
  ncos::core::contracts::TouchRuntimeState touch{};
  ncos::core::contracts::CameraRuntimeState camera = make_visual_camera(1000);
  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 1100, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kVisual),
                        static_cast<int>(attention.channel));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kStimulus),
                        static_cast<int>(attention.target));
  TEST_ASSERT_FALSE(interaction.session_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kIdle),
                        static_cast<int>(interaction.phase));
  TEST_ASSERT_TRUE(service.state().presence_active);
  TEST_ASSERT_TRUE(service.state().visual_signal_active);
  TEST_ASSERT_FALSE(service.state().visual_user_inference_active);
  TEST_ASSERT_EQUAL_UINT8(70, service.state().visual_presence_confidence_percent);
}

void test_perception_promotes_stable_visual_presence_to_basic_user_attention() {
  ncos::services::vision::PerceptionService service;
  TEST_ASSERT_TRUE(service.initialize(65, 1200));

  ncos::core::contracts::AudioRuntimeState audio{};
  ncos::core::contracts::TouchRuntimeState touch{};
  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  ncos::core::contracts::CameraRuntimeState camera = make_visual_camera(1200);
  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 1260, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kStimulus),
                        static_cast<int>(attention.target));
  TEST_ASSERT_FALSE(service.state().visual_user_inference_active);

  camera.last_capture_ms = 1320;
  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 1380, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kVisual),
                        static_cast<int>(attention.channel));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kUser),
                        static_cast<int>(attention.target));
  TEST_ASSERT_TRUE(service.state().visual_user_inference_active);
  TEST_ASSERT_EQUAL_UINT16(2, service.state().consecutive_visual_frames);
  TEST_ASSERT_TRUE(interaction.session_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kListening),
                        static_cast<int>(interaction.phase));
}

void test_perception_holds_visual_user_attention_briefly_after_signal_softens() {
  ncos::services::vision::PerceptionService service;
  TEST_ASSERT_TRUE(service.initialize(65, 1400));

  ncos::core::contracts::AudioRuntimeState audio{};
  ncos::core::contracts::TouchRuntimeState touch{};
  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  ncos::core::contracts::CameraRuntimeState camera = make_visual_camera(1400);
  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 1460, &attention, &interaction));
  camera.last_capture_ms = 1520;
  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 1580, &attention, &interaction));
  TEST_ASSERT_TRUE(service.state().visual_user_inference_active);

  camera.last_frame_width = 120;
  camera.last_frame_height = 90;
  camera.last_frame_bytes = 400;
  camera.last_capture_ms = 1640;
  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 1700, &attention, &interaction));
  TEST_ASSERT_TRUE(service.state().visual_user_inference_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kUser),
                        static_cast<int>(attention.target));

  camera.last_capture_ms = 1640;
  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 1985, &attention, &interaction));
  TEST_ASSERT_FALSE(service.state().visual_user_inference_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kStimulus),
                        static_cast<int>(attention.target));
  TEST_ASSERT_FALSE(interaction.session_active);
}

void test_perception_prioritizes_touch_attention_when_touch_is_high() {
  ncos::services::vision::PerceptionService service;
  TEST_ASSERT_TRUE(service.initialize(65, 2000));

  ncos::core::contracts::AudioRuntimeState audio{};

  ncos::core::contracts::TouchRuntimeState touch{};
  touch.initialized = true;
  touch.last_read_ok = true;
  touch.trigger_active = true;
  touch.normalized_level = 780;

  ncos::core::contracts::CameraRuntimeState camera = make_visual_camera(2000);
  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 2100, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kTouch),
                        static_cast<int>(attention.channel));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kUser),
                        static_cast<int>(attention.target));
  TEST_ASSERT_TRUE(attention.lock_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kActing),
                        static_cast<int>(interaction.phase));
}

void test_perception_keeps_auditory_user_path_distinct_from_visual_presence() {
  ncos::services::vision::PerceptionService service;
  TEST_ASSERT_TRUE(service.initialize(65, 2300));

  ncos::core::contracts::AudioRuntimeState audio{};
  audio.initialized = true;
  audio.input_ready = true;
  audio.last_capture_ok = true;
  audio.last_capture_samples = 512;
  audio.last_peak_level = 26000;

  ncos::core::contracts::TouchRuntimeState touch{};
  ncos::core::contracts::CameraRuntimeState camera = make_visual_camera(1000);
  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 2400, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kAuditory),
                        static_cast<int>(attention.channel));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kUser),
                        static_cast<int>(attention.target));
  TEST_ASSERT_TRUE(interaction.session_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kListening),
                        static_cast<int>(interaction.phase));
}

void test_perception_emits_idle_transition_when_touch_releases() {
  ncos::services::vision::PerceptionService service;
  TEST_ASSERT_TRUE(service.initialize(65, 2500));

  ncos::core::contracts::AudioRuntimeState audio{};
  ncos::core::contracts::CameraRuntimeState camera{};
  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  ncos::core::contracts::TouchRuntimeState touch_active{};
  touch_active.initialized = true;
  touch_active.last_read_ok = true;
  touch_active.trigger_active = true;
  touch_active.normalized_level = 760;

  TEST_ASSERT_TRUE(service.tick(audio, touch_active, camera, companion, 2520, &attention, &interaction));
  TEST_ASSERT_TRUE(service.state().attention_active);

  ncos::core::contracts::TouchRuntimeState touch_idle{};
  touch_idle.initialized = true;
  touch_idle.last_read_ok = true;
  touch_idle.normalized_level = 0;

  TEST_ASSERT_TRUE(service.tick(audio, touch_idle, camera, companion, 2580, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kTouch),
                        static_cast<int>(attention.channel));
  TEST_ASSERT_TRUE(attention.lock_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kActing),
                        static_cast<int>(interaction.phase));

  TEST_ASSERT_TRUE(service.tick(audio, touch_idle, camera, companion, 2800, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::PerceptionStage::Dormant),
                        static_cast<int>(service.state().stage));
  TEST_ASSERT_FALSE(service.state().presence_active);
  TEST_ASSERT_FALSE(service.state().attention_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kNone),
                        static_cast<int>(attention.target));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kIdle),
                        static_cast<int>(interaction.phase));
}

void test_perception_goes_dormant_on_critical_energy() {
  ncos::services::vision::PerceptionService service;
  TEST_ASSERT_TRUE(service.initialize(65, 3000));

  ncos::core::contracts::AudioRuntimeState audio{};
  ncos::core::contracts::TouchRuntimeState touch{};
  ncos::core::contracts::CameraRuntimeState camera{};

  ncos::core::contracts::CompanionSnapshot companion{};
  companion.energetic.mode = ncos::core::contracts::EnergyMode::kCritical;

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_FALSE(service.tick(audio, touch, camera, companion, 3200, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::PerceptionStage::Dormant),
                        static_cast<int>(service.state().stage));
  TEST_ASSERT_FALSE(service.state().presence_active);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_perception_detects_visual_presence_as_stimulus_attention_only);
  RUN_TEST(test_perception_promotes_stable_visual_presence_to_basic_user_attention);
  RUN_TEST(test_perception_holds_visual_user_attention_briefly_after_signal_softens);
  RUN_TEST(test_perception_prioritizes_touch_attention_when_touch_is_high);
  RUN_TEST(test_perception_keeps_auditory_user_path_distinct_from_visual_presence);
  RUN_TEST(test_perception_emits_idle_transition_when_touch_releases);
  RUN_TEST(test_perception_goes_dormant_on_critical_energy);
  return UNITY_END();
}

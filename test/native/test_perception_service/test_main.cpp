#include <unity.h>

#include "services/vision/perception_service.hpp"

// Native tests run with test_build_src = no.
#include "services/vision/perception_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_perception_detects_visual_presence_and_attention() {
  ncos::services::vision::PerceptionService service;
  TEST_ASSERT_TRUE(service.initialize(65, 1000));

  ncos::core::contracts::AudioRuntimeState audio{};
  ncos::core::contracts::TouchRuntimeState touch{};

  ncos::core::contracts::CameraRuntimeState camera{};
  camera.initialized = true;
  camera.capture_ready = true;
  camera.last_capture_ok = true;
  camera.last_capture_ms = 1000;
  camera.last_frame_width = 320;
  camera.last_frame_height = 240;
  camera.last_frame_bytes = 1600;

  ncos::core::contracts::CompanionSnapshot companion{};

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 1100, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kVisual),
                        static_cast<int>(attention.channel));
  TEST_ASSERT_TRUE(interaction.session_active);
  TEST_ASSERT_TRUE(service.state().presence_active);
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

  ncos::core::contracts::CameraRuntimeState camera{};

  ncos::core::contracts::CompanionSnapshot companion{};

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_TRUE(service.tick(audio, touch, camera, companion, 2100, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kTouch),
                        static_cast<int>(attention.channel));
  TEST_ASSERT_TRUE(attention.lock_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kActing),
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

  TEST_ASSERT_TRUE(service.tick(audio, touch_idle, camera, companion, 2640, &attention, &interaction));
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
  RUN_TEST(test_perception_detects_visual_presence_and_attention);
  RUN_TEST(test_perception_prioritizes_touch_attention_when_touch_is_high);
  RUN_TEST(test_perception_emits_idle_transition_when_touch_releases);
  RUN_TEST(test_perception_goes_dormant_on_critical_energy);
  return UNITY_END();
}

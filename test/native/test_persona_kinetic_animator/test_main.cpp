#include <unity.h>
#include "services/persona_face/models/persona_face_models.hpp"
#include "services/persona_face/models/persona_face_contracts.hpp"

// Native tests run with test_build_src = no.
#include "services/persona_face/persona_kinetic_animator.cpp"

using namespace ncos::services::persona_face;

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_initial_state_is_dirty_and_black(void) {
  PersonaKineticAnimator animator;
  PersonaRenderPayload payload = animator.get_current_frame();
  
  TEST_ASSERT_TRUE(payload.force_full_black_clear);
  TEST_ASSERT_TRUE(payload.frame_is_dirty);
  TEST_ASSERT_EQUAL_INT16(160, payload.left_current.x);
}

void test_play_transition_clears_black_flag(void) {
  PersonaKineticAnimator animator;
  PersonaExpressionTarget target;
  target.left_eye.x = 100;
  
  animator.play_transition(target);
  PersonaRenderPayload payload = animator.get_current_frame();
  
  TEST_ASSERT_FALSE(payload.force_full_black_clear);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_initial_state_is_dirty_and_black);
  RUN_TEST(test_play_transition_clears_black_flag);
  return UNITY_END();
}

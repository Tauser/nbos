#pragma once

#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "models/face/face_clip.hpp"
#include "services/face/face_compositor.hpp"

namespace ncos::services::face {

enum class FaceClipPlayerPhase : uint8_t {
  kIdle = 0,
  kPlaying = 1,
  kRecovering = 2,
};

class FaceClipPlayer final {
 public:
  explicit FaceClipPlayer(uint16_t owner_service = 0);

  void bind_owner(uint16_t owner_service);
  bool play(const ncos::models::face::FaceClip& clip,
            FaceCompositor* compositor,
            ncos::core::contracts::FaceRenderState* state,
            uint64_t now_ms);
  bool cancel(uint64_t now_ms, ncos::core::contracts::FaceRenderState* state);
  bool tick(uint64_t now_ms,
            FaceCompositor* compositor,
            ncos::core::contracts::FaceRenderState* state);

  bool active() const;
  bool recovering() const;
  FaceClipPlayerPhase phase() const;
  uint32_t active_clip_id() const;

 private:
  struct PoseSnapshot {
    ncos::models::face::GazeAnchor anchor = ncos::models::face::GazeAnchor::kCenter;
    ncos::models::face::GazeDirection direction = ncos::models::face::GazeDirection::kCenter;
    uint8_t focus_percent = 0;
    uint8_t lid_openness_percent = 100;
  };

  static uint8_t clamp_percent(uint8_t value);
  static uint8_t lerp_u8(uint8_t from, uint8_t to, uint64_t num, uint64_t den);

  void snapshot_from_state(PoseSnapshot* out, const ncos::core::contracts::FaceRenderState& state) const;
  void apply_pose(const ncos::models::face::FaceClipPose& pose,
                  ncos::core::contracts::FaceRenderState* state,
                  uint64_t now_ms) const;
  void apply_snapshot(const PoseSnapshot& pose,
                      ncos::core::contracts::FaceRenderState* state,
                      uint64_t now_ms) const;
  const ncos::models::face::FaceClipKeyframe* current_keyframe(uint64_t elapsed_ms) const;

  bool begin_recovery(uint64_t now_ms, ncos::core::contracts::FaceRenderState* state);
  void finish_and_release(FaceCompositor* compositor,
                          ncos::core::contracts::FaceRenderState* state,
                          uint64_t now_ms);

  uint16_t owner_service_ = 0;
  const ncos::models::face::FaceClip* active_clip_ = nullptr;
  FaceClipPlayerPhase phase_ = FaceClipPlayerPhase::kIdle;

  uint64_t clip_start_ms_ = 0;
  uint64_t recovery_start_ms_ = 0;
  uint64_t recovery_end_ms_ = 0;

  PoseSnapshot baseline_pose_{};
  PoseSnapshot recovery_from_{};
};

}  // namespace ncos::services::face


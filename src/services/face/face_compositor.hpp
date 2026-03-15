#pragma once

#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"

namespace ncos::services::face {

enum class FaceComposeRejectReason : uint8_t {
  kNone = 0,
  kInvalidState = 1,
  kOwnershipMismatch = 2,
  kHoldActive = 3,
  kCooldownActive = 4,
  kClaimRejected = 5,
};

struct FaceLayerTimingPolicy {
  uint32_t default_hold_ms = 0;
  uint32_t default_cooldown_ms = 0;
};

struct FaceLayerRequest {
  ncos::models::face::FaceLayer layer = ncos::models::face::FaceLayer::kBase;
  ncos::core::contracts::FaceLayerOwnerRole requester_role =
      ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
  uint16_t requester_service = 0;
  uint8_t priority = 0;
  uint32_t source_clip_id = 0;
  uint32_t hold_ms = 0;
  uint32_t cooldown_ms = 0;
};

struct FaceLayerDecision {
  bool granted = false;
  FaceComposeRejectReason reason = FaceComposeRejectReason::kNone;
  uint16_t active_owner_service = 0;
  uint64_t hold_until_ms = 0;
  uint64_t cooldown_until_ms = 0;
};

class FaceCompositor final {
 public:
  bool bind_state(ncos::core::contracts::FaceRenderState* state);

  FaceLayerDecision request_layer(const FaceLayerRequest& request, uint64_t now_ms);
  bool release_layer(ncos::models::face::FaceLayer layer,
                     uint16_t requester_service,
                     uint64_t now_ms,
                     uint32_t cooldown_ms = 0);
  bool can_write(ncos::models::face::FaceLayer layer, uint16_t requester_service) const;
  void tick(uint64_t now_ms);

  uint64_t hold_until_ms(ncos::models::face::FaceLayer layer) const;
  uint64_t cooldown_until_ms(ncos::models::face::FaceLayer layer) const;

 private:
  struct FaceLayerRuntime {
    uint64_t hold_until_ms = 0;
    uint64_t cooldown_until_ms = 0;
    uint16_t cooldown_blocked_service = 0;
  };

  static FaceLayerTimingPolicy default_timing_for(ncos::models::face::FaceLayer layer);

  ncos::core::contracts::FaceRenderState* state_ = nullptr;
  FaceLayerRuntime runtime_[ncos::models::face::kFaceLayerCount] = {};
};

}  // namespace ncos::services::face

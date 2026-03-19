#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::models::face {

inline constexpr size_t kFaceLayerCount = 6;

enum class FacePresetId : uint8_t {
  kNeutralBaseline = 1,
  kWarmWelcome = 2,
  kFocusedAttend = 3,
  kLowPowerCalm = 4,
};

enum class FaceShapeProfile : uint8_t {
  kCompanionBalanced = 1,
  kHeroicWide = 2,
  kCuriousTall = 3,
  kPlayfulRound = 4,
};

enum class GazeAnchor : uint8_t {
  kCenter = 1,
  kUser = 2,
  kStimulus = 3,
  kInternal = 4,
};

enum class GazeDirection : uint8_t {
  kCenter = 1,
  kLeft = 2,
  kRight = 3,
  kUp = 4,
  kDown = 5,
  kUpLeft = 6,
  kUpRight = 7,
  kDownLeft = 8,
  kDownRight = 9,
};

enum class BlinkPhase : uint8_t {
  kOpen = 1,
  kClosing = 2,
  kClosed = 3,
  kOpening = 4,
};

enum class MouthShape : uint8_t {
  kNeutral = 1,
  kSoftSmile = 2,
  kLine = 3,
  kSmallOpen = 4,
};

enum class BrowExpression : uint8_t {
  kNeutral = 1,
  kSoftRaise = 2,
  kFocus = 3,
  kConcern = 4,
};

enum class FaceLayer : uint8_t {
  kBase = 1,
  kBlink = 2,
  kGaze = 3,
  kModulation = 4,
  kTransient = 5,
  kClip = 6,
};

inline constexpr size_t face_layer_index(FaceLayer layer) {
  switch (layer) {
    case FaceLayer::kBase:
      return 0;
    case FaceLayer::kBlink:
      return 1;
    case FaceLayer::kGaze:
      return 2;
    case FaceLayer::kModulation:
      return 3;
    case FaceLayer::kTransient:
      return 4;
    case FaceLayer::kClip:
      return 5;
    default:
      return 0;
  }
}

struct EyePose {
  struct SideAdjustments {
    int8_t x_offset_px = 0;
    int8_t y_offset_px = 0;
    int8_t size_delta_percent = 0;
    int8_t openness_delta_percent = 0;
  };

  GazeAnchor anchor = GazeAnchor::kCenter;
  GazeDirection direction = GazeDirection::kCenter;
  uint8_t focus_percent = 0;
  SideAdjustments left_adjust{};
  SideAdjustments right_adjust{};
};

struct EyelidPose {
  BlinkPhase phase = BlinkPhase::kOpen;
  uint8_t openness_percent = 100;
};

struct MouthPose {
  MouthShape shape = MouthShape::kNeutral;
  uint8_t openness_percent = 0;
};

struct BrowPose {
  BrowExpression expression = BrowExpression::kNeutral;
  uint8_t intensity_percent = 0;
};

}  // namespace ncos::models::face


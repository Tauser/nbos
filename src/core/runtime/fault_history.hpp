#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::core::runtime {

enum class FaultCode : uint8_t {
  kUnknown = 0,
  kLifecycleFaulted = 1,
  kLifecycleWatchdogRegistrationFailed = 2,
  kDiagnosticsTaskRegistrationFailed = 3,
};

struct FaultEvent {
  FaultCode code = FaultCode::kUnknown;
  uint64_t timestamp_ms = 0;
  const char* message = "none";
};

class FaultHistory final {
 public:
  static constexpr size_t kCapacity = 16;

  void push(FaultCode code, uint64_t timestamp_ms, const char* message);
  uint32_t count() const;
  bool latest(FaultEvent* out_event) const;

 private:
  FaultEvent events_[kCapacity] = {};
  size_t head_ = 0;
  size_t size_ = 0;
  uint32_t total_count_ = 0;
};

}  // namespace ncos::core::runtime
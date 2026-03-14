#include "core/runtime/fault_history.hpp"

namespace ncos::core::runtime {

void FaultHistory::push(FaultCode code, uint64_t timestamp_ms, const char* message) {
  const size_t index = head_;
  events_[index].code = code;
  events_[index].timestamp_ms = timestamp_ms;
  events_[index].message = message == nullptr ? "unknown" : message;

  head_ = (head_ + 1U) % kCapacity;
  if (size_ < kCapacity) {
    ++size_;
  }

  ++total_count_;
}

uint32_t FaultHistory::count() const {
  return total_count_;
}

bool FaultHistory::latest(FaultEvent* out_event) const {
  if (out_event == nullptr || size_ == 0) {
    return false;
  }

  const size_t index = (head_ + kCapacity - 1U) % kCapacity;
  *out_event = events_[index];
  return true;
}

}  // namespace ncos::core::runtime
#pragma once

namespace ncos::app::lifecycle {

enum class SystemState {
  kPowerOn,
  kBooting,
  kRunning,
  kDegraded,
  kFaulted,
};

class SystemLifecycle final {
 public:
  SystemLifecycle() = default;

  void start_boot();
  void finish_boot(bool has_required_failures, bool has_warnings);
  void mark_fault();

  [[nodiscard]] SystemState state() const;
  [[nodiscard]] const char* state_name() const;

 private:
  SystemState state_ = SystemState::kPowerOn;
};

}  // namespace ncos::app::lifecycle

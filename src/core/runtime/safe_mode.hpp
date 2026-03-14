#pragma once

namespace ncos::core::runtime {

class SafeModeController final {
 public:
  void enter(const char* reason);
  bool active() const;
  const char* reason() const;

 private:
  bool active_ = false;
  const char* reason_ = "none";
};

}  // namespace ncos::core::runtime

#pragma once

namespace ncos::app::boot {

struct BootReport {
  bool has_required_failures = false;
  bool has_warnings = false;
};

class BootFlow final {
 public:
  BootReport execute();
};

}  // namespace ncos::app::boot

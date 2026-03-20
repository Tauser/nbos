#pragma once

namespace ncos::hal::platform {

constexpr bool running_native_tests() {
#ifdef NCOS_NATIVE_TESTS
  return true;
#else
  return false;
#endif
}

}  // namespace ncos::hal::platform

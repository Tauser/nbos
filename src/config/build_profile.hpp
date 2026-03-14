#pragma once

namespace ncos::config {

enum class BuildProfile {
  kDev,
  kProd,
  kUnknown,
};

constexpr BuildProfile detect_build_profile() {
#if defined(NCOS_PROFILE_DEV) && !defined(NCOS_PROFILE_PROD)
  return BuildProfile::kDev;
#elif defined(NCOS_PROFILE_PROD) && !defined(NCOS_PROFILE_DEV)
  return BuildProfile::kProd;
#else
  return BuildProfile::kUnknown;
#endif
}

constexpr bool is_build_profile_valid() {
  return detect_build_profile() != BuildProfile::kUnknown;
}

constexpr const char* build_profile_name() {
  switch (detect_build_profile()) {
    case BuildProfile::kDev:
      return "dev";
    case BuildProfile::kProd:
      return "prod";
    default:
      return "unknown";
  }
}

}  // namespace ncos::config

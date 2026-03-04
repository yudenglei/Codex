#pragma once
#include <algorithm>
#include <cstdint>

namespace cae {
using Id = uint32_t;
using ObjectId = Id;
using LayerId = uint16_t;
using StringId = uint32_t;

struct DbuValue {
  uint64_t raw = 0;
  static constexpr uint64_t kParamMask = 1ULL << 63;

  static DbuValue literal(int64_t v) { return DbuValue{static_cast<uint64_t>(v) & ~kParamMask}; }
  static DbuValue param(StringId pid) { return DbuValue{kParamMask | static_cast<uint64_t>(pid)}; }
  bool is_param() const { return (raw & kParamMask) != 0; }
  int64_t lit() const { return static_cast<int64_t>(raw & ~kParamMask); }
  StringId pid() const { return static_cast<StringId>(raw & ~kParamMask); }
};

inline DbuValue lit(int64_t v) { return DbuValue::literal(v); }
inline DbuValue param(StringId p) { return DbuValue::param(p); }

struct Point {
  DbuValue x = lit(0);
  DbuValue y = lit(0);
};

struct Box {
  int32_t x0 = 0;
  int32_t y0 = 0;
  int32_t x1 = 0;
  int32_t y1 = 0;
};

inline int32_t to_i32(int64_t v) {
  constexpr int64_t lo = static_cast<int64_t>(INT32_MIN);
  constexpr int64_t hi = static_cast<int64_t>(INT32_MAX);
  return static_cast<int32_t>(std::clamp(v, lo, hi));
}

inline bool intersects(const Box& a, const Box& b) {
  return !(a.x1 < b.x0 || b.x1 < a.x0 || a.y1 < b.y0 || b.y1 < a.y0);
}
}  // namespace cae

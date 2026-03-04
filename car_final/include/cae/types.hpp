#pragma once
#include <cstdint>

namespace cae {
using Id = uint32_t;
using LayerId = uint16_t;
using StringId = uint32_t;

struct Point {
  int32_t x = 0;
  int32_t y = 0;
};

struct Box {
  int32_t x0 = 0;
  int32_t y0 = 0;
  int32_t x1 = 0;
  int32_t y1 = 0;
};

inline bool intersects(const Box& a, const Box& b) {
  return !(a.x1 < b.x0 || b.x1 < a.x0 || a.y1 < b.y0 || b.y1 < a.y0);
}
}  // namespace cae

#pragma once
#include "cae/types.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace cae {
struct Segment {
  Point p0{};
  Point p1{};
  DbuValue width = lit(0);
};

struct Pad { StringId shape = 0; DbuValue size_x = lit(0); DbuValue size_y = lit(0); DbuValue rotation = lit(0); };
struct Drill { DbuValue diameter = lit(0); };

struct Layer { LayerId id = 0; StringId name = 0; };
struct Net { Id id = 0; StringId name = 0; StringId description = 0; };

struct PadstackDef { Id id = 0; StringId name = 0; std::vector<Pad> pads; Drill drill{}; };
struct Pin { Id id = 0; Id component = 0; Id net = 0; Id padstack = 0; Point loc{}; StringId name = 0; };
struct Via { Id id = 0; Id net = 0; Id padstack = 0; LayerId from_layer = 0; LayerId to_layer = 0; Point loc{}; };
struct Component { Id id = 0; StringId refdes = 0; StringId kind = 0; Point loc{}; DbuValue rotation = lit(0); bool mirrored = false; std::vector<Id> pins; StringId description = 0; };
struct Trace { Id id = 0; Id net = 0; LayerId layer = 0; std::vector<Segment> segments; };
struct BondWire { Id id = 0; Id net = 0; LayerId from_layer = 0; LayerId to_layer = 0; Point p0{}; Point p1{}; };

struct Board { Id id = 0; StringId name = 0; StringId description = 0; };
struct LayerStack { Id id = 0; StringId name = 0; std::vector<LayerId> layers; };
struct Port { Id id = 0; StringId name = 0; Id net = 0; LayerId layer = 0; Point loc{}; };
struct Surface { Id id = 0; StringId name = 0; LayerId layer = 0; std::vector<Point> outline; };
struct Symbol { Id id = 0; StringId name = 0; StringId category = 0; };
struct Text { Id id = 0; StringId content = 0; LayerId layer = 0; Point loc{}; DbuValue height = lit(0); };
struct Constraint { Id id = 0; StringId kind = 0; DbuValue value = lit(0); Id target_a = 0; Id target_b = 0; };

template <typename Resolve>
inline Box bbox(const Segment& s, Resolve&& resolve) {
  const int32_t x0 = to_i32(resolve(s.p0.x));
  const int32_t y0 = to_i32(resolve(s.p0.y));
  const int32_t x1 = to_i32(resolve(s.p1.x));
  const int32_t y1 = to_i32(resolve(s.p1.y));
  return {std::min(x0, x1), std::min(y0, y1), std::max(x0, x1), std::max(y0, y1)};
}

inline Box bbox_literal_only(const Segment& s) {
  auto resolve = [](DbuValue v) -> int64_t { return v.is_param() ? 0 : v.lit(); };
  return bbox(s, resolve);
}

template <typename Resolve>
inline Box bbox(const Trace& t, Resolve&& resolve) {
  if (t.segments.empty()) return {};
  Box b = bbox(t.segments.front(), resolve);
  for (size_t i = 1; i < t.segments.size(); ++i) {
    const auto s = bbox(t.segments[i], resolve);
    b.x0 = std::min(b.x0, s.x0); b.y0 = std::min(b.y0, s.y0);
    b.x1 = std::max(b.x1, s.x1); b.y1 = std::max(b.y1, s.y1);
  }
  return b;
}

inline Box bbox_literal_only(const Trace& t) {
  auto resolve = [](DbuValue v) -> int64_t { return v.is_param() ? 0 : v.lit(); };
  return bbox(t, resolve);
}
}  // namespace cae

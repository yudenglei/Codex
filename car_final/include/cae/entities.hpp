#pragma once
#include "cae/types.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace cae {
struct Segment {
  Point p0{};
  Point p1{};
  int32_t width = 0;
};

struct Pad { StringId shape = 0; int32_t size_x = 0; int32_t size_y = 0; int32_t rotation = 0; };
struct Drill { int32_t diameter = 0; };

struct Layer { LayerId id = 0; StringId name = 0; };
struct Net { Id id = 0; StringId name = 0; StringId description = 0; };

struct PadstackDef { Id id = 0; StringId name = 0; std::vector<Pad> pads; Drill drill{}; };
struct Pin { Id id = 0; Id component = 0; Id net = 0; Id padstack = 0; Point loc{}; StringId name = 0; };
struct Via { Id id = 0; Id net = 0; Id padstack = 0; LayerId from_layer = 0; LayerId to_layer = 0; Point loc{}; };
struct Component { Id id = 0; StringId refdes = 0; StringId kind = 0; Point loc{}; int32_t rotation = 0; bool mirrored = false; std::vector<Id> pins; StringId description = 0; };
struct Trace { Id id = 0; Id net = 0; LayerId layer = 0; std::vector<Segment> segments; };
struct BondWire { Id id = 0; Id net = 0; LayerId from_layer = 0; LayerId to_layer = 0; Point p0{}; Point p1{}; };

struct Board { Id id = 0; StringId name = 0; StringId description = 0; };
struct LayerStack { Id id = 0; StringId name = 0; std::vector<LayerId> layers; };
struct Port { Id id = 0; StringId name = 0; Id net = 0; LayerId layer = 0; Point loc{}; };
struct Surface { Id id = 0; StringId name = 0; LayerId layer = 0; std::vector<Point> outline; };
struct Symbol { Id id = 0; StringId name = 0; StringId category = 0; };
struct Text { Id id = 0; StringId content = 0; LayerId layer = 0; Point loc{}; int32_t height = 0; };
struct Constraint { Id id = 0; StringId kind = 0; int32_t value = 0; Id target_a = 0; Id target_b = 0; };

inline Box bbox(const Segment& s) {
  return {std::min(s.p0.x, s.p1.x), std::min(s.p0.y, s.p1.y), std::max(s.p0.x, s.p1.x), std::max(s.p0.y, s.p1.y)};
}

inline Box bbox(const Trace& t) {
  if (t.segments.empty()) return {};
  Box b = bbox(t.segments.front());
  for (size_t i = 1; i < t.segments.size(); ++i) {
    const auto s = bbox(t.segments[i]);
    b.x0 = std::min(b.x0, s.x0); b.y0 = std::min(b.y0, s.y0);
    b.x1 = std::max(b.x1, s.x1); b.y1 = std::max(b.y1, s.y1);
  }
  return b;
}
}  // namespace cae

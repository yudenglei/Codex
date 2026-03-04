#pragma once
#include "cae/entities.hpp"

#include <vector>

namespace cae {
struct ScenePrimitive {
  LayerId layer = 0;
  Box bounds{};
  Id source_id = 0;
};

class RenderCache {
 public:
  virtual ~RenderCache() = default;
  virtual void clear() = 0;
  virtual void put(const ScenePrimitive& p) = 0;
  virtual std::vector<ScenePrimitive> query(LayerId layer, Box box) const = 0;
};

class SceneAdapter {
 public:
  virtual ~SceneAdapter() = default;
  virtual std::vector<ScenePrimitive> collect_2d() const = 0;
  virtual std::vector<ScenePrimitive> collect_3d_proxy() const = 0;
};
}  // namespace cae

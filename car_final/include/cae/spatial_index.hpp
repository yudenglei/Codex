#pragma once
#include "cae/types.hpp"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace cae {

class QuadTree {
  struct Item { Id id; Box box; };
  struct Node {
    Box bounds{};
    std::vector<Item> items;
    std::array<std::unique_ptr<Node>, 4> child;
    bool leaf = true;
  };

 public:
  explicit QuadTree(Box world = {-1000000000, -1000000000, 1000000000, 1000000000}) {
    root_ = std::make_unique<Node>();
    root_->bounds = world;
  }

  void upsert(Id id, Box box) {
    erase(id);
    id2box_[id] = box;
    insert(root_.get(), {id, box}, 0);
  }

  void erase(Id id) {
    id2box_.erase(id);
    rebuild_if_needed_ = true;
  }

  std::vector<Id> query(Box region) {
    if (rebuild_if_needed_) rebuild();
    std::vector<Id> out;
    query_node(root_.get(), region, out);
    return out;
  }

 private:
  static constexpr int kMaxDepth = 8;
  static constexpr int kSplitThreshold = 16;

  static Box child_box(const Box& b, int idx) {
    int32_t mx = (b.x0 + b.x1) / 2;
    int32_t my = (b.y0 + b.y1) / 2;
    switch (idx) {
      case 0: return {b.x0, b.y0, mx, my};
      case 1: return {mx + 1, b.y0, b.x1, my};
      case 2: return {b.x0, my + 1, mx, b.y1};
      default: return {mx + 1, my + 1, b.x1, b.y1};
    }
  }

  static bool contains(const Box& outer, const Box& inner) {
    return outer.x0 <= inner.x0 && outer.y0 <= inner.y0 && outer.x1 >= inner.x1 && outer.y1 >= inner.y1;
  }

  void split(Node* n, int depth) {
    n->leaf = false;
    for (int i = 0; i < 4; ++i) {
      n->child[i] = std::make_unique<Node>();
      n->child[i]->bounds = child_box(n->bounds, i);
    }
    auto old = std::move(n->items);
    for (const auto& it : old) insert(n, it, depth);
  }

  void insert(Node* n, const Item& it, int depth) {
    if (!intersects(n->bounds, it.box)) return;
    if (!n->leaf) {
      for (int i = 0; i < 4; ++i) {
        if (contains(n->child[i]->bounds, it.box)) {
          insert(n->child[i].get(), it, depth + 1);
          return;
        }
      }
    }
    n->items.push_back(it);
    if (n->leaf && depth < kMaxDepth && static_cast<int>(n->items.size()) > kSplitThreshold) split(n, depth);
  }

  void query_node(Node* n, Box region, std::vector<Id>& out) const {
    if (!n || !intersects(n->bounds, region)) return;
    for (const auto& it : n->items) if (intersects(it.box, region)) out.push_back(it.id);
    if (!n->leaf) for (const auto& c : n->child) query_node(c.get(), region, out);
  }

  void rebuild() {
    Box world = root_->bounds;
    root_ = std::make_unique<Node>();
    root_->bounds = world;
    for (const auto& [id, box] : id2box_) insert(root_.get(), {id, box}, 0);
    rebuild_if_needed_ = false;
  }

  std::unique_ptr<Node> root_;
  std::unordered_map<Id, Box> id2box_;
  bool rebuild_if_needed_ = false;
};

class LayerSpatialIndex {
 public:
  void upsert(LayerId layer, Id id, Box box) { trees_[layer].upsert(id, box); }
  void erase(LayerId layer, Id id) { trees_[layer].erase(id); }
  std::vector<Id> query_on_layer(LayerId layer, Box region) { return trees_[layer].query(region); }

 private:
  std::unordered_map<LayerId, QuadTree> trees_;
};

}  // namespace cae

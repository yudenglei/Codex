#pragma once
#include "cae/entities.hpp"
#include "cae/param_expr.hpp"
#include "cae/reuse_vector.hpp"
#include "cae/scene_adapter.hpp"
#include "cae/spatial_index.hpp"
#include "cae/string_pool.hpp"
#include "cae/undo.hpp"

#include <algorithm>
#include <unordered_map>

namespace cae {

class BoardDb : public SceneAdapter {
 public:
  StringPool strings;
  UndoRedo tx;
  ParamTable params;

  ReuseVector<Layer> layers;
  ReuseVector<Net> nets;
  ReuseVector<PadstackDef> padstacks;
  ReuseVector<Component> components;
  ReuseVector<Pin> pins;
  ReuseVector<Via> vias;
  ReuseVector<Trace> traces;
  ReuseVector<BondWire> bondwires;
  ReuseVector<Board> boards;
  ReuseVector<LayerStack> layer_stacks;
  ReuseVector<Port> ports;
  ReuseVector<Surface> surfaces;
  ReuseVector<Symbol> symbols;
  ReuseVector<Text> texts;
  ReuseVector<Constraint> constraints;

  LayerSpatialIndex trace_index;

  void begin_tx(const std::string& l) { tx.begin_tx(l); }
  void commit_tx() { tx.commit_tx(); }
  void rollback_tx() { tx.rollback_tx(); }

  Id add_net(Net n) { return add_generic(nets, std::move(n)); }
  Id add_board(Board b) { return add_generic(boards, std::move(b)); }
  Id add_trace(Trace t) {
    Id id = add_generic(traces, t);
    auto* tr = traces.get(id);
    layer_traces_[tr->layer].push_back(id);
    trace_index.upsert(tr->layer, id, bbox(*tr, [this](DbuValue v){ return params.resolve(v);}));
    tx.push({[this, id, layer = tr->layer]() {
               traces.remove(id);
               auto& v = layer_traces_[layer];
               v.erase(std::remove(v.begin(), v.end(), id), v.end());
               trace_index.erase(layer, id);
             },
             [this, tr = *tr]() mutable {
               Id nid = traces.add(tr);
               tr.id = nid;
               traces.replace(nid, tr);
               layer_traces_[tr.layer].push_back(nid);
               trace_index.upsert(tr.layer, nid, bbox(tr, [this](DbuValue v){ return params.resolve(v);}));
             },
             "add_trace"});
    return id;
  }

  bool replace_trace(Id id, const Trace& newer) {
    auto* old = traces.get(id);
    if (!old) return false;
    Trace before = *old;
    Trace after = newer;
    after.id = id;

    traces.replace(id, after);  // klayout 风格：remove old + add new 的容器替换语义等价
    trace_index.upsert(after.layer, id, bbox(after, [this](DbuValue v){ return params.resolve(v);}));

    tx.push({[this, id, before]() {
               traces.replace(id, before);
               trace_index.upsert(before.layer, id, bbox(before, [this](DbuValue v){ return params.resolve(v);}));
             },
             [this, id, after]() {
               traces.replace(id, after);
               trace_index.upsert(after.layer, id, bbox(after, [this](DbuValue v){ return params.resolve(v);}));
             },
             "replace_trace"});
    return true;
  }

  template <typename T>
  bool batch_replace(ReuseVector<T>& vec, const std::vector<std::pair<Id, T>>& updates) {
    begin_tx("batch_replace");
    for (const auto& [id, nv] : updates) {
      auto* ov = vec.get(id);
      if (!ov) { rollback_tx(); return false; }
      T old = *ov;
      vec.replace(id, nv);
      tx.push({[&vec, id, old]() { vec.replace(id, old); },
               [&vec, id, nv]() { vec.replace(id, nv); },
               "batch_replace_item"});
    }
    commit_tx();
    return true;
  }

  std::vector<Id> query_trace_on_layer(LayerId layer, Box region) { return trace_index.query_on_layer(layer, region); }

  std::vector<ScenePrimitive> collect_2d() const override {
    std::vector<ScenePrimitive> out;
    traces.for_each([&](Id id, const Trace& t) { out.push_back({t.layer, bbox(t, [this](DbuValue v){ return params.resolve(v);}), id}); });
    texts.for_each([&](Id id, const Text& t) { const auto x=to_i32(params.resolve(t.loc.x)); const auto y=to_i32(params.resolve(t.loc.y)); out.push_back({t.layer, {x,y,x,y}, id}); });
    return out;
  }

  std::vector<ScenePrimitive> collect_3d_proxy() const override {
    std::vector<ScenePrimitive> out;
    surfaces.for_each([&](Id id, const Surface& s) {
      if (s.outline.empty()) return;
      Box b{to_i32(params.resolve(s.outline[0].x)), to_i32(params.resolve(s.outline[0].y)), to_i32(params.resolve(s.outline[0].x)), to_i32(params.resolve(s.outline[0].y))};
      for (const auto& p : s.outline) { const auto px=to_i32(params.resolve(p.x)); const auto py=to_i32(params.resolve(p.y)); b.x0 = std::min(b.x0, px); b.y0 = std::min(b.y0, py); b.x1 = std::max(b.x1, px); b.y1 = std::max(b.y1, py); }
      out.push_back({s.layer, b, id});
    });
    return out;
  }

 private:
  template <typename T>
  Id add_generic(ReuseVector<T>& vec, T obj) {
    Id id = vec.add(obj);
    obj.id = id;
    vec.replace(id, obj);
    tx.push({[&vec, id]() { vec.remove(id); },
             [&vec, obj]() mutable {
               Id rid = vec.add(obj);
               obj.id = rid;
               vec.replace(rid, obj);
             },
             "add"});
    return id;
  }

  std::unordered_map<LayerId, std::vector<Id>> layer_traces_;
};

}  // namespace cae

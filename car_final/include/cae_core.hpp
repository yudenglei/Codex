#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace cae {

using Id = uint32_t;
using LayerId = uint16_t;
using StringId = uint32_t;

struct Point {
  int32_t x = 0;
  int32_t y = 0;
};

struct Segment {
  Point p0{};
  Point p1{};
  int32_t width = 0;
};

struct Pad {
  StringId shape = 0;
  int32_t size_x = 0;
  int32_t size_y = 0;
  int32_t rotation = 0;
};

struct Drill {
  int32_t diameter = 0;
};

struct PadstackDef {
  Id id = 0;
  StringId name = 0;
  std::vector<Pad> pads;
  Drill drill{};
};

struct Net {
  Id id = 0;
  StringId name = 0;
};

struct Layer {
  LayerId id = 0;
  StringId name = 0;
};

struct Pin {
  Id id = 0;
  Id component = 0;
  Id net = 0;
  Id padstack = 0;
  Point loc{};
};

struct Via {
  Id id = 0;
  Id net = 0;
  Id padstack = 0;
  LayerId from_layer = 0;
  LayerId to_layer = 0;
  Point loc{};
};

struct Component {
  Id id = 0;
  StringId refdes = 0;
  Point loc{};
  int32_t rotation = 0;
  bool mirrored = false;
  std::vector<Id> pins;
};

struct Trace {
  Id id = 0;
  Id net = 0;
  LayerId layer = 0;
  std::vector<Segment> segments;
};

struct BondWire {
  Id id = 0;
  Id net = 0;
  LayerId from_layer = 0;
  LayerId to_layer = 0;
  Point p0{};
  Point p1{};
};

class StringPool {
 public:
  StringId intern(std::string_view s) {
    auto it = to_id_.find(std::string(s));
    if (it != to_id_.end()) return it->second;
    StringId id = static_cast<StringId>(strings_.size() + 1);
    strings_.emplace_back(s);
    to_id_[strings_.back()] = id;
    return id;
  }

  std::string_view view(StringId id) const {
    if (id == 0 || id > strings_.size()) return {};
    return strings_[id - 1];
  }

  std::vector<StringId> find_prefix(std::string_view prefix) const {
    std::vector<StringId> out;
    for (size_t i = 0; i < strings_.size(); ++i) {
      if (strings_[i].rfind(prefix, 0) == 0) out.push_back(static_cast<StringId>(i + 1));
    }
    return out;
  }

 private:
  std::vector<std::string> strings_;
  std::unordered_map<std::string, StringId> to_id_;
};

template <typename T>
class ReuseVector {
 public:
  Id add(const T& v) {
    if (!free_.empty()) {
      Id idx = free_.back();
      free_.pop_back();
      data_[idx - 1] = v;
      used_[idx - 1] = true;
      return idx;
    }
    data_.push_back(v);
    used_.push_back(true);
    return static_cast<Id>(data_.size());
  }

  bool remove(Id id) {
    if (!contains(id)) return false;
    used_[id - 1] = false;
    free_.push_back(id);
    return true;
  }

  bool replace(Id id, const T& v) {
    if (!contains(id)) return false;
    data_[id - 1] = v;
    return true;
  }

  T* get(Id id) {
    if (!contains(id)) return nullptr;
    return &data_[id - 1];
  }

  const T* get(Id id) const {
    if (!contains(id)) return nullptr;
    return &data_[id - 1];
  }

  bool contains(Id id) const {
    return id > 0 && id <= used_.size() && used_[id - 1];
  }

 private:
  std::vector<T> data_;
  std::vector<bool> used_;
  std::vector<Id> free_;
};

class UndoRedo {
 public:
  struct Op {
    std::function<void()> undo;
    std::function<void()> redo;
    std::string label;
  };

  void push(Op op) {
    if (cursor_ < ops_.size()) ops_.erase(ops_.begin() + static_cast<long>(cursor_), ops_.end());
    ops_.push_back(std::move(op));
    cursor_ = ops_.size();
  }

  bool undo() {
    if (cursor_ == 0) return false;
    --cursor_;
    ops_[cursor_].undo();
    return true;
  }

  bool redo() {
    if (cursor_ >= ops_.size()) return false;
    ops_[cursor_].redo();
    ++cursor_;
    return true;
  }

 private:
  std::vector<Op> ops_;
  size_t cursor_ = 0;
};

class ParamTable {
 public:
  void set_var(StringId name, double v) { vars_[name] = v; }

  double eval_add_mul(StringId a, char op, StringId b) const {
    const double av = at(a);
    const double bv = at(b);
    if (op == '+') return av + bv;
    if (op == '*') return av * bv;
    return 0.0;
  }

 private:
  double at(StringId id) const {
    auto it = vars_.find(id);
    return it == vars_.end() ? 0.0 : it->second;
  }

  std::unordered_map<StringId, double> vars_;
};

class BoardDb {
 public:
  StringPool strings;
  UndoRedo tx;
  ReuseVector<Layer> layers;
  ReuseVector<Net> nets;
  ReuseVector<PadstackDef> padstacks;
  ReuseVector<Component> components;
  ReuseVector<Pin> pins;
  ReuseVector<Via> vias;
  ReuseVector<Trace> traces;
  ReuseVector<BondWire> bondwires;

  Id add_net(Net n) {
    const Id id = nets.add(n);
    n.id = id;
    nets.replace(id, n);
    tx.push({[this, id]() { nets.remove(id); },
             [this, n]() mutable {
               const Id rid = nets.add(n);
               n.id = rid;
               nets.replace(rid, n);
             },
             "add_net"});
    return id;
  }

  Id add_trace(Trace t) {
    const Id id = traces.add(t);
    t.id = id;
    traces.replace(id, t);
    layer_traces_[t.layer].push_back(id);
    tx.push({[this, t, id]() {
               traces.remove(id);
               auto& v = layer_traces_[t.layer];
               v.erase(std::remove(v.begin(), v.end(), id), v.end());
             },
             [this, t]() {
               Id nid = traces.add(t);
               Trace cp = t;
               cp.id = nid;
               traces.replace(nid, cp);
               layer_traces_[cp.layer].push_back(nid);
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
    traces.replace(id, after);
    tx.push({[this, id, before]() { traces.replace(id, before); },
             [this, id, after]() { traces.replace(id, after); },
             "replace_trace"});
    return true;
  }

  const std::vector<Id>& trace_ids_on_layer(LayerId lid) const {
    static const std::vector<Id> empty;
    auto it = layer_traces_.find(lid);
    return it == layer_traces_.end() ? empty : it->second;
  }

 private:
  std::unordered_map<LayerId, std::vector<Id>> layer_traces_;
};

}  // namespace cae

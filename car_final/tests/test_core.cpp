#include "cae_core.hpp"

#include <cassert>

int main() {
  cae::BoardDb db;
  auto sn = db.strings.intern("NET1");
  auto sa = db.strings.intern("a");
  auto sb = db.strings.intern("b");

  cae::Net n{};
  n.name = sn;
  auto net_id = db.add_net(n);
  assert(db.nets.get(net_id) != nullptr);

  cae::Trace t{};
  t.net = net_id;
  t.layer = 2;
  t.segments.push_back({{0, 0}, {10, 10}, 5});
  auto tid = db.add_trace(t);
  assert(db.trace_ids_on_layer(2).size() == 1);

  cae::Trace t2 = *db.traces.get(tid);
  t2.segments[0].width = 8;
  assert(db.replace_trace(tid, t2));
  assert(db.traces.get(tid)->segments[0].width == 8);

  assert(db.tx.undo());
  assert(db.traces.get(tid)->segments[0].width == 5);
  assert(db.tx.redo());
  assert(db.traces.get(tid)->segments[0].width == 8);

  cae::ParamTable p;
  p.set_var(sa, 3);
  p.set_var(sb, 4);
  assert(p.eval_add_mul(sa, '+', sb) == 7);
  assert(p.eval_add_mul(sa, '*', sb) == 12);

  auto pref = db.strings.find_prefix("NE");
  assert(!pref.empty());
  return 0;
}

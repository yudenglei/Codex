#include "cae_core.hpp"

#include <cassert>

int main() {
  cae::BoardDb db;
  auto s_net = db.strings.intern("NET1");
  auto s_desc = db.strings.intern("power return net");
  auto s_a = db.strings.intern("a");
  auto s_b = db.strings.intern("b");

  cae::Net n{};
  n.name = s_net;
  n.description = s_desc;
  auto net_id = db.add_net(n);
  assert(db.nets.get(net_id));

  cae::Trace t{};
  t.net = net_id;
  t.layer = 2;
  t.segments.push_back({{cae::lit(0), cae::lit(0)}, {cae::lit(10), cae::lit(10)}, cae::lit(5)});
  auto tid = db.add_trace(t);
  auto hits = db.query_trace_on_layer(2, {0, 0, 10, 10});
  assert(hits.size() == 1 && hits[0] == tid);

  cae::Trace t2 = *db.traces.get(tid);
  t2.segments[0].width = cae::lit(8);
  assert(db.replace_trace(tid, t2));
  assert(db.traces.get(tid)->segments[0].width.lit() == 8);
  assert(db.tx.undo());
  assert(db.traces.get(tid)->segments[0].width.lit() == 5);
  assert(db.tx.redo());

  auto ft = db.strings.search_full_text("power");
  assert(!ft.empty());

  db.begin_tx("batch");
  cae::Board b{};
  b.name = db.strings.intern("MainBoard");
  db.add_board(b);
  db.commit_tx();
  assert(db.tx.undo());

  // Requested Grok-style usage:
  db.expr_pool.setVariable("a", 10);
  db.expr_pool.setVariable("b", 13);

  cae::ObjectId traceId = db.add_trace(5, cae::Trace{});
  assert(db.traces.get(traceId) != nullptr);
  db.traces.get(traceId)->segments.push_back({{cae::lit(0), cae::lit(0)}, {cae::lit(20), cae::lit(0)}, cae::lit(1)});
  assert(db.set_trace_width_expression(traceId, "a + b + max(a, b)"));
  assert(db.params.resolve(db.traces.get(traceId)->segments[0].width) == 36);

  auto hits_expr = db.query_trace_on_layer(5, {0, -1, 20, 1});
  assert(!hits_expr.empty());

  // Backward-compatible API still works.
  db.params.set_var(s_a, 3);
  db.params.set_var(s_b, 4);
  auto v = db.params.eval("a+b");
  assert(v == 7);

  auto p2d = db.collect_2d();
  assert(!p2d.empty());

  return 0;
}

#include "cae_core.hpp"

#include <iostream>

int main() {
  cae::BoardDb db;
  auto n_name = db.strings.intern("NET_CLK");
  auto n_desc = db.strings.intern("clock net high speed");

  cae::Net n{};
  n.name = n_name;
  n.description = n_desc;
  cae::Id net_id = db.add_net(n);

  db.expr_pool.setVariable("a", 10);
  db.expr_pool.setVariable("b", 13);

  cae::Trace tr{};
  tr.net = net_id;
  auto tid = db.add_trace(1, tr);

  auto* tp = db.traces.get(tid);
  tp->segments.push_back({{cae::lit(0), cae::lit(0)}, {cae::lit(100), cae::lit(0)}, cae::lit(1)});
  db.set_trace_width_expression(tid, "a + b + max(a, b)");

  std::cout << "trace=" << tid << " width_expr_value=" << db.params.resolve(db.traces.get(tid)->segments[0].width) << "\n";

  auto hits = db.strings.search_full_text("clock");
  std::cout << "fulltext_hits=" << hits.size() << " layer_hits=" << db.query_trace_on_layer(1, {0, -1, 101, 1}).size() << "\n";
  return 0;
}

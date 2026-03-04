#include "cae_core.hpp"

#include <iostream>

int main() {
  cae::BoardDb db;
  auto n_name = db.strings.intern("NET_CLK");
  auto n_desc = db.strings.intern("clock net high speed");
  auto sid_a = db.strings.intern("a");
  auto sid_b = db.strings.intern("b");

  cae::Net n{};
  n.name = n_name;
  n.description = n_desc;
  cae::Id net_id = db.add_net(n);

  cae::Trace tr{};
  tr.net = net_id;
  tr.layer = 1;
  tr.segments.push_back({{cae::lit(0), cae::lit(0)}, {cae::lit(100), cae::lit(0)}, cae::lit(10)});
  auto tid = db.add_trace(tr);

  db.params.set_var(sid_a, 5);
  db.params.set_var(sid_b, 6);
  std::cout << "trace=" << tid << " expr=" << db.params.eval(std::to_string(sid_a) + "*" + std::to_string(sid_b)) << "\n";

  auto hits = db.strings.search_full_text("clock");
  std::cout << "fulltext_hits=" << hits.size() << " layer_hits=" << db.query_trace_on_layer(1, {0, -1, 101, 1}).size() << "\n";
  return 0;
}

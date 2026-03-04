#include "cae_core.hpp"

#include <iostream>

int main() {
  cae::BoardDb db;
  auto s_gnd = db.strings.intern("GND");
  auto s_a = db.strings.intern("a");
  auto s_b = db.strings.intern("b");

  cae::Net gnd{};
  gnd.name = s_gnd;
  auto net_id = db.add_net(gnd);

  cae::Trace t{};
  t.net = net_id;
  t.layer = 1;
  t.segments.push_back({{0, 0}, {1000, 0}, 100});
  auto trace_id = db.add_trace(t);

  cae::ParamTable params;
  params.set_var(s_a, 10.0);
  params.set_var(s_b, 20.0);
  std::cout << "trace=" << trace_id << " expr(a*b)=" << params.eval_add_mul(s_a, '*', s_b) << "\n";

  db.tx.undo();
  db.tx.redo();
  return 0;
}

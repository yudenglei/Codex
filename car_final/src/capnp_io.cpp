#include "cae/capnp_io.hpp"

#ifdef CAE_ENABLE_CAPNP
#include "car_pcb.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace cae {

#ifdef CAE_ENABLE_CAPNP

static void write_dbu(::DBUValue::Builder b, DbuValue v) { b.setRaw(v.raw); }
static DbuValue read_dbu(::DBUValue::Reader r) { return DbuValue{r.getRaw()}; }
static void write_point(::Point::Builder b, const Point& p) { write_dbu(b.initX(), p.x); write_dbu(b.initY(), p.y); }
static Point read_point(::Point::Reader r) { return {read_dbu(r.getX()), read_dbu(r.getY())}; }

bool save_capnp(const BoardDb& db, const std::string& path) {
  ::capnp::MallocMessageBuilder message;
  auto root = message.initRoot<::DbSnapshot>();

  std::vector<std::pair<StringId, std::string>> strings;
  db.strings.for_each([&](StringId id, std::string_view s) { strings.emplace_back(id, std::string(s)); });
  auto rs = root.initStrings(strings.size());
  for (size_t i = 0; i < strings.size(); ++i) { rs[i].setId(strings[i].first); rs[i].setValue(strings[i].second); }

  std::vector<Layer> layers; db.layers.for_each([&](Id, const Layer& o) { layers.push_back(o); });
  auto rl = root.initLayers(layers.size());
  for (size_t i = 0; i < layers.size(); ++i) { rl[i].setId(layers[i].id); rl[i].setName(layers[i].name); }

  std::vector<Net> nets; db.nets.for_each([&](Id, const Net& o) { nets.push_back(o); });
  auto rn = root.initNets(nets.size());
  for (size_t i = 0; i < nets.size(); ++i) { rn[i].setId(nets[i].id); rn[i].setName(nets[i].name); rn[i].setDescription(nets[i].description); }

  std::vector<Trace> traces; db.traces.for_each([&](Id, const Trace& o) { traces.push_back(o); });
  auto rt = root.initTraces(traces.size());
  for (size_t i = 0; i < traces.size(); ++i) {
    rt[i].setId(traces[i].id); rt[i].setNet(traces[i].net); rt[i].setLayer(traces[i].layer);
    auto segs = rt[i].initSegments(traces[i].segments.size());
    for (size_t k = 0; k < traces[i].segments.size(); ++k) {
      write_point(segs[k].initP0(), traces[i].segments[k].p0);
      write_point(segs[k].initP1(), traces[i].segments[k].p1);
      write_dbu(segs[k].initWidth(), traces[i].segments[k].width);
    }
  }

  root.initPadstacks(0); root.initComponents(0); root.initPins(0); root.initVias(0); root.initBondwires(0);
  root.initBoards(0); root.initLayerStacks(0); root.initPorts(0); root.initSurfaces(0); root.initSymbols(0); root.initTexts(0); root.initConstraints(0);

  int fd = ::open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (fd < 0) return false;
  ::capnp::writePackedMessageToFd(fd, message);
  ::close(fd);
  return true;
}

bool load_capnp(BoardDb& db, const std::string& path) {
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) return false;
  ::capnp::PackedFdMessageReader reader(fd);
  auto root = reader.getRoot<::DbSnapshot>();

  for (auto n : root.getNets()) {
    Net net{}; net.name = n.getName(); net.description = n.getDescription();
    db.add_net(net);
  }
  for (auto t : root.getTraces()) {
    Trace tr{}; tr.net = t.getNet(); tr.layer = t.getLayer();
    for (auto s : t.getSegments()) {
      Segment seg{}; seg.p0 = read_point(s.getP0()); seg.p1 = read_point(s.getP1()); seg.width = read_dbu(s.getWidth());
      tr.segments.push_back(seg);
    }
    db.add_trace(tr);
  }
  ::close(fd);
  return true;
}

#else
bool save_capnp(const BoardDb&, const std::string&) { return false; }
bool load_capnp(BoardDb&, const std::string&) { return false; }
#endif

}  // namespace cae

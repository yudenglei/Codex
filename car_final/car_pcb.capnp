@0x9ccbf911fcb2a371;

struct Point {
  x @0 :Int32;
  y @1 :Int32;
}

struct Segment {
  p0 @0 :Point;
  p1 @1 :Point;
  width @2 :Int32;
}

struct Pad {
  shape @0 :UInt32;
  sizeX @1 :Int32;
  sizeY @2 :Int32;
  rotation @3 :Int32;
}

struct Drill {
  diameter @0 :Int32;
}

struct Layer {
  id @0 :UInt16;
  name @1 :UInt32;
}

struct Net {
  id @0 :UInt32;
  name @1 :UInt32;
  description @2 :UInt32;
}

struct PadstackDef {
  id @0 :UInt32;
  name @1 :UInt32;
  drill @2 :Drill;
  pads @3 :List(Pad);
}

struct Pin {
  id @0 :UInt32;
  component @1 :UInt32;
  net @2 :UInt32;
  padstack @3 :UInt32;
  loc @4 :Point;
  name @5 :UInt32;
}

struct Via {
  id @0 :UInt32;
  net @1 :UInt32;
  padstack @2 :UInt32;
  fromLayer @3 :UInt16;
  toLayer @4 :UInt16;
  loc @5 :Point;
}

struct Component {
  id @0 :UInt32;
  refdes @1 :UInt32;
  kind @2 :UInt32;
  loc @3 :Point;
  rotation @4 :Int32;
  mirrored @5 :Bool;
  pins @6 :List(UInt32);
  description @7 :UInt32;
}

struct Trace {
  id @0 :UInt32;
  net @1 :UInt32;
  layer @2 :UInt16;
  segments @3 :List(Segment);
}

struct BondWire {
  id @0 :UInt32;
  net @1 :UInt32;
  fromLayer @2 :UInt16;
  toLayer @3 :UInt16;
  p0 @4 :Point;
  p1 @5 :Point;
}

struct Board {
  id @0 :UInt32;
  name @1 :UInt32;
  description @2 :UInt32;
}

struct LayerStack {
  id @0 :UInt32;
  name @1 :UInt32;
  layers @2 :List(UInt16);
}

struct Port {
  id @0 :UInt32;
  name @1 :UInt32;
  net @2 :UInt32;
  layer @3 :UInt16;
  loc @4 :Point;
}

struct Surface {
  id @0 :UInt32;
  name @1 :UInt32;
  layer @2 :UInt16;
  outline @3 :List(Point);
}

struct Symbol {
  id @0 :UInt32;
  name @1 :UInt32;
  category @2 :UInt32;
}

struct Text {
  id @0 :UInt32;
  content @1 :UInt32;
  layer @2 :UInt16;
  loc @3 :Point;
  height @4 :Int32;
}

struct Constraint {
  id @0 :UInt32;
  kind @1 :UInt32;
  value @2 :Int32;
  targetA @3 :UInt32;
  targetB @4 :UInt32;
}

struct StringEntry {
  id @0 :UInt32;
  value @1 :Text;
}

struct DbSnapshot {
  strings @0 :List(StringEntry);
  layers @1 :List(Layer);
  nets @2 :List(Net);
  padstacks @3 :List(PadstackDef);
  components @4 :List(Component);
  pins @5 :List(Pin);
  vias @6 :List(Via);
  traces @7 :List(Trace);
  bondwires @8 :List(BondWire);
  boards @9 :List(Board);
  layerStacks @10 :List(LayerStack);
  ports @11 :List(Port);
  surfaces @12 :List(Surface);
  symbols @13 :List(Symbol);
  texts @14 :List(Text);
  constraints @15 :List(Constraint);
}

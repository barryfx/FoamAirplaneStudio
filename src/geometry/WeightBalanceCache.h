#pragma once
#include "gui/WeightBalanceState.h"
#include <TopoDS_Shape.hxx>
#include <QByteArray>
#include <algorithm>
#include <thread>

namespace designrc::geometry {
// Logical hardware threads, rounded down; one worker is required even when the
// platform cannot report concurrency or has only one logical processor.
inline unsigned balanceWorkerLimit(unsigned available=std::thread::hardware_concurrency()) {
  return std::max(1u,(available/10)*9+(available%10)*9/10);
}
// Repeated Assembly placement creates new TopLoc datums for the same numeric
// transform. Compare immutable topology, orientation and transform values, not
// datum identity. Rebuilt/cut topology must miss even with identical bounds.
inline bool sameMassShape(const TopoDS_Shape& a,const TopoDS_Shape& b) {
  if(a.IsNull()||b.IsNull())return a.IsNull()&&b.IsNull();
  if(!a.IsPartner(b)||a.Orientation()!=b.Orientation())return false;
  const auto& x=a.Location().Transformation();const auto& y=b.Location().Transformation();
  for(int row=1;row<=3;++row)for(int column=1;column<=4;++column)
    if(x.Value(row,column)!=y.Value(row,column))return false;
  return true;
}
struct MaterialMeasurementCache {
  struct Entry {TopoDS_Shape source;double volume=0;QPointF moment;};
  std::vector<Entry> entries;
  std::size_t integrations=0; // Successful uncached integrations, for validation.
};
struct FiberglassMeasurementCache {
  struct Entry {
    std::vector<TopoDS_Shape> sources;
    QByteArray projection;
    gui::FoamMassProperties::Covering measured;
  };
  std::vector<Entry> entries;
  std::size_t integrations=0;
};
}

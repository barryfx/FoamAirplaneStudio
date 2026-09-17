#pragma once
#include "geometry/ProcessingControl.h"
#include <ShapeUpgrade_UnifySameDomain.hxx>
namespace designrc::geometry {
// Ruled loft correspondence creates many coplanar subdivisions. Merge only
// coincident faces/collinear edges at kernel tolerance; never reduce section
// samples or smooth across a profile corner. Inputs remain worker-owned and
// safe-input mode preserves shapes still referenced by other pipeline stages.
inline TopoDS_Shape simplifyFuselageTopology(const TopoDS_Shape& shape,const ProcessingControl& processing) {
  processing.checkpoint();ShapeUpgrade_UnifySameDomain unify{shape,true,true,false};
  unify.SetSafeInputMode(true);unify.SetLinearTolerance(1e-7);unify.History().Nullify();
  unify.Build();processing.checkpoint();return unify.Shape();
}
}

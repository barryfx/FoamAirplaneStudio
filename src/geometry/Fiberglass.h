#pragma once
#include "gui/ProjectDocument.h"
#include "geometry/Assembly.h"
#include <functional>
namespace designrc::geometry {
// Projection returns reference-scene coordinates; direction points out of the
// selected side. Both consume Assembly-space points. No solid generation.
struct FiberglassProjection {
  std::function<QPointF(const gp_Pnt&)> point;
  std::function<gp_Dir(const gp_Pnt&)> outward;
  std::function<void(const gp_Pnt&)> prepareTriangle;
};
QPainterPath fiberglassRegion(gui::SketchLayer layer,const QPainterPath& outline);
gui::FoamMassProperties::Covering measureFiberglass(const TopoDS_Shape& shape,
    const QPainterPath& region,const FiberglassProjection& projection,bool wrap,bool parallelMesh=true);
std::vector<gui::FoamMassProperties::Covering> fiberglassMassProperties(
    const gui::ProjectDocument& project,const AssemblyParts& originals,const AssemblyParts& placed,unsigned workers=0);
}

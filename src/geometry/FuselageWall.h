#pragma once
#include "geometry/ProcessingControl.h"
#include <QPointF>
#include <TopoDS_Shape.hxx>
#include <functional>
#include <vector>
#include <optional>
namespace designrc::geometry {
struct FuselageWallSection {
  double x{}, thickness{};
  std::vector<QPointF> perimeter; // Physical Y/Z, clockwise.
};
// Full-section planar inset; no cavity when the requested wall consumes it.
std::optional<std::vector<QPointF>> insetFuselageSection(const FuselageWallSection& section,const ProcessingControl& processing={});
TopoDS_Shape hollowFuselage(const TopoDS_Shape& outside,const std::vector<FuselageWallSection>& sections,bool openNose,bool openTail,
    const std::function<void(const char*)>& progress,const ProcessingControl& processing,TopoDS_Shape* innerCavity=nullptr,bool mirrorConstruction=false);
}

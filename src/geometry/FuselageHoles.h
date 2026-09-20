#pragma once
#include "geometry/FuselageCut.h"
namespace designrc::geometry {
// Four layers: Top/Bottom (Top View), Left/Right (Side View).
TopoDS_Shape cutFuselageHoles(const TopoDS_Shape& body,const TopoDS_Shape& cavity,
    const std::vector<gui::SketchLayer>& holes,const std::vector<gui::SketchLayer>& outlines,
    const std::array<FuselageCutProjection,2>& projections,const ProcessingControl& processing={});
}

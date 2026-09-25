#pragma once
#include "gui/SketchEditor.h"
#include "geometry/ProcessingControl.h"
#include "geometry/FuselageAlignment.h"
#include <TopoDS_Shape.hxx>
#include <array>
#include <functional>
namespace designrc::geometry {
struct FuselageCutProjection {double noseX{},scale{1},transverseOrigin{};};
// Largest post-cut solid is the main body. Other cut pieces remain unchanged.
TopoDS_Shape splitFuselageMainBody(const TopoDS_Shape& body,
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={},
    const FuselageAlignmentSpec* alignment=nullptr);
// The body already contains separate left/right parts. Retain their main solids,
// join only detached cut-out pieces across the seam, and add mating hardware.
TopoDS_Shape finishFuselageHalves(const TopoDS_Shape& body,
    const std::function<void(const char*)>& progress,const ProcessingControl& processing,
    const FuselageAlignmentSpec& alignment);
// Layer 0 projects Top (X/Y) through Z; layer 1 projects Side (X/Z) through Y.
TopoDS_Shape cutFuselage(const TopoDS_Shape& body,const std::vector<gui::SketchLayer>& cuts,
    const std::array<FuselageCutProjection,2>& projections,
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={});
}

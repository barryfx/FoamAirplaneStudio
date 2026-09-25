#pragma once
#include "gui/SketchEditor.h"
#include "geometry/ProcessingControl.h"
#include <TopoDS_Shape.hxx>
namespace designrc::geometry {
void validateStabilizerCuts(const std::vector<gui::SketchLayer>& loops);
// Loops use chord/span model coordinates. Horizontal loops cut both mirrored sides.
TopoDS_Shape cutStabilizerShapes(const TopoDS_Shape& body,const std::vector<gui::SketchLayer>& loops,
    bool horizontal,const ProcessingControl& processing={},bool allowEmpty=false);
}

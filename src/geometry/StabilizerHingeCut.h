#pragma once
#include "geometry/ProcessingControl.h"
#include "gui/SketchEditor.h"
#include "gui/ControlSurfaceState.h"
#include <TopoDS_Shape.hxx>
namespace designrc::geometry {
// Points are already mapped to model X=chord, Y=span, Z=thickness.
TopoDS_Shape cutStabilizerHinge(const TopoDS_Shape& solid,const gui::SketchLayer& lines,
    gui::HingeCut hinge,const ProcessingControl& processing={},bool preferSpan=false);
}

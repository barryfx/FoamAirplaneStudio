#pragma once
#include "geometry/ProcessingControl.h"
#include "gui/ControlSurfaceState.h"
#include <TopoDS_Shape.hxx>
namespace designrc::geometry {
// Production clearance is fixed at 1/16 inch; zero supports legacy benchmark parity.
TopoDS_Shape cutControlSurfaces(const TopoDS_Shape& half,
    const std::array<gui::ControlSurface,2>& controls, QPointF chordAxis,
    QPointF spanAxis,double root,double scale,double endClearanceMm=25.4/16.0,const ProcessingControl& processing={});
}

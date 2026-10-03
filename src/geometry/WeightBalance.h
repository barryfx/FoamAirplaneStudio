#pragma once
#include "gui/WeightBalanceState.h"
#include "geometry/Assembly.h"
#include "gui/SketchEditor.h"
namespace designrc::geometry {
gui::FoamMassProperties foamMassProperties(const AssemblyParts& parts,unsigned workers=0);
// Root leading edge in the same chord frame and scale used by WingSolidBuilder.
double wingRootLeadingEdgeX(const std::vector<gui::SketchLayer>& panels,
    const std::vector<gui::ConstrainedLine>& stations,std::optional<double> wingspanMm);
}

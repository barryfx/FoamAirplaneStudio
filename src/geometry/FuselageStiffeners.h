#pragma once
#include "geometry/FuselageWall.h"
#include "geometry/SparMaterial.h"
#include "gui/StiffenerState.h"
namespace designrc::geometry {
// Full body in fuselage X/Y/Z; sections describe the original external skin.
TopoDS_Shape cutFuselageStiffeners(const TopoDS_Shape& body,const std::vector<FuselageWallSection>& sections,
    double length,const gui::StiffenerState& settings,std::vector<SparMaterial>& materials,
    const ProcessingControl& control={});
}

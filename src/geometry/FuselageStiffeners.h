#pragma once
#include "geometry/FuselageWall.h"
#include "geometry/SparMaterial.h"
#include "gui/StiffenerState.h"
namespace designrc::geometry {
// Body in fuselage X/Y/Z; sections describe the original external skin.
// rightHalf limits cutting and material records to the right side; the caller
// reflects the finished half and its material records afterwards.
TopoDS_Shape cutFuselageStiffeners(const TopoDS_Shape& body,const std::vector<FuselageWallSection>& sections,
    double length,const gui::StiffenerState& settings,std::vector<SparMaterial>& materials,
    const ProcessingControl& control={},bool rightHalf=false);
}

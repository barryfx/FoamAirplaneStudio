#pragma once
#include "geometry/ProcessingControl.h"
#include <TopoDS_Shape.hxx>
#include <utility>
#include <vector>
#include <functional>
namespace designrc::geometry {
// Finished, unsplit main body only. Bay ranges are local unfolded span in mm.
TopoDS_Shape lighteningCavities(const TopoDS_Shape& main,double wall,
    const std::vector<std::pair<double,double>>& bays,
    const std::function<std::pair<double,double>(double,double)>& splitRange,const ProcessingControl& control={});
}

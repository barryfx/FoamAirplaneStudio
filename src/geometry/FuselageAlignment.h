#pragma once
#include "geometry/ProcessingControl.h"
#include <TopoDS_Shape.hxx>
#include <array>
#include <functional>
#include <utility>
#include <vector>
namespace designrc::geometry {
struct FuselageAlignmentSpec {
  // Before hatch cuts: retains the true top/bottom skin at missing seam regions.
  TopoDS_Shape seamReference;
  // Sorted physical X / nominal wall thickness, with the same smooth blend as the loft.
  std::vector<std::pair<double,double>> wallStations;
  bool separateHalves=false; // Y=0 is a mating face rather than interior material.
};
// Halves may arrive in either order. Pins project from negative Y into positive Y.
std::array<TopoDS_Shape,2> addFuselageAlignmentPins(const TopoDS_Shape& mainBody,
    const std::array<TopoDS_Shape,2>& halves,const FuselageAlignmentSpec& specification,
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={});
}

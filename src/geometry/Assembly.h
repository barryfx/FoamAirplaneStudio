#pragma once
#include "geometry/ProcessingControl.h"
#include "gui/AssemblyState.h"
#include <TopoDS_Shape.hxx>
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace designrc::geometry {
struct AssemblyParts {
  TopoDS_Shape fuselage,wing,horizontal,vertical,elevator,rudder;
};
struct AssemblyCutResult {
  AssemblyParts parts;
  std::vector<std::string> collisions;
};
gui::AssemblyState initialAssemblyPlacement(const AssemblyParts& parts);
AssemblyParts placeAssembly(const AssemblyParts& originals,const gui::AssemblyState& state);
TopoDS_Shape assemblyShape(const AssemblyParts& parts);
// Operates on private copies; cached originals and displayed meshes stay immutable.
AssemblyCutResult cutAssemblyIntersections(const AssemblyParts& placed,
    const std::function<void(const char*)>& progress={},const ProcessingControl& control={});
}

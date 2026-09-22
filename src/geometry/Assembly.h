#pragma once
#include "geometry/ProcessingControl.h"
#include "gui/AssemblyState.h"
#include <TopoDS_Shape.hxx>
#include <array>
#include <functional>
#include <string>
#include <vector>
#include <optional>
#include <gp_Pln.hxx>

namespace designrc::geometry {
struct AssemblyPart {
  std::string name;
  TopoDS_Shape shape;
  // Present only for formers: manufacturing section in its local mid-plane.
  std::optional<gp_Pln> formerPlane;
  std::string id; // Source component identity, independent of its display name.
};
struct AssemblyParts {
  TopoDS_Shape fuselage,wing,horizontal,vertical,elevator,rudder;
  std::vector<AssemblyPart> fuselageParts; // Body manufacturing pieces only.
  std::vector<AssemblyPart> inserts; // Standalone formers/tray; never fused or seat-cut.
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

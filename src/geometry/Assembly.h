#pragma once
#include "geometry/SparMaterial.h"
#include "geometry/ProcessingControl.h"
#include "gui/AssemblyState.h"
#include <TopoDS_Shape.hxx>
#include <array>
#include <functional>
#include <string>
#include <vector>
#include <optional>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>

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
  std::vector<AssemblyPart> inserts; // Separate parts: wing cuts formers; tray stays unchanged.
  std::array<gp_Pnt,3> rootCenters{}; // Source root mid-chord, before Assembly placement.
  std::vector<SparMaterial> sparMaterials;
};
struct AssemblyCutResult {
  AssemblyParts parts;
  std::vector<std::string> collisions;
};
gui::AssemblyState initialAssemblyPlacement(const AssemblyParts& parts);
gp_Trsf assemblyComponentPlacement(const AssemblyParts& originals,const gui::AssemblyState& state,std::size_t component);
AssemblyParts placeAssembly(const AssemblyParts& originals,const gui::AssemblyState& state);
TopoDS_Shape assemblyShape(const AssemblyParts& parts);
// Operates on private copies; cached originals and displayed meshes stay immutable.
AssemblyCutResult cutAssemblyIntersections(const AssemblyParts& placed,
    const std::function<void(const char*)>& progress={},const ProcessingControl& control={});
}

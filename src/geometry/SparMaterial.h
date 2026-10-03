#pragma once
#include <gp_Pnt.hxx>
#include <string>
namespace designrc::geometry {
// Density-independent carbon stock properties in its source component frame.
// Kept separate from foam shapes: these are neither export parts nor cutting tools.
struct SparMaterial {
  std::string name;
  double volumeMm3=0;
  gp_Pnt center;
  bool fuselage=false; // Fuselage stiffeners stay in the body frame during wing placement.
};
}

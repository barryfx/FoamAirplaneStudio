#pragma once
#include "gui/SketchEditor.h"
#include <QString>
#include <array>

namespace designrc::gui {
// Indices follow the four component workspaces: Wing, Fuselage, Horiz, Vert.
enum class CoverSide { Top, Bottom, Left, Right };
inline constexpr double gramsPerSquareMeterPerOzYard = 33.9057474749;
struct FiberglassPatch {
  QString name="Patch 1";
  bool wrap=true;
  CoverSide side=CoverSide::Top;
  double clothGm2=50;
  bool imperialCloth=false;
  bool projectClothUnits=true; // imperialCloth applies only to an explicit override.
  bool automaticResin=true;
  double resinThicknessMm=0.046;
};
inline double defaultResinThickness(double clothGm2) {
  // Light woven cloth: bulk density 900 kg/m³; E-glass 2550 kg/m³.
  // Fill its interstices, then allow a 0.01 mm thin surface coat.
  return clothGm2*(1.0/900-1.0/2550)+0.01;
}
inline double resinThickness(const FiberglassPatch& patch) {
  return patch.automaticResin ? defaultResinThickness(patch.clothGm2) : patch.resinThicknessMm;
}
struct FiberglassState {
  SketchState sketch;
  std::vector<FiberglassPatch> patches{1};
};
}

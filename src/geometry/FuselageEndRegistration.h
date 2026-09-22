#pragma once
#include "gui/SketchEditor.h"
#include <optional>
#include <cmath>
#include <vector>
namespace designrc::geometry {
inline constexpr double fuselageEndToleranceMm=0.5;
// Model-only sampled outline: original sketch points and saved data stay intact.
struct RegisteredFuselageOutline {
  std::vector<QPointF> boundary;
  double noseX{},tailX{},scale{};
  bool flatNose=false,flatTail=false;
  bool noseStation(double x) const {return std::abs(x-noseX)*scale<=(flatNose?fuselageEndToleranceMm:1e-6);}
  bool tailStation(double x) const {return std::abs(x-tailX)*scale<=(flatTail?fuselageEndToleranceMm:1e-6);}
};
RegisteredFuselageOutline registerFuselageEnds(const gui::SketchLayer& outline,
    std::optional<double> lengthMm={});
}

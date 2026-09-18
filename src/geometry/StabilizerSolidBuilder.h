#pragma once
#include "geometry/ProcessingControl.h"
#include "gui/SketchEditor.h"
#include "gui/ControlSurfaceState.h"
#include "domain/AirfoilProfile.h"
#include <TopoDS_Shape.hxx>
#include <functional>
namespace designrc::geometry {
struct StabilizerSolidInput {
  gui::SketchLayer outline;
  domain::AirfoilProfile airfoil;
  double millimetersPerSceneUnit = 1;
  bool horizontal = true;
  gui::SketchLayer hingeLines;
  gui::HingeCut hingeCut=gui::HingeCut::Tape;
  std::vector<gui::SketchLayer> cutShapes;
};
TopoDS_Shape buildStabilizerSolid(const StabilizerSolidInput& input,
    const std::function<void(const char*)>& progress = {}, const ProcessingControl& control = {});
}

#pragma once
#include "geometry/SparMaterial.h"
#include "geometry/ProcessingControl.h"
#include "gui/AirfoilLibrary.h"
#include <TopoDS_Shape.hxx>
#include <functional>
#include "gui/ControlSurfaceState.h"
#include "gui/SparState.h"
#include "gui/LighteningState.h"
namespace designrc::geometry {
// Scene coordinates in, millimetres out. Manual wingspan is the full span.
struct WingSolidInput {
  std::vector<gui::SketchLayer> panels;
  std::vector<gui::ConstrainedLine> stations;
  std::vector<gui::LibraryAirfoil> airfoils;
  std::optional<double> wingspanMm;
  std::vector<double> dihedralDegrees; // Relative root angles; empty means all zero.
  gui::PanelControls controls{1};
  gui::PanelSpars spars{1};
  gui::LighteningState lightening;
};
struct WingBuildOptions {
  ProcessingControl processing;
  unsigned maxPanelThreads=0; // 0: bounded automatic concurrency; 1: sequential benchmark.
  std::vector<SparMaterial>* materials=nullptr; // Published only after successful generation.
};
struct WingBuildResult { TopoDS_Shape shape; std::vector<SparMaterial> spars; };
WingBuildResult buildWingModel(const WingSolidInput& input,const std::function<void(const char*)>& progress={},const WingBuildOptions& options={});
// Progress calls are serialized but may originate on any worker thread.
TopoDS_Shape buildWingSolid(const WingSolidInput& input, const std::function<void(const char*)>& progress = {},const WingBuildOptions& options = {});
}

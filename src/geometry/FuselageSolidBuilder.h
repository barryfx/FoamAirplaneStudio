#pragma once
#include "gui/SketchEditor.h"
#include "geometry/ProcessingControl.h"
#include <TopoDS_Shape.hxx>
#include <QRectF>
#include <functional>
namespace designrc::geometry {
struct FuselageSolidInput {
  std::vector<gui::SketchLayer> outlines;
  std::vector<gui::ConstrainedLine> stations;
  std::vector<gui::SketchLayer> profiles;
  std::optional<double> lengthMm; // Empty: Side View scene coordinates are millimetres.
  bool thicken=false;
  std::vector<gui::SketchLayer> cuts; // Optional Top/Side cut paths, applied after thickening.
  std::optional<QRectF> servoTray; // Side View rectangle; height is tray thickness.
  std::vector<QRectF> formers; // Side View masks; width is physical thickness after scaling.
};
struct FuselageBuildResult {TopoDS_Shape shape,body,servoTray,servoTrayTopFaces;std::vector<TopoDS_Shape> formers;};
FuselageBuildResult buildFuselageModel(const FuselageSolidInput& input,
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={});
// 64 clockwise samples; indices 0/16/32/48 match drawn top/right/bottom/left.
std::vector<QPointF> sampleFuselageProfile(const gui::SketchLayer& profile);
TopoDS_Shape buildFuselageSolid(const FuselageSolidInput& input,
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={});
}

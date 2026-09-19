#pragma once

#include <QPoint>
#include "gui/ReferenceImage.h"
#include "geometry/FuselageSolidBuilder.h"
#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <Graphic3d_TransformPers.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <TopoDS_Shape.hxx>

#include <vector>
#include <optional>
#include <array>

class QMouseEvent;
class QPaintEngine;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QWheelEvent;

namespace designrc::gui {

enum class CameraView { Reset, Top, Bottom, Front, Back, Left, Right };
struct CameraState {
  std::array<double,3> eye{}, center{}, up{};
  double scale{}, fov{};
  int projection{};
};

class OcctViewport final : public QWidget {
public:
  explicit OcctViewport(QWidget* parent = nullptr);

  void displayShape(const TopoDS_Shape& shape, bool fit=true);
  void displayMaterialShapes(const TopoDS_Shape& wood,
                             const TopoDS_Shape& carbonFiber,
                             const TopoDS_Shape& aluminum,
                             const TopoDS_Shape& steel,
                             const TopoDS_Shape& fiberglass);
  void displayAssembly(const std::array<TopoDS_Shape,4>& parts,int selected);
  void setAssemblyReference(const ProjectReference& reference, const geometry::FuselageSideTransform& transform);
  void clearShape();
  void fitAll();
  void setCameraView(CameraView cameraView);
  std::optional<CameraState> cameraState() const;
  void restoreCamera(const std::optional<CameraState>& state);
  void resetCamera();

protected:
  QPaintEngine* paintEngine() const override;
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  void initializeViewer();
  void displayPendingShapes();
  void displayViewGizmo();
  void clearAssemblyReference();
  void redraw();

  Handle(V3d_Viewer) viewer_;
  Handle(V3d_View) view_;
  Handle(AIS_InteractiveContext) context_;
  std::vector<Handle(AIS_Shape)> displayedShapes_;
  Handle(Graphic3d_TransformPers) viewGizmoPersistence_;
  std::vector<Handle(AIS_InteractiveObject)> viewGizmoObjects_;
  TopoDS_Shape pendingWoodShape_;
  TopoDS_Shape pendingCarbonFiberShape_;
  TopoDS_Shape pendingAluminumShape_;
  TopoDS_Shape pendingSteelShape_;
  TopoDS_Shape pendingFiberglassShape_;
  QPoint lastMousePosition_;
  std::optional<std::array<TopoDS_Shape,4>> pendingAssembly_;
  int assemblySelected_=-1;
  std::vector<Handle(AIS_Shape)> referenceObjects_;
  std::vector<ReferencePage> assemblyReferencePages_;
  bool assemblyReferenceToScale_=false;
  geometry::FuselageSideTransform assemblyReferenceTransform_{};
  bool initialized_{false};
  bool foamAppearance_{false};
  bool orbiting_{false};
  bool panning_{false};
};

} // namespace designrc::gui

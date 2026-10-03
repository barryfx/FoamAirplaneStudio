#pragma once

#include "gui/TechnicalDrawing.h"

#include <QGraphicsView>
#include "gui/ReferenceImage.h"
#include "gui/SketchEditor.h"
#include "gui/ServoTrayEditor.h"
#include "gui/FormerEditor.h"
#include "gui/ControlSurfaceEditor.h"
#include <QString>
#include <array>

class QGraphicsScene;
class QResizeEvent;
class QShowEvent;
class QWheelEvent;

namespace designrc::gui {
struct PlanViewState { double zoom{1}; QPointF center; };

class PlanViewport final : public QGraphicsView {
public:
  explicit PlanViewport(QWidget* parent = nullptr);
  std::function<void(QPainter&)> balanceOverlay;

  void setDocument(const TechnicalDrawingDocument& document);
  void clearPlan();
  void setReferenceBackground(const std::vector<ReferencePage>& pages, bool toScale);
  void fitAll();
  PlanViewState viewState() const;
  void restoreView(const PlanViewState& state);
  ControlSurfaceEditor& controlSurfaceEditor() { return *controlSurfaceEditor_; }
  FormerEditor& formerEditor() { return *formerEditor_; }
  ServoTrayEditor& servoTrayEditor() { return *servoTrayEditor_; }
  SketchEditor& fuselageHoleEditor() { return *fuselageHoleEditor_; }
  SketchEditor& fiberglassEditor(int index) { return *fiberglassEditors_.at(index); }
  SketchEditor& fuselageCutEditor() { return *fuselageCutEditor_; }
  SketchEditor& fuselageProfileEditor() { return *fuselageProfileEditor_; }
  SketchEditor& fuselageSketchEditor() { return *fuselageSketchEditor_; }
  SketchEditor& stabilizerSketchEditor(int index) { return *stabilizerSketchEditors_.at(index); }
  SketchEditor& stabilizerCutEditor(int index) { return *stabilizerCutEditors_.at(index); }
  SketchEditor& stabilizerHingeEditor(int index) { return *stabilizerHingeEditors_.at(index); }
  SketchEditor& sketchEditor() { return *sketchEditor_; }
  SketchEditor& airfoilSketchEditor() { return *airfoilSketchEditor_; }
  [[nodiscard]] bool exportPdf(const QString& path, QString& error) const;

protected:
  void drawForeground(QPainter* painter, const QRectF& rect) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  ControlSurfaceEditor* controlSurfaceEditor_{};
  std::array<SketchEditor*, 2> stabilizerSketchEditors_{};
  std::array<SketchEditor*, 2> stabilizerHingeEditors_{};
  std::array<SketchEditor*,2> stabilizerCutEditors_{};
  SketchEditor* sketchEditor_{};
  SketchEditor* fuselageSketchEditor_{};
  SketchEditor* fuselageProfileEditor_{};
  SketchEditor* fuselageCutEditor_{};
  SketchEditor* fuselageHoleEditor_{};
  std::array<SketchEditor*,4> fiberglassEditors_{};
  ServoTrayEditor* servoTrayEditor_{};
  FormerEditor* formerEditor_{};
  SketchEditor* airfoilSketchEditor_{};
  QGraphicsScene* scene_{};
  TechnicalDrawingDocument document_;
  std::vector<ReferencePage> referencePages_;
  bool referenceToScale_{false};
  bool fitOnNextResize_{false};
  bool fittingWidth_{false};
  QSize lastViewSize_;
  std::optional<PlanViewState> restoredView_;
  void applyRestoredView();
  void applyZoomAnchor();
  std::optional<std::pair<QPointF,QPointF>> zoomAnchor_; // Scene point, viewport cursor.
  bool anchoringZoom_=false;
};

} // namespace designrc::gui


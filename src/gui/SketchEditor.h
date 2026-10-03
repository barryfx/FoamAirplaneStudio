#pragma once

#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <vector>
#include <functional>
#include <optional>
#include "gui/ConstrainedLineEditor.h"

class QGraphicsView;
class QPainter;

namespace designrc::gui {
enum class SketchTool { None, Line, Spline, Circle };
enum class SketchAppearance { Outline, Cut, Hole, Fiberglass };
struct SketchCurve {
  SketchTool type;
  std::vector<std::size_t> points;
};
struct SketchLayer {
  std::vector<QPointF> points;
  std::vector<SketchCurve> curves;
  std::optional<std::size_t> leadingEdge; // Stabilizer root endpoint; follows point edits.
};
bool isSketchEndpoint(const SketchLayer& layer, std::size_t index);

struct SketchState {
  std::vector<SketchLayer> layers{1};
  std::vector<QPointF> pending;
  SketchTool tool{SketchTool::None};
  int active{}, selected{-1};
  bool editing{};
};

// Reusable scene-coordinate sketch model, interaction controller and overlay.
// The view owns this object; no graphics items survive scene rebuilds.
class SketchEditor final : public QObject {
  Q_OBJECT
public:
  explicit SketchEditor(QGraphicsView* view, SketchAppearance appearance = SketchAppearance::Outline);
  void setLayerCount(int count);
  void setLayerSelectionMode(bool enabled) { layerSelectionMode_=enabled; }
  void deleteActiveLayer();
  void setActiveLayer(int index);
  void setEditing(bool enabled);
  void setTool(SketchTool tool);
  void setSelectingLeadingEdge(bool enabled);
  bool selectingLeadingEdge() const { return selectingLeadingEdge_; }
  void setSnapAcrossLayers(bool enabled) { snapAcrossLayers_ = enabled; }
  void setContinuousLineMode(bool enabled) { continuousLineMode_=enabled; }
  void setClosedLoopMode(bool enabled) { closedLoopMode_ = enabled; }
  void setShowOpenEndpoints(bool enabled);
  void setHighlightedLayer(int index);
  void setEscapeEndsSession(bool enabled) { escapeEndsSession_ = enabled; }
  void reset();
  void finish();
  SketchState state() const;
  void restoreState(const SketchState& state);
  void mapPoints(const std::function<QPointF(QPointF)>& map);
  void paint(QPainter& painter, bool activeOnly=false) const;
  const std::vector<SketchLayer>& layers() const { return layers_; }
  SketchTool tool() const { return tool_; }
  int activeLayer() const { return active_; }
  int selectedCurve() const { return selected_; }
  ConstrainedLineEditor& stationEditor() { return stations_; }
  static QPainterPath fittedPath(const std::vector<QPointF>& points, SketchTool type);
signals:
  void changed();
  void stationsChanged();
  void stationSelectionChanged(int index);
  void sessionFinished();
protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
private:
  int nearest(QPointF position) const;
  int curveAt(QPointF position) const;
  void deleteSelected();
  void pick(QPointF position);
  void refresh();
  QGraphicsView* view_;
  SketchAppearance appearance_;
  std::vector<SketchLayer> layers_{1};
  std::vector<QPointF> pending_;
  std::optional<QPointF> circlePreview_;
  SketchTool tool_{SketchTool::None};
  int active_{0};
  int dragging_{-1};
  int selected_{-1};
  int highlightedLayer_{-1};
  bool editing_{false};
  bool selectingLeadingEdge_{false};
  bool closedLoopMode_{false};
  bool showOpenEndpoints_{false};
  bool continuousLineMode_=false;
  bool layerSelectionMode_=false;
  bool snapAcrossLayers_{true};
  bool escapeEndsSession_{false};
  ConstrainedLineEditor stations_;
};
} // namespace designrc::gui

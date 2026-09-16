#pragma once
#include <QPointF>
#include <optional>
#include <vector>
class QGraphicsView;
class QPainter;
class QEvent;
namespace designrc::gui {
class SketchEditor;
struct CurveAnchor {
  int layer{};
  int curve{};
  double parameter{}; // Fraction of displayed curve arc length.
  QPointF position;
};
enum class LineAlignment { Free, Horizontal, Vertical };
struct ConstrainedLine {
  CurveAnchor first, second;
  LineAlignment alignment{LineAlignment::Free};
  std::optional<std::size_t> airfoil;
};
struct StationState {
  std::vector<ConstrainedLine> lines;
  int selected{-1};
  std::optional<CurveAnchor> first;
};
inline bool stationDirectionsAgree(const ConstrainedLine& a,const ConstrainedLine& b) {
  return QPointF::dotProduct(a.second.position-a.first.position,b.second.position-b.first.position)>0;
}
// Reusable two-point lines whose endpoints remain attached to source curves.
class ConstrainedLineEditor {
public:
  ConstrainedLineEditor(QGraphicsView& view, SketchEditor& source);
  void setEnabled(bool enabled);
  void setActivePanel(int panel);
  int activePanel() const { return panel_; }
  bool allPanelsDefined() const;
  bool enabled() const { return enabled_; }
  void setSelectionEnabled(bool enabled);
  bool selectionEnabled() const { return selectionOnly_; }
  void assignSelectedAirfoil(std::size_t airfoil);
  bool event(QEvent* event, bool onViewport);
  void paint(QPainter& painter) const;
  void reset();
  StationState state() const { return {lines_, moving_>=0 ? moving_ : selected_, first_}; }
  void restoreState(const StationState& state);
  void sourceCurveRemoved(int layer, int curve);
  void synchronize();
  const std::vector<ConstrainedLine>& lines() const { return lines_; }
  int selectedLine() const { return selected_; }
  std::optional<CurveAnchor> hoverPoint() const { return hover_; }
private:
  std::optional<CurveAnchor> project(QPointF point, int layer = -1, int curve = -1,
      bool requireNear = true) const;
  std::optional<CurveAnchor> intersect(const CurveAnchor& onCurve, QPointF through,
      LineAlignment alignment) const;
  void updateHover(QPointF point);
  void cancel();
  void dropMove();
  void notify();
  QGraphicsView& view_;
  SketchEditor& source_;
  std::vector<ConstrainedLine> lines_;
  std::optional<CurveAnchor> first_, hover_;
  std::optional<ConstrainedLine> beforeMove_;
  LineAlignment previewAlignment_{LineAlignment::Free};
  int panel_{};
  int selected_{-1}, moving_{-1}, movingEnd_{};
  bool enabled_{};
  bool selectionOnly_{};
};
}

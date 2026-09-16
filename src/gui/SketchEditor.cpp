#include "gui/SketchEditor.h"
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <Geom2dAPI_Interpolate.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <cmath>

namespace designrc::gui {
SketchEditor::SketchEditor(QGraphicsView* view) : QObject{view}, view_{view}, stations_{*view, *this} {
  view_->viewport()->installEventFilter(this);
  view_->installEventFilter(this);
}
void SketchEditor::refresh() { stations_.synchronize(); view_->viewport()->update(); emit changed(); }
SketchState SketchEditor::state() const { return {layers_,pending_,tool_,active_,selected_,editing_}; }
void SketchEditor::restoreState(const SketchState& state) {
  layers_=state.layers; pending_=state.pending; tool_=state.tool;
  active_=state.active; selected_=state.selected; editing_=state.editing; dragging_=-1;
  view_->viewport()->setCursor(editing_ && tool_!=SketchTool::None ? Qt::CrossCursor : Qt::ArrowCursor);
  view_->viewport()->update();
}
void SketchEditor::mapPoints(const std::function<QPointF(QPointF)>& map) {
  finish();
  for (auto& layer : layers_) for (auto& point : layer.points) point = map(point);
  refresh();
}
void SketchEditor::setLayerCount(int count) {
  finish();
  // Component UIs own their limits; reusable sketch collections may contain
  // more layers than the wing panel spinner permits.
  layers_.resize(std::max(count, 1));
  selected_ = -1;
  active_ = std::min(active_, static_cast<int>(layers_.size()) - 1);
  dragging_ = -1;
  refresh();
}
void SketchEditor::setActiveLayer(int index) {
  if (index < 0 || index >= static_cast<int>(layers_.size())) return;
  finish(); active_ = index; dragging_ = -1; selected_ = -1; view_->viewport()->update(); emit changed();
}
void SketchEditor::setEditing(bool enabled) {
  if (enabled) { stations_.setEnabled(false); stations_.setSelectionEnabled(false); }
  if (!enabled) finish();
  editing_ = enabled; dragging_ = -1;
  if (!enabled) selected_ = -1;
  view_->viewport()->setCursor((enabled && tool_ != SketchTool::None) || stations_.enabled()
      ? Qt::CrossCursor : Qt::ArrowCursor);
  view_->viewport()->update();
}
void SketchEditor::setTool(SketchTool tool) {
  finish(); tool_ = tool; dragging_ = -1; selected_ = -1;
  setEditing(editing_);
  if (editing_ && tool != SketchTool::None) view_->setFocus();
}
void SketchEditor::reset() {
  stations_.reset();
  pending_.clear(); layers_ = std::vector<SketchLayer>(1);
  active_ = 0; dragging_ = -1; selected_ = -1; tool_ = SketchTool::None; editing_ = false; refresh();
}
int SketchEditor::curveAt(QPointF position) const {
  QPainterPathStroker hitArea;
  hitArea.setWidth(16); // Eight screen pixels on either side, independent of zoom.
  hitArea.setCapStyle(Qt::RoundCap);
  hitArea.setJoinStyle(Qt::RoundJoin);
  const auto& layer = layers_[active_];
  const auto transform = view_->viewportTransform();
  // Prefer the last drawn curve when curves overlap.
  for (int i = static_cast<int>(layer.curves.size()) - 1; i >= 0; --i) {
    const auto& curve = layer.curves[i];
    std::vector<QPointF> points;
    for (auto id : curve.points) points.push_back(layer.points[id]);
    if (hitArea.createStroke(transform.map(fittedPath(points, curve.type))).contains(transform.map(position)))
      return i;
  }
  return -1;
}
void SketchEditor::deleteSelected() {
  auto& layer = layers_[active_];
  if (selected_ < 0 || selected_ >= static_cast<int>(layer.curves.size())) return;
  stations_.sourceCurveRemoved(active_, selected_);
  layer.curves.erase(layer.curves.begin() + selected_);
  // Keep shared junctions; remove only points no remaining curve references.
  std::vector<bool> used(layer.points.size());
  for (const auto& curve : layer.curves) for (auto id : curve.points) used[id] = true;
  std::vector<std::size_t> remap(layer.points.size());
  std::vector<QPointF> points;
  for (std::size_t i = 0; i < layer.points.size(); ++i)
    if (used[i]) { remap[i] = points.size(); points.push_back(layer.points[i]); }
  for (auto& curve : layer.curves) for (auto& id : curve.points) id = remap[id];
  layer.points = std::move(points);
  selected_ = -1; dragging_ = -1; refresh();
}
int SketchEditor::nearest(QPointF position) const {
  const auto screen = view_->viewportTransform().map(position);
  const auto& points = layers_[active_].points;
  double best = 8.0; int result = -1;
  for (std::size_t i = 0; i < points.size(); ++i) {
    const auto delta = view_->viewportTransform().map(points[i]) - screen;
    const double distance = std::hypot(delta.x(), delta.y());
    if (distance <= best) { best = distance; result = static_cast<int>(i); }
  }
  return result;
}
QPainterPath SketchEditor::fittedPath(const std::vector<QPointF>& points, SketchTool type) {
  QPainterPath path;
  if (points.empty()) return path;
  path.moveTo(points.front());
  if (points.size() < 2) return path;
  if (type == SketchTool::Line || points.size() == 2) { path.lineTo(points.back()); return path; }
  try {
    const bool periodic = points.size() >= 4 && points.front() == points.back();
    const int count = static_cast<int>(points.size()) - (periodic ? 1 : 0);
    occ::handle<NCollection_HArray1<gp_Pnt2d>> nodes =
        new NCollection_HArray1<gp_Pnt2d>(1, count);
    for (int i = 0; i < count; ++i)
      nodes->SetValue(i + 1, gp_Pnt2d{points[i].x(), points[i].y()});
    Geom2dAPI_Interpolate fit{nodes, periodic, 1e-9};
    fit.Perform();
    if (!fit.IsDone()) return {};
    const auto& curve = fit.Curve();
    // Tessellation is display-only; interpolation nodes remain the source data.
    const int samples = std::max(128, static_cast<int>(points.size()) * 32);
    for (int i = 1; i <= samples; ++i) {
      const auto p = curve->Value(curve->FirstParameter() +
          (curve->LastParameter() - curve->FirstParameter()) * i / samples);
      path.lineTo(p.X(), p.Y());
    }
  } catch (const Standard_Failure&) { return {}; }
  return path;
}
void SketchEditor::pick(QPointF position) {
  const int snapped = nearest(position);
  if (snapped >= 0) position = layers_[active_].points[snapped];
  for (const auto& layer : layers_) {
    if (snapped >= 0) break;
    for (auto point : layer.points)
      if (QLineF{view_->viewportTransform().map(point), view_->viewportTransform().map(position)}.length() <= 8)
        { position = point; break; }
  }
  for (auto point : pending_)
    if (QLineF{view_->viewportTransform().map(point), view_->viewportTransform().map(position)}.length() <= 8)
      { position = point; break; }
  // Repeated nodes cannot define an interpolating curve. Do not create orphans.
  if (closedLoopMode_ && pending_.size() >= 3 && position == pending_.front()) {
    pending_.push_back(position); finish(); return;
  }
  if (std::any_of(pending_.begin(), pending_.end(), [position](QPointF p) {
      return QLineF{p, position}.length() < 1e-8;
    })) return;
  pending_.push_back(position);
  if (tool_ == SketchTool::Line && pending_.size() == 2) finish();
  view_->viewport()->update();
}
void SketchEditor::finish() {
  if(pending_.empty())return; // Navigation must not resynchronize unchanged anchors.
  if (pending_.size() >= 2 && !fittedPath(pending_, tool_).isEmpty()) {
    auto& layer = layers_[active_];
    SketchCurve curve{tool_, {}};
    for (auto point : pending_) {
      const auto found = std::find(layer.points.begin(), layer.points.end(), point);
      if (found == layer.points.end()) {
        curve.points.push_back(layer.points.size()); layer.points.push_back(point);
      } else curve.points.push_back(static_cast<std::size_t>(found - layer.points.begin()));
    }
    layer.curves.push_back(std::move(curve));
  }
  pending_.clear(); refresh();
}
bool SketchEditor::eventFilter(QObject* watched, QEvent* event) {
  if (stations_.enabled() || stations_.selectionEnabled()) return stations_.event(event, watched == view_->viewport());
  if (!editing_) return false;
  if (event->type() == QEvent::KeyPress && tool_ == SketchTool::None &&
      static_cast<QKeyEvent*>(event)->key() == Qt::Key_Delete) {
    deleteSelected(); return true;
  }
  if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
    finish(); dragging_ = -1; selected_ = -1; view_->viewport()->update();
    if (escapeEndsSession_) emit sessionFinished();
    return true;
  }
  if (watched != view_->viewport()) return false;
  if (event->type() == QEvent::MouseButtonPress) {
    auto* mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() != Qt::LeftButton) return false;
    view_->setFocus();
    const auto position = view_->mapToScene(mouse->position().toPoint());
    if (tool_ != SketchTool::None) pick(position);
    else {
      dragging_ = nearest(position);
      selected_ = curveAt(position);
      view_->viewport()->update();
    }
    return true;
  }
  if (event->type() == QEvent::MouseMove && dragging_ >= 0) {
    auto* mouse = static_cast<QMouseEvent*>(event);
    if (!(mouse->buttons() & Qt::LeftButton)) { dragging_ = -1; return false; }
    auto& layer = layers_[active_];
    selected_ = -1;
    const auto before = layer.points[dragging_];
    layer.points[dragging_] = view_->mapToScene(mouse->position().toPoint());
    for (const auto& curve : layer.curves) {
      std::vector<QPointF> points;
      for (auto id : curve.points) points.push_back(layer.points[id]);
      auto uniquePoints = points;
      if (closedLoopMode_ && uniquePoints.size() > 3 && uniquePoints.front() == uniquePoints.back()) uniquePoints.pop_back();
      if (std::any_of(uniquePoints.begin(), uniquePoints.end(), [&](QPointF p) {
          return std::count(uniquePoints.begin(), uniquePoints.end(), p) > 1;
        }) || fittedPath(points, curve.type).isEmpty()) {
        layer.points[dragging_] = before; break;
      }
    }
    refresh(); return true;
  }
  if (event->type() == QEvent::MouseButtonRelease &&
      static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
    dragging_ = -1; return true;
  }
  return false;
}
void SketchEditor::paint(QPainter& painter) const {
  painter.save();
  const double radius = 4.0 / std::max(1e-9, view_->transform().m11());
  for (int i = 0; i < static_cast<int>(layers_.size()); ++i) {
    const auto& layer = layers_[i];
    // Keep completed outlines equally legible outside Outline mode. The dark
    // border separates light blue from white paper; blue stands out on ink.
    QPen pen{QColor{80, 200, 255}};
    pen.setCosmetic(true); pen.setWidthF(3);
    QPen border{QColor{20, 65, 95}};
    border.setCosmetic(true); border.setWidthF(5);
    painter.setBrush(Qt::NoBrush);
    for (const auto& curve : layer.curves) {
      std::vector<QPointF> points;
      for (auto id : curve.points) points.push_back(layer.points[id]);
      const auto path = fittedPath(points, curve.type);
      painter.setPen(border); painter.drawPath(path);
      painter.setPen(pen); painter.drawPath(path);
    }
    if (i == active_ && editing_) {
      painter.setPen(pen);
      painter.setBrush(Qt::white);
      for (auto point : layer.points) painter.drawEllipse(point, radius, radius);
    }
  }
  if (editing_ && selected_ >= 0) {
    const auto& layer = layers_[active_];
    const auto& curve = layer.curves[selected_];
    std::vector<QPointF> points;
    for (auto id : curve.points) points.push_back(layer.points[id]);
    const auto path = fittedPath(points, curve.type);
    painter.setBrush(Qt::NoBrush);
    QPen halo{Qt::white}; halo.setCosmetic(true); halo.setWidthF(7);
    painter.setPen(halo); painter.drawPath(path);
    QPen highlight{QColor{255, 65, 0}}; highlight.setCosmetic(true); highlight.setWidthF(4);
    painter.setPen(highlight); painter.drawPath(path);
  }
  if (editing_) {
    QPen pen{QColor{220, 110, 0}}; pen.setCosmetic(true); pen.setStyle(Qt::DashLine);
    painter.setPen(pen); painter.setBrush(Qt::white);
    painter.drawPath(fittedPath(pending_, tool_));
    for (auto point : pending_) painter.drawEllipse(point, radius, radius);
  }
  painter.restore();
  stations_.paint(painter);
}
} // namespace designrc::gui

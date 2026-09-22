#include "gui/SketchBoundary.h"
#include <QLineF>
#include <cmath>
namespace designrc::gui {
std::optional<std::vector<QPointF>> closedSketchBoundary(const SketchLayer& layer) {
  if(layer.curves.size()==1&&layer.curves[0].type==SketchTool::Circle) {
    const auto& ids=layer.curves[0].points;
    if(ids.size()!=2||layer.points.size()!=2||ids[0]>=2||ids[1]>=2||ids[0]==ids[1])return {};
    const auto path=SketchEditor::fittedPath({layer.points[ids[0]],layer.points[ids[1]]},SketchTool::Circle);
    if(path.isEmpty())return {};
    std::vector<QPointF> boundary;
    for(int i=0;i<path.elementCount();++i){const auto e=path.elementAt(i);boundary.push_back({e.x,e.y});}
    boundary.back()=boundary.front();return boundary;
  }
  if (layer.points.size() < 3 || layer.curves.empty()) return {};
  std::vector<int> degree(layer.points.size());
  for (const auto& curve : layer.curves) {
    if (curve.points.size() < 2) return {};
    for (std::size_t i = 1; i < curve.points.size(); ++i) {
      if (curve.points[i - 1] >= degree.size() || curve.points[i] >= degree.size()) return {};
      ++degree[curve.points[i - 1]]; ++degree[curve.points[i]];
    }
  }
  for (int d : degree) if (d != 2) return {};
  std::vector<bool> used(layer.curves.size());
  const auto start = layer.curves.front().points.front();
  auto current = start;
  std::vector<QPointF> boundary;
  for (std::size_t step = 0; step < layer.curves.size(); ++step) {
    int next = -1; bool reverse = false;
    for (int i = 0; i < static_cast<int>(layer.curves.size()); ++i) {
      if (used[i]) continue;
      if (layer.curves[i].points.front() == current) { next = i; break; }
      if (layer.curves[i].points.back() == current) { next = i; reverse = true; break; }
    }
    if (next < 0) return {};
    used[next] = true;
    const auto& curve = layer.curves[next];
    std::vector<QPointF> points;
    for (auto id : curve.points) points.push_back(layer.points[id]);
    const auto path = SketchEditor::fittedPath(points, curve.type);
    if (path.isEmpty()) return {};
    for (int j = 0; j < path.elementCount(); ++j) {
      const auto element = path.elementAt(reverse ? path.elementCount() - j - 1 : j);
      QPointF point{element.x, element.y};
      if (boundary.empty() || QLineF{boundary.back(), point}.length() > 1e-9) boundary.push_back(point);
    }
    current = reverse ? curve.points.front() : curve.points.back();
    if (current == start && step + 1 != layer.curves.size()) return {}; // More than one loop.
  }
  if (current != start || boundary.size() < 4) return {};
  boundary.back() = boundary.front();
  double twiceArea = 0;
  // Translate before the shoelace sum to avoid cancellation far from the origin.
  for (std::size_t i = 1; i < boundary.size(); ++i) {
    const auto a = boundary[i - 1] - boundary.front(), b = boundary[i] - boundary.front();
    twiceArea += a.x() * b.y() - a.y() * b.x();
  }
  if (!std::isfinite(twiceArea) || std::abs(twiceArea) < 1e-9) return {};
  return boundary;
}
}

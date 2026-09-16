#include "gui/AirfoilLibrary.h"
#include <QFile>
#include <QFileInfo>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>
namespace designrc::gui {
std::optional<std::vector<QPointF>> closedAirfoilBoundary(const SketchLayer& layer) {
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
bool AirfoilLibrary::loadDat(const QString& path, QString& error) {
  error.clear();
  QFile file{path};
  if (!file.open(QIODevice::ReadOnly)) { error = file.errorString(); return false; }
  QString content = QString::fromUtf8(file.readAll());
  if (content.startsWith(QChar{0xfeff})) content.remove(0, 1);
  content = content.trimmed();
  if (content.isEmpty()) { error = "The airfoil file is empty."; return false; }
  const QString first = content.section('\n', 0, 0).trimmed();
  std::istringstream firstValues{first.toStdString()};
  double x, y;
  const bool coordinatesFirst = static_cast<bool>(firstValues >> x >> y);
  QString name = coordinatesFirst ? QFileInfo{path}.completeBaseName() : first;
  if (name.isEmpty()) name = "Imported airfoil";
  if (coordinatesFirst) content.prepend(name + '\n');
  try {
    // Lednicer DATs declare two LE-to-TE surface counts after the name.
    // Convert that layout into the contour ordering used by the inherited importer.
    std::vector<domain::Point2> coordinates;
    std::istringstream rows{content.section('\n', 1).toStdString()};
    std::string row;
    while (std::getline(rows, row)) {
      std::replace(row.begin(), row.end(), ',', ' ');
      std::istringstream values{row}; domain::Point2 point;
      if (values >> point.x >> point.y) coordinates.push_back(point);
    }
    if (!coordinates.empty() && coordinates.front().x >= 2 && coordinates.front().y >= 2 &&
        std::floor(coordinates.front().x) == coordinates.front().x &&
        std::floor(coordinates.front().y) == coordinates.front().y &&
        coordinates.front().x + coordinates.front().y == static_cast<double>(coordinates.size() - 1)) {
      const auto upperCount = static_cast<std::size_t>(coordinates.front().x);
      std::vector<domain::Point2> upper(coordinates.begin() + 1, coordinates.begin() + 1 + upperCount);
      std::vector<domain::Point2> lower(coordinates.begin() + 1 + upperCount, coordinates.end());
      if (upper.front().x < upper.back().x) std::reverse(upper.begin(), upper.end());
      if (lower.front().x > lower.back().x) std::reverse(lower.begin(), lower.end());
      std::ostringstream normalized; normalized << name.toStdString() << '\n' << std::setprecision(17);
      for (auto point : upper) normalized << point.x << ' ' << point.y << '\n';
      for (auto point : lower) normalized << point.x << ' ' << point.y << '\n';
      content = QString::fromStdString(normalized.str());
    }
    std::istringstream input{content.toUtf8().toStdString()};
    auto profile = domain::AirfoilProfile::fromDat(input);
    // Reject unusable input before it reaches the selectable library.
    const auto samples = profile.resampled(41);
    for (auto point : samples) if (!std::isfinite(point.x) || !std::isfinite(point.y))
      throw std::runtime_error("Airfoil coordinates must be finite.");
    const auto [low, high] = std::minmax_element(samples.begin(), samples.end(),
        [](auto a, auto b) { return a.y < b.y; });
    if (high->y - low->y < 1e-9) throw std::runtime_error("Airfoil has no thickness.");
    LibraryAirfoil entry{name, std::move(profile), {}, {}};
    for (auto point : entry.imported->outline()) entry.boundary.emplace_back(point.x, point.y);
    entries_.push_back(std::move(entry)); return true;
  } catch (const std::exception& exception) {
    error = QString::fromUtf8(exception.what()); return false;
  }
}
bool AirfoilLibrary::addSketch(const QString& name, const SketchLayer& sketch, QString& error) {
  error.clear();
  auto boundary = closedAirfoilBoundary(sketch);
  if (!boundary) {
    error = "The airfoil sketch must contain exactly one closed loop with nonzero area. It was not added to the airfoil list.";
    return false;
  }
  entries_.push_back({name.trimmed().isEmpty() ? "Sketched airfoil" : name.trimmed(), {}, sketch, std::move(*boundary)});
  return true;
}
}

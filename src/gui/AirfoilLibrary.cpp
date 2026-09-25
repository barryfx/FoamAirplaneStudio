#include "gui/SketchBoundary.h"
#include "gui/AirfoilLibrary.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QLineF>
#include <locale>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>
namespace designrc::gui {
std::optional<std::vector<QPointF>> closedAirfoilBoundary(const SketchLayer& layer) {
  return closedSketchBoundary(layer);
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
void AirfoilLibrary::addProfile(const QString& name, const domain::AirfoilProfile& profile) {
  LibraryAirfoil entry{name,profile,{},{}};
  for(auto point:profile.outline())entry.boundary.emplace_back(point.x,point.y);
  entries_.push_back(std::move(entry));
}
domain::AirfoilProfile normalizedAirfoil(const LibraryAirfoil& entry) {
    auto profile=entry.imported;
    if(!profile) {
      auto points=entry.boundary;
      if(points.size()>1&&QLineF{points.front(),points.back()}.length()<1e-8)points.pop_back();
      if(points.size()<3)throw std::runtime_error("The traced airfoil has too few points.");
      const auto leading=std::min_element(points.begin(),points.end(),[](auto a,auto b){return a.x()<b.x();});
      const double trailingX=std::max_element(points.begin(),points.end(),[](auto a,auto b){return a.x()<b.x();})->x();
      const double chord=trailingX-leading->x();
      if(chord<=1e-8)throw std::runtime_error("The traced airfoil has no horizontal chord.");
      const int start=static_cast<int>(leading-points.begin()),count=static_cast<int>(points.size());
      auto surface=[&](int step) {
        std::vector<QPointF> result;
        for(int n=0;n<count;++n) {
          const auto p=points[(start+step*n+count)%count];result.push_back(p);
          if(trailingX-p.x()<1e-8)break;
        }
        return result;
      };
      auto first=surface(1),second=surface(-1);
      const double trailingY=(first.back().y()+second.back().y())*.5;
      std::reverse(first.begin(),first.end());
      first.insert(first.end(),std::next(second.begin()),second.end());
      std::ostringstream dat;dat.imbue(std::locale::classic());dat<<"Trace\n"<<std::setprecision(17);
      for(auto p:first) {
        const double x=(p.x()-leading->x())/chord;
        dat<<x<<' '<<(leading->y()+x*(trailingY-leading->y())-p.y())/chord<<'\n';
      }
      std::istringstream input{dat.str()};input.imbue(std::locale::classic());
      profile=domain::AirfoilProfile::fromDat(input);
    }
    return *profile;
}
bool AirfoilLibrary::exportDat(std::size_t index, const QString& path, QString& error) const {
  error.clear();
  try {
    const auto& entry=entries_.at(index);
    const auto profile=normalizedAirfoil(entry);
    // Selig ordering: upper TE -> LE -> lower TE, with one shared LE sample.
    std::ostringstream dat;dat.imbue(std::locale::classic());
    dat<<entry.name.simplified().toUtf8().toStdString()<<'\n'<<std::fixed<<std::setprecision(10);
    // 35 samples on each surface share the LE: 35 + 35 - 1 = 69 rows.
    for(auto point:profile.resampled(35))dat<<point.x<<' '<<point.y<<'\n';
    const auto bytes=QByteArray::fromStdString(dat.str());
    QSaveFile file{path};
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()) {
      error=file.errorString();return false;
    }
    return true;
  }catch(const std::exception& exception){error=QString::fromUtf8(exception.what());return false;}
}
}

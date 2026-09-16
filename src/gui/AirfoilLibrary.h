#pragma once
#include "gui/SketchEditor.h"
#include "domain/AirfoilProfile.h"
#include <QString>
#include <optional>
namespace designrc::gui {
struct LibraryAirfoil {
  QString name;
  std::optional<domain::AirfoilProfile> imported;
  std::optional<SketchLayer> sketch;
  std::vector<QPointF> boundary;
};
// Returns a traversal of exactly one closed, non-degenerate sketch loop.
std::optional<std::vector<QPointF>> closedAirfoilBoundary(const SketchLayer& layer);
class AirfoilLibrary {
public:
  bool loadDat(const QString& path, QString& error);
  bool addSketch(const QString& name, const SketchLayer& sketch, QString& error);
  const std::vector<LibraryAirfoil>& entries() const { return entries_; }
  void clear() { entries_.clear(); }
  void restore(std::vector<LibraryAirfoil> entries) { entries_=std::move(entries); }
private:
  std::vector<LibraryAirfoil> entries_;
};
}

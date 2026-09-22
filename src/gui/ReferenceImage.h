#pragma once

#include <QImage>
#include <QString>
#include <QSizeF>
#include <optional>
#include <vector>

namespace designrc::gui {
enum class ProjectUnits { Millimeters, Inches };

struct ReferencePage {
  QImage pixels;
  std::optional<QSizeF> physicalSizeMm;
};

struct ReferenceImage {
  QString path;
  std::vector<ReferencePage> pages;
  bool empty() const { return pages.empty(); }
  std::optional<QSizeF> physicalSizeMm;
  ProjectUnits nativeUnits{ProjectUnits::Millimeters};
};

// Loads a raster image or all PDF pages in document order, atomically.
[[nodiscard]] ReferenceImage loadReferenceImage(const QString& path, QString& error);
struct ProjectReference {
  ReferenceImage image;
  bool toScale{false};
  ProjectUnits units{ProjectUnits::Millimeters};
  std::optional<double> wingspanMm;
  std::optional<double> fuselageLengthMm; // Legacy file field; not used for scaling.
};
} // namespace designrc::gui


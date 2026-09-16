#include "gui/ReferenceImage.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPdfDocument>

#include <algorithm>
#include <cmath>

namespace designrc::gui {
namespace {
struct Density { double x{}, y{}; ProjectUnits units{ProjectUnits::Millimeters}; };
unsigned be16(const QByteArray& b, qsizetype p) {
  return (static_cast<unsigned char>(b[p]) << 8) | static_cast<unsigned char>(b[p + 1]);
}
unsigned be32(const QByteArray& b, qsizetype p) {
  return (be16(b, p) << 16) | be16(b, p + 2);
}
// Only explicit physical metadata is accepted; QImage supplies a default DPI
// even for images without physical metadata, which must not imply actual scale.
std::optional<Density> exifDensity(const QByteArray& payload) {
  if (!payload.startsWith(QByteArray("Exif\0\0", 6))) return {};
  const auto b = payload.mid(6);
  if (b.size() < 8) return {};
  const bool little = b.startsWith("II");
  if (!little && !b.startsWith("MM")) return {};
  auto u16 = [&](qsizetype p) -> unsigned {
    return little ? static_cast<unsigned char>(b[p]) |
        (static_cast<unsigned char>(b[p + 1]) << 8) : be16(b, p);
  };
  auto u32 = [&](qsizetype p) -> unsigned {
    return little ? u16(p) | (u16(p + 2) << 16) : be32(b, p);
  };
  if (u16(2) != 42) return {};
  const qsizetype offset = u32(4);
  if (offset < 8 || offset + 2 > b.size()) return {};
  const unsigned count = u16(offset);
  double x = 0, y = 0;
  unsigned unit = 0;
  for (unsigned i = 0; i < count; ++i) {
    const qsizetype p = offset + 2 + i * 12;
    if (p + 12 > b.size()) return {};
    const unsigned tag = u16(p), type = u16(p + 2);
    if (u32(p + 4) != 1) continue;
    if (tag == 0x128 && type == 3) unit = u16(p + 8);
    if ((tag == 0x11a || tag == 0x11b) && type == 5) {
      const qsizetype r = u32(p + 8);
      if (r + 8 > b.size() || u32(r + 4) == 0) continue;
      const double value = double(u32(r)) / u32(r + 4);
      if (tag == 0x11a) x = value; else y = value;
    }
  }
  if (x <= 0 || y <= 0 || (unit != 2 && unit != 3)) return {};
  return Density{x / (unit == 2 ? 25.4 : 10.0),
                 y / (unit == 2 ? 25.4 : 10.0),
                 unit == 2 ? ProjectUnits::Inches : ProjectUnits::Millimeters};
}
std::optional<Density> rasterDensity(const QString& path) {
  QFile file{path};
  if (!file.open(QIODevice::ReadOnly)) return {};
  const auto signature = file.read(8);
  if (signature == QByteArray::fromHex("89504e470d0a1a0a")) {
    while (!file.atEnd()) {
      const auto header = file.read(8);
      if (header.size() != 8) break;
      const auto length = be32(header, 0);
      if (header.mid(4) == "pHYs" && length == 9) {
        const auto b = file.read(9);
        if (b.size() == 9 && b[8] == 1 && be32(b, 0) > 0 && be32(b, 4) > 0)
          return Density{be32(b, 0) / 1000.0, be32(b, 4) / 1000.0, ProjectUnits::Millimeters};
        return {};
      }
      if (header.mid(4) == "IDAT" || header.mid(4) == "IEND") break;
      if (!file.seek(file.pos() + qint64(length) + 4)) break;
    }
  } else if (signature.startsWith(QByteArray::fromHex("ffd8"))) {
    file.seek(2);
    std::optional<Density> jfif;
    while (!file.atEnd()) {
      auto marker = file.read(2);
      if (marker.size() != 2 || static_cast<unsigned char>(marker[0]) != 0xff) break;
      while (static_cast<unsigned char>(marker[1]) == 0xff) {
        const auto next = file.read(1);
        if (next.isEmpty()) return jfif;
        marker[1] = next[0];
      }
      const unsigned type = static_cast<unsigned char>(marker[1]);
      if (type == 0xda || type == 0xd9) break;
      const auto size = file.read(2);
      if (size.size() != 2 || be16(size, 0) < 2) break;
      const auto b = file.read(be16(size, 0) - 2);
      if (type == 0xe1) {
        if (const auto exif = exifDensity(b)) return exif;
      }
      if (type == 0xe0 && b.size() >= 12 && b.startsWith(QByteArray("JFIF\0", 5))) {
        const unsigned unit = static_cast<unsigned char>(b[7]);
        if ((unit == 1 || unit == 2) && be16(b, 8) && be16(b, 10))
          jfif = Density{be16(b, 8) / (unit == 1 ? 25.4 : 10.0),
                         be16(b, 10) / (unit == 1 ? 25.4 : 10.0),
                         unit == 1 ? ProjectUnits::Inches : ProjectUnits::Millimeters};
      }
    }
    return jfif;
  }
  return {};
}
}

ReferenceImage loadReferenceImage(const QString& path, QString& error) {
  error.clear();
  ReferenceImage result;
  result.path = QFileInfo{path}.absoluteFilePath();
  if (QFileInfo{path}.suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
    QPdfDocument pdf;
    if (pdf.load(path) != QPdfDocument::Error::None || pdf.pageCount() < 1) {
      error = "Unable to open this PDF. It may be damaged or password-protected.";
      return {};
    }
    double maxSide = 0, area = 0, width = 0, height = 0;
    for (int page = 0; page < pdf.pageCount(); ++page) {
      const auto points = pdf.pagePointSize(page);
      if (points.isEmpty()) { error = "A PDF page has no valid size."; return {}; }
      maxSide = std::max({maxSide, points.width(), points.height()});
      area += points.width() * points.height();
      width = std::max(width, points.width());
      height += points.height();
    }
    result.physicalSizeMm = QSizeF{width, height} * (25.4 / 72.0);
    result.nativeUnits = ProjectUnits::Inches;
    // One common raster scale preserves relative page sizes. Bound aggregate
    // pixel memory as well as each page, without altering physical dimensions.
    const double scale = std::min({2.0, 4096.0 / maxSide, std::sqrt(32.e6 / area)});
    for (int page = 0; page < pdf.pageCount(); ++page) {
      const auto points = pdf.pagePointSize(page);
      auto pixels = pdf.render(page, QSize{std::max(1, qRound(points.width() * scale)),
                                            std::max(1, qRound(points.height() * scale))});
      if (pixels.isNull()) {
        error = QString{"Unable to render PDF page %1."}.arg(page + 1);
        return {};
      }
      result.pages.push_back({std::move(pixels), points * (25.4 / 72.0)});
    }
  } else {
    QImageReader reader{path};
    reader.setAutoTransform(true);
    const auto originalSize = reader.size();
    const auto transform = reader.transformation();
    auto pixels = reader.read();
    if (pixels.isNull()) { error = reader.errorString(); return {}; }
    if (const auto density = rasterDensity(path)) {
      QSizeF size{originalSize.width() / density->x, originalSize.height() / density->y};
      if (int(transform) & 4) size.transpose();
      result.physicalSizeMm = size;
      result.nativeUnits = density->units;
    }
    result.pages.push_back({std::move(pixels), result.physicalSizeMm});
  }
  return result;
}
} // namespace designrc::gui

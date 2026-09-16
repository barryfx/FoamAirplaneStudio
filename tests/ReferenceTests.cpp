#include "gui/ReferenceImage.h"
#include "gui/ReferencePanel.h"
#include "gui/ReferenceWorkflow.h"
#include <QAction>
#include <QToolBar>
#include <limits>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QLineEdit>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <cassert>
#include <cmath>

int main(int argc, char** argv) {
  QApplication app{argc, argv};
  using namespace designrc::gui;
  QTemporaryDir temp;
  assert(temp.isValid());
  QImage raster{600, 300, QImage::Format_RGB32};
  raster.fill(Qt::white);
  raster.setDotsPerMeterX(10000);
  raster.setDotsPerMeterY(10000);
  const auto pngPath = temp.filePath("reference.png");
  assert(raster.save(pngPath));
  QString error;
  const auto png = loadReferenceImage(pngPath, error);
  assert(!png.empty() && png.physicalSizeMm);
  assert(std::abs(png.physicalSizeMm->width() - 60.0) < 0.01);
  assert(png.nativeUnits == ProjectUnits::Millimeters);

  // Remove the physical-density chunk from a valid PNG. The decoder's fallback
  // DPI must not make an otherwise unscaled reference look physically calibrated.
  QFile pngFile{pngPath};
  assert(pngFile.open(QIODevice::ReadOnly));
  auto noDensity = pngFile.readAll();
  const auto densityTag = noDensity.indexOf("pHYs");
  assert(densityTag >= 4);
  noDensity.remove(densityTag - 4, 21);
  QFile unscaledFile{temp.filePath("unscaled.png")};
  assert(unscaledFile.open(QIODevice::WriteOnly));
  assert(unscaledFile.write(noDensity) == noDensity.size());
  unscaledFile.close();
  const auto unscaled = loadReferenceImage(unscaledFile.fileName(), error);
  assert(!unscaled.empty() && !unscaled.physicalSizeMm);

  const auto jpgPath = temp.filePath("reference.jpg");
  assert(raster.save(jpgPath));
  const auto jpg = loadReferenceImage(jpgPath, error);
  assert(!jpg.empty() && jpg.physicalSizeMm);
  assert(std::abs(jpg.physicalSizeMm->width() - 60.0) < 0.5);

  const auto pdfPath = temp.filePath("reference.pdf");
  {
    QPdfWriter writer{pdfPath};
    writer.setPageSize(QPageSize{QSizeF{152.4, 76.2}, QPageSize::Millimeter});
    QPainter painter{&writer};
    painter.drawLine(0, 0, 100, 100);
    writer.newPage();
    painter.drawLine(0, 100, 100, 0);
  }
  const auto pdf = loadReferenceImage(pdfPath, error);
  assert(!pdf.empty() && pdf.physicalSizeMm);
  assert(std::abs(pdf.physicalSizeMm->width() - 152.4) < 0.1);
  assert(pdf.pages.size() == 2);
  assert(std::abs(pdf.physicalSizeMm->height() - 152.4) < 0.1);
  assert(pdf.nativeUnits == ProjectUnits::Inches);
  assert(loadReferenceImage(temp.filePath("missing.png"), error).empty());

  ReferencePanel panel;
  QToolBar toolbar;
  for (const auto* name : {"Reference", "Wing", "Fuselage", "Horiz Stab", "Vert Stab", "Assembly", "Export"})
    toolbar.addAction(name);
  auto checkToolbar = [&](const ProjectReference& reference, bool wingEnabled) {
    applyReferenceWorkflow(toolbar, reference);
    assert(toolbar.actions()[0]->isEnabled());
    assert(toolbar.actions()[1]->isEnabled() == wingEnabled);
    for (int i = 2; i < toolbar.actions().size(); ++i) assert(!toolbar.actions()[i]->isEnabled());
  };
  QObject::connect(&panel, &ReferencePanel::referenceChanged, &toolbar, [&] {
    applyReferenceWorkflow(toolbar, panel.projectReference());
  });
  checkToolbar(panel.projectReference(), false);
  ProjectReference scaled;
  scaled.image = png;
  checkToolbar(scaled, false); // Image alone is insufficient.
  scaled.toScale = true; checkToolbar(scaled, true);
  scaled.image = pdf; checkToolbar(scaled, true);
  scaled.image.pages.back().physicalSizeMm.reset(); checkToolbar(scaled, false);
  scaled.image = unscaled; checkToolbar(scaled, false);
  scaled.image = {}; checkToolbar(scaled, false);
  scaled.toScale = false;
  scaled.wingspanMm = 100; scaled.fuselageLengthMm = 50;
  checkToolbar(scaled, true); // Manual dimensions need no reference image.
  scaled.image = unscaled; checkToolbar(scaled, true);
  scaled.wingspanMm = 0; checkToolbar(scaled, false);
  scaled.wingspanMm = std::numeric_limits<double>::quiet_NaN(); checkToolbar(scaled, false);
  scaled.wingspanMm = 100; scaled.fuselageLengthMm = -1; checkToolbar(scaled, false);
  scaled.fuselageLengthMm = std::numeric_limits<double>::infinity(); checkToolbar(scaled, false);
  auto* span = panel.findChild<QLineEdit*>("referenceWingspan");
  auto* length = panel.findChild<QLineEdit*>("referenceFuselageLength");
  auto* units = panel.findChild<QComboBox*>("projectUnits");
  assert(span && length && units);
  span->setText("254.0");
  assert(!toolbar.actions()[1]->isEnabled());
  length->setText("127.0");
  assert(toolbar.actions()[1]->isEnabled());
  units->setCurrentIndex(1);
  assert(span->text()=="254.0 mm");
  assert(length->text()=="127.0 mm");
  assert(panel.projectReference().wingspanMm == 254.0);
  span->setText("500 mm");length->setText("10 in");
  assert(panel.projectReference().wingspanMm==500);
  assert(panel.projectReference().fuselageLengthMm==254);
  units->setCurrentIndex(0);
  assert(span->text()=="500 mm" && length->text()=="10 in");
  span->setText("2.5 inches");assert(panel.projectReference().wingspanMm==63.5);
  span->setText("3 cm");assert(!panel.projectReference().wingspanMm);
  span->setText("");
  assert(!panel.projectReference().wingspanMm);
  assert(!toolbar.actions()[1]->isEnabled());
  span->setText("10"); assert(toolbar.actions()[1]->isEnabled());
  length->setText("invalid"); assert(!toolbar.actions()[1]->isEnabled());
  length->setText("5"); assert(toolbar.actions()[1]->isEnabled());
  panel.reset();
  checkToolbar(panel.projectReference(), false);
  assert(!panel.projectReference().fuselageLengthMm);
  assert(panel.projectReference().units == ProjectUnits::Millimeters);
}



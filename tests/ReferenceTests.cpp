#include "gui/WingCalibration.h"
#include <QRadioButton>
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
#include <QLabel>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>
#include "TestCheck.h"
#include <cmath>

int main(int argc, char** argv) {
  QApplication app{argc, argv};
  using namespace designrc::gui;
  QTemporaryDir temp;
  TEST_CHECK(temp.isValid());
  QImage raster{600, 300, QImage::Format_RGB32};
  raster.fill(Qt::white);
  raster.setDotsPerMeterX(10000);
  raster.setDotsPerMeterY(10000);
  const auto pngPath = temp.filePath("reference.png");
  TEST_CHECK(raster.save(pngPath));
  QString error;
  const auto png = loadReferenceImage(pngPath, error);
  TEST_CHECK(!png.empty() && png.physicalSizeMm);
  TEST_CHECK(std::abs(png.physicalSizeMm->width() - 60.0) < 0.01);
  TEST_CHECK(png.nativeUnits == ProjectUnits::Millimeters);

  // Remove the physical-density chunk from a valid PNG. The decoder's fallback
  // DPI must not make an otherwise unscaled reference look physically calibrated.
  QFile pngFile{pngPath};
  TEST_CHECK(pngFile.open(QIODevice::ReadOnly));
  auto noDensity = pngFile.readAll();
  const auto densityTag = noDensity.indexOf("pHYs");
  TEST_CHECK(densityTag >= 4);
  noDensity.remove(densityTag - 4, 21);
  QFile unscaledFile{temp.filePath("unscaled.png")};
  TEST_CHECK(unscaledFile.open(QIODevice::WriteOnly));
  TEST_CHECK(unscaledFile.write(noDensity) == noDensity.size());
  unscaledFile.close();
  const auto unscaled = loadReferenceImage(unscaledFile.fileName(), error);
  TEST_CHECK(!unscaled.empty() && !unscaled.physicalSizeMm);

  const auto jpgPath = temp.filePath("reference.jpg");
  TEST_CHECK(raster.save(jpgPath));
  const auto jpg = loadReferenceImage(jpgPath, error);
  TEST_CHECK(!jpg.empty() && jpg.physicalSizeMm);
  TEST_CHECK(std::abs(jpg.physicalSizeMm->width() - 60.0) < 0.5);

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
  TEST_CHECK(!pdf.empty() && pdf.physicalSizeMm);
  TEST_CHECK(std::abs(pdf.physicalSizeMm->width() - 152.4) < 0.1);
  TEST_CHECK(pdf.pages.size() == 2);
  TEST_CHECK(std::abs(pdf.physicalSizeMm->height() - 152.4) < 0.1);
  TEST_CHECK(pdf.nativeUnits == ProjectUnits::Inches);
  TEST_CHECK(loadReferenceImage(temp.filePath("missing.png"), error).empty());

  ReferencePanel panel;
  const auto* instructions=panel.findChild<QLabel*>("referenceInstructions");
  TEST_CHECK(instructions&&instructions->text().contains("PDF")&&instructions->text().contains("PNG")&&instructions->text().contains("Wingspan"));
  TEST_CHECK(panel.findChild<QRadioButton*>("referenceToScale")->text()=="Use Reference Image Scale");
  QToolBar toolbar;
  for (const auto* name : {"Reference", "Wing", "Fuselage", "Horiz Stab", "Vert Stab", "Assembly", "Export"})
    toolbar.addAction(name);
  auto checkToolbar = [&](const ProjectReference& reference, bool wingEnabled) {
    applyReferenceWorkflow(toolbar, reference);
    TEST_CHECK(toolbar.actions()[0]->isEnabled());
    TEST_CHECK(toolbar.actions()[1]->isEnabled() == wingEnabled);
    for (int i = 2; i < toolbar.actions().size(); ++i) TEST_CHECK(!toolbar.actions()[i]->isEnabled());
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
  scaled.wingspanMm = 100;
  checkToolbar(scaled, true); // Manual dimensions need no reference image.
  scaled.image = unscaled; checkToolbar(scaled, true);
  scaled.wingspanMm = 0; checkToolbar(scaled, false);
  scaled.wingspanMm = std::numeric_limits<double>::quiet_NaN(); checkToolbar(scaled, false);
  scaled.wingspanMm = 100; scaled.fuselageLengthMm = -1; checkToolbar(scaled, true);
  scaled.fuselageLengthMm = std::numeric_limits<double>::infinity(); checkToolbar(scaled, true);
  auto* span = panel.findChild<QLineEdit*>("referenceWingspan");
  auto* length = panel.findChild<QLineEdit*>("referenceFuselageLength");
  auto* units = panel.findChild<QComboBox*>("projectUnits");
  TEST_CHECK(span && !length && units);
  TEST_CHECK(panel.findChild<QRadioButton*>("referenceToScale")->text()=="Use Reference Image Scale");
  span->setText("254.0");

  TEST_CHECK(toolbar.actions()[1]->isEnabled());
  units->setCurrentIndex(1);
  TEST_CHECK(span->text()=="254.0 mm");
  TEST_CHECK(panel.projectReference().wingspanMm == 254.0);
  span->setText("500 mm");
  TEST_CHECK(panel.projectReference().wingspanMm==500);
  units->setCurrentIndex(0);
  TEST_CHECK(span->text()=="500 mm");
  span->setText("2.5 inches");TEST_CHECK(panel.projectReference().wingspanMm==63.5);
  span->setText("3 cm");TEST_CHECK(!panel.projectReference().wingspanMm);
  span->setText("");
  TEST_CHECK(!panel.projectReference().wingspanMm);
  TEST_CHECK(!toolbar.actions()[1]->isEnabled());
  span->setText("10"); TEST_CHECK(toolbar.actions()[1]->isEnabled());
  span->setText("invalid"); TEST_CHECK(!toolbar.actions()[1]->isEnabled());
  span->setText("5"); TEST_CHECK(toolbar.actions()[1]->isEnabled());
  panel.reset();
  checkToolbar(panel.projectReference(), false);
  TEST_CHECK(!panel.projectReference().fuselageLengthMm);
  TEST_CHECK(panel.projectReference().units == ProjectUnits::Millimeters);
  // Pure drawing calibration: 100 units of half-span represents 1000 mm full span.
  SketchLayer wing{{{10,10},{110,10},{110,80},{10,80}},
      {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}}}};
  std::vector<ConstrainedLine> stations{{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0}};
  auto calibration=wingCalibration({wing},stations,1000);
  TEST_CHECK(std::abs(calibration.scale-5)<1e-9);
  TEST_CHECK(std::abs(calibration.leadingEdgeX-50)<1e-9);
  TEST_CHECK(std::abs(wingCalibration({wing},stations,2000).scale-10)<1e-9);
  TEST_CHECK(wingCalibration({wing},stations,std::nullopt).scale==1);
  // Translation and a 90-degree drawing rotation do not change physical scale.
  for(auto& point:wing.points)point={-point.y()+500,point.x()+300};
  for(auto& station:stations) {
    auto rotate=[](QPointF point){return QPointF{-point.y()+500,point.x()+300};};
    station.first.position=rotate(station.first.position);station.second.position=rotate(station.second.position);
  }
  TEST_CHECK(std::abs(wingCalibration({wing},stations,1000).scale-5)<1e-9);

  const auto capture=qEnvironmentVariable("FOAM_REFERENCE_CAPTURE");
  if(!capture.isEmpty()) {
    panel.resize(350,400);panel.show();app.processEvents();
    TEST_CHECK(panel.grab().save(capture));
  }

}



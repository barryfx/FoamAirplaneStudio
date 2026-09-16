#include "gui/AirfoilPanel.h"
#include "gui/PlanViewport.h"
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QTemporaryDir>
#include <QSettings>
#include <QTimer>
#include <QTabBar>
#include <cassert>
#include <cmath>
using namespace designrc::gui;
int main(int argc, char** argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication app{argc, argv};
  app.setOrganizationName("AirfoilPanelTests"); app.setApplicationName("Isolated");
  QTemporaryDir temporary; assert(temporary.isValid());
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
  const QByteArray coordinates{"1 0\n0.5 0.08\n0 0\n0.5 -0.08\n1 0\n"};
  auto write = [&](const QString& name, const QByteArray& data) {
    const auto path = temporary.filePath(name); QFile file{path};
    assert(file.open(QIODevice::WriteOnly)); assert(file.write(data) == data.size()); return path;
  };
  const auto named = write("name-from-file.dat", "Test foil\n" + coordinates);
  const auto unnamed = write("Fallback name.dat", coordinates);
  const auto bad = write("bad.dat", "Bad\n0 0\n1 1\n");
  AirfoilLibrary library; QString error;
  assert(library.loadDat(named, error)); assert(library.entries()[0].name == "Test foil");
  assert(library.loadDat(unnamed, error)); assert(library.entries()[1].name == "Fallback name");
  assert(library.entries()[1].imported->outline().size() == 5);
  assert(!library.loadDat(bad, error) && !error.isEmpty()); assert(library.entries().size() == 2);
  const auto shifted = write("shifted.dat", "Shifted\n12 0\n11 0.16\n10 0\n11 -0.16\n12 0\n");
  assert(library.loadDat(shifted, error));
  const auto& normalized = library.entries().back().imported->outline();
  assert(std::abs(normalized[3].x - 0.5) < 1e-9 && std::abs(normalized[4].x - 1) < 1e-9);
  const auto lednicer = write("lednicer.dat", "Two surfaces\n3 3\n0 0\n0.5 0.08\n1 0\n\n0 0\n0.5 -0.08\n1 0\n");
  assert(library.loadDat(lednicer, error)); assert(library.entries().back().imported->resampled(11).size() == 21);
  SketchLayer triangle{{{0, 0}, {100, 20}, {100, -20}},
      {{SketchTool::Line, {0, 1}}, {SketchTool::Line, {1, 2}}, {SketchTool::Line, {2, 0}}}};
  assert(closedAirfoilBoundary(triangle));
  auto open = triangle; open.curves.pop_back(); assert(!closedAirfoilBoundary(open));
  auto two = triangle;
  for (auto point : triangle.points) two.points.push_back(point + QPointF{200, 0});
  for (auto curve : triangle.curves) { for (auto& id : curve.points) id += 3; two.curves.push_back(curve); }
  assert(!closedAirfoilBoundary(two));
  SketchLayer periodic{{{0, 0}, {50, 20}, {100, 0}, {50, -20}}, {{SketchTool::Spline, {0, 1, 2, 3, 0}}}};
  assert(closedAirfoilBoundary(periodic));

  PlanViewport view; view.resize(1000, 700); view.show();
  AirfoilPanel panel{view}; panel.resize(340, 650); panel.show(); app.processEvents();
  auto mouse = [&](QEvent::Type type, QPointF point) {
    const auto local = view.mapFromScene(point);
    QMouseEvent event{type, QPointF{local}, QPointF{view.viewport()->mapToGlobal(local)}, Qt::LeftButton,
        type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier};
    QApplication::sendEvent(view.viewport(), &event);
  };
  auto click = [&](QPointF p) { mouse(QEvent::MouseButtonPress, p); mouse(QEvent::MouseButtonRelease, p); };
  auto key = [&](int k) { QKeyEvent event{QEvent::KeyPress, k, Qt::NoModifier}; QApplication::sendEvent(&view, &event); };
  auto& wing = view.sketchEditor(); wing.setEditing(true); wing.setTool(SketchTool::Line);
  click({100, 100}); click({700, 100}); click({700, 400}); click({100, 400});
  wing.setEditing(false); wing.stationEditor().setEnabled(true);
  click({200, 100}); click({200, 400}); click({600, 100}); click({600, 400});
  const auto outline = wing.layers()[0].points;
  auto& stations = wing.stationEditor(); assert(stations.lines().size() == 2);
  panel.setActive(true);
  // Exercise the real Load button and shared file dialog, not only the loader.
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget()); assert(dialog);
    dialog->selectFile(named); QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
  });
  panel.findChild<QPushButton*>("loadAirfoilDat")->click();
  assert(panel.library().entries().size() == 1);
  auto* first = panel.findChild<QRadioButton*>("airfoilChoice0"); assert(first && first->isChecked());
  assert(panel.loadAirfoil(unnamed, error));
  auto* second = panel.findChild<QRadioButton*>("airfoilChoice1"); assert(second);
  click({200, 250}); assert(stations.selectedLine() == 0 && stations.lines()[0].airfoil == 0);
  second->click(); assert(stations.lines()[0].airfoil == 1 && !first->isChecked());
  click({600, 250}); assert(stations.lines()[1].airfoil == 1);
  first->click(); assert(stations.lines()[1].airfoil == 0);
  click({200, 250}); assert(second->isChecked() && !first->isChecked());
  click({600, 250}); assert(first->isChecked() && !second->isChecked());
  key(Qt::Key_Delete); assert(stations.lines().size() == 2);
  click(stations.lines()[1].first.position); click({550, 100});
  assert(std::abs(stations.lines()[1].first.position.x() - 600) < 1);
  assert(panel.allStationsAssigned());
  auto* sketchButton = panel.findChild<QPushButton*>("sketchAirfoil");
  auto startSketch = [&](const QString& name) {
    QTimer::singleShot(0, [name] {
      auto* input = qobject_cast<QInputDialog*>(QApplication::activeModalWidget()); assert(input);
      input->setTextValue(name); input->accept();
    });
    sketchButton->click(); assert(sketchButton->isChecked());
    assert(!panel.findChild<QPushButton*>("loadAirfoilDat")->isEnabled()); assert(!first->isEnabled());
    assert(panel.findChild<QPushButton*>("airfoilLine")->isVisible());
  };
  startSketch("Traced foil");
  panel.findChild<QPushButton*>("airfoilLine")->click();
  click({100, 520}); click({350, 470}); click({350, 470}); click({650, 520});
  click({650, 520}); click({350, 550}); click({350, 550}); click({100, 520});
  key(Qt::Key_Escape);
  assert(!sketchButton->isChecked()); assert(first->isEnabled());
  assert(panel.library().entries().size() == 3 && panel.library().entries()[2].name == "Traced foil");
  assert(wing.layers()[0].points == outline && stations.lines().size() == 2);
  startSketch("Periodic foil"); panel.findChild<QPushButton*>("airfoilSpline")->click();
  click({100, 520}); click({350, 470}); click({650, 520}); click({350, 550}); click({100, 520});
  sketchButton->click(); assert(panel.library().entries().size() == 4);
  bool warned = false;
  startSketch("Open draft"); panel.findChild<QPushButton*>("airfoilLine")->click();
  click({100, 520}); click({350, 470});
  QTimer::singleShot(0, [&] { auto* warning = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()); assert(warning); warned = true; warning->accept(); });
  key(Qt::Key_Escape); assert(warned && !sketchButton->isChecked() && panel.library().entries().size() == 4);
  startSketch("Two loops"); panel.findChild<QPushButton*>("airfoilLine")->click();
  for (const auto offset : {QPointF{0, 0}, QPointF{350, 0}}) {
    click(QPointF{100, 500} + offset); click(QPointF{200, 450} + offset);
    click(QPointF{200, 450} + offset); click(QPointF{200, 550} + offset);
    click(QPointF{200, 550} + offset); click(QPointF{100, 500} + offset);
  }
  warned = false;
  QTimer::singleShot(0, [&] { auto* warning = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()); assert(warning); warned = true; warning->accept(); });
  sketchButton->click(); assert(warned && panel.library().entries().size() == 4);
  QTimer::singleShot(0, [] { auto* input = qobject_cast<QInputDialog*>(QApplication::activeModalWidget()); assert(input); input->reject(); });
  sketchButton->click(); assert(!sketchButton->isChecked());
  const QString capture = qEnvironmentVariable("FOAM_AIRFOIL_CAPTURE");
  if (!capture.isEmpty()) { assert(panel.grab().save(capture)); assert(view.grab().save(capture + ".viewport.png")); }
  panel.setActive(false); key(Qt::Key_Delete); assert(stations.lines().size() == 2);
  panel.setActive(true); click({200, 250}); assert(second->isChecked());
  auto twoPanels=wing.state();twoPanels.layers.push_back(twoPanels.layers[0]);wing.restoreState(twoPanels);
  auto bothStations=stations.state();for(int i=0;i<2;++i){auto copy=bothStations.lines[i];copy.first.layer=copy.second.layer=1;bothStations.lines.push_back(copy);}
  stations.restoreState(bothStations);panel.setPanelCount(2);
  auto* panelTabs=panel.findChild<QTabBar*>("airfoilPanelTabs");assert(panelTabs->count()==2);
  const auto inboardAssignment=stations.lines()[0].airfoil;
  panelTabs->setCurrentIndex(1);assert(stations.lines()[stations.selectedLine()].first.layer==1);
  auto* sharedSketch=panel.findChild<QRadioButton*>("airfoilChoice3");assert(sharedSketch);sharedSketch->click();
  assert(stations.lines()[0].airfoil==inboardAssignment && stations.lines()[2].airfoil==3);
  assert(panel.loadAirfoil(named,error));assert(panel.library().entries().size()==5);
  panelTabs->setCurrentIndex(0);assert(panel.findChild<QRadioButton*>("airfoilChoice4"));
  assert(stations.lines()[stations.selectedLine()].first.layer==0);
  panelTabs->setCurrentIndex(1);assert(sharedSketch->isChecked());
  const auto savedPanels=panel.state();assert(savedPanels.panel==1 && savedPanels.panelChoices.size()==2);
  panel.reset(); assert(panel.library().entries().empty());
  return 0;
}

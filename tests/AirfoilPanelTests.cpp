#include "gui/AirfoilPanel.h"
#include "gui/AirfoilSmoothing.h"
#include "gui/AirfoilSmoothingDialog.h"
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
#include <QSlider>
#include <QLabel>
#include <QLineEdit>
#include <cassert>
#include <cmath>
#include <numbers>
#include <sstream>
using namespace designrc::gui;
void smoothingChecks() {
  std::ostringstream dat;dat.precision(17);dat<<"Noisy hand trace\n";
  const auto clean=[](double x,bool upper) {
    const double camber=.08*x*(1-x),half=.18*std::sqrt(x)*(1-x)+.01*x;
    return camber+(upper?half:-half);
  };
  for(int i=160;i>=-160;--i) {
    const double x=.5*(1-std::cos(std::numbers::pi*std::abs(i)/160.));
    const double noise=.003*std::sin(32*std::numbers::pi*x)*std::sin(std::numbers::pi*x);
    dat<<x<<' '<<clean(x,i>=0)+(i>=0?noise:-noise)<<'\n';
  }
  std::istringstream input{dat.str()};const auto original=designrc::domain::AirfoilProfile::fromDat(input);
  const auto saved=original.outline();
  for(int strength:{0,25,60,100}) {
    const auto smooth=smoothAirfoil(original,strength);const auto points=smooth.resampled(161);
    double before=0,after=0;
    for(int i=0;i<321;++i) {
      const auto p=points[i];assert(std::isfinite(p.y)&&p.x>=0&&p.x<=1);
      before+=std::pow(saved[i].y-clean(saved[i].x,i<=160),2);
      after+=std::pow(p.y-clean(p.x,i<=160),2);
    }
    assert(after<before*.35); // Removes trace noise rather than interpolating it.
    assert(points.front().y==saved.front().y&&points.back().y==saved.back().y&&points[160].y==saved[160].y);
    for(int i=1;i<160;++i)assert(points[160-i].y>points[160+i].y);
    assert(std::abs(airfoilMetrics(smooth).thickness-airfoilMetrics(original).thickness)<.01);
    // A rounded nose has a common, nearly vertical tangent on both surfaces.
    // Finite first segments approximate the vertical tangent (within 6 degrees).
    const double dx=points[159].x;assert(dx/std::abs(points[159].y-points[160].y)<.1);
    assert(dx/std::abs(points[161].y-points[160].y)<.1);
  }
  for(std::size_t i=0;i<saved.size();++i)assert(original.outline()[i].y==saved[i].y);
  const auto gentle=smoothAirfoil(original,0).outline(),strong=smoothAirfoil(original,100).outline();
  double changed=0;for(std::size_t i=0;i<gentle.size();++i)changed+=std::abs(gentle[i].y-strong[i].y);
  assert(changed>1e-6);
  bool rejected=false;try{smoothAirfoil(original,101);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
  std::istringstream crossing{"Crossing\n1 0\n.8 -.05\n.4 .1\n0 0\n.4 -.1\n.8 .05\n1 0\n"};
  rejected=false;try{smoothAirfoil(designrc::domain::AirfoilProfile::fromDat(crossing),25);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
  const auto symmetric=smoothAirfoil(designrc::domain::AirfoilProfile::nacaSymmetric(.12),25).resampled(161);
  for(int i=0;i<161;++i)assert(std::abs(symmetric[160-i].y+symmetric[160+i].y)<1e-9);
  std::istringstream flat{"Flat\n1 0\n.5 0\n0 0\n.5 0\n1 0\n"};
  rejected=false;try{smoothAirfoil(designrc::domain::AirfoilProfile::fromDat(flat),25);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
}
int main(int argc, char** argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication app{argc, argv};
  app.setOrganizationName("AirfoilPanelTests"); app.setApplicationName("Isolated");
  QTemporaryDir temporary; assert(temporary.isValid());
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
  smoothingChecks();
  if(argc>1) {
    AirfoilLibrary regression;QString error;assert(regression.loadDat(QString::fromLocal8Bit(argv[1]),error));
    const auto source=*regression.entries()[0].imported;
    const auto prepared=airfoilSmoothingSource(source);
    assert(prepared.outline().front().y==0);
    assert(prepared.outline().back().y<-.0073&&prepared.outline().back().y>-.0076);
    for(int strength:{0,25,100}) {
      const auto smoothed=smoothAirfoil(source,strength).resampled(161);
      assert(smoothed.back().y==prepared.outline().back().y);
      assert(std::abs(smoothed[320].y-smoothed[319].y)<.0001); // No upward hook into the cap.
      for(int i=1;i<160;++i)assert(smoothed[160-i].y>smoothed[160+i].y);
    }
    assert(source.outline().back().y==0); // Read-only source.
    // Reversed contour and exact vertical closure also preserve the lower endpoint.
    auto reverse=source.outline();std::reverse(reverse.begin(),reverse.end());
    std::ostringstream dat;dat.precision(17);dat<<"Reversed\n";
    for(auto p:reverse)dat<<p.x<<' '<<p.y<<'\n';
    std::istringstream input{dat.str()};
    const auto reversed=smoothAirfoil(designrc::domain::AirfoilProfile::fromDat(input),25);
    assert(std::abs(reversed.outline().back().y-prepared.outline().back().y)<1e-10);
    std::istringstream vertical{"Vertical cap\n1 0\n.8 .05\n0 0\n.8 -.01\n1 -.01\n1 0\n"};
    assert(std::abs(smoothAirfoil(designrc::domain::AirfoilProfile::fromDat(vertical),25).outline().back().y+.01)<1e-10);
    AirfoilSmoothingDialog dialog{source,"Baby Buzzard smoothed"};dialog.show();app.processEvents();
    assert(dialog.width()>=1000&&dialog.height()>=680);
    const auto capture=qEnvironmentVariable("FOAM_TE_CAPTURE");if(!capture.isEmpty())assert(dialog.grab().save(capture));
  }
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
  auto checkExport=[&](const AirfoilLibrary& source,std::size_t index) {
    const auto path=temporary.filePath("exported.dat");
    assert(source.exportDat(index,path,error));
    QFile file{path};assert(file.open(QIODevice::ReadOnly));
    std::istringstream input{file.readAll().toStdString()};
    std::string header;std::getline(input,header);
    assert(header==source.entries()[index].name.toStdString());
    std::vector<designrc::domain::Point2> points;double x,y;
    while(input>>x>>y)points.push_back({x,y});
    assert(points.size()==69&&points.front().x==1&&points.back().x==1&&points[34].x==0);
    for(int i=0;i<=34;++i) {
      const double expected=.5*(1-std::cos(std::numbers::pi*i/34.));
      assert(std::abs(points[34-i].x-expected)<1e-9);
      assert(std::abs(points[34+i].x-expected)<1e-9);
    }
    AirfoilLibrary reimport;assert(reimport.loadDat(path,error));
    return points;
  };
  const auto shiftedPoints=checkExport(library,2); // Shifted/scaled imported chord.
  assert(std::abs(shiftedPoints[17].y-.08)<1e-9&&std::abs(shiftedPoints[51].y+.08)<1e-9);
  assert(shiftedPoints.front().y==0&&shiftedPoints.back().y==0);
  checkExport(library,3); // Lednicer input exported in Selig ordering.
  const auto blunt=write("blunt.dat","Blunt\n1 0.02\n0.5 0.08\n0 0\n0.5 -0.08\n1 -0.02\n");
  assert(library.loadDat(blunt,error));const auto bluntPoints=checkExport(library,4);
  assert(std::abs(bluntPoints.front().y-.02)<1e-9&&std::abs(bluntPoints.back().y+.02)<1e-9);
  assert(library.addSketch("Periodic",periodic,error));checkExport(library,5);
  SketchLayer flatTe{{{10,30},{60,15},{110,28},{110,32},{60,40}},
    {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},
     {SketchTool::Line,{3,4}},{SketchTool::Line,{4,0}}}};
  assert(library.addSketch("Blunt trace",flatTe,error));const auto traced=checkExport(library,6);
  assert(std::abs(traced.front().y-.02)<1e-9&&std::abs(traced.back().y+.02)<1e-9);
  assert(traced[34].y==0);
  const auto smoothedTrace=smoothAirfoil(normalizedAirfoil(library.entries()[6]),25).outline();
  assert(std::abs(smoothedTrace.front().y-.02)<1e-9&&std::abs(smoothedTrace.back().y+.02)<1e-9);
  assert(!library.exportDat(999,temporary.filePath("invalid.dat"),error)&&!error.isEmpty());
  assert(!library.exportDat(0,temporary.filePath("missing/fail.dat"),error)&&!error.isEmpty());

  PlanViewport view; view.resize(1000, 700); view.show();
  AirfoilPanel panel{view}; panel.resize(340, 650); panel.show(); app.processEvents();
  auto* exportButton=panel.findChild<QPushButton*>("exportAirfoilDat");assert(exportButton&&!exportButton->isEnabled());
  auto* smoothButton=panel.findChild<QPushButton*>("smoothAirfoil");assert(smoothButton&&!smoothButton->isEnabled());
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
  assert(exportButton->isEnabled());
  const auto exportedPath=temporary.filePath("selected.dat");
  QTimer::singleShot(0,[&] {
    auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());assert(dialog);
    assert(dialog->acceptMode()==QFileDialog::AcceptSave);
    dialog->selectFile(exportedPath);QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
  });
  exportButton->click();
  AirfoilLibrary exported;assert(exported.loadDat(exportedPath,error));
  assert(exported.entries()[0].name=="Fallback name");
  QTimer::singleShot(0,[] {
    auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());assert(dialog);dialog->reject();
  });
  exportButton->click();assert(panel.library().entries().size()==2&&stations.lines()[0].airfoil==1);
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
    assert(!exportButton->isEnabled());
    assert(!smoothButton->isEnabled());
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
  // Preview/cancel never changes the library or station assignments.
  assert(smoothButton->isEnabled());
  QTimer::singleShot(0,[] {
    auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog&&dialog->objectName()=="airfoilSmoothingDialog");
    dialog->reject();
  });smoothButton->click();assert(panel.library().entries().size()==5);
  QTimer::singleShot(0,[&] {
    auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
    auto* slider=dialog->findChild<QSlider*>("airfoilSmoothingStrength");assert(slider&&slider->value()==25);
    auto* metrics=dialog->findChild<QLabel*>("airfoilSmoothingMetrics");assert(metrics->text().contains("Maximum thickness"));
    slider->setValue(80);assert(metrics->text().contains("Maximum camber"));
    auto* name=dialog->findChild<QLineEdit*>("smoothedAirfoilName");assert(name->text().contains("Smoothed"));
    auto* save=dialog->findChild<QPushButton*>("saveSmoothedAirfoil");
    name->clear();assert(!save->isEnabled());name->setText("Smooth copy");assert(save->isEnabled());
    const auto capture=qEnvironmentVariable("FOAM_SMOOTH_CAPTURE");if(!capture.isEmpty())assert(dialog->grab().save(capture));
    save->click();
  });smoothButton->click();
  assert(panel.library().entries().size()==6&&panel.library().entries().back().name=="Smooth copy");
  assert(panel.state().chosen==savedPanels.chosen&&stations.lines()[2].airfoil==3);
  assert(panel.library().entries()[3].boundary==savedPanels.entries[3].boundary);
  checkExport(panel.library(),5); // Smoothed copies still export exactly 69 points.
  const auto withCopy=panel.state();panel.restoreState(savedPanels);assert(panel.library().entries().size()==5);
  panel.restoreState(withCopy);assert(panel.library().entries().size()==6);
  panel.reset(); assert(panel.library().entries().empty());
  assert(!exportButton->isEnabled());
  assert(!smoothButton->isEnabled());
  return 0;
}

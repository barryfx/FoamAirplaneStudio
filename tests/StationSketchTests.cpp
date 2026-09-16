#include "gui/PlanViewport.h"
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QMessageBox>
#include <QTimer>
#include <cassert>
#include <cmath>
using namespace designrc::gui;
int main(int argc, char** argv) {
  QApplication app{argc, argv};
  PlanViewport view; view.resize(1000, 700); view.show(); app.processEvents();
  auto& sketch = view.sketchEditor(); auto& stations = sketch.stationEditor();
  auto mouse = [&](QEvent::Type type, QPointF point) {
    const auto local = view.mapFromScene(point);
    QMouseEvent event{type, QPointF{local}, QPointF{view.viewport()->mapToGlobal(local)},
        type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
        type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier};
    QApplication::sendEvent(view.viewport(), &event);
  };
  auto click = [&](QPointF p) { mouse(QEvent::MouseButtonPress, p); mouse(QEvent::MouseButtonRelease, p); };
  auto move = [&](QPointF p) { mouse(QEvent::MouseMove, p); };
  auto key = [&](int k) { QKeyEvent event{QEvent::KeyPress, k, Qt::NoModifier}; QApplication::sendEvent(&view, &event); };
  auto line = [&](QPointF a, QPointF b) { sketch.setTool(SketchTool::Line); click(a); click(b); };
  sketch.setEditing(true);
  line({100, 100}, {700, 100}); line({700, 100}, {700, 400}); line({700, 400}, {100, 400});
  const auto originalOutline = sketch.layers()[0].points;
  sketch.setEditing(false); stations.setEnabled(true);
  move({200, 104}); assert(stations.hoverPoint());
  assert(std::abs(stations.hoverPoint()->position.y() - 100) < 1);
  move({200, 150}); assert(!stations.hoverPoint());
  click({200, 104}); move({250, 397});
  assert(stations.hoverPoint()); assert(std::abs(stations.hoverPoint()->position.x() - 200) < 1);
  click({250, 397}); assert(stations.lines().size() == 1);
  assert(stations.lines()[0].alignment == LineAlignment::Vertical);
  assert(std::abs(stations.lines()[0].first.position.x() - stations.lines()[0].second.position.x()) < 1e-8);
  // Reject a reversed second station immediately, with a corrective popup.
  bool warned=false;
  click({600,400});
  QTimer::singleShot(0,[&]{
    auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
    assert(box && box->windowTitle()=="Conflicting LE/TE orientation");
    assert(box->text().contains("not added") && box->text().contains("leading edge first"));
    warned=true;box->accept();
  });
  click({600,100});assert(warned && stations.lines().size()==1 && !stations.state().first);
  // Consecutive placement without re-arming, outside the angle snap threshold.
  click({400, 100}); click({550, 400});
  assert(stations.lines().size() == 2 && stations.lines()[1].alignment == LineAlignment::Free);
  assert(sketch.layers()[0].points == originalOutline);
  // Selection, Escape deselection and endpoint click/move/click with paired motion.
  click({200, 250}); assert(stations.selectedLine() == 0); key(Qt::Key_Escape);
  assert(stations.selectedLine() == -1);
  click({200, 100}); move({300, 130});
  assert(std::abs(stations.lines()[0].second.position.x() - 300) < 1);
  key(Qt::Key_Escape); assert(std::abs(stations.lines()[0].first.position.x() - 300) < 1);
  assert(std::abs(stations.lines()[0].second.position.x() - 300) < 1);
  move({350, 100}); assert(std::abs(stations.lines()[0].first.position.x() - 300) < 1);
  click({300, 100}); move({200, 100}); click({200, 100});
  move({250, 100}); assert(std::abs(stations.lines()[0].first.position.x() - 200) < 1);
  click({200, 100}); move({300, 100}); click({300, 100});
  assert(std::abs(stations.lines()[0].first.position.x() - 300) < 1);
  assert(std::abs(stations.lines()[0].second.position.x() - 300) < 1);
  // Exiting station mode freezes its geometry and cancels any partial placement.
  click({600, 100}); stations.setEnabled(false);
  click({600, 400}); key(Qt::Key_Delete); assert(stations.lines().size() == 2);
  stations.setEnabled(true); click({300, 250});
  const QString capture = qEnvironmentVariable("FOAM_STATION_CAPTURE");
  if (!capture.isEmpty()) assert(view.grab().save(capture));
  key(Qt::Key_Delete); assert(stations.lines().size() == 1);
  assert(sketch.layers()[0].points == originalOutline);
  // Free stations slide one endpoint while retaining the other attachment.
  const auto other = stations.lines()[0].second.position;
  click({400, 100}); move({450, 80}); click({450, 80});
  assert(stations.lines()[0].second.position == other);
  // Changing outline mode cannot delete or select a station.
  stations.setEnabled(false); sketch.setEditing(true); sketch.setTool(SketchTool::None);
  click({500, 250}); key(Qt::Key_Delete); assert(stations.lines().size() == 1);
  // Removing an attached outline curve invalidates its stations.
  click({500, 100}); key(Qt::Key_Delete); assert(stations.lines().empty());
  sketch.reset(); sketch.setEditing(true);
  line({100, 100}, {100, 500}); line({400, 100}, {400, 500});
  sketch.setEditing(false); stations.setEnabled(true);
  click({100, 200}); click({400, 240});
  assert(stations.lines().size() == 1 && stations.lines()[0].alignment == LineAlignment::Horizontal);
  click({100, 200}); move({110, 300}); click({110, 300});
  assert(std::abs(stations.lines()[0].first.position.y() - stations.lines()[0].second.position.y()) < 1e-8);
  assert(std::abs(stations.lines()[0].second.position.y() - 300) < 1);
  // Spline projection and axis intersection use the fitted outline path.
  sketch.reset(); sketch.setEditing(true);
  sketch.setTool(SketchTool::Spline); click({100, 100}); click({400, 140}); click({700, 100}); key(Qt::Key_Escape);
  line({100, 400}, {700, 400});
  sketch.setEditing(false); stations.setEnabled(true);
  move({400, 140}); assert(stations.hoverPoint());
  click({400, 140}); click({440, 400});
  assert(stations.lines().size() == 1 && stations.lines()[0].alignment == LineAlignment::Vertical);
  click(stations.lines()[0].first.position); move({500, 100}); click({500, 100});
  assert(std::abs(stations.lines()[0].first.position.x() - stations.lines()[0].second.position.x()) < 1e-8);
  assert(sketch.layers()[0].points.size() == 5);
  sketch.reset(); assert(stations.lines().empty());
  sketch.setEditing(true);
  line({100, 100}, {700, 100}); line({100, 400}, {700, 400});
  sketch.setEditing(false); stations.setEnabled(true);
  click({200, 100}); click({275, 400}); // 14.0 degrees from vertical.
  click({400, 100}); click({485, 400}); // 15.8 degrees: no snap.
  assert(stations.lines()[0].alignment == LineAlignment::Vertical);
  assert(stations.lines()[1].alignment == LineAlignment::Free);
  view.scale(2, 2); move({550, 100 + 6 / view.transform().m11()}); assert(stations.hoverPoint());
  move({550, 100 + 12 / view.transform().m11()}); assert(!stations.hoverPoint());
  sketch.reset(); sketch.setEditing(true);
  line({100, 100}, {700, 100}); line({200, 400}, {500, 400});
  sketch.setEditing(false); stations.setEnabled(true);
  click({300, 100}); click({300, 400});
  const auto before = stations.lines()[0].first.position;
  click(before); move({600, 100}); click({600, 100}); // No TE intersection here.
  assert(stations.lines()[0].first.position == before);
  move({400, 100}); assert(stations.lines()[0].first.position == before); // Click dropped it.
  key(Qt::Key_Escape);
  sketch.reset();SketchState panels;
  panels.layers={{{{100,100},{700,100},{700,400},{100,400}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{2,3}}}}};
  panels.layers.push_back(panels.layers[0]);sketch.restoreState(panels);stations.setEnabled(true);
  click({200,100});click({200,400});click({500,100});click({500,400});
  assert(stations.lines().size()==2 && !stations.allPanelsDefined());
  stations.setActivePanel(1);
  click({200,100});click({200,400});click({500,100});click({500,400});
  assert(stations.lines().size()==4 && stations.allPanelsDefined());
  assert(stations.lines()[2].first.layer==1 && stations.lines()[2].second.layer==1);
  click({200,250});key(Qt::Key_Delete);
  assert(stations.lines().size()==3 && stations.lines()[0].first.layer==0 && !stations.allPanelsDefined());
  stations.setSelectionEnabled(true);assert(stations.lines()[stations.selectedLine()].first.layer==1);
  key(Qt::Key_Escape);assert(stations.selectedLine()>=0);
  stations.setActivePanel(0);assert(stations.lines()[stations.selectedLine()].first.layer==0);
  return 0;
}

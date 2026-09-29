#include "gui/PlanViewport.h"
#include "gui/WingOutlinePanel.h"
#include <QApplication>
#include <QEventLoop>
#include <QGraphicsScene>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include "TestCheck.h"
#include <iostream>
#include <cstdlib>
#include <cmath>
using namespace designrc::gui;

class ResizeCounter : public QObject {
public:
  int count = 0;
  bool eventFilter(QObject*, QEvent* event) override {
    if (event->type() == QEvent::Resize) ++count;
    return false;
  }
};

void checkRestoredView(QApplication& app) {
  // GentleLady's saved view used to oscillate between both scrollbars and
  // neither scrollbar, flooding the UI with thousands of resize events.
  PlanViewport restored;
  restored.resize(1552, 874);
  restored.show(); app.processEvents();
  restored.scene()->setSceneRect(0, 0, 1067.0716457790797, 1249.150841946072);
  ResizeCounter resizes;
  restored.viewport()->installEventFilter(&resizes);
  const PlanViewState saved{0.5863954309366215, {801.5069272441148,649.7322112340589}};
  restored.restoreView(saved);
  QEventLoop loop;
  QTimer::singleShot(500, &loop, &QEventLoop::quit);
  loop.exec();
  TEST_CHECK(resizes.count < 10);
  TEST_CHECK(std::abs(restored.viewState().zoom-saved.zoom) < 1e-9);
  TEST_CHECK(QLineF(restored.viewState().center,saved.center).length() < 3/saved.zoom);
  const int settled = resizes.count;
  QTimer::singleShot(100, &loop, &QEventLoop::quit);
  loop.exec();
  TEST_CHECK(resizes.count == settled);
}

int main(int argc, char** argv) {
  QApplication app{argc, argv};
  QApplication::setStyle("Fusion");
  checkRestoredView(app);
  PlanViewport view; view.resize(1000, 700); view.show(); app.processEvents();
  auto& editor = view.sketchEditor();
  WingOutlinePanel panel{editor}; panel.show(); app.processEvents();
  editor.setEditing(true);
  auto mouse = [&](QEvent::Type type, QPointF scene, Qt::MouseButton button, Qt::MouseButtons buttons) {
    const QPoint local = view.mapFromScene(scene);
    QMouseEvent event{type, QPointF{local}, QPointF{view.viewport()->mapToGlobal(local)}, button, buttons, Qt::NoModifier};
    QApplication::sendEvent(view.viewport(), &event);
  };
  auto click = [&](QPointF p) {
    mouse(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton);
    mouse(QEvent::MouseButtonRelease, p, Qt::LeftButton, Qt::NoButton);
  };
  auto escape = [&] { QKeyEvent event{QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier}; QApplication::sendEvent(&view, &event); };
  auto* tabs = panel.findChild<QTabWidget*>("wingPanelTabs");
  auto buttons = tabs->currentWidget()->findChildren<QPushButton*>();
  buttons[0]->click(); TEST_CHECK(editor.tool() == SketchTool::Line);
  click({100, 100}); click({300, 100});
  TEST_CHECK(editor.layers()[0].curves.size() == 1);
  TEST_CHECK(!panel.outlinesDefined());
  TEST_CHECK(editor.tool() == SketchTool::Line);
  // Snap by screen distance, preserving a single shared junction identity.
  view.scale(2, 2);
  const auto junction = editor.layers()[0].points.back();
  click(junction + QPointF{3 / view.transform().m11(), 0}); click({400, 200});
  TEST_CHECK(editor.layers()[0].points.size() == 3);
  TEST_CHECK(panel.outlinesDefined());
  TEST_CHECK(editor.layers()[0].curves[0].points.back() == editor.layers()[0].curves[1].points.front());
  buttons[1]->click(); TEST_CHECK(!buttons[0]->isChecked()); TEST_CHECK(buttons[1]->isChecked());
  click({400, 200}); click({500, 300}); click({600, 150});
  TEST_CHECK(editor.layers()[0].curves.size() == 2);
  escape(); TEST_CHECK(editor.layers()[0].curves.size() == 3);
  TEST_CHECK(editor.tool() == SketchTool::Spline && buttons[1]->isChecked());
  const auto path = SketchEditor::fittedPath({{0, 0}, {50, 100}, {100, 0}}, SketchTool::Spline);
  TEST_CHECK(path.elementCount() > 3); TEST_CHECK(path.boundingRect().height() > 99);
  buttons[1]->click(); TEST_CHECK(editor.tool() == SketchTool::None);
  const auto before = editor.layers()[0].points[1];
  mouse(QEvent::MouseButtonPress, before, Qt::LeftButton, Qt::LeftButton);
  mouse(QEvent::MouseMove, {320, 130}, Qt::NoButton, Qt::LeftButton);
  mouse(QEvent::MouseButtonRelease, {320, 130}, Qt::LeftButton, Qt::NoButton);
  TEST_CHECK(QLineF(editor.layers()[0].points[1], QPointF{320, 130}).length() < 2);
  auto* count = panel.findChild<QSpinBox*>("wingPanelCount"); count->setValue(3);
  TEST_CHECK(tabs->count() == 3 && tabs->tabText(2) == "3");
  TEST_CHECK(!panel.outlinesDefined());
  tabs->setCurrentIndex(1); TEST_CHECK(editor.activeLayer() == 1);
  const auto firstPanel = editor.layers()[0].points;
  click(firstPanel[0]); // Cannot move a different panel's point.
  mouse(QEvent::MouseMove, {110, 130}, Qt::NoButton, Qt::LeftButton);
  TEST_CHECK(editor.layers()[0].points == firstPanel);
  editor.setTool(SketchTool::Line); click({50, 50}); click({150, 50});
  TEST_CHECK(editor.layers()[1].curves.size() == 1 && editor.layers()[0].curves.size() == 3);
  editor.setEditing(false); click({200, 200}); click({300, 300});
  TEST_CHECK(editor.layers()[1].curves.size() == 1);
  // Rebuilding reference graphics must preserve all sketch layers.
  QImage image{1000, 1500, QImage::Format_RGB32}; image.fill(Qt::white);
  view.setReferenceBackground({{image, QSizeF{100, 150}}}, false);
  TEST_CHECK(editor.layers()[0].curves.size() == 3 && editor.layers()[1].curves.size() == 1);
  TEST_CHECK(!view.grab().isNull()); // Exercise painting active and inactive curves.
  const auto oldPoint = editor.layers()[0].points[0];
  view.setReferenceBackground({{image, QSizeF{100, 150}}}, true);
  TEST_CHECK((QLineF{editor.layers()[0].points[0], oldPoint * 0.1}.length() < 1e-8));
  view.setReferenceBackground({{image, QSizeF{100, 150}}}, false);
  TEST_CHECK((QLineF{editor.layers()[0].points[0], oldPoint}.length() < 1e-8));
  panel.reset(); TEST_CHECK(tabs->count() == 1 && count->value() == 1);
  TEST_CHECK(editor.layers().size() == 1 && editor.layers()[0].curves.empty());
  editor.setEditing(true); editor.setTool(SketchTool::Line); click({100, 100}); escape();
  TEST_CHECK(editor.layers()[0].points.empty());
  click({100, 100}); click({100, 100}); escape();
  TEST_CHECK(editor.layers()[0].curves.empty());
  auto remove = [&] { QKeyEvent event{QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier}; QApplication::sendEvent(&view, &event); };
  click({100, 100}); click({300, 100});
  click({300, 100}); click({400, 200});
  editor.setTool(SketchTool::None);
  click({200, 100}); TEST_CHECK(editor.selectedCurve() == 0);
  const auto highlighted = view.grab().toImage();
  const QString capture = qEnvironmentVariable("FOAM_SKETCH_CAPTURE");
  if (!capture.isEmpty()) TEST_CHECK(highlighted.save(capture));
  escape(); TEST_CHECK(editor.selectedCurve() == -1);
  TEST_CHECK(highlighted != view.grab().toImage());
  view.scale(2, 2);
  click({200, 100 + 6 / view.transform().m11()}); TEST_CHECK(editor.selectedCurve() == 0);
  click({200, 100 + 12 / view.transform().m11()}); TEST_CHECK(editor.selectedCurve() == -1);
  remove(); TEST_CHECK(editor.layers()[0].curves.size() == 2);
  click({200, 100}); remove();
  TEST_CHECK(editor.layers()[0].curves.size() == 1 && editor.layers()[0].points.size() == 2);
  TEST_CHECK(editor.selectedCurve() == -1);
  // The retained edge still has valid shared endpoint indices after compaction.
  const auto& retained = editor.layers()[0];
  TEST_CHECK(retained.points[retained.curves[0].points.front()].x() > 298);
  editor.setTool(SketchTool::Spline);
  click({100, 300}); click({200, 400}); click({300, 300}); escape();
  editor.setTool(SketchTool::None);
  std::vector<QPointF> splinePoints;
  for (auto id : editor.layers()[0].curves.back().points) splinePoints.push_back(editor.layers()[0].points[id]);
  const auto splinePath = SketchEditor::fittedPath(splinePoints, SketchTool::Spline);
  click(splinePath.pointAtPercent(0.3)); TEST_CHECK(editor.selectedCurve() == 1);
  editor.setLayerCount(2); TEST_CHECK(editor.selectedCurve() == -1);
  editor.setActiveLayer(1); click(splinePath.pointAtPercent(0.3));
  TEST_CHECK(editor.selectedCurve() == -1); remove();
  TEST_CHECK(editor.layers()[0].curves.size() == 2);
  editor.setActiveLayer(0); click(splinePath.pointAtPercent(0.3)); remove();
  TEST_CHECK(editor.layers()[0].curves.size() == 1);
  click({350, 150}); TEST_CHECK(editor.selectedCurve() == 0);
  editor.setTool(SketchTool::Line); TEST_CHECK(editor.selectedCurve() == -1);
  remove(); TEST_CHECK(editor.layers()[0].curves.size() == 1);
  editor.setTool(SketchTool::None); click({350, 150});
  editor.setEditing(false); TEST_CHECK(editor.selectedCurve() == -1); remove();
  TEST_CHECK(editor.layers()[0].curves.size() == 1);
  // Paint explicitly while inactive: outlines must remain, without point handles.
  QImage inactive{1000, 700, QImage::Format_RGB32}; inactive.fill(Qt::white);
  { QPainter painter{&inactive}; editor.paint(painter); }
  TEST_CHECK(inactive.pixelColor(350, 150) != QColor{Qt::white});
  // Reference ink and white paper both need a clearly visible inactive outline.
  QImage contrast{1000, 700, QImage::Format_RGB32}; contrast.fill(Qt::black);
  {
    QPainter painter{&contrast};
    painter.fillRect(350, 0, 650, 700, Qt::white);
    editor.paint(painter);
  }
  const auto lightBlue = QColor(80, 200, 255);
  TEST_CHECK(contrast.pixelColor(325, 125) == lightBlue);
  TEST_CHECK(contrast.pixelColor(375, 175) == lightBlue);
  if (!capture.isEmpty()) TEST_CHECK(contrast.save(capture + ".inactive.png"));
  // Exercise the actual viewport foreground after leaving the editing panel.
  panel.hide(); view.fitAll(); app.processEvents();
  view.ensureVisible(QRectF{300, 100, 100, 100}, 20, 20);
  const auto screenPoint = view.viewport()->mapTo(&view, view.mapFromScene({350, 150}));
  const auto visible = view.grab().toImage();
  const auto imagePoint = QPointF{screenPoint} * visible.devicePixelRatio();
  if (!capture.isEmpty()) TEST_CHECK(visible.save(capture + ".viewport.png"));
  // Mapping a logical point and then rounding at fractional display scaling
  // can land on an antialiased edge pixel. Require the exact outline color in
  // its small physical-pixel neighborhood, rather than one rounded pixel.
  bool blueVisible=false;
  const auto sample=imagePoint.toPoint();
  for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx) {
    const auto pixel=sample+QPoint{dx,dy};
    if(visible.rect().contains(pixel)&&visible.pixelColor(pixel)==lightBlue)blueVisible=true;
  }
  TEST_CHECK(blueVisible);
  editor.setEditing(true); click({350, 150}); remove();
  TEST_CHECK(editor.layers()[0].curves.empty() && editor.layers()[0].points.empty());
  // Three-panel wing: every inner panel has independent LE and TE chains.
  panel.reset(); count->setValue(3); editor.setEditing(true);
  auto line = [&](QPointF a, QPointF b) {
    editor.setTool(SketchTool::Line); click(a); click(b);
  };
  line({100, 100}, {200, 100}); line({200, 100}, {300, 100});
  TEST_CHECK(!panel.panelOutlineDefined(0));
  line({100, 200}, {200, 200}); line({200, 200}, {300, 200});
  TEST_CHECK(panel.panelOutlineDefined(0)); TEST_CHECK(!panel.outlinesDefined());
  tabs->setCurrentIndex(1);
  editor.setTool(SketchTool::Spline);
  click({300, 100}); click({400, 110}); click({500, 100}); escape();
  click({300, 200}); click({400, 190}); click({500, 200}); escape();
  TEST_CHECK(panel.panelOutlineDefined(1)); TEST_CHECK(!panel.outlinesDefined());
  tabs->setCurrentIndex(2);
  line({500, 100}, {700, 100}); line({500, 200}, {700, 200});
  TEST_CHECK(!panel.panelOutlineDefined(2)); TEST_CHECK(!panel.outlinesDefined());
  line({700, 100}, {700, 200}); // Only the last panel needs a tip connection.
  TEST_CHECK(panel.panelOutlineDefined(2)); TEST_CHECK(panel.outlinesDefined());
  tabs->setCurrentIndex(0); TEST_CHECK(panel.outlinesDefined());
  // A tip-like connection on an inner panel is not the required LE/TE pair.
  line({300, 100}, {300, 200}); TEST_CHECK(!panel.outlinesDefined());
  editor.setTool(SketchTool::None); click({300, 150}); remove();
  TEST_CHECK(panel.outlinesDefined());
  // Removing part of any inner panel invalidates the whole wing.
  click({150, 100}); remove(); click({250, 100}); remove();
  TEST_CHECK(!panel.panelOutlineDefined(0)); TEST_CHECK(!panel.outlinesDefined());
  line({100, 100}, {300, 100}); TEST_CHECK(panel.outlinesDefined());
  count->setValue(4); TEST_CHECK(!panel.outlinesDefined()); // New empty tip panel.
  count->setValue(3); TEST_CHECK(panel.outlinesDefined());
  // Stabilizer Cut uses layer selection, but Delete must target one curve.
  panel.reset();editor.setLayerSelectionMode(true);editor.setEditing(true);
  line({100,100},{300,100});line({300,100},{300,250});line({300,250},{100,100});
  editor.setTool(SketchTool::None);click({200,100});TEST_CHECK(editor.selectedCurve()==0);remove();
  TEST_CHECK(editor.layers().size()==1&&editor.layers()[0].curves.size()==2);
  editor.setTool(SketchTool::Circle);click({500,300});click({540,300});
  editor.setTool(SketchTool::None);click({540,300});TEST_CHECK(editor.selectedCurve()==2);remove();
  TEST_CHECK(editor.layers()[0].curves.size()==2);
  return 0;
}

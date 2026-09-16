#include "gui/PlanViewport.h"
#include "gui/WingOutlinePanel.h"
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <cassert>
#include <iostream>
#include <cstdlib>
#undef assert
#define assert(condition) do { if(!(condition)) { std::cerr << #condition << " at line " << __LINE__ << '\n'; std::exit(1); } } while(false)
#include <cmath>
using namespace designrc::gui;

int main(int argc, char** argv) {
  QApplication app{argc, argv};
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
  buttons[0]->click(); assert(editor.tool() == SketchTool::Line);
  click({100, 100}); click({300, 100});
  assert(editor.layers()[0].curves.size() == 1);
  assert(!panel.outlinesDefined());
  assert(editor.tool() == SketchTool::Line);
  // Snap by screen distance, preserving a single shared junction identity.
  view.scale(2, 2);
  const auto junction = editor.layers()[0].points.back();
  click(junction + QPointF{3 / view.transform().m11(), 0}); click({400, 200});
  assert(editor.layers()[0].points.size() == 3);
  assert(panel.outlinesDefined());
  assert(editor.layers()[0].curves[0].points.back() == editor.layers()[0].curves[1].points.front());
  buttons[1]->click(); assert(!buttons[0]->isChecked()); assert(buttons[1]->isChecked());
  click({400, 200}); click({500, 300}); click({600, 150});
  assert(editor.layers()[0].curves.size() == 2);
  escape(); assert(editor.layers()[0].curves.size() == 3);
  assert(editor.tool() == SketchTool::Spline && buttons[1]->isChecked());
  const auto path = SketchEditor::fittedPath({{0, 0}, {50, 100}, {100, 0}}, SketchTool::Spline);
  assert(path.elementCount() > 3); assert(path.boundingRect().height() > 99);
  buttons[1]->click(); assert(editor.tool() == SketchTool::None);
  const auto before = editor.layers()[0].points[1];
  mouse(QEvent::MouseButtonPress, before, Qt::LeftButton, Qt::LeftButton);
  mouse(QEvent::MouseMove, {320, 130}, Qt::NoButton, Qt::LeftButton);
  mouse(QEvent::MouseButtonRelease, {320, 130}, Qt::LeftButton, Qt::NoButton);
  assert(QLineF(editor.layers()[0].points[1], QPointF{320, 130}).length() < 2);
  auto* count = panel.findChild<QSpinBox*>("wingPanelCount"); count->setValue(3);
  assert(tabs->count() == 3 && tabs->tabText(2) == "3");
  assert(!panel.outlinesDefined());
  tabs->setCurrentIndex(1); assert(editor.activeLayer() == 1);
  const auto firstPanel = editor.layers()[0].points;
  click(firstPanel[0]); // Cannot move a different panel's point.
  mouse(QEvent::MouseMove, {110, 130}, Qt::NoButton, Qt::LeftButton);
  assert(editor.layers()[0].points == firstPanel);
  editor.setTool(SketchTool::Line); click({50, 50}); click({150, 50});
  assert(editor.layers()[1].curves.size() == 1 && editor.layers()[0].curves.size() == 3);
  editor.setEditing(false); click({200, 200}); click({300, 300});
  assert(editor.layers()[1].curves.size() == 1);
  // Rebuilding reference graphics must preserve all sketch layers.
  QImage image{1000, 1500, QImage::Format_RGB32}; image.fill(Qt::white);
  view.setReferenceBackground({{image, QSizeF{100, 150}}}, false);
  assert(editor.layers()[0].curves.size() == 3 && editor.layers()[1].curves.size() == 1);
  assert(!view.grab().isNull()); // Exercise painting active and inactive curves.
  const auto oldPoint = editor.layers()[0].points[0];
  view.setReferenceBackground({{image, QSizeF{100, 150}}}, true);
  assert((QLineF{editor.layers()[0].points[0], oldPoint * 0.1}.length() < 1e-8));
  view.setReferenceBackground({{image, QSizeF{100, 150}}}, false);
  assert((QLineF{editor.layers()[0].points[0], oldPoint}.length() < 1e-8));
  panel.reset(); assert(tabs->count() == 1 && count->value() == 1);
  assert(editor.layers().size() == 1 && editor.layers()[0].curves.empty());
  editor.setEditing(true); editor.setTool(SketchTool::Line); click({100, 100}); escape();
  assert(editor.layers()[0].points.empty());
  click({100, 100}); click({100, 100}); escape();
  assert(editor.layers()[0].curves.empty());
  auto remove = [&] { QKeyEvent event{QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier}; QApplication::sendEvent(&view, &event); };
  click({100, 100}); click({300, 100});
  click({300, 100}); click({400, 200});
  editor.setTool(SketchTool::None);
  click({200, 100}); assert(editor.selectedCurve() == 0);
  const auto highlighted = view.grab().toImage();
  const QString capture = qEnvironmentVariable("FOAM_SKETCH_CAPTURE");
  if (!capture.isEmpty()) assert(highlighted.save(capture));
  escape(); assert(editor.selectedCurve() == -1);
  assert(highlighted != view.grab().toImage());
  view.scale(2, 2);
  click({200, 100 + 6 / view.transform().m11()}); assert(editor.selectedCurve() == 0);
  click({200, 100 + 12 / view.transform().m11()}); assert(editor.selectedCurve() == -1);
  remove(); assert(editor.layers()[0].curves.size() == 2);
  click({200, 100}); remove();
  assert(editor.layers()[0].curves.size() == 1 && editor.layers()[0].points.size() == 2);
  assert(editor.selectedCurve() == -1);
  // The retained edge still has valid shared endpoint indices after compaction.
  const auto& retained = editor.layers()[0];
  assert(retained.points[retained.curves[0].points.front()].x() > 298);
  editor.setTool(SketchTool::Spline);
  click({100, 300}); click({200, 400}); click({300, 300}); escape();
  editor.setTool(SketchTool::None);
  std::vector<QPointF> splinePoints;
  for (auto id : editor.layers()[0].curves.back().points) splinePoints.push_back(editor.layers()[0].points[id]);
  const auto splinePath = SketchEditor::fittedPath(splinePoints, SketchTool::Spline);
  click(splinePath.pointAtPercent(0.3)); assert(editor.selectedCurve() == 1);
  editor.setLayerCount(2); assert(editor.selectedCurve() == -1);
  editor.setActiveLayer(1); click(splinePath.pointAtPercent(0.3));
  assert(editor.selectedCurve() == -1); remove();
  assert(editor.layers()[0].curves.size() == 2);
  editor.setActiveLayer(0); click(splinePath.pointAtPercent(0.3)); remove();
  assert(editor.layers()[0].curves.size() == 1);
  click({350, 150}); assert(editor.selectedCurve() == 0);
  editor.setTool(SketchTool::Line); assert(editor.selectedCurve() == -1);
  remove(); assert(editor.layers()[0].curves.size() == 1);
  editor.setTool(SketchTool::None); click({350, 150});
  editor.setEditing(false); assert(editor.selectedCurve() == -1); remove();
  assert(editor.layers()[0].curves.size() == 1);
  // Paint explicitly while inactive: outlines must remain, without point handles.
  QImage inactive{1000, 700, QImage::Format_RGB32}; inactive.fill(Qt::white);
  { QPainter painter{&inactive}; editor.paint(painter); }
  assert(inactive.pixelColor(350, 150) != QColor{Qt::white});
  // Reference ink and white paper both need a clearly visible inactive outline.
  QImage contrast{1000, 700, QImage::Format_RGB32}; contrast.fill(Qt::black);
  {
    QPainter painter{&contrast};
    painter.fillRect(350, 0, 650, 700, Qt::white);
    editor.paint(painter);
  }
  const auto lightBlue = QColor(80, 200, 255);
  assert(contrast.pixelColor(325, 125) == lightBlue);
  assert(contrast.pixelColor(375, 175) == lightBlue);
  if (!capture.isEmpty()) assert(contrast.save(capture + ".inactive.png"));
  // Exercise the actual viewport foreground after leaving the editing panel.
  panel.hide(); view.fitAll(); app.processEvents();
  view.ensureVisible(QRectF{300, 100, 100, 100}, 20, 20);
  const auto screenPoint = view.viewport()->mapTo(&view, view.mapFromScene({350, 150}));
  const auto visible = view.grab().toImage();
  const auto imagePoint = QPointF{screenPoint} * visible.devicePixelRatio();
  if (!capture.isEmpty()) assert(visible.save(capture + ".viewport.png"));
  // Mapping a logical point and then rounding at fractional display scaling
  // can land on an antialiased edge pixel. Require the exact outline color in
  // its small physical-pixel neighborhood, rather than one rounded pixel.
  bool blueVisible=false;
  const auto sample=imagePoint.toPoint();
  for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx) {
    const auto pixel=sample+QPoint{dx,dy};
    if(visible.rect().contains(pixel)&&visible.pixelColor(pixel)==lightBlue)blueVisible=true;
  }
  assert(blueVisible);
  editor.setEditing(true); click({350, 150}); remove();
  assert(editor.layers()[0].curves.empty() && editor.layers()[0].points.empty());
  // Three-panel wing: every inner panel has independent LE and TE chains.
  panel.reset(); count->setValue(3); editor.setEditing(true);
  auto line = [&](QPointF a, QPointF b) {
    editor.setTool(SketchTool::Line); click(a); click(b);
  };
  line({100, 100}, {200, 100}); line({200, 100}, {300, 100});
  assert(!panel.panelOutlineDefined(0));
  line({100, 200}, {200, 200}); line({200, 200}, {300, 200});
  assert(panel.panelOutlineDefined(0)); assert(!panel.outlinesDefined());
  tabs->setCurrentIndex(1);
  editor.setTool(SketchTool::Spline);
  click({300, 100}); click({400, 110}); click({500, 100}); escape();
  click({300, 200}); click({400, 190}); click({500, 200}); escape();
  assert(panel.panelOutlineDefined(1)); assert(!panel.outlinesDefined());
  tabs->setCurrentIndex(2);
  line({500, 100}, {700, 100}); line({500, 200}, {700, 200});
  assert(!panel.panelOutlineDefined(2)); assert(!panel.outlinesDefined());
  line({700, 100}, {700, 200}); // Only the last panel needs a tip connection.
  assert(panel.panelOutlineDefined(2)); assert(panel.outlinesDefined());
  tabs->setCurrentIndex(0); assert(panel.outlinesDefined());
  // A tip-like connection on an inner panel is not the required LE/TE pair.
  line({300, 100}, {300, 200}); assert(!panel.outlinesDefined());
  editor.setTool(SketchTool::None); click({300, 150}); remove();
  assert(panel.outlinesDefined());
  // Removing part of any inner panel invalidates the whole wing.
  click({150, 100}); remove(); click({250, 100}); remove();
  assert(!panel.panelOutlineDefined(0)); assert(!panel.outlinesDefined());
  line({100, 100}, {300, 100}); assert(panel.outlinesDefined());
  count->setValue(4); assert(!panel.outlinesDefined()); // New empty tip panel.
  count->setValue(3); assert(panel.outlinesDefined());
  return 0;
}

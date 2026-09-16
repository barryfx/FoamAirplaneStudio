#include "gui/PlanViewport.h"
#include "gui/TechnicalDrawing.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QScrollBar>
#include <QWheelEvent>
#include <cmath>

#include <cassert>

int main(int argc, char* argv[]) {
  QApplication application{argc, argv};
  designrc::gui::PlanViewport viewport;
  designrc::gui::TechnicalDrawingDocument document;
  document.pageBoundsMm = QRectF{0.0, 0.0, 100.0, 80.0};
  QPainterPath outline;
  outline.addRect(QRectF{10.0, 15.0, 30.0, 20.0});
  document.paths.push_back({outline, Qt::black, Qt::transparent, 0.25});

  viewport.setDocument(document);
  assert(viewport.scene()->sceneRect().contains(document.pageBoundsMm));
  assert(!viewport.scene()->items().empty());
  viewport.clearPlan();
  assert(viewport.scene()->items().empty());
  QImage image{400, 900, QImage::Format_RGB32};
  image.fill(Qt::white);
  viewport.resize(640, 480);
  viewport.show();
  viewport.setReferenceBackground({{image, {}}, {image, {}}}, false);
  application.processEvents();
  assert(viewport.scene()->items().size() == 2);
  assert(std::abs(viewport.transform().m11() * 400 - viewport.viewport()->width()) <= 2);
  assert(viewport.verticalScrollBar()->maximum() > 0);
  viewport.resize(800, 480);
  application.processEvents();
  assert(std::abs(viewport.transform().m11() * 400 - viewport.viewport()->width()) <= 2);
  assert(viewport.scene()->sceneRect().height() == 1800);
  // A wheel step must increase magnification, including when it causes a
  // horizontal scrollbar to appear and therefore resizes the inner viewport.
  const double fittedScale = viewport.transform().m11();
  const QPointF center = viewport.viewport()->rect().center();
  QWheelEvent zoomIn{center, viewport.viewport()->mapToGlobal(center.toPoint()),
      QPoint{}, QPoint{0, 120}, Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false};
  QApplication::sendEvent(viewport.viewport(), &zoomIn);
  application.processEvents();
  assert(viewport.transform().m11() > fittedScale * 1.1);
  assert(viewport.horizontalScrollBar()->maximum() > 0);
  const double zoomedScale = viewport.transform().m11();

  // Explicit scrollbar movement changes the visible region, not magnification.
  viewport.verticalScrollBar()->setValue(viewport.verticalScrollBar()->maximum() / 2);
  application.processEvents();
  assert(viewport.verticalScrollBar()->value() > 0);
  assert(std::abs(viewport.transform().m11() - zoomedScale) < 1e-10);

  QWheelEvent zoomOut{center, viewport.viewport()->mapToGlobal(center.toPoint()),
      QPoint{}, QPoint{0, -120}, Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false};
  QApplication::sendEvent(viewport.viewport(), &zoomOut);
  application.processEvents();
  assert(std::abs(viewport.transform().m11() - fittedScale) < 1e-8);
  // The cursor's scene point remains fixed with nonzero scroll positions,
  // when bars appear/disappear, and with trackpad pixel deltas.
  const QPoint cursor{173,119};
  auto anchoredZoom=[&](int delta,bool pixels=false) {
    const auto before=viewport.mapToScene(cursor);
    QWheelEvent event{QPointF{cursor},viewport.viewport()->mapToGlobal(cursor),
      pixels?QPoint{0,delta}:QPoint{},pixels?QPoint{}:QPoint{0,delta},
      Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false};
    QApplication::sendEvent(viewport.viewport(),&event);application.processEvents();
    const auto after=viewport.mapToScene(cursor);
    assert((QLineF{before,after}.length()*viewport.transform().m11()<2.0));
  };
  for(int i=0;i<5;++i)anchoredZoom(120);
  viewport.horizontalScrollBar()->setValue(viewport.horizontalScrollBar()->maximum()/2);
  viewport.verticalScrollBar()->setValue(viewport.verticalScrollBar()->maximum()/2);
  anchoredZoom(80,true);
  for(int i=0;i<12;++i)anchoredZoom(-120);
  assert(viewport.horizontalScrollBar()->maximum()<=1);
  const auto savedView=viewport.viewState();
  viewport.fitAll();application.processEvents();
  viewport.restoreView(savedView);application.processEvents();
  assert((QLineF{savedView.center,viewport.viewState().center}.length()*savedView.zoom<2));
  viewport.fitAll();
  application.processEvents();
  assert(std::abs(viewport.transform().m11() * 400 - viewport.viewport()->width()) <= 2);
  assert(viewport.dragMode() == QGraphicsView::NoDrag);
  const auto capture=qEnvironmentVariable("FOAM_ZOOM_CAPTURE");
  if(!capture.isEmpty())assert(viewport.grab().save(capture));
  return 0;
}




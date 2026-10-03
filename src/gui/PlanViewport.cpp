#include "gui/PlanViewport.h"

#include <QBrush>
#include <QFont>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QFileInfo>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPen>
#include <QResizeEvent>
#include <QShowEvent>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace designrc::gui {

PlanViewport::PlanViewport(QWidget* parent) : QGraphicsView{parent},
    scene_{new QGraphicsScene{this}} {
  setScene(scene_);
  setBackgroundBrush(QColor{232, 232, 232});
  setRenderHint(QPainter::Antialiasing, true);
  setDragMode(QGraphicsView::NoDrag);
  setTransformationAnchor(QGraphicsView::NoAnchor);
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  setFrameShape(QFrame::NoFrame);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
  for(auto* bar:{horizontalScrollBar(),verticalScrollBar()}) {
    // Layout can change scrollbar values while a restored view is hidden.
    // Only explicit user scrolling should discard that pending restoration.
    connect(bar,&QScrollBar::actionTriggered,this,[this]{restoredView_.reset();});
    connect(bar,&QScrollBar::sliderMoved,this,[this]{restoredView_.reset();});
  }
  for (auto& editor : stabilizerSketchEditors_) editor = new SketchEditor{this};
  for(auto& editor:stabilizerHingeEditors_)editor=new SketchEditor{this};
  for(auto& editor:stabilizerCutEditors_)editor=new SketchEditor{this, SketchAppearance::Cut};
  for(auto& editor:fiberglassEditors_)editor=new SketchEditor{this, SketchAppearance::Fiberglass};
  sketchEditor_ = new SketchEditor{this};
  airfoilSketchEditor_ = new SketchEditor{this};
  airfoilSketchEditor_->setClosedLoopMode(true);
  airfoilSketchEditor_->setEscapeEndsSession(true);
  fuselageSketchEditor_ = new SketchEditor{this};
  fuselageSketchEditor_->setClosedLoopMode(true);
  fuselageSketchEditor_->stationEditor().setVerticalPlacement(true);
  fuselageSketchEditor_->setLayerCount(2);
  fuselageProfileEditor_ = new SketchEditor{this};
  fuselageProfileEditor_->setClosedLoopMode(true);
  fuselageProfileEditor_->setSnapAcrossLayers(false);
  fuselageCutEditor_ = new SketchEditor{this, SketchAppearance::Cut};
  fuselageCutEditor_->setLayerCount(4);
  fuselageCutEditor_->setClosedLoopMode(true);
  fuselageCutEditor_->setSnapAcrossLayers(false);
  fuselageHoleEditor_=new SketchEditor{this, SketchAppearance::Hole};fuselageHoleEditor_->setLayerCount(4);
  fuselageHoleEditor_->setClosedLoopMode(true);fuselageHoleEditor_->setSnapAcrossLayers(false);
  servoTrayEditor_=new ServoTrayEditor{*this};
  formerEditor_=new FormerEditor{*this};
  formerEditor_->tray=[this]{return servoTrayEditor_->state().rectangle;};
  servoTrayEditor_->acceptRectangle=[this](const QRectF& r){return formerEditor_->allowsTray(r);};
  controlSurfaceEditor_ = new ControlSurfaceEditor{*this};
  scene_->setSceneRect(0, 0, 1000, 700);
}

void PlanViewport::drawForeground(QPainter* painter, const QRectF& rect) {
  QGraphicsView::drawForeground(painter, rect);
  sketchEditor_->paint(*painter);
  for (auto* editor : stabilizerSketchEditors_) editor->paint(*painter);
  for(auto* editor:stabilizerHingeEditors_)editor->paint(*painter);
  for(auto* editor:stabilizerCutEditors_)editor->paint(*painter);
  airfoilSketchEditor_->paint(*painter);
  fuselageSketchEditor_->paint(*painter);
  controlSurfaceEditor_->paint(*painter);
  fuselageProfileEditor_->paint(*painter);
  fuselageCutEditor_->paint(*painter);
  fuselageHoleEditor_->paint(*painter);
  for(auto* editor:fiberglassEditors_)editor->paint(*painter);
  servoTrayEditor_->paint(*painter);
  formerEditor_->paint(*painter);
  if(balanceOverlay)balanceOverlay(*painter);
}

void PlanViewport::setDocument(const TechnicalDrawingDocument& document) {
  scene_->clear();
  document_ = document;
  QRectF referenceBounds;
  double top = 0;
  for (const auto& page : referencePages_) {
    const QSizeF size = referenceToScale_ && page.physicalSizeMm
        ? *page.physicalSizeMm : QSizeF{page.pixels.size()};
    auto* background = scene_->addPixmap(QPixmap::fromImage(page.pixels));
    background->setTransformationMode(Qt::SmoothTransformation);
    background->setTransform(QTransform::fromScale(
        size.width() / page.pixels.width(), size.height() / page.pixels.height()));
    background->setPos(0, top);
    background->setZValue(-200.0);
    referenceBounds = referenceBounds.united(QRectF{QPointF{0, top}, size});
    top += size.height();
  }
  const auto bounds = referenceBounds.united(document.pageBoundsMm);
  scene_->setSceneRect(bounds.isEmpty() ? QRectF{0, 0, 1000, 700} : bounds);
  if (document_.empty()) return;

  auto* page = scene_->addRect(document.pageBoundsMm,
      QPen{QColor{185, 185, 185}, 0.0}, QBrush{referencePages_.empty() ? Qt::white : Qt::transparent});
  page->setZValue(-100.0);

  for (const auto& drawingPath : document.paths) {
    QPen pen{drawingPath.stroke};
    pen.setWidthF(drawingPath.lineWidthMm >= 0.45 ? 1.4 : 1.0);
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::MiterJoin);
    auto* item = scene_->addPath(drawingPath.path, pen, QBrush{drawingPath.fill});
    item->setZValue(drawingPath.fill.alpha() == 0 ? 2.0 : 1.0);
  }
  for (const auto& drawingText : document.texts) {
    QFont font;
    font.setFamily("Arial");
    font.setPointSizeF(drawingText.heightMm * 72.0 / 25.4);
    auto* item = scene_->addSimpleText(drawingText.text, font);
    item->setBrush(QColor{55, 55, 55});
    item->setPos(drawingText.position);
    item->setRotation(drawingText.rotationDegrees);
    item->setZValue(3.0);
  }
  scene_->setSceneRect(scene_->itemsBoundingRect());
  fitOnNextResize_ = true;
  fitAll();
  fitOnNextResize_ = true;
}

void PlanViewport::clearPlan() {
  restoredView_.reset();
  referencePages_.clear();
  referenceToScale_ = false;
  scene_->clear();
  document_ = {};
  scene_->setSceneRect(0, 0, 1000, 700);
  setSceneRect(scene_->sceneRect());zoomAnchor_.reset();
  sketchEditor_->reset();
  for (auto* editor : stabilizerSketchEditors_) editor->reset();
  for(auto* editor:stabilizerHingeEditors_)editor->reset();
  for(auto* editor:stabilizerCutEditors_)editor->reset();
  for(auto* editor:fiberglassEditors_)editor->reset();
  airfoilSketchEditor_->reset();
  fuselageProfileEditor_->reset();
  servoTrayEditor_->restore({});
  formerEditor_->restore({});
  fuselageCutEditor_->reset();fuselageCutEditor_->setLayerCount(4);
  fuselageHoleEditor_->reset();fuselageHoleEditor_->setLayerCount(4);
  fuselageSketchEditor_->reset();
  fuselageSketchEditor_->setLayerCount(2);
  controlSurfaceEditor_->restore({});
  resetTransform();
  fitOnNextResize_ = false;
}

void PlanViewport::setReferenceBackground(const std::vector<ReferencePage>& pages, bool toScale) {
  bool unchanged = referenceToScale_ == toScale && referencePages_.size() == pages.size();
  for (std::size_t i = 0; unchanged && i < pages.size(); ++i)
    unchanged = referencePages_[i].pixels.cacheKey() == pages[i].pixels.cacheKey()
        && referencePages_[i].physicalSizeMm == pages[i].physicalSizeMm;
  if (unchanged) return;
  bool sameImages = !pages.empty() && referencePages_.size() == pages.size();
  for (std::size_t i = 0; sameImages && i < pages.size(); ++i)
    sameImages = referencePages_[i].pixels.cacheKey() == pages[i].pixels.cacheKey();
  if (sameImages && referenceToScale_ != toScale) {
    // Preserve tracing alignment when the same reference switches between
    // pixel preview and physical coordinates, including stacked PDF pages.
    const auto remap = [&](QPointF point) {
      double oldTop = 0, newTop = 0;
      for (std::size_t i = 0; i < pages.size(); ++i) {
        const auto oldSize = referenceToScale_ && referencePages_[i].physicalSizeMm
            ? *referencePages_[i].physicalSizeMm : QSizeF{referencePages_[i].pixels.size()};
        const auto newSize = toScale && pages[i].physicalSizeMm
            ? *pages[i].physicalSizeMm : QSizeF{pages[i].pixels.size()};
        if (point.y() < oldTop + oldSize.height() || i + 1 == pages.size())
          return QPointF{point.x() * newSize.width() / oldSize.width(),
              newTop + (point.y() - oldTop) * newSize.height() / oldSize.height()};
        oldTop += oldSize.height(); newTop += newSize.height();
      }
      return point;
    };
    sketchEditor_->mapPoints(remap);
    for (auto* editor : stabilizerSketchEditors_) editor->mapPoints(remap);
    for(auto* editor:stabilizerHingeEditors_)editor->mapPoints(remap);
    for(auto* editor:stabilizerCutEditors_)editor->mapPoints(remap);
    for(auto* editor:fiberglassEditors_)editor->mapPoints(remap);
    airfoilSketchEditor_->mapPoints(remap);
    fuselageSketchEditor_->mapPoints(remap);
    fuselageProfileEditor_->mapPoints(remap);
    servoTrayEditor_->mapPoints(remap);
    formerEditor_->mapPoints(remap);
    fuselageCutEditor_->mapPoints(remap);
    fuselageHoleEditor_->mapPoints(remap);
    controlSurfaceEditor_->mapPoints(remap);
  }
  referencePages_ = pages;
  referenceToScale_ = toScale;
  setDocument(document_);
  fitAll();
  verticalScrollBar()->setValue(verticalScrollBar()->minimum());
}

void PlanViewport::fitAll() {
  restoredView_.reset();
  if (fittingWidth_ || scene_->sceneRect().width() <= 0) return;
  fittingWidth_ = true;
  zoomAnchor_.reset();
  setSceneRect(scene_->sceneRect());
  const QRectF bounds = scene_->sceneRect();
  const double oldScale = transform().m11();
  const double top = oldScale > 0 ? verticalScrollBar()->value() / oldScale : 0;
  // Account for the scrollbar before setting the transform. Predicting its
  // presence avoids fit/resize oscillation for content near viewport height.
  const QSize available = maximumViewportSize();
  double width = available.width();
  const double scaleWithoutBar = width / bounds.width();
  if (bounds.height() * scaleWithoutBar > available.height())
    width -= verticalScrollBar()->sizeHint().width();
  const double factor = std::max(1.0, width) / bounds.width();
  setTransform(QTransform::fromScale(factor, factor));
  verticalScrollBar()->setValue(qRound(top * factor));
  fitOnNextResize_ = false;
  fittingWidth_ = false;
}
bool PlanViewport::exportPdf(const QString& path, QString& error) const {
  if (document_.empty() || document_.pageBoundsMm.isEmpty()) {
    error = "Generate a valid plan before exporting it.";
    return false;
  }

  QPdfWriter writer{path};
  writer.setTitle("DesignRC Full-Scale Wing Plan");
  writer.setCreator("DesignRC");
  writer.setResolution(600);
  const QPageSize pageSize{document_.pageBoundsMm.size(), QPageSize::Millimeter,
                           "DesignRC Plan", QPageSize::ExactMatch};
  if (!pageSize.isValid() || !writer.setPageSize(pageSize) ||
      !writer.setPageMargins(QMarginsF{}, QPageLayout::Millimeter)) {
    error = "The full-scale plan page size is not supported by the PDF writer.";
    return false;
  }

  QPainter painter{&writer};
  if (!painter.isActive()) {
    error = QString{"Unable to create PDF file: %1"}.arg(path);
    return false;
  }
  const QRect target = writer.pageLayout().paintRectPixels(writer.resolution());
  scene_->render(&painter, QRectF{target}, document_.pageBoundsMm,
                 Qt::IgnoreAspectRatio);
  painter.end();
  if (!QFileInfo::exists(path) || QFileInfo{path}.size() == 0) {
    error = QString{"Unable to finish PDF file: %1"}.arg(path);
    return false;
  }
  return true;
}

void PlanViewport::resizeEvent(QResizeEvent* event) {
  // Scrollbar appearance also resizes the viewport; it must not cancel zoom.
  // Only a change in the outer view size requests a new width fit.
  const bool viewResized = lastViewSize_ != size();
  lastViewSize_ = size();
  QGraphicsView::resizeEvent(event);
  if (zoomAnchor_) applyZoomAnchor();
  else if (restoredView_) applyRestoredView();
  else if (viewResized) fitAll();
}

void PlanViewport::showEvent(QShowEvent* event) {
  QGraphicsView::showEvent(event);
  QTimer::singleShot(0, this, [this] {
    if(restoredView_) {applyRestoredView();restoredView_.reset();}
    else fitAll();
  });
}
PlanViewState PlanViewport::viewState() const {
  if(restoredView_&&!isVisible())return *restoredView_;
  return {transform().m11(),mapToScene(viewport()->rect().center())};
}
void PlanViewport::applyRestoredView() {
  if(!restoredView_) return;
  const auto saved=*restoredView_;
  setTransform(QTransform::fromScale(saved.zoom,saved.zoom));
  // Use bounds independent of scrollbar visibility. Using viewport()->size()
  // makes the scene alternate between fitting and overflowing as the bars
  // appear/disappear, endlessly queuing resize events after project restore.
  const QSizeF visible=QSizeF{maximumViewportSize()}/saved.zoom;
  setSceneRect(scene_->sceneRect().united(QRectF{saved.center-QPointF{visible.width()/2,visible.height()/2},visible}));
  centerOn(saved.center);
}
void PlanViewport::restoreView(const PlanViewState& state) {
  restoredView_=state;applyRestoredView();
}

void PlanViewport::applyZoomAnchor() {
  if(!zoomAnchor_ || anchoringZoom_)return;
  anchoringZoom_=true;
  const auto [point,cursor]=*zoomAnchor_;
  const double zoom=transform().m11();
  const QRectF visible{point-cursor/zoom,QSizeF{viewport()->size()}/zoom};
  // Permit the margin required to anchor a zoom-out near an image edge.
  // Keep the scene's content bounds unchanged so Fit View still fits the image.
  setSceneRect(scene_->sceneRect().united(visible));
  centerOn(point+(QPointF{viewport()->width()/2.0,viewport()->height()/2.0}-cursor)/zoom);
  anchoringZoom_=false;
}

void PlanViewport::wheelEvent(QWheelEvent* event) {
  restoredView_.reset();
  // Wheel/trackpad gestures zoom only. Scrolling is an explicit scrollbar action.
  const double delta = !event->angleDelta().isNull()
      ? event->angleDelta().y() : event->pixelDelta().y();
  const double factor = std::pow(1.0015, delta);
  const double targetScale = transform().m11() * factor;
  if (targetScale >= 0.00001 && targetScale <= 1000.0) {
    fittingWidth_ = true;
    const auto cursor=event->position();
    zoomAnchor_=std::pair{mapToScene(cursor.toPoint()),cursor};
    scale(factor, factor);
    applyZoomAnchor();
    // Scrollbar visibility can settle after the wheel event returns.
    QTimer::singleShot(0,this,[this]{applyZoomAnchor();zoomAnchor_.reset();});
    fittingWidth_ = false;
  }
  event->accept();
}

} // namespace designrc::gui


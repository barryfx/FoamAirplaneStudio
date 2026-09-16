#include "gui/ControlSurfaceEditor.h"
#include <QGraphicsView>
#include <algorithm>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QLineF>
#include <QPainterPath>
#include <QPainterPathStroker>
namespace designrc::gui {
ControlSurfaceEditor::ControlSurfaceEditor(QGraphicsView& view):QObject{&view},view_{view} {
  view.viewport()->installEventFilter(this); view.installEventFilter(this);
  view.setMouseTracking(true); view.viewport()->setMouseTracking(true);
}
void ControlSurfaceEditor::refresh(bool dataChanged) {
  view_.viewport()->update();
  emit drawingStateChanged();
  if(editing_) view_.viewport()->setCursor(state_.drawing>=0 ? Qt::CrossCursor : Qt::ArrowCursor);
  if(dataChanged && changed) changed();
}
void ControlSurfaceEditor::restore(const ControlSurfaceState& state) {state_=state;selected_=-1;hover_=state_.first.value_or(QPointF{});refresh();}
void ControlSurfaceEditor::setPanelCount(int count) {
  count=std::clamp(count,1,100);if(count==static_cast<int>(state_.panels.size()))return;
  cancel();state_.panels.resize(count);state_.panel=std::min(state_.panel,count-1);refresh();
}
void ControlSurfaceEditor::setPanel(int panel) {
  panel=std::clamp(panel,0,static_cast<int>(state_.panels.size())-1);if(panel==state_.panel)return;
  const bool draft=state_.first.has_value();state_.first.reset();state_.drawing=-1;selected_=-1;
  state_.panel=panel;refresh(draft);
}
void ControlSurfaceEditor::setEditing(bool enabled) {
  const bool leaving=editing_ && !enabled;
  if(!enabled)selected_=-1;
  editing_=enabled;
  if(leaving)finishEditing();else refresh();
}
void ControlSurfaceEditor::setEnabled(int index,bool enabled) {
  state_.panels[state_.panel].at(index).enabled=enabled;
  if(selected_==index)selected_=-1;
  if(enabled) begin(index);
  else if(state_.drawing==index) {state_.drawing=-1;state_.first.reset();}
  refresh(true);
}
void ControlSurfaceEditor::setHinge(int index,HingeCut hinge) {state_.panels[state_.panel].at(index).hinge=hinge;refresh(true);}
void ControlSurfaceEditor::begin(int index) {
  if(!state_.panels[state_.panel].at(index).enabled) return;
  selected_=-1;state_.drawing=index;state_.first.reset();
  if(drawingRequested) drawingRequested();
  view_.setFocus();refresh();
}
void ControlSurfaceEditor::cancel() {const bool dataChanged=state_.first.has_value();selected_=-1;state_.drawing=-1;state_.first.reset();refresh(dataChanged);}
void ControlSurfaceEditor::finishEditing() {
  bool dataChanged=state_.first.has_value();
  state_.drawing=-1;state_.first.reset();selected_=-1;
  for(auto& panel:state_.panels)for(auto& surface:panel) {
    if(surface.enabled && !surface.rectangle) {surface.enabled=false;dataChanged=true;}
  }
  refresh(dataChanged);
}
void ControlSurfaceEditor::commit(QPointF point) {
  const QRectF rectangle=QRectF{*state_.first,point}.normalized();
  if(rectangle.width()<1e-6 || rectangle.height()<1e-6) return;
  auto candidate=state_.panels;candidate[state_.panel].at(state_.drawing).rectangle=rectangle;
  if(controlSurfacesOverlap(candidate)) {
    state_.first.reset();refresh(true);emit overlapRejected();return;
  }
  state_.panels[state_.panel].at(state_.drawing).rectangle=rectangle;
  state_.first.reset();state_.drawing=-1;refresh(true);
}
bool ControlSurfaceEditor::eventFilter(QObject* object,QEvent* event) {
  if(!editing_) return false;
  if(event->type()==QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key()==Qt::Key_Escape) {
    cancel();return true;
  }
  if(state_.drawing<0 && event->type()==QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key()==Qt::Key_Delete) {
    if(selected_>=0) {state_.panels[state_.panel].at(selected_).rectangle.reset();selected_=-1;refresh(true);}
    return true;
  }
  if(object!=view_.viewport()) return false;
  if(state_.drawing<0) {
    if(event->type()==QEvent::MouseButtonPress) {
      auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
      selected_=-1;
      // Hit testing in viewport pixels keeps the edge tolerance stable at any zoom.
      for(int i=1;i>=0;--i) {
        const auto& surface=state_.panels[state_.panel][i];
        if(!surface.enabled || !surface.rectangle)continue;
        QPainterPath path;path.addPolygon(view_.mapFromScene(*surface.rectangle));path.closeSubpath();
        QPainterPathStroker stroke;stroke.setWidth(14);
        if(path.contains(mouse->position()) || stroke.createStroke(path).contains(mouse->position())) {selected_=i;break;}
      }
      view_.setFocus();refresh();return true;
    }
    return event->type()==QEvent::MouseButtonRelease && static_cast<QMouseEvent*>(event)->button()==Qt::LeftButton;
  }

  if(event->type()==QEvent::MouseMove) {
    hover_=view_.mapToScene(static_cast<QMouseEvent*>(event)->position().toPoint());refresh();return true;
  }
  if(event->type()==QEvent::MouseButtonPress) {
    auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
    pressed_=mouse->position();hover_=view_.mapToScene(pressed_.toPoint());
    if(state_.first) commit(hover_); else {state_.first=hover_;refresh(true);}
    return true;
  }
  if(event->type()==QEvent::MouseButtonRelease) {
    auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
    if(state_.first && QLineF{pressed_,mouse->position()}.length()>3)
      commit(view_.mapToScene(mouse->position().toPoint()));
    return true;
  }
  return false;
}
void ControlSurfaceEditor::paint(QPainter& painter) const {
  painter.save();
  for(int panel=0;panel<static_cast<int>(state_.panels.size());++panel)for(int i=0;i<2;++i) {
    if(!state_.panels[panel][i].enabled)continue;
    const QColor color=i==0?QColor{255,80,210}:QColor{255,185,35};
    auto draw=[&](QRectF rect,bool preview) {
      const bool selected=!preview && editing_ && panel==state_.panel && selected_==i;
      QPen border{Qt::black,selected?8.0:5.0};border.setCosmetic(true);painter.setPen(border);painter.setBrush(Qt::NoBrush);painter.drawRect(rect);
      QPen pen{selected?QColor{80,240,255}:color,selected?5.0:2.5,preview?Qt::DashLine:Qt::SolidLine};pen.setCosmetic(true);painter.setPen(pen);
      auto fill=selected?QColor{80,240,255}:color;fill.setAlpha(selected?70:25);painter.setBrush(fill);painter.drawRect(rect);
    };
    if(state_.panels[panel][i].rectangle)draw(*state_.panels[panel][i].rectangle,false);
    if(editing_ && panel==state_.panel && state_.drawing==i && state_.first)draw(QRectF{*state_.first,hover_}.normalized(),true);
  }
  painter.restore();
}
void ControlSurfaceEditor::mapPoints(const std::function<QPointF(QPointF)>& map) {
  for(auto& panel:state_.panels)for(auto& surface:panel)if(surface.rectangle)
    surface.rectangle=QRectF{map(surface.rectangle->topLeft()),map(surface.rectangle->bottomRight())}.normalized();
  if(state_.first)state_.first=map(*state_.first);
  refresh(true);
}
}

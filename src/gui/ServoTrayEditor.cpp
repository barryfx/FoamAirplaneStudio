#include "gui/ServoTrayEditor.h"
#include <QGraphicsView>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QLineF>
#include <array>
namespace designrc::gui {
namespace {std::array<QPointF,4> corners(QRectF r){return {r.topLeft(),r.topRight(),r.bottomRight(),r.bottomLeft()};}}
ServoTrayEditor::ServoTrayEditor(QGraphicsView& view):QObject{&view},view_{view} {
  view.viewport()->installEventFilter(this);view.installEventFilter(this);
  view.viewport()->setMouseTracking(true);
}
void ServoTrayEditor::refresh(bool data) {
  view_.viewport()->update();if(editing_)view_.viewport()->setCursor(state_.drawing?Qt::CrossCursor:Qt::ArrowCursor);
  emit controlsChanged();if(data)emit changed();
}
void ServoTrayEditor::restore(const ServoTrayState& state){state_=state;state_.first.reset();state_.drawing=false;selected_=false;drag_=-1;hover_=state.first.value_or(QPointF{});refresh();}
void ServoTrayEditor::setEditing(bool enabled) {
  if(editing_==enabled)return;
  editing_=enabled;drag_=-1;selected_=false;
  if(!enabled)cancel();else refresh();
}
void ServoTrayEditor::setDimensions(QSizeF size,QPointF center) {
  const auto previous=state_.rectangle;
  if(previous)center=previous->center();
  const QRectF candidate{center-QPointF{size.width()/2,size.height()/2},size};
  if(acceptRectangle&&!acceptRectangle(candidate)){emit message("Tray placement rejected: the servo tray cannot overlap a former.");refresh();return;}
  emit message({});state_.rectangle=candidate;
  state_.first.reset();state_.drawing=false;selected_=true;refresh(previous!=state_.rectangle);
}
void ServoTrayEditor::cancel(){const bool draft=state_.first.has_value();state_.first.reset();state_.drawing=false;drag_=-1;selected_=false;refresh(draft);}
void ServoTrayEditor::remove(){const bool had=state_.rectangle.has_value()||state_.first.has_value();state_={};selected_=false;drag_=-1;refresh(had);}
bool ServoTrayEditor::eventFilter(QObject* object,QEvent* event) {
  if(!editing_)return false;
  if(event->type()==QEvent::KeyPress) {
    const auto key=static_cast<QKeyEvent*>(event)->key();
    if(key==Qt::Key_Escape){cancel();return true;}
    if(key==Qt::Key_Delete&&selected_&&!state_.drawing){remove();return true;}
  }
  if(object!=view_.viewport())return false;
  if(event->type()==QEvent::MouseButtonPress) {
    auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
    view_.setFocus();pressed_=mouse->position();hover_=view_.mapToScene(pressed_.toPoint());
    selected_=false;drag_=-1;
    if(state_.rectangle) {
      const double tolerance=7/std::max(1e-9,view_.transform().m11());
      if(drag_<0&&state_.rectangle->adjusted(-tolerance,-tolerance,tolerance,tolerance).contains(hover_))drag_=4;
      if(drag_>=0){selected_=true;original_=*state_.rectangle;dragStart_=hover_;}
    }
    refresh();return true;
  }
  if(event->type()==QEvent::MouseMove) {
    auto* mouse=static_cast<QMouseEvent*>(event);hover_=view_.mapToScene(mouse->position().toPoint());
    if(state_.drawing){refresh();return true;}
    if(drag_>=0&&(mouse->buttons()&Qt::LeftButton)) {
      const auto candidate=original_.translated(hover_-dragStart_);
      if(acceptRectangle&&!acceptRectangle(candidate)){emit message("Tray movement rejected: the servo tray cannot overlap a former.");return true;}
      emit message({});
      if(candidate.width()>1e-6&&candidate.height()>1e-6&&candidate!=state_.rectangle){state_.rectangle=candidate;refresh(true);}return true;
    }
  }
  if(event->type()==QEvent::MouseButtonRelease) {
    auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
    drag_=-1;return true;
  }
  return false;
}
void ServoTrayEditor::paint(QPainter& painter) const {
  painter.save();
  auto draw=[&](QRectF rect,bool preview) {
    QPen border{Qt::black,5};border.setCosmetic(true);painter.setPen(border);painter.setBrush(Qt::NoBrush);painter.drawRect(rect);
    QPen pen{QColor{255,180,35},3,preview?Qt::DashLine:Qt::SolidLine};pen.setCosmetic(true);painter.setPen(pen);
    painter.setBrush(QColor{255,180,35,35});painter.drawRect(rect);

  };
  if(state_.rectangle)draw(*state_.rectangle,false);
  if(editing_&&state_.first)draw(QRectF{*state_.first,hover_}.normalized(),true);
  painter.restore();
}
void ServoTrayEditor::mapPoints(const std::function<QPointF(QPointF)>& map) {
  if(state_.rectangle)state_.rectangle=QRectF{map(state_.rectangle->topLeft()),map(state_.rectangle->bottomRight())}.normalized();
  if(state_.first)state_.first=map(*state_.first);drag_=-1;refresh(true);
}
}

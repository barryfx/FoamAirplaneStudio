#include "gui/FormerEditor.h"
#include <QGraphicsView>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>
namespace designrc::gui {
FormerEditor::FormerEditor(QGraphicsView& view):QObject{&view},view_{view}{view.viewport()->installEventFilter(this);view.installEventFilter(this);}
void FormerEditor::refresh(bool data){view_.viewport()->update();emit controlsChanged();if(data)emit changed();}
void FormerEditor::restore(const FormerState& state){state_=state;state_.rotationDegrees.resize(state_.rectangles.size(),0);selected_=drag_=-1;refresh();}
void FormerEditor::setEditing(bool enabled){if(editing_==enabled)return;editing_=enabled;drag_=-1;if(!enabled)selected_=-1;refresh();}
void FormerEditor::preserveThicknessAtScale(double scale) {
  if(!std::isfinite(scale)||scale<=0||scale==scale_)return;
  // Widths are stored in drawing coordinates. Preserve each physical width,
  // including mixed plywood sizes, without moving its center or height.
  for(auto& r:state_.rectangles) {
    const auto center=r.center();r.setWidth(r.width()*scale_/scale);r.moveCenter(center);
  }
  scale_=scale;drag_=-1;refresh(true);
}
bool FormerEditor::allowsTray(const QRectF& r) const {for(std::size_t i=0;i<state_.rectangles.size();++i)if(formerMasksOverlap(r,0,state_.rectangles[i],formerAngle(state_.rotationDegrees,i)))return false;return true;}
bool FormerEditor::allowed(const QRectF& r,int ignore,double angle) const {
  if(!std::isfinite(r.x())||!std::isfinite(r.y())||!std::isfinite(r.width())||!std::isfinite(r.height())||r.width()<=0||r.height()<=0)return false;
  for(int i=0;i<static_cast<int>(state_.rectangles.size());++i)if(i!=ignore&&formerMasksOverlap(r,angle,state_.rectangles[i],formerAngle(state_.rotationDegrees,i)))return false;
  if(tray)if(const auto t=tray();t&&formerMasksOverlap(r,angle,*t))return false;
  return true;
}
void FormerEditor::setThickness(double mm){
  if(!std::isfinite(mm)||mm<=0||mm>10000){emit message("Former thickness must be greater than zero (maximum 10000 mm).");return;}
  if(selected_>=0){auto r=state_.rectangles[selected_];const auto center=r.center();r.setWidth(mm/scale_);r.moveCenter(center);
    if(!allowed(r,selected_,formerAngle(state_.rotationDegrees,selected_))){emit message("Thickness rejected: formers cannot overlap each other or the servo tray.");refresh();return;}
    state_.rectangles[selected_]=r;
  }
  state_.thicknessMm=mm;emit message({});refresh(true);
}
void FormerEditor::setRotation(double degrees){
  if(selected_<0)return;
  if(!std::isfinite(degrees)||std::abs(degrees)>360){emit message("Rotation Angle must be between -360 and 360 degrees.");refresh();return;}
  if(!allowed(state_.rectangles[selected_],selected_,degrees)){emit message("Rotation rejected: formers cannot overlap each other or the servo tray.");refresh();return;}
  state_.rotationDegrees[selected_]=degrees;emit message({});refresh(true);
}
void FormerEditor::add(){
  const double w=state_.thicknessMm/scale_,margin=std::max(5./scale_,side_.height()*.05);
  if(side_.width()<=w||state_.rectangles.size()>=1000){emit message("No room for another former at this thickness.");return;}
  std::vector<double> candidates{side_.center().x()-w/2,side_.left(),side_.right()-w};
  for(std::size_t i=0;i<state_.rectangles.size();++i){const auto r=formerPolygon(state_.rectangles[i],formerAngle(state_.rotationDegrees,i)).boundingRect();candidates.push_back(r.right());candidates.push_back(r.left()-w);}
  if(tray)if(const auto t=tray()){candidates.push_back(t->right());candidates.push_back(t->left()-w);}
  std::sort(candidates.begin(),candidates.end(),[&](double a,double b){return std::abs(a+w/2-side_.center().x())<std::abs(b+w/2-side_.center().x());});
  for(double x:candidates){QRectF r{x,side_.top()-margin,w,side_.height()+2*margin};
    if(x>=side_.left()-1e-7&&r.right()<=side_.right()+1e-7&&allowed(r)){
      state_.rectangles.push_back(r);state_.rotationDegrees.push_back(0);selected_=static_cast<int>(state_.rectangles.size())-1;emit message({});refresh(true);view_.setFocus();return;
    }
  }
  emit message("No non-overlapping position is available. Move/delete a former or reduce its thickness.");
}
void FormerEditor::remove(){if(selected_<0)return;state_.rectangles.erase(state_.rectangles.begin()+selected_);state_.rotationDegrees.erase(state_.rotationDegrees.begin()+selected_);selected_=drag_=-1;emit message({});refresh(true);}
bool FormerEditor::eventFilter(QObject* object,QEvent* event){
  if(!editing_)return false;
  if(event->type()==QEvent::KeyPress){const int key=static_cast<QKeyEvent*>(event)->key();if(key==Qt::Key_Delete){remove();return true;}if(key==Qt::Key_Escape){selected_=drag_=-1;refresh();return true;}}
  if(object!=view_.viewport())return false;
  const double tolerance=7/std::max(1e-9,view_.transform().m11());
  if(event->type()==QEvent::MouseButtonPress){auto* m=static_cast<QMouseEvent*>(event);if(m->button()!=Qt::LeftButton)return false;
    start_=view_.mapToScene(m->position().toPoint());selected_=drag_=-1;
    for(int i=static_cast<int>(state_.rectangles.size())-1;i>=0;--i){const auto r=state_.rectangles[i];const auto local=formerTransform(r,formerAngle(state_.rotationDegrees,i)).inverted().map(start_);if(!r.adjusted(-tolerance,-tolerance,tolerance,tolerance).contains(local))continue;
      selected_=i;original_=r;drag_=std::abs(local.y()-r.top())<=tolerance?1:std::abs(local.y()-r.bottom())<=tolerance?2:0;break;
    }
    view_.setFocus();emit message({});refresh();return true;
  }
  if(event->type()==QEvent::MouseMove&&drag_>=0){auto* m=static_cast<QMouseEvent*>(event);if(!(m->buttons()&Qt::LeftButton))return false;
    const auto p=view_.mapToScene(m->position().toPoint());auto r=original_;
    if(drag_==0)r.translate(p-start_);else {
      const auto t=formerTransform(original_,formerAngle(state_.rotationDegrees,selected_));const auto local=t.inverted().map(p);
      if(drag_==1)r.setTop(local.y());else r.setBottom(local.y());
      r.moveCenter(t.map(r.center()));
    }
    if(allowed(r,selected_,formerAngle(state_.rotationDegrees,selected_))){state_.rectangles[selected_]=r;emit message({});refresh(true);}else emit message("Placement rejected: keep a positive height and avoid other formers and the servo tray.");return true;
  }
  if(event->type()==QEvent::MouseButtonRelease){auto* m=static_cast<QMouseEvent*>(event);if(m->button()==Qt::LeftButton){drag_=-1;return true;}}
  return false;
}
void FormerEditor::paint(QPainter& p) const {
  p.save();for(int i=0;i<static_cast<int>(state_.rectangles.size());++i){const auto r=state_.rectangles[i];QPen pen{QColor{80,230,120},editing_&&i==selected_?4.:2.};pen.setCosmetic(true);p.setPen(pen);p.setBrush(QColor{80,230,120,45});p.drawPolygon(formerPolygon(r,formerAngle(state_.rotationDegrees,i)));
    if(editing_&&i==selected_){const double h=4/std::max(1e-9,view_.transform().m11());p.setBrush(Qt::white);for(double y:{r.top(),r.bottom()}){const auto at=formerTransform(r,formerAngle(state_.rotationDegrees,i)).map(QPointF{r.center().x(),y});p.drawRect(QRectF{at.x()-h,at.y()-h,2*h,2*h});}}
  }p.restore();
}
void FormerEditor::mapPoints(const std::function<QPointF(QPointF)>& map){for(auto& r:state_.rectangles)r=QRectF{map(r.topLeft()),map(r.bottomRight())}.normalized();drag_=-1;refresh(true);}
}

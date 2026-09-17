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
void FormerEditor::restore(const FormerState& state){state_=state;selected_=drag_=-1;refresh();}
void FormerEditor::setEditing(bool enabled){if(editing_==enabled)return;editing_=enabled;drag_=-1;if(!enabled)selected_=-1;refresh();}
bool FormerEditor::allowsTray(const QRectF& r) const {for(const auto& other:state_.rectangles)if(rectanglesOverlap(r,other))return false;return true;}
bool FormerEditor::allowed(const QRectF& r,int ignore) const {
  if(!std::isfinite(r.x())||!std::isfinite(r.y())||!std::isfinite(r.width())||!std::isfinite(r.height())||r.width()<=0||r.height()<=0)return false;
  for(int i=0;i<static_cast<int>(state_.rectangles.size());++i)if(i!=ignore&&rectanglesOverlap(r,state_.rectangles[i]))return false;
  if(tray)if(const auto t=tray();t&&rectanglesOverlap(r,*t))return false;
  return true;
}
void FormerEditor::setThickness(double mm){
  if(!std::isfinite(mm)||mm<=0||mm>10000){emit message("Former thickness must be greater than zero (maximum 10000 mm).");return;}
  if(selected_>=0){auto r=state_.rectangles[selected_];const auto center=r.center();r.setWidth(mm/scale_);r.moveCenter(center);
    if(!allowed(r,selected_)){emit message("Thickness rejected: formers cannot overlap each other or the servo tray.");refresh();return;}
    state_.rectangles[selected_]=r;
  }
  state_.thicknessMm=mm;emit message({});refresh(true);
}
void FormerEditor::add(){
  const double w=state_.thicknessMm/scale_,margin=std::max(5./scale_,side_.height()*.05);
  if(side_.width()<=w||state_.rectangles.size()>=1000){emit message("No room for another former at this thickness.");return;}
  std::vector<double> candidates{side_.center().x()-w/2,side_.left(),side_.right()-w};
  for(const auto& r:state_.rectangles){candidates.push_back(r.right());candidates.push_back(r.left()-w);}
  if(tray)if(const auto t=tray()){candidates.push_back(t->right());candidates.push_back(t->left()-w);}
  std::sort(candidates.begin(),candidates.end(),[&](double a,double b){return std::abs(a+w/2-side_.center().x())<std::abs(b+w/2-side_.center().x());});
  for(double x:candidates){QRectF r{x,side_.top()-margin,w,side_.height()+2*margin};
    if(x>=side_.left()-1e-7&&r.right()<=side_.right()+1e-7&&allowed(r)){
      state_.rectangles.push_back(r);selected_=static_cast<int>(state_.rectangles.size())-1;emit message({});refresh(true);view_.setFocus();return;
    }
  }
  emit message("No non-overlapping position is available. Move/delete a former or reduce its thickness.");
}
void FormerEditor::remove(){if(selected_<0)return;state_.rectangles.erase(state_.rectangles.begin()+selected_);selected_=drag_=-1;emit message({});refresh(true);}
bool FormerEditor::eventFilter(QObject* object,QEvent* event){
  if(!editing_)return false;
  if(event->type()==QEvent::KeyPress){const int key=static_cast<QKeyEvent*>(event)->key();if(key==Qt::Key_Delete){remove();return true;}if(key==Qt::Key_Escape){selected_=drag_=-1;refresh();return true;}}
  if(object!=view_.viewport())return false;
  const double tolerance=7/std::max(1e-9,view_.transform().m11());
  if(event->type()==QEvent::MouseButtonPress){auto* m=static_cast<QMouseEvent*>(event);if(m->button()!=Qt::LeftButton)return false;
    start_=view_.mapToScene(m->position().toPoint());selected_=drag_=-1;
    for(int i=static_cast<int>(state_.rectangles.size())-1;i>=0;--i){const auto r=state_.rectangles[i];if(!r.adjusted(-tolerance,-tolerance,tolerance,tolerance).contains(start_))continue;
      selected_=i;original_=r;drag_=std::abs(start_.y()-r.top())<=tolerance?1:std::abs(start_.y()-r.bottom())<=tolerance?2:0;break;
    }
    view_.setFocus();emit message({});refresh();return true;
  }
  if(event->type()==QEvent::MouseMove&&drag_>=0){auto* m=static_cast<QMouseEvent*>(event);if(!(m->buttons()&Qt::LeftButton))return false;
    const auto p=view_.mapToScene(m->position().toPoint());auto r=original_;
    if(drag_==0)r.translate(p-start_);else if(drag_==1)r.setTop(p.y());else r.setBottom(p.y());
    if(allowed(r,selected_)){state_.rectangles[selected_]=r;emit message({});refresh(true);}else emit message("Placement rejected: keep a positive height and avoid other formers and the servo tray.");return true;
  }
  if(event->type()==QEvent::MouseButtonRelease){auto* m=static_cast<QMouseEvent*>(event);if(m->button()==Qt::LeftButton){drag_=-1;return true;}}
  return false;
}
void FormerEditor::paint(QPainter& p) const {
  p.save();for(int i=0;i<static_cast<int>(state_.rectangles.size());++i){const auto r=state_.rectangles[i];QPen pen{QColor{80,230,120},editing_&&i==selected_?4.:2.};pen.setCosmetic(true);p.setPen(pen);p.setBrush(QColor{80,230,120,45});p.drawRect(r);
    if(editing_&&i==selected_){const double h=4/std::max(1e-9,view_.transform().m11());p.setBrush(Qt::white);for(double y:{r.top(),r.bottom()})p.drawRect(QRectF{r.center().x()-h,y-h,2*h,2*h});}
  }p.restore();
}
void FormerEditor::mapPoints(const std::function<QPointF(QPointF)>& map){for(auto& r:state_.rectangles)r=QRectF{map(r.topLeft()),map(r.bottomRight())}.normalized();drag_=-1;refresh(true);}
}

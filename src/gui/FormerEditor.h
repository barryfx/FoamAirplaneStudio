#pragma once
#include <QObject>
#include <QRectF>
#include <functional>
#include <optional>
#include <vector>
class QGraphicsView;
class QPainter;
namespace designrc::gui {
struct FormerState { std::vector<QRectF> rectangles; double thicknessMm=3; };
inline bool rectanglesOverlap(const QRectF& a,const QRectF& b) {
  const auto i=a.intersected(b);return i.width()>1e-7&&i.height()>1e-7;
}
class FormerEditor final : public QObject {
  Q_OBJECT
public:
  explicit FormerEditor(QGraphicsView&);
  const FormerState& state() const{return state_;}
  int selected() const{return selected_;}
  void restore(const FormerState&);
  void setEditing(bool);
  void configure(double scale,QRectF side){scale_=scale;side_=side;}
  void setThickness(double mm);
  void add();
  void remove();
  bool allowsTray(const QRectF&) const;
  void paint(QPainter&) const;
  void mapPoints(const std::function<QPointF(QPointF)>&);
  std::function<std::optional<QRectF>()> tray;
signals:
  void changed();
  void controlsChanged();
  void message(const QString&);
protected:
  bool eventFilter(QObject*,QEvent*) override;
private:
  bool allowed(const QRectF&,int ignore=-1) const;
  void refresh(bool data=false);
  QGraphicsView& view_;FormerState state_;QRectF side_,original_;
  double scale_=1;bool editing_=false;int selected_=-1,drag_=-1;
  QPointF start_;
};
}

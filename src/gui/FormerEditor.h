#pragma once
#include "gui/LengthUnit.h"
#include <QObject>
#include <QRectF>
#include <functional>
#include <optional>
#include <vector>
#include "gui/FormerPlacement.h"
class QGraphicsView;
class QPainter;
namespace designrc::gui {
struct FormerState { std::vector<QRectF> rectangles; double thicknessMm=3; std::vector<double> rotationDegrees; LengthUnit thicknessUnit=LengthUnit::Default; std::vector<LengthUnit> thicknessUnits; };
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
  void configure(double scale,QRectF side){if(scale>0)scale_=scale;side_=side;}
  void preserveThicknessAtScale(double scale);
  void setThickness(double mm,std::optional<LengthUnit> unit={});
  void setRotation(double degrees);
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
  bool allowed(const QRectF&,int ignore=-1,double angle=0) const;
  void refresh(bool data=false);
  QGraphicsView& view_;FormerState state_;QRectF side_,original_;
  double scale_=1;bool editing_=false;int selected_=-1,drag_=-1;
  QPointF start_;
};
}

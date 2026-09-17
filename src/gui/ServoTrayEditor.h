#pragma once
#include <QObject>
#include <QRectF>
#include <optional>
#include <functional>
class QGraphicsView;
class QPainter;
namespace designrc::gui {
struct ServoTrayState {
  std::optional<QRectF> rectangle;
  std::optional<QPointF> first;
  bool drawing{};
};
class ServoTrayEditor final : public QObject {
  Q_OBJECT
public:
  explicit ServoTrayEditor(QGraphicsView& view);
  const ServoTrayState& state() const{return state_;}
  void restore(const ServoTrayState& state);
  void setEditing(bool enabled);
  void setDimensions(QSizeF size,QPointF center);
  std::function<bool(const QRectF&)> acceptRectangle;
  void cancel();
  void remove();
  void paint(QPainter& painter) const;
  void mapPoints(const std::function<QPointF(QPointF)>& map);
signals:
  void changed();
  void controlsChanged();
  void message(const QString&);
protected:
  bool eventFilter(QObject*,QEvent*) override;
private:
  void refresh(bool changed=false);
  QGraphicsView& view_;
  ServoTrayState state_;
  bool editing_{},selected_{};
  int drag_=-1; // 4 moves the fixed-size rectangle.
  QPointF hover_,pressed_,dragStart_;
  QRectF original_;
};
}

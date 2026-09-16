#pragma once
#include "gui/ControlSurfaceState.h"
#include <QObject>
#include <functional>
class QGraphicsView;
class QPainter;
namespace designrc::gui {
class ControlSurfaceEditor final : public QObject {
  Q_OBJECT
public:
  explicit ControlSurfaceEditor(QGraphicsView& view);
  const ControlSurfaceState& state() const { return state_; }
  int selectedRectangle() const { return selected_; }
  void restore(const ControlSurfaceState& state);
  void setEditing(bool enabled);
  void setPanelCount(int count);
  void setPanel(int panel);
  int panel() const { return state_.panel; }
  void setEnabled(int index, bool enabled);
  void setHinge(int index, HingeCut hinge);
  void begin(int index);
  void cancel();
  void finishEditing();
  void paint(QPainter& painter) const;
  void mapPoints(const std::function<QPointF(QPointF)>& map);
  std::function<void()> changed, drawingRequested;
signals:
  void drawingStateChanged();
  void overlapRejected();
protected:
  bool eventFilter(QObject*,QEvent*) override;
private:
  QGraphicsView& view_;
  ControlSurfaceState state_;
  bool editing_ = false;
  int selected_ = -1;
  QPointF hover_, pressed_;
  void refresh(bool dataChanged=false);
  void commit(QPointF point);
};
}

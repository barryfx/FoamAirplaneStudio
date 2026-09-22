#pragma once
#include "gui/SketchEditor.h"
#include <QWidget>
class QGraphicsView;
class QLabel;
class QPushButton;
namespace designrc::gui {
class FuselageProfilePanel final : public QWidget {
public:
  FuselageProfilePanel(QGraphicsView& view, SketchEditor& source, SketchEditor& profiles, QWidget* parent);
  void setActive(bool active);
  void selectStation();
  void restoreControls();
  bool allProfilesClosed() const;
protected:
  bool eventFilter(QObject* watched,QEvent* event) override;
private:
  void chooseTool(SketchTool tool,bool checked);
  enum class Action { None, Copy, Move };
  void setAction(Action action);
  void syncActions();
  void pasteProfile();
  int profileAt(QPointF point) const;
  void dragProfile(QPointF point);
  void includeProfileBounds();
  QGraphicsView& view_;
  SketchEditor &source_, &profiles_;
  QWidget* tools_{};
  QLabel* selection_{};
  QPushButton* remove_{};
  QPushButton *copy_{},*paste_{},*move_{};
  std::optional<SketchLayer> clipboard_;
  Action action_=Action::None;
  int dragged_=-1;
  int selectedOrphan_=-1;
  QPointF dragStart_;
  std::vector<QPointF> dragPoints_;
  bool active_{};
};
}

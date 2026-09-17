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
  QGraphicsView& view_;
  SketchEditor &source_, &profiles_;
  QWidget* tools_{};
  QLabel* selection_{};
  QPushButton* remove_{};
  bool active_{};
};
}

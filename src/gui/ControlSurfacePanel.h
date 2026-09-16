#pragma once
#include <QWidget>
#include <array>
class QCheckBox;
class QRadioButton;
class QPushButton;
class QTabBar;
namespace designrc::gui {
class ControlSurfaceEditor;
class ControlSurfacePanel final : public QWidget {
public:
  ControlSurfacePanel(ControlSurfaceEditor& editor,QWidget* parent=nullptr);
  void restoreControls();
private:
  void updateDrawingButtons();
  std::array<QPushButton*,2> draw_{};
  QTabBar* tabs_{};
  ControlSurfaceEditor& editor_;
  std::array<QCheckBox*,2> checks_{};
  std::array<QWidget*,2> details_{};
  std::array<std::array<QRadioButton*,2>,2> hinges_{};
};
}

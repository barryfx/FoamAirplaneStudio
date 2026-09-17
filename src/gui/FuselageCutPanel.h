#pragma once
#include "gui/SketchEditor.h"
#include <QWidget>
#include <array>
class QPushButton;
namespace designrc::gui {
class FuselageCutPanel final : public QWidget {
public:
  FuselageCutPanel(SketchEditor& editor,QWidget* parent=nullptr);
  void setActive(bool active);
  void restoreControls();
private:
  void sync();
  SketchEditor& editor_;
  bool active_{};
  std::array<QPushButton*,2> views_{},tools_{};
};
}

#pragma once
#include "gui/SketchEditor.h"
#include <QWidget>
#include <vector>
class QPushButton;
class QComboBox;
namespace designrc::gui {
class FuselageCutPanel final : public QWidget {
public:
  FuselageCutPanel(SketchEditor& editor,QWidget* parent=nullptr,bool holes=false,SketchEditor* outlines=nullptr);
  void setActive(bool active,bool warn=true);
  void restoreControls();
private:
  void sync();
  SketchEditor& editor_;
  SketchEditor* outlines_{};
  bool active_{},holes_{},syncing_{};
  int selectedPath_=-1;
  SketchState accepted_;
  QComboBox* paths_{};
  QPushButton* remove_{};
  std::vector<QPushButton*> views_,tools_;
};
}

#pragma once
#include "gui/SketchEditor.h"
#include <QWidget>
class QComboBox;
namespace designrc::gui {
class StabilizerCutPanel final : public QWidget {
public:
  StabilizerCutPanel(SketchEditor& editor,bool horizontal,QWidget* parent=nullptr);
  void setActive(bool active,bool warn=true);
  void restoreControls();
private:
  SketchEditor& editor_;
  QComboBox* shapes_{};
  bool active_=false;
};
}

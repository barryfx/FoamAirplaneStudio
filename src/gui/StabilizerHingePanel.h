#pragma once
#include "gui/SketchEditor.h"
#include "gui/ControlSurfaceState.h"
#include <QWidget>
#include <functional>
class QRadioButton;
namespace designrc::gui {
class StabilizerHingePanel final : public QWidget {
public:
  StabilizerHingePanel(SketchEditor& editor, bool horizontal, QWidget* parent=nullptr);
  void setActive(bool active);
  void restore(HingeCut cut);
  HingeCut cut() const { return cut_; }
  std::function<void()> changed;
private:
  SketchEditor& editor_;
  HingeCut cut_=HingeCut::Tape;
  QRadioButton* tape_{};
  QRadioButton* standard_{};
};
}

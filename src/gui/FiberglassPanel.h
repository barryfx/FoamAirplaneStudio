#pragma once
#include "gui/FiberglassState.h"
#include "gui/ReferenceImage.h"
#include <QWidget>
#include <functional>
class QComboBox;
class QLineEdit;
class QRadioButton;
class QCheckBox;
class QDoubleSpinBox;
namespace designrc::gui {
class FiberglassPanel final : public QWidget {
public:
  FiberglassPanel(SketchEditor& editor,int component,QWidget* parent=nullptr);
  FiberglassState state() const;
  void restore(const FiberglassState& state);
  void configure(ProjectUnits units);
  void setActive(bool active);
  std::function<void()> changed;
private:
  void refresh();
  void edit();
  SketchEditor& editor_;
  int component_;
  bool refreshing_=false;
  ProjectUnits units_=ProjectUnits::Millimeters;
  std::vector<FiberglassPatch> patches_{1};
  QComboBox *list_{},*side_{},*clothUnits_{};
  QLineEdit* name_{};
  QRadioButton *wrap_{},*oneSide_{};
  QCheckBox* automatic_{};
  QDoubleSpinBox *cloth_{},*resin_{};
};
}

#pragma once
#include "gui/StiffenerState.h"
#include "gui/ReferenceImage.h"
#include <QWidget>
#include <functional>
class QComboBox;class QSpinBox;class QDoubleSpinBox;class QLineEdit;class QFormLayout;
namespace designrc::gui {
class StiffenerPanel final : public QWidget {
public:
  explicit StiffenerPanel(QWidget* parent=nullptr);
  const StiffenerState& state() const {return state_;}
  void restore(const StiffenerState& state);
  void setUnits(ProjectUnits units);
  std::function<void()> changed;
private:
  void refresh();
  bool updating_=false;
  StiffenerState state_;
  ProjectUnits widthUnits_=ProjectUnits::Millimeters, heightUnits_=ProjectUnits::Millimeters, diameterUnits_=ProjectUnits::Millimeters;
  QFormLayout* form_{};
  QSpinBox* count_{};QComboBox* shape_{};
  QDoubleSpinBox *start_{},*stop_{};
  QLineEdit *width_{},*height_{},*diameter_{};
};
}

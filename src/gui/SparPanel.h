#pragma once
#include "gui/SparState.h"
#include "gui/ReferenceImage.h"
#include <QWidget>
#include <functional>
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QTabBar;
namespace designrc::gui {
class SparPanel final : public QWidget {
public:
  explicit SparPanel(QWidget* parent=nullptr);
  const PanelSpars& state() const {return state_;}
  void restore(const PanelSpars& state,ProjectUnits units,int selected=0);
  void setPanelCount(int count);
  int selectedPanel() const;
  void setUnits(ProjectUnits units);
  std::function<void()> changed;
private:
  void updateControls();
  PanelSpars state_{1};
  QTabBar* tabs_{};
  ProjectUnits units_=ProjectUnits::Millimeters;
  std::array<QCheckBox*,3> checks_{};
  std::array<QWidget*,3> details_{};
  std::array<QComboBox*,3> shapes_{};
  std::array<std::array<QDoubleSpinBox*,2>,3> values_{};
  std::array<std::array<QLineEdit*,2>,3> lengths_{};
  std::array<QLabel*,3> sizeLabels_{},heightLabels_{};
};
}

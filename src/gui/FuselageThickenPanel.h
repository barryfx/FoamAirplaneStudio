#pragma once
#include "gui/SketchEditor.h"
#include "gui/ReferenceImage.h"
#include <QWidget>
#include <functional>
class QFormLayout;
class QLabel;
namespace designrc::gui {
class FuselageThickenPanel final : public QWidget {
public:
  FuselageThickenPanel(SketchEditor& source,QWidget* parent);
  void enter(double wingLeadingEdge);
  void synchronize(double wingLeadingEdge);
  void restore(bool enabled);
  void setUnits(ProjectUnits units);
  bool enabled() const { return enabled_; }
  std::function<void()> changed;
private:
  void rebuild();
  SketchEditor& source_;
  QFormLayout* fields_{};
  QLabel* defaults_{};
  bool enabled_{},updating_{};
  ProjectUnits units_{ProjectUnits::Millimeters};
};
}

#pragma once
#include "gui/WeightBalanceState.h"
#include "gui/ReferenceImage.h"
#include "geometry/FuselageSolidBuilder.h"
#include <QWidget>
#include <functional>
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QLabel;
class QPainter;
namespace designrc::gui {
class PlanViewport;
class WeightBalancePanel final : public QWidget {
public:
  WeightBalancePanel(PlanViewport& view,QWidget* parent=nullptr);
  const WeightBalanceState& state() const { return state_; }
  void restore(const WeightBalanceState& state);
  void configure(ProjectUnits units,geometry::FuselageSideTransform transform,QPointF initialCenter);
  void setActive(bool active);
  void setFoam(std::optional<FoamMassProperties> foam,std::optional<double> leadingEdge,QString message);
  void paint(QPainter& painter) const;
  QRectF partRectangle(int index) const;
  int selected() const;
  std::function<void()> changed;
  std::function<void(const QString&)> resultsChanged;
protected:
  bool eventFilter(QObject* watched,QEvent* event) override;
private:
  void editPart(bool add);
  void deletePart();
  void refreshList(int selected);
  void updateResults();
  QPointF sceneToModel(QPointF point) const;
  PlanViewport& view_;
  WeightBalanceState state_;
  geometry::FuselageSideTransform transform_{0,0,1};
  ProjectUnits units_=ProjectUnits::Millimeters;
  QPointF initialCenter_,dragOffset_;
  bool active_=false,dragging_=false;
  QComboBox* parts_{};
  QDoubleSpinBox* density_{};
  QDoubleSpinBox* plywoodDensity_{};
  QPushButton *edit_{},*delete_{};
  QLabel* results_{};
  std::optional<FoamMassProperties> foam_;
  std::optional<double> leadingEdge_;
  QString unavailable_;
};
}

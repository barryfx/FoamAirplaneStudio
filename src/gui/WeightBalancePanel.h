#pragma once
#include "gui/WeightBalanceState.h"
#include "gui/ReferenceImage.h"
#include "geometry/FuselageSolidBuilder.h"
#include <QWidget>
#include <QPixmap>
#include <QLineF>
#include <functional>
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QLabel;
class QPainter;
class QTableWidget;
namespace designrc::gui {
class PlanViewport;
class WeightBalancePanel final : public QWidget {
public:
  WeightBalancePanel(PlanViewport& view,QWidget* parent=nullptr);
  const WeightBalanceState& state() const { return state_; }
  void restore(const WeightBalanceState& state);
  void configure(ProjectUnits units,geometry::FuselageSideTransform transform,QPointF initialCenter);
  void setActive(bool active);
  void setWingArea(std::optional<double> mm2);
  void setCgHeightLine(std::optional<QLineF> line);
  std::optional<QPointF> cgScenePosition() const;
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
  QDoubleSpinBox* carbonFiberDensity_{};
  QPushButton *edit_{},*delete_{};
  QLabel* results_{};
  QTableWidget* breakdown_{};
  std::optional<FoamMassProperties> foam_;
  std::optional<double> leadingEdge_;
  std::optional<double> wingAreaMm2_;
  std::optional<QLineF> cgHeightLine_;
  QPixmap cgSymbol_{":/graphics/cg-symbol.png"};
  QString unavailable_;
};
}

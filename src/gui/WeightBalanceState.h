#pragma once
#include <QPointF>
#include <QString>
#include <vector>

namespace designrc::gui {
inline constexpr double gramsPerOunce = 28.349523125;
struct BalancePart {
  QString name;
  double widthMm=20, heightMm=20, lengthMm=40, grams=10;
  QPointF centerMm; // Fuselage X/Z; Y=0. Uniform part mass acts at this center.
  bool ounces=false;
};
struct WeightBalanceState {
  double densityKgM3=32.5; // Representative XPS; editable for the actual stock.
  double plywoodDensityKgM3=680; // Birch aircraft plywood, including tray/formers.
  std::vector<BalancePart> parts;
};
struct FoamMassProperties {
  double volumeMm3=0;
  QPointF centroidMm;
  double plywoodVolumeMm3=0;
  QPointF plywoodCentroidMm;
};
struct BalanceResult { double grams=0; QPointF centerMm; };
inline BalanceResult calculateBalance(const WeightBalanceState& state,const FoamMassProperties& foam) {
  BalanceResult result;
  result.grams=foam.volumeMm3*state.densityKgM3*1e-6;
  QPointF moment=foam.centroidMm*result.grams;
  const double plywoodGrams=foam.plywoodVolumeMm3*state.plywoodDensityKgM3*1e-6;
  result.grams+=plywoodGrams;moment+=foam.plywoodCentroidMm*plywoodGrams;
  for(const auto& part:state.parts) { result.grams+=part.grams;moment+=part.centerMm*part.grams; }
  if(result.grams>0)result.centerMm=moment/result.grams;
  return result;
}
}

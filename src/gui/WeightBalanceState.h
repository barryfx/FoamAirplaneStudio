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
  double densityKgM3=25.63; // User-selected foam default; editable for the actual stock.
  double plywoodDensityKgM3=680; // Birch aircraft plywood, including tray/formers.
  std::vector<BalancePart> parts;
  double carbonFiberDensityKgM3=1540; // Carbon/epoxy composite material, excluding the tube bore.
  double resinDensityKgM3=1180; // Solvent-free cured laminating epoxy starting value.
};
struct ComponentVolume {
  QString name;
  double volumeMm3=0;
  bool plywood=false;
  bool carbonFiber=false;
};
struct FoamMassProperties {
  double volumeMm3=0;
  QPointF centroidMm;
  double plywoodVolumeMm3=0;
  QPointF plywoodCentroidMm;
  std::vector<ComponentVolume> components;
  double carbonFiberVolumeMm3=0;
  QPointF carbonFiberCentroidMm;
  struct Covering {
    QString name;
    double areaMm2=0,clothGrams=0,resinVolumeMm3=0;
    QPointF centroidMm;
  };
  std::vector<Covering> fiberglass;
};
struct BalanceResult { double grams=0; QPointF centerMm; };
inline BalanceResult calculateBalance(const WeightBalanceState& state,const FoamMassProperties& foam) {
  BalanceResult result;
  result.grams=foam.volumeMm3*state.densityKgM3*1e-6;
  QPointF moment=foam.centroidMm*result.grams;
  const double plywoodGrams=foam.plywoodVolumeMm3*state.plywoodDensityKgM3*1e-6;
  result.grams+=plywoodGrams;moment+=foam.plywoodCentroidMm*plywoodGrams;
  const double carbonGrams=foam.carbonFiberVolumeMm3*state.carbonFiberDensityKgM3*1e-6;
  result.grams+=carbonGrams;moment+=foam.carbonFiberCentroidMm*carbonGrams;
  for(const auto& part:state.parts) { result.grams+=part.grams;moment+=part.centerMm*part.grams; }
  for(const auto& patch:foam.fiberglass) {
    const double grams=patch.clothGrams+patch.resinVolumeMm3*state.resinDensityKgM3*1e-6;
    result.grams+=grams;moment+=patch.centroidMm*grams;
  }
  if(result.grams>0)result.centerMm=moment/result.grams;
  return result;
}
}

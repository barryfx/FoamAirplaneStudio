#pragma once
#include "gui/WeightBalanceState.h"
#include "gui/ReferenceImage.h"
#include <QByteArray>
#include <optional>
namespace designrc::gui {
struct ProjectDocument;
struct StatisticsBalance {
  FoamMassProperties materials;
  double leadingEdgeMm=0;
  QByteArray sourceKey;
};
struct AirplaneStatistics {
  std::optional<double> wingspanMm,wingAreaMm2,rootChordMm,aspectRatio,fuselageLengthMm;
  std::optional<double> horizontalAreaMm2,verticalAreaMm2,weightGrams,cgFromLeadingEdgeMm,wingLoadingGramsPerDm2;
  std::optional<StatisticsBalance> balance;
};
// Sketch-only planform measurements; never builds wing/fuselage solids.
AirplaneStatistics outlineStatistics(const ProjectDocument& project);
QString statisticsText(const AirplaneStatistics& statistics,ProjectUnits units);
}

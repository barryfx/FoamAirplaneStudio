#include "gui/WingCalibration.h"
#include "geometry/WeightBalance.h"
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace designrc::geometry {
gui::FoamMassProperties foamMassProperties(const AssemblyParts& parts) {
  gui::FoamMassProperties result;QPointF moment;
  // Fuselage already contains its manufacturing pieces. Do not count its part
  // records twice. Standalone formers and tray use their own material density.
  for(const auto* shape:{&parts.fuselage,&parts.wing,&parts.horizontal,&parts.vertical,&parts.elevator,&parts.rudder}) {
    for(TopExp_Explorer solid{*shape,TopAbs_SOLID};solid.More();solid.Next()) {
      GProp_GProps mass;BRepGProp::VolumeProperties(solid.Current(),mass,1e-7);
      const double volume=mass.Mass();const auto center=mass.CentreOfMass();
      if(!std::isfinite(volume)||volume<=0||!std::isfinite(center.X())||!std::isfinite(center.Z()))
        throw std::runtime_error("Cannot measure a model solid's volume and centroid.");
      result.volumeMm3+=volume;moment+=QPointF{center.X(),center.Z()}*volume;
    }
  }
  if(result.volumeMm3<=0)throw std::runtime_error("Assembly contains no measurable foam solids.");
  result.centroidMm=moment/result.volumeMm3;moment={};
  for(const auto& insert:parts.inserts)for(TopExp_Explorer solid{insert.shape,TopAbs_SOLID};solid.More();solid.Next()) {
    GProp_GProps mass;BRepGProp::VolumeProperties(solid.Current(),mass,1e-7);
    const double volume=mass.Mass();const auto center=mass.CentreOfMass();
    if(!std::isfinite(volume)||volume<=0||!std::isfinite(center.X())||!std::isfinite(center.Z()))
      throw std::runtime_error("Cannot measure a plywood insert's volume and centroid.");
    result.plywoodVolumeMm3+=volume;moment+=QPointF{center.X(),center.Z()}*volume;
  }
  if(result.plywoodVolumeMm3>0)result.plywoodCentroidMm=moment/result.plywoodVolumeMm3;
  return result;
}
double wingRootLeadingEdgeX(const std::vector<gui::SketchLayer>& panels,
    const std::vector<gui::ConstrainedLine>& stations,std::optional<double> wingspanMm) {
  return gui::wingCalibration(panels,stations,wingspanMm).leadingEdgeX;
}
}

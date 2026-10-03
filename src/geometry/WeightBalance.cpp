#include "gui/WingCalibration.h"
#include "geometry/WeightBalance.h"
#include "processing/IndexedTasks.h"
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace designrc::geometry {
gui::FoamMassProperties foamMassProperties(const AssemblyParts& parts,unsigned workers) {
  gui::FoamMassProperties result;QPointF moment;
  // Fuselage already contains its manufacturing pieces. Do not count its part
  // records twice. Standalone formers and tray use their own material density.
  const QString names[]{"Fuselage","Wing","Horizontal stabilizer","Vertical stabilizer","Elevator","Rudder"};
  const TopoDS_Shape* shapes[]{&parts.fuselage,&parts.wing,&parts.horizontal,&parts.vertical,&parts.elevator,&parts.rudder};
  struct Measurement {double volume=0;QPointF moment;};
  std::vector<Measurement> measured(6+parts.inserts.size());
  // Workers read immutable Assembly geometry and own their property accumulators.
  // Reduce by input order after joining so worker scheduling cannot change CG.
  processing::runIndexedTasks(measured.size(),[&](std::size_t index,std::stop_token) {
    const auto& shape=index<6?*shapes[index]:parts.inserts[index-6].shape;
    auto& value=measured[index];
    for(TopExp_Explorer solid{shape,TopAbs_SOLID};solid.More();solid.Next()) {
      GProp_GProps mass;BRepGProp::VolumeProperties(solid.Current(),mass,1e-7);
      const double volume=mass.Mass();const auto center=mass.CentreOfMass();
      if(!std::isfinite(volume)||volume<=0||!std::isfinite(center.X())||!std::isfinite(center.Z()))
        throw std::runtime_error("Cannot measure a model solid's volume and centroid.");
      value.volume+=volume;value.moment+=QPointF{center.X(),center.Z()}*volume;
    }
  },{},workers);
  for(std::size_t i=0;i<6;++i) {
    result.volumeMm3+=measured[i].volume;moment+=measured[i].moment;
    result.components.push_back({names[i],measured[i].volume});
  }
  if(result.volumeMm3<=0)throw std::runtime_error("Assembly contains no measurable foam solids.");
  result.centroidMm=moment/result.volumeMm3;moment={};
  for(std::size_t i=0;i<parts.inserts.size();++i) {
    const auto& value=measured[6+i];result.plywoodVolumeMm3+=value.volume;moment+=value.moment;
    result.components.push_back({QString::fromStdString(parts.inserts[i].name),value.volume,true});
  }
  if(result.plywoodVolumeMm3>0)result.plywoodCentroidMm=moment/result.plywoodVolumeMm3;
  moment={};
  for(const auto& spar:parts.sparMaterials) {
    if(!std::isfinite(spar.volumeMm3)||spar.volumeMm3<=0 || !std::isfinite(spar.center.X())||!std::isfinite(spar.center.Z()))
      throw std::runtime_error("Cannot measure a carbon fiber spar's volume and centroid.");
    result.carbonFiberVolumeMm3+=spar.volumeMm3;moment+=QPointF{spar.center.X(),spar.center.Z()}*spar.volumeMm3;
    result.components.push_back({QString::fromStdString(spar.name),spar.volumeMm3,false,true});
  }
  if(result.carbonFiberVolumeMm3>0)result.carbonFiberCentroidMm=moment/result.carbonFiberVolumeMm3;
  return result;
}
double wingRootLeadingEdgeX(const std::vector<gui::SketchLayer>& panels,
    const std::vector<gui::ConstrainedLine>& stations,std::optional<double> wingspanMm) {
  return gui::wingCalibration(panels,stations,wingspanMm).leadingEdgeX;
}
}

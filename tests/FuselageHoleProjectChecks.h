#pragma once
#include "geometry/FuselageEndRegistration.h"
#include "geometry/FuselageSolidBuilder.h"
#include "gui/WingCalibration.h"
#include "gui/SketchBoundary.h"
#include <BRepCheck_Analyzer.hxx>

// Replay just the hole stage on captured pre-hole geometry. This keeps a real
// project's regression reproducible without rebuilding its loft and retainers.
inline void projectHoleChecks(const QString& projectPath,const char* bodyPath,
                              const char* cavityPath,bool expectInterference) {
  using namespace designrc;
  QString error;const auto project=gui::readProject(projectPath,error);CHECK(project);
  const auto sideBoundary=gui::closedSketchBoundary(project->fuselage.layers[1]);CHECK(sideBoundary);
  double left=sideBoundary->front().x(),right=left;
  for(auto point:*sideBoundary){left=std::min(left,point.x());right=std::max(right,point.x());}
  const double scale=project->reference.toScale?1.:
      gui::wingCalibration(project->wing.layers,project->stations.lines,project->reference.wingspanMm).scale;
  const double length=(right-left)*scale;
  const auto side=geometry::registerFuselageEnds(project->fuselage.layers[1],length);
  const auto top=geometry::registerFuselageEnds(project->fuselage.layers[0],length);
  double low=1e100,high=-1e100;
  for(auto point:top.boundary)if(std::abs(point.x()-top.noseX)<1e-8){low=std::min(low,point.y());high=std::max(high,point.y());}
  CHECK(low<=high);
  const auto transform=geometry::fuselageSideTransform(project->fuselage.layers[1],length);
  TopoDS_Shape body,cavity;BRep_Builder builder;
  CHECK(BRepTools::Read(body,bodyPath,builder));CHECK(BRepTools::Read(cavity,cavityPath,builder));
  const double originalVolume=volume(body);bool interference=false;
  try {
    const auto result=geometry::cutFuselageHoles(body,cavity,project->fuselageHoles.layers,project->fuselage.layers,
        {{{top.noseX,top.scale,(low+high)/2},{side.noseX,side.scale,transform.verticalOrigin}}});
    CHECK(!expectInterference);CHECK(BRepCheck_Analyzer{result}.IsValid());
    CHECK(count(result)==count(body));CHECK(volume(result)<originalVolume-1e-4);
    std::cout<<"Project hole stage passed; removed volume "<<originalVolume-volume(result)<<std::endl;
  }catch(const std::runtime_error& failure) {
    if(!expectInterference)throw;
    const std::string text=failure.what();
    CHECK(text.find("Top hole 1 overlaps a wall parallel to the cut direction")!=std::string::npos);
    std::cout<<text<<std::endl;interference=true;
  }
  CHECK(interference==expectInterference);CHECK(std::abs(volume(body)-originalVolume)<1e-7);
}

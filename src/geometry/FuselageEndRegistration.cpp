#include "geometry/FuselageEndRegistration.h"
#include "gui/SketchBoundary.h"
#include <QLineF>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
RegisteredFuselageOutline registerFuselageEnds(const gui::SketchLayer& outline,std::optional<double> lengthMm) {
  auto loop=gui::closedSketchBoundary(outline);
  if(!loop)throw std::runtime_error("Every fuselage outline must be a closed loop.");
  const auto [left,right]=std::minmax_element(loop->begin(),loop->end(),[](auto a,auto b){return a.x()<b.x();});
  RegisteredFuselageOutline result{*loop,left->x(),right->x()};
  const double width=result.tailX-result.noseX;
  if(width<1e-8)throw std::runtime_error("Fuselage outline needs a nose-to-tail length.");
  result.scale=lengthMm.value_or(width)/width;
  if(!std::isfinite(result.scale)||result.scale<=0)throw std::runtime_error("Fuselage length must be positive.");
  // Only explicit straight end edges qualify; a curved or pointed nose is not
  // flattened simply because samples happen to be close to its extremum.
  constexpr double tangentTwoDegrees=0.03492076949174773;
  for(bool nose:{true,false}) {
    const double plane=nose?result.noseX:result.tailX;
    const gui::SketchCurve* chosen=nullptr;double height=0;
    for(const auto& curve:outline.curves) {
      if(curve.type!=gui::SketchTool::Line||curve.points.size()!=2)continue;
      const auto a=outline.points[curve.points[0]],b=outline.points[curve.points[1]];
      const double dx=std::abs(a.x()-b.x()),dy=std::abs(a.y()-b.y());
      if(dy<=height||dx>dy*tangentTwoDegrees||dx*result.scale>fuselageEndToleranceMm)continue;
      const double extreme=nose?std::min(a.x(),b.x()):std::max(a.x(),b.x());
      if(std::abs(extreme-plane)*result.scale>1e-7)continue;
      chosen=&curve;height=dy;
    }
    if(!chosen)continue;
    const auto a=outline.points[chosen->points[0]],b=outline.points[chosen->points[1]];
    for(auto& point:result.boundary)
      if(QLineF{point,a}.length()<1e-8||QLineF{point,b}.length()<1e-8)point.setX(plane);
    if(nose)result.flatNose=true;else result.flatTail=true;
  }
  return result;
}
}

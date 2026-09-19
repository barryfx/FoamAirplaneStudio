#include "geometry/FuselageSolidBuilder.h"
#include "geometry/FuselageTopology.h"
#include "geometry/FuselageWall.h"
#include "geometry/FuselageCut.h"
#include "geometry/ServoTray.h"
#include "geometry/Formers.h"
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include "gui/SketchBoundary.h"
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <QLineF>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
namespace {
using Loop=std::vector<QPointF>;
QRectF bounds(const Loop& points) {
  double xmin=points.front().x(),xmax=xmin,ymin=points.front().y(),ymax=ymin;
  for(auto p:points){xmin=std::min(xmin,p.x());xmax=std::max(xmax,p.x());ymin=std::min(ymin,p.y());ymax=std::max(ymax,p.y());}
  return {QPointF{xmin,ymin},QPointF{xmax,ymax}};
}
Loop boundary(const gui::SketchLayer& layer) {
  auto loop=gui::closedSketchBoundary(layer);
  if(!loop)throw std::runtime_error("Every outline and station profile must be a closed loop.");
  return *loop;
}
std::pair<double,double> span(const Loop& loop,double x) {
  std::vector<double> hits;
  for(std::size_t i=1;i<loop.size();++i) {
    const auto a=loop[i-1],b=loop[i];
    if(x<std::min(a.x(),b.x())-1e-8 || x>std::max(a.x(),b.x())+1e-8)continue;
    if(std::abs(b.x()-a.x())<1e-10) {hits.push_back(a.y());hits.push_back(b.y());}
    else hits.push_back(a.y()+(b.y()-a.y())*std::clamp((x-a.x())/(b.x()-a.x()),0.,1.));
  }
  std::sort(hits.begin(),hits.end());
  hits.erase(std::unique(hits.begin(),hits.end(),[](double a,double b){return std::abs(a-b)<1e-7;}),hits.end());
  if(hits.empty() || hits.size()>2)throw std::runtime_error("An outline has an ambiguous vertical section. Remove folds or multiple crossings.");
  return {hits.front(),hits.back()};
}
// Use intersections with the drawing's horizontal/vertical centerlines, not
// the highest vertex: tiny roof slopes can put that vertex on opposite corners
// of adjacent profiles. Matching four directional landmarks preserves drawn up.
Loop normalizedProfile(Loop loop) {
  loop.pop_back();const auto box=bounds(loop);
  if(box.width()<1e-8 || box.height()<1e-8)throw std::runtime_error("A profile has zero width or height.");
  for(auto& p:loop)p={(p.x()-box.left())/box.width(),(p.y()-box.top())/box.height()};
  double area=0;for(std::size_t i=0;i<loop.size();++i){auto a=loop[i],b=loop[(i+1)%loop.size()];area+=a.x()*b.y()-a.y()*b.x();}
  if(area<0)std::reverse(loop.begin(),loop.end());
  loop.push_back(loop.front());
  std::vector<double> distances{0};
  for(std::size_t i=1;i<loop.size();++i)distances.push_back(distances.back()+QLineF{loop[i-1],loop[i]}.length());
  const double perimeter=distances.back();
  std::array<double,4> anchors{};
  for(int direction=0;direction<4;++direction) {
    const bool vertical=direction%2==0;
    double best=-1;bool found=false;
    for(std::size_t edge=1;edge<loop.size();++edge) {
      const auto a=loop[edge-1],b=loop[edge];
      const double av=vertical?a.x():a.y(),bv=vertical?b.x():b.y();
      if(std::abs(bv-av)<1e-12)continue;
      const double t=(.5-av)/(bv-av);if(t<0||t>1)continue;
      const auto point=a+(b-a)*t;
      const double radial=direction==0?.5-point.y():direction==1?point.x()-.5:direction==2?point.y()-.5:.5-point.x();
      if(radial>best) {best=radial;found=true;anchors[direction]=distances[edge-1]+t*(distances[edge]-distances[edge-1]);}
    }
    if(!found||best<=1e-9)throw std::runtime_error("A profile must cross each side of its drawing centerlines to preserve its up direction.");
  }
  // Unwrap exactly one clockwise turn starting at the top centerline crossing.
  for(int i=1;i<4;++i)while(anchors[i]<=anchors[i-1]+1e-10)anchors[i]+=perimeter;
  if(anchors[3]>=anchors[0]+perimeter-1e-10)
    throw std::runtime_error("Profile centerline crossings are out of order. Remove folds around its center.");
  Loop result;
  for(int quadrant=0;quadrant<4;++quadrant) {
    const double begin=anchors[quadrant],end=quadrant==3?anchors[0]+perimeter:anchors[quadrant+1];
    for(int i=0;i<16;++i) {
      const double d=std::fmod(begin+(end-begin)*i/16.,perimeter);
      const auto it=std::upper_bound(distances.begin(),distances.end(),d);
      const auto edge=std::min(static_cast<std::size_t>(it-distances.begin()),loop.size()-1);
      const double t=(d-distances[edge-1])/(distances[edge]-distances[edge-1]);
      result.push_back(loop[edge-1]*(1-t)+loop[edge]*t);
    }
  }
  const auto sampled=bounds(result);for(auto& p:result)p={(p.x()-sampled.left())/sampled.width(),(p.y()-sampled.top())/sampled.height()};
  return result;
}

// The four longitudinal rails are the upper/lower boundaries of both views.
// Include every sampled rail breakpoint when measuring error; a narrow shoulder
// must not disappear merely because it falls between uniform loft sections.
std::vector<double> guidedPositions(const Loop& top,const Loop& side,double length,
    const std::vector<double>& mandatory,const ProcessingControl& processing) {
  const auto tb=bounds(top),sb=bounds(side);
  auto candidates=mandatory;
  for(auto p:top)candidates.push_back((p.x()-tb.left())/tb.width());
  for(auto p:side)candidates.push_back((p.x()-sb.left())/sb.width());
  std::sort(candidates.begin(),candidates.end());
  candidates.erase(std::unique(candidates.begin(),candidates.end(),[](double a,double b){return std::abs(a-b)<1e-10;}),candidates.end());
  std::vector<std::array<double,4>> rails;
  for(double t:candidates) {
    processing.checkpoint();
    const auto w=span(top,tb.left()+t*tb.width()),h=span(side,sb.left()+t*sb.width());
    rails.push_back({w.first*length/tb.width(),w.second*length/tb.width(),h.first*length/sb.width(),h.second*length/sb.width()});
  }
  std::vector<bool> keep(candidates.size());
  for(double t:mandatory) {
    auto at=std::lower_bound(candidates.begin(),candidates.end(),t-1e-10);
    keep[static_cast<std::size_t>(at-candidates.begin())]=true;
  }
  std::vector<std::pair<std::size_t,std::size_t>> intervals;
  std::size_t previous=0;
  for(std::size_t i=1;i<keep.size();++i)if(keep[i]){intervals.emplace_back(previous,i);previous=i;}
  while(!intervals.empty()) {
    processing.checkpoint();const auto [begin,end]=intervals.back();intervals.pop_back();
    double worst=.1;std::size_t split=begin; // Maximum rail interpolation deviation, millimetres.
    for(std::size_t i=begin+1;i<end;++i) {
      const double t=(candidates[i]-candidates[begin])/(candidates[end]-candidates[begin]);
      for(int rail=0;rail<4;++rail) {
        const double deviation=std::abs(rails[i][rail]-(rails[begin][rail]*(1-t)+rails[end][rail]*t));
        if(deviation>worst){worst=deviation;split=i;}
      }
    }
    if(split!=begin){keep[split]=true;intervals.emplace_back(begin,split);intervals.emplace_back(split,end);}
  }
  std::vector<double> result;for(std::size_t i=0;i<keep.size();++i)if(keep[i])result.push_back(candidates[i]);
  return result;
}
}
std::vector<QPointF> sampleFuselageProfile(const gui::SketchLayer& profile) {
  return normalizedProfile(boundary(profile));
}
FuselageSideTransform fuselageSideTransform(const gui::SketchLayer& layer,std::optional<double> lengthMm) {
  const auto loop=boundary(layer);const auto box=bounds(loop);
  if(box.width()<1e-8)throw std::runtime_error("Fuselage outline needs a nose-to-tail length.");
  const auto nose=span(loop,box.left());
  return {box.left(),(nose.first+nose.second)/2,lengthMm.value_or(box.width())/box.width()};
}
FuselageBuildResult buildFuselageModel(const FuselageSolidInput& input,const std::function<void(const char*)>& progress,const ProcessingControl& processing) {
  processing.checkpoint();
  if(!input.formers.empty()&&!input.thicken)throw std::runtime_error("Enter Thicken before generating formers; formers need inner walls.");
  if(input.servoTray&&!input.thicken)throw std::runtime_error("Enter Thicken before generating a servo tray; the tray needs inner fuselage walls.");
  if(input.outlines.size()!=2 || input.stations.empty() || (input.lengthMm && (!std::isfinite(*input.lengthMm) || *input.lengthMm<=0)))
    throw std::runtime_error("Define both outlines, a profile at every station, and a positive Fuselage Length.");
  if(progress)progress("Fuselage: aligning Top and Side outlines at the nose...");
  const auto top=boundary(input.outlines[0]),side=boundary(input.outlines[1]);
  const auto tb=bounds(top),sb=bounds(side);
  if(tb.width()<1e-8||sb.width()<1e-8)throw std::runtime_error("Fuselage outlines need a nose-to-tail length.");
  const double length=input.lengthMm.value_or(sb.width());
  const double topScale=length/tb.width(),sideScale=length/sb.width();
  const auto topNose=span(top,tb.left());
  const double lateralOrigin=(topNose.first+topNose.second)/2;
  const double verticalOrigin=fuselageSideTransform(input.outlines[1],input.lengthMm).verticalOrigin;
  struct Section {double t;Loop profile;double wall;};std::vector<Section> sections;
  for(const auto& station:input.stations) {
    processing.checkpoint();
    if(!station.profile || *station.profile>=input.profiles.size())throw std::runtime_error("Assign a closed profile to every station.");
    const double t=(station.first.position.x()-sb.left())/sb.width();
    if(t<0||t>1)throw std::runtime_error("A station lies outside the Side View outline.");
    if(input.thicken&&(!station.thicknessMm||!std::isfinite(*station.thicknessMm)||*station.thicknessMm<=0))
      throw std::runtime_error("Set a positive wall thickness for every fuselage station.");
    sections.push_back({t,sampleFuselageProfile(input.profiles[*station.profile]),station.thicknessMm.value_or(0)});
  }
  std::sort(sections.begin(),sections.end(),[](const auto& a,const auto& b){return a.t<b.t;});
  const bool openNose=sections.front().t*length<=1e-6;
  const bool openTail=(1-sections.back().t)*length<=1e-6;
  std::vector<double> positions;for(int i=0;i<=32;++i)positions.push_back(i/32.);
  for(const auto& s:sections)positions.push_back(s.t);
  if(input.thicken) {
    for(std::size_t i=1;i<sections.size();++i)for(int j=1;j<8;++j)positions.push_back(sections[i-1].t+(sections[i].t-sections[i-1].t)*j/8.);
    if(!openTail&&sections.back().wall<length)positions.push_back(1-sections.back().wall/length);
    if(!openNose&&sections.front().wall<length)positions.push_back(sections.front().wall/length);
  }
  std::sort(positions.begin(),positions.end());positions.erase(std::unique(positions.begin(),positions.end(),[](double a,double b){return std::abs(a-b)<1e-8;}),positions.end());
  if(progress)progress("Fuselage: following Top and Side outline guides...");
  positions=guidedPositions(top,side,length,positions,processing);
  BRepOffsetAPI_ThruSections loft{true,true,1e-7};loft.CheckCompatibility(false);
  std::vector<FuselageWallSection> walls;
  for(double t:positions) {
    processing.checkpoint();const auto w=span(top,tb.left()+t*tb.width()),h=span(side,sb.left()+t*sb.width());
    double width=(w.second-w.first)*topScale,height=(h.second-h.first)*sideScale;
    const double center=(.5*(w.first+w.second)-lateralOrigin)*topScale;
    const double ztop=(verticalOrigin-h.first)*sideScale;
    auto upper=std::upper_bound(sections.begin(),sections.end(),t,[](double v,const Section& s){return v<s.t;});
    const auto& a=upper==sections.begin()?sections.front():*(upper-1);
    const auto& b=upper==sections.end()?sections.back():*upper;
    const double mix=b.t>a.t?std::clamp((t-a.t)/(b.t-a.t),0.,1.):0.;
    const double blend=mix*mix*(3-2*mix);
    FuselageWallSection wall{t*length,a.wall*(1-blend)+b.wall*blend,{}};
    if(width<1e-7&&height<1e-7&&(t==0||t==1)) {walls.push_back(wall);loft.AddVertex(BRepBuilderAPI_MakeVertex{gp_Pnt{t*length,center,ztop}});continue;}
    if((width<1e-7||height<1e-7)&&t>0&&t<1)throw std::runtime_error("An outline pinches to zero inside the fuselage.");
    // A line-shaped end has no closed section. Resolve it with a sub-micron
    // finite cap; point-shaped ends above use an exact OCCT vertex.
    width=std::max(width,1e-4);height=std::max(height,1e-4);
    Loop interpolated;for(std::size_t i=0;i<a.profile.size();++i)interpolated.push_back(a.profile[i]*(1-mix)+b.profile[i]*mix);
    const auto box=bounds(interpolated);BRepBuilderAPI_MakePolygon wire;
    for(auto p:interpolated) {
      const double y=center+width*((p.x()-box.left())/box.width()-.5),z=ztop-height*(p.y()-box.top())/box.height();
      wire.Add(gp_Pnt{t*length,y,z});wall.perimeter.push_back({y,z});
    }
    walls.push_back(std::move(wall));
    wire.Close();if(!wire.IsDone())throw std::runtime_error("Could not construct a profile section.");loft.AddWire(wire.Wire());
  }
  if(progress)progress("Fuselage: lofting and closing the solid...");
  {auto range=processing.range();loft.Build(range);}processing.checkpoint();
  if(!loft.IsDone())throw std::runtime_error("Fuselage loft failed. Check profile loops and outline crossings.");
  if(progress)progress("Fuselage: merging coincident outer loft faces...");
  auto shape=simplifyFuselageTopology(loft.Shape(),processing);
  if(!BRepCheck_Analyzer{shape,true,processing.parallel}.IsValid())throw std::runtime_error("Fuselage loft is not a valid solid. Check for crossing profiles.");
  int solids=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++solids;
  GProp_GProps props;BRepGProp::VolumeProperties(shape,props);
  if(solids!=1||std::abs(props.Mass())<1e-9)throw std::runtime_error("Fuselage did not produce one solid with positive volume.");
  if(props.Mass()<0)shape.Reverse();
  TopoDS_Shape cavity;FuselageBuildResult result;
  if(input.thicken)shape=hollowFuselage(shape,walls,openNose,openTail,progress,processing,(input.servoTray||!input.formers.empty())?&cavity:nullptr);
  if(input.servoTray) {
    const auto& r=*input.servoTray;
    const QRectF physical{(r.left()-sb.left())*sideScale,(verticalOrigin-r.bottom())*sideScale,r.width()*sideScale,r.height()*sideScale};
    const auto tray=addServoTray(shape,cavity,physical,progress,processing);
    shape=tray.body;result.servoTray=tray.tray;result.servoTrayTopFaces=tray.topFaces;
  }
  if(!input.formers.empty()) {
    auto physical=[&](const QRectF& r){return QRectF{(r.left()-sb.left())*sideScale,(verticalOrigin-r.bottom())*sideScale,r.width()*sideScale,r.height()*sideScale};};
    std::vector<QRectF> formers;for(const auto& r:input.formers)formers.push_back(physical(r));
    result.formers=buildFormers(cavity,shape,formers,input.servoTray?std::optional<QRectF>{physical(*input.servoTray)}:std::nullopt,progress,processing,input.formerRotationDegrees);
    auto inserts=result.formers;if(!result.servoTray.IsNull())inserts.push_back(result.servoTray);
    shape=addFormerRetainers(shape,cavity,formers,inserts,progress,processing,input.formerRotationDegrees);
  }
  FuselageAlignmentSpec alignment;alignment.seamReference=shape;
  if(input.thicken)for(const auto& section:sections)alignment.wallStations.emplace_back(section.t*length,section.wall);
  if(!input.cuts.empty())shape=cutFuselage(shape,input.cuts,
      {{{tb.left(),topScale,lateralOrigin},{sb.left(),sideScale,verticalOrigin}}},progress,processing);
  shape=splitFuselageMainBody(shape,progress,processing,&alignment);
  result.body=shape;
  if(!result.servoTray.IsNull()||!result.formers.empty()) {
    BRep_Builder builder;TopoDS_Compound assembly;builder.MakeCompound(assembly);
    builder.Add(assembly,shape);if(!result.servoTray.IsNull())builder.Add(assembly,result.servoTray);
    for(const auto& former:result.formers)builder.Add(assembly,former);shape=assembly;
  }
  processing.checkpoint();if(progress)progress("Fuselage: preparing display mesh...");
  IMeshTools_Parameters parameters;parameters.Deflection=.2;parameters.Angle=.3;parameters.InParallel=processing.parallel;
  BRepMesh_IncrementalMesh mesh{shape,parameters,processing.range()};processing.checkpoint();
  if(!mesh.IsDone())throw std::runtime_error("Fuselage display meshing failed.");
  result.shape=shape;return result;
}
TopoDS_Shape buildFuselageSolid(const FuselageSolidInput& input,const std::function<void(const char*)>& progress,const ProcessingControl& processing) {
  return buildFuselageModel(input,progress,processing).shape;
}
}

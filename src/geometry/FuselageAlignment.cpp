#include "geometry/FuselageAlignment.h"
#include "geometry/FuselageProcessing.h"
#include "geometry/FuselageMaterialSpans.h"
#include "processing/IndexedTasks.h"
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SClassifier.hxx>
#include <BRepClass3d_SolidExplorer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <IntCurvesFace_Intersector.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Ax2.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <memory>
namespace designrc::geometry {
namespace {
constexpr double tolerance=1e-7;
constexpr double pinDepth=3.,holeDepth=3.5,rootDepth=.25,blindStock=.1;
double volume(const TopoDS_Shape& shape,const ProcessingControl& control) {GProp_GProps mass;fuselageVolumeProperties(shape,mass,control);return mass.Mass();}
template<class Operation> TopoDS_Shape booleanOp(const TopoDS_Shape& body,
    const NCollection_List<TopoDS_Shape>& tools,const ProcessingControl& processing) {
  processing.checkpoint();Operation operation;NCollection_List<TopoDS_Shape> arguments;arguments.Append(body);
  operation.SetArguments(arguments);operation.SetTools(tools);operation.SetNonDestructive(true);operation.SetRunParallel(processing.parallel);operation.SetFuzzyValue(tolerance);
  {auto range=processing.range();operation.Build(range);}processing.checkpoint();
  if(!operation.IsDone()||operation.HasErrors()||operation.Shape().IsNull())throw std::runtime_error("Could not create fuselage alignment pins and sockets.");
  return operation.Shape();
}
TopoDS_Shape cylinder(double x,double z,double radius,double begin,double end) {
  return BRepPrimAPI_MakeCylinder{gp_Ax2{gp_Pnt{x,begin,z},gp_Dir{0,1,0}},radius,end-begin}.Shape();
}
double wallAt(double x,const std::vector<std::pair<double,double>>& stations) {
  if(stations.empty())return 4.;
  auto upper=std::upper_bound(stations.begin(),stations.end(),x,[](double value,const auto& station){return value<station.first;});
  if(upper==stations.begin())return upper->second;
  if(upper==stations.end())return stations.back().second;
  const auto& a=*(upper-1);const auto& b=*upper;const double t=(x-a.first)/(b.first-a.first),blend=t*t*(3-2*t);
  return a.second*(1-blend)+b.second*blend;
}
void requireOneSolid(const TopoDS_Shape& shape,const ProcessingControl& control) {
  int count=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++count;
  if(count!=1||volume(shape,control)<=tolerance||!fuselageValid(shape,control))throw std::runtime_error("Alignment pins or sockets disconnected or invalidated a fuselage half.");
}
}
std::array<TopoDS_Shape,2> addFuselageAlignmentPins(const TopoDS_Shape& mainBody,
    const std::array<TopoDS_Shape,2>& halves,const FuselageAlignmentSpec& specification,
    const std::function<void(const char*)>& progress,const ProcessingControl& processing) {
  processing.checkpoint();
  for(std::size_t i=0;i<specification.wallStations.size();++i) {
    const auto [x,wall]=specification.wallStations[i];
    if(!std::isfinite(x)||!std::isfinite(wall)||wall<=0||(i&&x<=specification.wallStations[i-1].first))
      throw std::runtime_error("Invalid wall thickness stations for fuselage alignment pins.");
  }
  const auto& reference=specification.seamReference.IsNull()?mainBody:specification.seamReference;
  Bnd_Box bounds;fuselageBounds(reference,bounds,processing);
  double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
  // End positions belong to the retained main body, even if a user cut has
  // detached a nose/tail segment. The reference supplies skin heights only.
  Bnd_Box mainBounds;fuselageBounds(mainBody,mainBounds,processing);
  double unusedY0,unusedZ0,unusedY1,unusedZ1;
  mainBounds.Get(x0,unusedY0,unusedZ0,x1,unusedY1,unusedZ1);
  struct Pin {double x,z,radius;};
  const bool concurrent=processing.parallel;
  const auto locate=[&](std::size_t index,const std::vector<Pin>& placed,const ProcessingControl& control) -> Pin {
    control.checkpoint();
    const bool top=index<2,forward=index%2==0;
    const std::string location=std::string{forward?"forward ":"aft "}+(top?"top":"bottom");
    if(!concurrent&&progress)progress(("Fuselage: locating "+location+" alignment pin...").c_str());
    // Each search owns lazy face intersectors and deep CAD copies.
    // Exact containment Booleans cannot mutate another search's operands.
    const auto taskBody=concurrent?BRepBuilderAPI_Copy{mainBody,true,false}.Shape():mainBody;
    control.checkpoint();
    const auto taskReference=concurrent?BRepBuilderAPI_Copy{reference,true,false}.Shape():reference;
    control.checkpoint();
    FuselageMaterialSpans skin{taskReference,z0-1,z1+1,control},
        material{taskBody,z0-1,z1+1,control};
    std::vector<double> fractions;
    for(int i=0;i<=15;++i)fractions.push_back(forward?.05+i*.02:.65+i*.02);
    const double target=forward?.15:.75;
    std::stable_sort(fractions.begin(),fractions.end(),[&](double a,double b){return std::abs(a-target)<std::abs(b-target);});
    for(double fraction:fractions) {
      const double x=x0+fraction*(x1-x0);const auto spans=skin.at(x,0,control);if(spans.empty())continue;
      const auto [lower,upper]=top?spans.back():spans.front();
      const double wall=std::min(wallAt(x,specification.wallStations),upper-lower);
      const double radius=.5*std::min(4.,wall);
      if(radius<1e-4)continue;
      const double preferredZ=top?upper-wall*.5:lower+wall*.5;
      double centerMin=std::max(lower+radius,top?upper-wall-radius:lower+radius);
      double centerMax=std::min(upper-radius,top?upper-radius:lower+wall+radius);
      // Cheap cross-section screening precedes the exact containment Boolean.
      // Sampling is only a rejection filter, never the final fit criterion.
      bool fits=true;
      for(double y:{-rootDepth,0.,1.5,holeDepth+blindStock}) {
        for(double dx:{-radius,-.7071067811865476*radius,0.,.7071067811865476*radius,radius}) {
          const double dz=std::sqrt(std::max(0.,radius*radius-dx*dx));
          const auto available=material.at(x+dx,y,control);
          double bestLow=0,bestHigh=-1;
          for(const auto& span:available) {
            const double low=std::max(centerMin,span.first+dz),high=std::min(centerMax,span.second-dz);
            if(high>=low-1e-6&&high-low>bestHigh-bestLow){bestLow=low;bestHigh=std::max(low,high);}
          }
          if(bestHigh<bestLow){fits=false;break;}
          centerMin=bestLow;centerMax=bestHigh;
        }
        if(!fits)break;
      }
      if(!fits)continue;
      // A sloping skin may require moving slightly inward from the nominal
      // midpoint. Keep the full disk inside the same mating-material interval.
      double z=std::clamp(preferredZ,centerMin,centerMax);bool contained=false;
      // Sampling may miss the extremum of a curved/sloping skin. If the preferred
      // centre fails exact containment, try the centre of the available range.
      for(double candidateZ:{z,.5*(centerMin+centerMax)}) {
        if(std::any_of(placed.begin(),placed.end(),[&](const Pin& pin){return std::hypot(pin.x-x,pin.z-candidateZ)<=pin.radius+radius+.1;}))continue;
        const auto envelope=cylinder(x,candidateZ,radius,-rootDepth,holeDepth+blindStock);
        NCollection_List<TopoDS_Shape> tools;tools.Append(envelope);
        const double expected=volume(envelope,control);
        double containedVolume=0;
        if(specification.separateHalves) {
          // The halves only share their mating face. Their intersection volumes
          // are additive; separate Commons avoid treating the entire touching
          // pair as an interfering Boolean argument for every candidate pin.
          for(TopExp_Explorer half{taskBody,TopAbs_SOLID};half.More();half.Next())
            containedVolume+=volume(booleanOp<BRepAlgoAPI_Common>(half.Current(),tools,control),control);
        } else containedVolume=volume(booleanOp<BRepAlgoAPI_Common>(taskBody,tools,control),control);
        if(std::abs(containedVolume-expected)<=std::max(1e-6,expected*1e-6)) {
          z=candidateZ;contained=true;break;
        }
      }
      if(!contained)continue;
      return {x,z,radius};
    }
    throw std::runtime_error("Cannot place the "+location+" alignment pin: no supported mating surface for a 3 mm pin and 3.5 mm blind socket near that end.");
  };
  std::array<Pin,4> candidates{};
  if(concurrent&&progress)progress("Fuselage: locating four alignment pins in parallel...");
  processing::runIndexedTasks(candidates.size(),[&](std::size_t i,std::stop_token stop) {
    candidates[i]=locate(i,{},ProcessingControl{stop,processing.parallel});
  },processing.stop,concurrent?0u:1u);
  processing.checkpoint();
  std::vector<Pin> placed;NCollection_List<TopoDS_Shape> pins,sockets;
  double pinVolume=0,socketVolume=0;
  for(std::size_t i=0;i<candidates.size();++i) {
    processing.checkpoint();auto candidate=candidates[i];
    // Resolve rare shared-space conflicts in the original top/bottom, fore/aft
    // order. A retry sees every earlier accepted pin, preserving serial choices.
    if(std::any_of(placed.begin(),placed.end(),[&](const Pin& other) {
      return std::hypot(other.x-candidate.x,other.z-candidate.z)<=other.radius+candidate.radius+.1;
    }))candidate=locate(i,placed,processing);
    const auto [x,z,radius]=candidate;
    pins.Append(cylinder(x,z,radius,-rootDepth,pinDepth));
    sockets.Append(cylinder(x,z,radius,-tolerance,holeDepth));
    pinVolume+=std::acos(-1.)*radius*radius*pinDepth;
    socketVolume+=std::acos(-1.)*radius*radius*holeDepth;
    placed.push_back(candidate);
  }
  GProp_GProps first;fuselageVolumeProperties(halves[0],first,processing);
  const std::size_t pinSide=first.CentreOfMass().Y()<0?0:1,socketSide=1-pinSide;
  if(progress)progress("Fuselage: adding four alignment pins and matching blind sockets...");
  auto result=halves;
  processing::runIndexedTasks(2,[&](std::size_t task,std::stop_token stop) {
    const ProcessingControl control{stop,processing.parallel};control.checkpoint();
    const auto side=task==0?pinSide:socketSide;
    const auto operand=concurrent?BRepBuilderAPI_Copy{halves[side],true,false}.Shape():halves[side];
    result[side]=task==0?booleanOp<BRepAlgoAPI_Fuse>(operand,pins,control):
        booleanOp<BRepAlgoAPI_Cut>(operand,sockets,control);
    requireOneSolid(result[side],control);
  },processing.stop,concurrent?0u:1u);
  processing.checkpoint();
  const double volumeTolerance=std::max(1e-5,(pinVolume+socketVolume)*1e-5);
  if(std::abs(volume(result[pinSide],processing)-volume(halves[pinSide],processing)-pinVolume)>volumeTolerance||
      std::abs(volume(halves[socketSide],processing)-volume(result[socketSide],processing)-socketVolume)>volumeTolerance)
    throw std::runtime_error("Fuselage alignment pin or socket dimensions failed validation.");
  return result;
}
}

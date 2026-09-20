#include "geometry/FuselageAlignment.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Ax2.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
namespace designrc::geometry {
namespace {
constexpr double tolerance=1e-7;
constexpr double pinDepth=3.,holeDepth=3.5,rootDepth=.25,blindStock=.1;
double volume(const TopoDS_Shape& shape) {GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass);return mass.Mass();}
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
// Accelerated vertical intersections return material intervals, rather than
// assuming that the body has only an outer and inner face at a given X.
class MaterialSpans {
public:
  MaterialSpans(const TopoDS_Shape& body,double minimum,double maximum)
      : classifier_(body),minimum_(minimum),maximum_(maximum) {intersector_.Load(body,tolerance);}
  std::vector<std::pair<double,double>> at(double x,double y,const ProcessingControl& processing) {
    processing.checkpoint();intersector_.Perform(gp_Lin{gp_Pnt{x,y,minimum_},gp_Dir{0,0,1}},0,maximum_-minimum_);
    if(!intersector_.IsDone())throw std::runtime_error("Could not inspect the fuselage mating surfaces.");
    std::vector<double> heights;
    for(int i=1;i<=intersector_.NbPnt();++i)heights.push_back(intersector_.Pnt(i).Z());
    std::sort(heights.begin(),heights.end());
    heights.erase(std::unique(heights.begin(),heights.end(),[](double a,double b){return std::abs(a-b)<tolerance;}),heights.end());
    std::vector<std::pair<double,double>> spans;
    for(std::size_t i=1;i<heights.size();++i) {
      classifier_.Perform(gp_Pnt{x,y,.5*(heights[i-1]+heights[i])},tolerance);
      if(classifier_.State()==TopAbs_IN)spans.emplace_back(heights[i-1],heights[i]);
    }
    return spans;
  }
private:
  IntCurvesFace_ShapeIntersector intersector_;
  BRepClass3d_SolidClassifier classifier_;
  double minimum_,maximum_;
};
double wallAt(double x,const std::vector<std::pair<double,double>>& stations) {
  if(stations.empty())return 4.;
  auto upper=std::upper_bound(stations.begin(),stations.end(),x,[](double value,const auto& station){return value<station.first;});
  if(upper==stations.begin())return upper->second;
  if(upper==stations.end())return stations.back().second;
  const auto& a=*(upper-1);const auto& b=*upper;const double t=(x-a.first)/(b.first-a.first),blend=t*t*(3-2*t);
  return a.second*(1-blend)+b.second*blend;
}
void requireOneSolid(const TopoDS_Shape& shape,bool parallel) {
  int count=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++count;
  if(count!=1||volume(shape)<=tolerance||!BRepCheck_Analyzer{shape,true,parallel}.IsValid())throw std::runtime_error("Alignment pins or sockets disconnected or invalidated a fuselage half.");
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
  Bnd_Box bounds;BRepBndLib::AddOptimal(reference,bounds,false,false);
  double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
  // End positions belong to the retained main body, even if a user cut has
  // detached a nose/tail segment. The reference supplies skin heights only.
  Bnd_Box mainBounds;BRepBndLib::AddOptimal(mainBody,mainBounds,false,false);
  double unusedY0,unusedZ0,unusedY1,unusedZ1;
  mainBounds.Get(x0,unusedY0,unusedZ0,x1,unusedY1,unusedZ1);
  MaterialSpans skin{reference,z0-1,z1+1},material{mainBody,z0-1,z1+1};
  struct Pin {double x,z,radius;};std::vector<Pin> placed;
  NCollection_List<TopoDS_Shape> pins,sockets;double pinVolume=0,socketVolume=0;
  for(bool top:{true,false})for(bool forward:{true,false}) {
    const std::string location=std::string{forward?"forward ":"aft "}+(top?"top":"bottom");
    if(progress)progress(("Fuselage: locating "+location+" alignment pin...").c_str());
    std::vector<double> fractions;
    for(int i=0;i<=15;++i)fractions.push_back(forward?.05+i*.02:.65+i*.02);
    const double target=forward?.15:.75;
    std::stable_sort(fractions.begin(),fractions.end(),[&](double a,double b){return std::abs(a-target)<std::abs(b-target);});
    bool found=false;
    for(double fraction:fractions) {
      const double x=x0+fraction*(x1-x0);const auto spans=skin.at(x,0,processing);if(spans.empty())continue;
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
          const auto available=material.at(x+dx,y,processing);
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
        const double expected=volume(envelope);
        if(std::abs(volume(booleanOp<BRepAlgoAPI_Common>(mainBody,tools,processing))-expected)<=std::max(1e-6,expected*1e-6)) {
          z=candidateZ;contained=true;break;
        }
      }
      if(!contained)continue;
      pins.Append(cylinder(x,z,radius,-rootDepth,pinDepth));
      sockets.Append(cylinder(x,z,radius,-tolerance,holeDepth));
      pinVolume+=std::acos(-1.)*radius*radius*pinDepth;socketVolume+=std::acos(-1.)*radius*radius*holeDepth;
      placed.push_back({x,z,radius});found=true;break;
    }
    if(!found)throw std::runtime_error("Cannot place the "+location+" alignment pin: no supported mating surface for a 3 mm pin and 3.5 mm blind socket near that end.");
  }
  GProp_GProps first;BRepGProp::VolumeProperties(halves[0],first);
  const std::size_t pinSide=first.CentreOfMass().Y()<0?0:1,socketSide=1-pinSide;
  if(progress)progress("Fuselage: adding four alignment pins and matching blind sockets...");
  auto result=halves;
  result[pinSide]=booleanOp<BRepAlgoAPI_Fuse>(halves[pinSide],pins,processing);
  result[socketSide]=booleanOp<BRepAlgoAPI_Cut>(halves[socketSide],sockets,processing);
  for(const auto& half:result)requireOneSolid(half,processing.parallel);
  const double volumeTolerance=std::max(1e-5,(pinVolume+socketVolume)*1e-5);
  if(std::abs(volume(result[pinSide])-volume(halves[pinSide])-pinVolume)>volumeTolerance||
      std::abs(volume(halves[socketSide])-volume(result[socketSide])-socketVolume)>volumeTolerance)
    throw std::runtime_error("Fuselage alignment pin or socket dimensions failed validation.");
  return result;
}
}

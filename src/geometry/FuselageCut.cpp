#include "geometry/FuselageCut.h"
#include <BRepAlgoAPI_Splitter.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Pln.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
namespace designrc::geometry {
namespace {
int bodyCount(const TopoDS_Shape& shape) {int count=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++count;return count;}
// Return connected, nonbranching paths in curve order, allowing multiple paths
// and closed paths in each view. Reversal is recorded without editing sketches.
using Path=std::vector<std::pair<std::size_t,bool>>;
std::vector<Path> paths(const gui::SketchLayer& layer) {
  std::vector<std::vector<std::size_t>> at(layer.points.size());
  for(std::size_t i=0;i<layer.curves.size();++i) {
    const auto& curve=layer.curves[i];
    if(curve.points.size()<2)throw std::runtime_error("A cut segment needs at least two points.");
    for(auto id:curve.points)if(id>=at.size())throw std::runtime_error("A cut segment has an invalid point.");
    at[curve.points.front()].push_back(i);at[curve.points.back()].push_back(i);
  }
  for(const auto& edges:at)if(edges.size()>2)throw std::runtime_error("Cut paths must not branch. Use separate connected paths.");
  std::vector<bool> used(layer.curves.size());std::vector<Path> result;
  auto walk=[&](std::size_t start) {
    Path path;auto node=start;
    while(true) {
      auto edge=std::find_if(at[node].begin(),at[node].end(),[&](auto i){return !used[i];});
      if(edge==at[node].end())break;
      const auto i=*edge;used[i]=true;const auto& curve=layer.curves[i];
      const bool reverse=curve.points.front()!=node;path.emplace_back(i,reverse);
      node=reverse?curve.points.front():curve.points.back();
    }
    if(!path.empty())result.push_back(std::move(path));
  };
  for(std::size_t i=0;i<at.size();++i)if(at[i].size()==1)walk(i);
  for(std::size_t i=0;i<layer.curves.size();++i)if(!used[i])walk(layer.curves[i].points.front());
  return result;
}
}
TopoDS_Shape splitFuselageMainBody(const TopoDS_Shape& body,
    const std::function<void(const char*)>& progress,const ProcessingControl& processing,
    const FuselageAlignmentSpec* alignment) {
  processing.checkpoint();
  if(progress)progress("Fuselage: splitting main body into left/right halves; preserving cut-out parts...");
  std::vector<TopoDS_Shape> parts;std::vector<double> volumes;
  for(TopExp_Explorer e{body,TopAbs_SOLID};e.More();e.Next()) {
    parts.push_back(e.Current());GProp_GProps mass;BRepGProp::VolumeProperties(e.Current(),mass);volumes.push_back(mass.Mass());
  }
  if(parts.empty())throw std::runtime_error("No main fuselage body to split.");
  const auto index=static_cast<std::size_t>(std::max_element(volumes.begin(),volumes.end())-volumes.begin());
  for(std::size_t i=0;i<parts.size();++i)if(i!=index&&std::abs(volumes[i]-volumes[index])<=std::max(1e-8,volumes[index]*1e-8))
    throw std::runtime_error("Cuts leave equally sized bodies; the main fuselage body cannot be identified.");
  // The registered longitudinal centre plane is Y=0, through the aligned noses.
  const auto plane=BRepBuilderAPI_MakeFace{gp_Pln{gp_Pnt{0,0,0},gp_Dir{0,1,0}}}.Shape();
  BRepAlgoAPI_Splitter splitter;NCollection_List<TopoDS_Shape> args,tools;
  args.Append(parts[index]);tools.Append(plane);splitter.SetArguments(args);splitter.SetTools(tools);
  splitter.SetNonDestructive(true);splitter.SetFuzzyValue(1e-7);
  {auto range=processing.range();splitter.Build(range);}processing.checkpoint();
  if(!splitter.IsDone()||splitter.HasErrors()||bodyCount(splitter.Shape())!=2)
    throw std::runtime_error("The main fuselage must cross the centre plane to form two connected halves.");
  BRep_Builder builder;TopoDS_Compound result;builder.MakeCompound(result);double volume=0;
  std::array<TopoDS_Shape,2> halves;std::size_t halfIndex=0;
  for(TopExp_Explorer e{splitter.Shape(),TopAbs_SOLID};e.More();e.Next()) {
    processing.checkpoint();GProp_GProps mass;BRepGProp::VolumeProperties(e.Current(),mass);
    if(mass.Mass()<=1e-9||!BRepCheck_Analyzer{e.Current()}.IsValid())throw std::runtime_error("Invalid fuselage half after centre split.");
    volume+=mass.Mass();halves[halfIndex++]=e.Current();
  }
  if(std::abs(volume-volumes[index])>std::max(1e-3,volumes[index]*1e-6))throw std::runtime_error("Centre split did not conserve fuselage material.");
  if(alignment)halves=addFuselageAlignmentPins(parts[index],halves,*alignment,progress,processing);
  for(const auto& half:halves)builder.Add(result,half);
  for(std::size_t i=0;i<parts.size();++i)if(i!=index)builder.Add(result,parts[i]);
  return result;
}
TopoDS_Shape cutFuselage(const TopoDS_Shape& body,const std::vector<gui::SketchLayer>& cuts,
    const std::array<FuselageCutProjection,2>& projections,const std::function<void(const char*)>& progress,
    const ProcessingControl& processing) {
  processing.checkpoint();if(cuts.empty())return body;
  if(cuts.size()!=2)throw std::runtime_error("Cut sketches require Top and Side views.");
  Bnd_Box box;BRepBndLib::AddOptimal(body,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
  const double margin=std::max({x1-x0,y1-y0,z1-z0,1.});auto result=body;
  for(int view=0;view<2;++view) {
    const auto& layer=cuts[view];const auto& projection=projections[view];
    const auto viewPaths=paths(layer);
    for(std::size_t n=0;n<viewPaths.size();++n) {
      processing.checkpoint();
      const std::string label=std::string{view==0?"Top":"Side"}+" View cut "+std::to_string(n+1);
      if(progress)progress(("Fuselage: splitting with "+label+"...").c_str());
      auto point=[&](std::size_t id) {
        const auto p=layer.points[id];const double x=(p.x()-projection.noseX)*projection.scale;
        return view==0?gp_Pnt{x,(p.y()-projection.transverseOrigin)*projection.scale,z0-margin}:
            gp_Pnt{x,y0-margin,(projection.transverseOrigin-p.y())*projection.scale};
      };
      BRepBuilderAPI_MakeWire wire;
      for(const auto& [index,reverse]:viewPaths[n]) {
        processing.checkpoint();const auto& curve=layer.curves[index];auto ids=curve.points;
        if(reverse)std::reverse(ids.begin(),ids.end());
        TopoDS_Edge edge;
        if(curve.type==gui::SketchTool::Line||ids.size()==2) {
          if(point(ids.front()).Distance(point(ids.back()))<1e-7)throw std::runtime_error(label+" contains a zero-length segment.");
          edge=BRepBuilderAPI_MakeEdge{point(ids.front()),point(ids.back())}.Edge();
        } else {
          const bool periodic=ids.size()>=4&&ids.front()==ids.back();
          const int count=static_cast<int>(ids.size())-(periodic?1:0);
          occ::handle<NCollection_HArray1<gp_Pnt>> nodes=new NCollection_HArray1<gp_Pnt>{1,count};
          for(int i=0;i<count;++i)nodes->SetValue(i+1,point(ids[i]));
          GeomAPI_Interpolate fit{nodes,periodic,1e-9};fit.Perform();
          if(!fit.IsDone())throw std::runtime_error(label+" spline could not be fitted.");
          edge=BRepBuilderAPI_MakeEdge{fit.Curve()}.Edge();
        }
        wire.Add(edge);if(!wire.IsDone())throw std::runtime_error(label+" segments do not connect.");
      }
      const auto direction=view==0?gp_Vec{0,0,z1-z0+2*margin}:gp_Vec{0,y1-y0+2*margin,0};
      const auto sheet=BRepPrimAPI_MakePrism{wire.Wire(),direction}.Shape();
      BRepAlgoAPI_Splitter splitter;NCollection_List<TopoDS_Shape> arguments,tools;
      arguments.Append(result);tools.Append(sheet);splitter.SetArguments(arguments);splitter.SetTools(tools);
      splitter.SetNonDestructive(true);splitter.SetFuzzyValue(1e-7);
      {auto range=processing.range();splitter.Build(range);}processing.checkpoint();
      if(!splitter.IsDone()||splitter.HasErrors())throw std::runtime_error(label+" failed. Check for self-intersections or overlapping segments.");
      const auto split=splitter.Shape();
      if(bodyCount(split)<=bodyCount(result))throw std::runtime_error(label+" did not separate a body. Extend the path to or beyond both outline edges, or close the path.");
      BRep_Builder builder;TopoDS_Compound solids;builder.MakeCompound(solids);double volume=0;
      for(TopExp_Explorer e{split,TopAbs_SOLID};e.More();e.Next()) {
        processing.checkpoint();const auto solid=e.Current();
        if(!BRepCheck_Analyzer{solid}.IsValid())throw std::runtime_error(label+" produced an invalid body.");
        GProp_GProps mass;BRepGProp::VolumeProperties(solid,mass);
        if(mass.Mass()<=1e-9)throw std::runtime_error(label+" produced an empty or reversed body.");
        volume+=mass.Mass();builder.Add(solids,solid);
      }
      GProp_GProps before;BRepGProp::VolumeProperties(result,before);
      if(std::abs(volume-before.Mass())>std::max(1e-3,std::abs(before.Mass())*1e-6))
        throw std::runtime_error(label+" did not conserve the fuselage material volume.");
      result=solids;
    }
  }
  return result;
}
}

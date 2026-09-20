#include "geometry/FuselageHoles.h"
#include "gui/SketchPaths.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <Geom_BSplineCurve.hxx>
#include <NCollection_HArray1.hxx>
#include <TopExp_Explorer.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
namespace {
std::array<double,6> bounds(const TopoDS_Shape& shape){Bnd_Box b;BRepBndLib::AddOptimal(shape,b,false,false);std::array<double,6> r;b.Get(r[0],r[1],r[2],r[3],r[4],r[5]);return r;}
double volume(const TopoDS_Shape& shape){GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass);return mass.Mass();}
TopoDS_Shape cut(const TopoDS_Shape& a,const TopoDS_Shape& b,const ProcessingControl& control) {
  control.checkpoint();BRepAlgoAPI_Cut op;NCollection_List<TopoDS_Shape> args,tools;args.Append(a);tools.Append(b);
  op.SetArguments(args);op.SetTools(tools);op.SetNonDestructive(true);op.SetRunParallel(control.parallel);op.SetFuzzyValue(1e-7);
  {auto range=control.range();op.Build(range);}control.checkpoint();
  if(!op.IsDone()||op.HasErrors())throw std::runtime_error("Could not cut the fuselage hole.");return op.Shape();
}
}
TopoDS_Shape cutFuselageHoles(const TopoDS_Shape& body,const TopoDS_Shape& cavity,
    const std::vector<gui::SketchLayer>& holes,const std::vector<gui::SketchLayer>& outlines,
    const std::array<FuselageCutProjection,2>& projections,const ProcessingControl& control) {
  if(holes.empty())return body;
  if(holes.size()!=4||outlines.size()!=2)throw std::runtime_error("Invalid fuselage hole views.");
  auto result=body;const auto b=bounds(body);const double margin=std::max({b[3]-b[0],b[4]-b[1],b[5]-b[2],1.});
  for(int wall=0;wall<4;++wall)for(const auto& path:gui::sketchPaths(holes[wall])) {
    control.checkpoint();const auto& layer=path.layer;
    if(!gui::closedSketchBoundary(layer))throw std::runtime_error("Close or delete every incomplete hole before generating the fuselage.");
    const int view=wall<2?0:1,axis=wall<2?2:1;const bool positive=wall==0||wall==3;
    if(!gui::sketchPathInside(layer,outlines[view]))throw std::runtime_error("Every hole must fit entirely inside its fuselage outline.");
    if(cavity.IsNull())throw std::runtime_error("Holes require a hollow fuselage with an inner cavity.");
    const auto p=projections[view];const double near=positive?b[axis+3]+margin:b[axis]-margin;
    const double far=positive?b[axis]-margin:b[axis+3]+margin;
    auto point=[&](std::size_t id){const auto q=layer.points[id];const double x=(q.x()-p.noseX)*p.scale,t=(p.transverseOrigin-q.y())*p.scale;return view==0?gp_Pnt{x,-t,near}:gp_Pnt{x,near,t};};
    BRepBuilderAPI_MakeWire wire;std::vector<bool> used(layer.curves.size());auto current=layer.curves.front().points.front();
    for(std::size_t n=0;n<layer.curves.size();++n) {
      control.checkpoint();std::size_t next=layer.curves.size();bool reverse=false;
      for(std::size_t i=0;i<layer.curves.size();++i)if(!used[i]) {
        if(layer.curves[i].points.front()==current){next=i;break;}
        if(layer.curves[i].points.back()==current){next=i;reverse=true;break;}
      }
      if(next==layer.curves.size())throw std::runtime_error("Hole segments do not connect.");
      used[next]=true;auto ids=layer.curves[next].points;if(reverse)std::reverse(ids.begin(),ids.end());current=ids.back();
      if(layer.curves[next].type==gui::SketchTool::Line||ids.size()==2)wire.Add(BRepBuilderAPI_MakeEdge{point(ids.front()),point(ids.back())}.Edge());
      else {
        const bool periodic=ids.front()==ids.back();const int count=static_cast<int>(ids.size())-(periodic?1:0);
        occ::handle<NCollection_HArray1<gp_Pnt>> nodes=new NCollection_HArray1<gp_Pnt>{1,count};for(int i=0;i<count;++i)nodes->SetValue(i+1,point(ids[i]));
        GeomAPI_Interpolate fit{nodes,periodic,1e-9};fit.Perform();if(!fit.IsDone())throw std::runtime_error("Could not fit the hole spline.");
        wire.Add(BRepBuilderAPI_MakeEdge{fit.Curve()}.Edge());
      }
    }
    if(!wire.IsDone())throw std::runtime_error("Hole segments do not form a wire.");
    BRepBuilderAPI_MakeFace face{wire.Wire(),true};
    if(!face.IsDone()||!BRepCheck_Analyzer{face.Face()}.IsValid())throw std::runtime_error("A hole must be a simple closed loop without self-intersections.");
    const auto prism=BRepPrimAPI_MakePrism{face.Face(),view==0?gp_Vec{0,0,far-near}:gp_Vec{0,far-near,0}}.Shape();
    // The cavity separates the through-prism into near and far components.
    // Only the solid touching the chosen exterior starting plane may remove
    // material. If it reaches the far plane too, this hole can cut both walls.
    const auto separated=cut(prism,cavity,control);TopoDS_Shape tool;
    for(TopExp_Explorer e{separated,TopAbs_SOLID};e.More();e.Next()) {
      const auto box=bounds(e.Current());const double start=positive?box[axis+3]:box[axis];
      if(std::abs(start-near)>1e-5)continue;
      const double end=positive?box[axis]:box[axis+3];
      if(std::abs(end-far)<1e-5||!tool.IsNull())throw std::runtime_error("Move or resize the hole: its footprint must reach the inner cavity across the whole loop to cut only one wall.");
      tool=e.Current();
    }
    if(tool.IsNull())throw std::runtime_error("Could not isolate the selected wall for this hole.");
    const auto remaining=cut(result,tool,control);int count=0;
    for(TopExp_Explorer e{remaining,TopAbs_SOLID};e.More();e.Next()) {
      if(!BRepCheck_Analyzer{e.Current()}.IsValid()||volume(e.Current())<=1e-9)throw std::runtime_error("Hole produced an invalid fuselage wall.");++count;
    }
    if(!count)throw std::runtime_error("Holes remove the entire fuselage.");
    if(volume(result)-volume(remaining)<=1e-7)throw std::runtime_error("Hole does not remove material from the selected wall; check its position or overlapping holes.");
    result=remaining;
  }
  return result;
}
}

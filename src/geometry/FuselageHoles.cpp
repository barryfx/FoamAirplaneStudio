#include "geometry/FuselageHoles.h"
#include "geometry/FuselageProcessing.h"
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
#include <TopoDS.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
namespace {
std::array<double,6> bounds(const TopoDS_Shape& shape,const ProcessingControl& control){Bnd_Box b;fuselageBounds(shape,b,control);std::array<double,6> r;b.Get(r[0],r[1],r[2],r[3],r[4],r[5]);return r;}
double volume(const TopoDS_Shape& shape,const ProcessingControl& control){GProp_GProps mass;fuselageVolumeProperties(shape,mass,control);return mass.Mass();}
bool parallelWallInterference(const TopoDS_Shape& rim,const TopoDS_Shape& body,const TopoDS_Shape& cavity,
    int axis,bool positive,double extent,const ProcessingControl& control) {
  control.checkpoint();IntCurvesFace_ShapeIntersector ray,material;ray.Load(cavity,1e-7);bool materialLoaded=false;
  const gp_Dir direction=axis==2?gp_Dir{0,0,positive?-1.:1.}:gp_Dir{0,positive?-1.:1.,0};
  // Diagnose a failed isolation independently of its Boolean result. A path
  // from the hole rim that never reaches the cavity crosses side/end-wall
  // material along the cutting direction. Sampling may miss a narrow region,
  // so it only supplies a positive diagnosis; it never approves a failed cut.
  for(TopExp_Explorer e{rim,TopAbs_EDGE};e.More();e.Next()) {
    BRepAdaptor_Curve curve{TopoDS::Edge(e.Current())};
    for(int i=0;i<=32;++i) {
      control.checkpoint();const double t=curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*i/32.;
      const gp_Lin path{curve.Value(t),direction};
      ray.PerformNearest(path,0,extent);control.checkpoint();
      if(ray.IsDone()&&ray.NbPnt()==0) {
        if(!materialLoaded){material.Load(body,1e-7);materialLoaded=true;}
        material.Perform(path,0,extent);control.checkpoint();
        if(material.IsDone()&&material.NbPnt()>=2)return true;
      }
    }
  }
  return false;
}
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
  auto result=body;const auto b=bounds(body,control);const double margin=std::max({b[3]-b[0],b[4]-b[1],b[5]-b[2],1.});
  for(int wall=0;wall<4;++wall) {
    const auto paths=gui::sketchPaths(holes[wall]);
    for(std::size_t index=0;index<paths.size();++index) {
    control.checkpoint();const auto& layer=paths[index].layer;
    const std::string location=std::string{"Fuselage > Holes: "}+std::array{"Top","Bottom","Left","Right"}[wall]+" hole "+std::to_string(index+1);
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
    if(!face.IsDone()||!fuselageValid(face.Face(),control))throw std::runtime_error("A hole must be a simple closed loop without self-intersections.");
    const auto prism=BRepPrimAPI_MakePrism{face.Face(),view==0?gp_Vec{0,0,far-near}:gp_Vec{0,far-near,0}}.Shape();
    // The cavity separates the through-prism into near and far components.
    // Only the solid touching the chosen exterior starting plane may remove
    // material. If it reaches the far plane too, this hole can cut both walls.
    const auto separated=cut(prism,cavity,control);TopoDS_Shape tool;
    for(TopExp_Explorer e{separated,TopAbs_SOLID};e.More();e.Next()) {
      const auto box=bounds(e.Current(),control);const double start=positive?box[axis+3]:box[axis];
      if(std::abs(start-near)>1e-5)continue;
      const double end=positive?box[axis]:box[axis+3];
      if(std::abs(end-far)<1e-5||!tool.IsNull()) {
        if(parallelWallInterference(wire.Wire(),result,cavity,axis,positive,std::abs(far-near),control))
          throw std::runtime_error(location+" overlaps a wall parallel to the cut direction. Move or shrink it to fit over the inner cavity.");
        throw std::runtime_error(location+" could not be isolated from the opposite wall. Check its inner-cavity clearance; the CAD cut may also have failed.");
      }
      tool=e.Current();
    }
    if(tool.IsNull())throw std::runtime_error(location+": the CAD cut could not isolate the selected wall.");
    const auto remaining=cut(result,tool,control);int count=0;
    for(TopExp_Explorer e{remaining,TopAbs_SOLID};e.More();e.Next()) {
      if(!fuselageValid(e.Current(),control)||volume(e.Current(),control)<=1e-9)throw std::runtime_error("Hole produced an invalid fuselage wall.");++count;
    }
    if(!count)throw std::runtime_error("Holes remove the entire fuselage.");
    if(volume(result,control)-volume(remaining,control)<=1e-7)throw std::runtime_error("Hole does not remove material from the selected wall; check its position or overlapping holes.");
    result=remaining;
    }
  }
  return result;
}
}

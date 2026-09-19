#include "geometry/StabilizerCut.h"
#include "gui/SketchBoundary.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <Geom_BSplineCurve.hxx>
#include <NCollection_HArray1.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Ax2.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
TopoDS_Shape cutStabilizerShapes(const TopoDS_Shape& body,const std::vector<gui::SketchLayer>& loops,
    bool horizontal,const ProcessingControl& processing,bool allowEmpty) {
  auto result=body;
  Bnd_Box bounds;BRepBndLib::Add(body,bounds);double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
  const double margin=std::max({x1-x0,y1-y0,z1-z0,1.});
  for(const auto& layer:loops) {
    processing.checkpoint();if(layer.curves.empty()&&layer.points.empty())continue;
    if(!gui::closedSketchBoundary(layer))throw std::runtime_error("Close or delete every incomplete Cut Shape before generating the stabilizer.");
    auto point=[&](std::size_t id){const auto p=layer.points[id];return gp_Pnt{p.x(),p.y(),z0-margin};};
    BRepBuilderAPI_MakeWire wire;std::vector<bool> used(layer.curves.size());auto current=layer.curves[0].points.front();
    for(std::size_t n=0;n<layer.curves.size();++n) {
      processing.checkpoint();std::size_t next=layer.curves.size();bool reverse=false;
      for(std::size_t i=0;i<layer.curves.size();++i)if(!used[i]) {
        if(layer.curves[i].points.front()==current){next=i;break;}
        if(layer.curves[i].points.back()==current){next=i;reverse=true;break;}
      }
      if(next==layer.curves.size())throw std::runtime_error("Cut Shape segments must form one closed loop.");
      used[next]=true;const auto& curve=layer.curves[next];auto ids=curve.points;if(reverse)std::reverse(ids.begin(),ids.end());current=ids.back();
      if(curve.type==gui::SketchTool::Line || ids.size()==2)wire.Add(BRepBuilderAPI_MakeEdge{point(ids.front()),point(ids.back())}.Edge());
      else {
        const bool periodic=ids.size()>=4&&ids.front()==ids.back();const int count=static_cast<int>(ids.size())-(periodic?1:0);
        occ::handle<NCollection_HArray1<gp_Pnt>> nodes=new NCollection_HArray1<gp_Pnt>{1,count};
        for(int i=0;i<count;++i)nodes->SetValue(i+1,point(ids[i]));
        GeomAPI_Interpolate fit{nodes,periodic,1e-9};fit.Perform();
        if(!fit.IsDone())throw std::runtime_error("Cut Shape spline could not be fitted.");
        wire.Add(BRepBuilderAPI_MakeEdge{fit.Curve()}.Edge());
      }
      if(!wire.IsDone())throw std::runtime_error("Cut Shape segments do not connect.");
    }
    BRepBuilderAPI_MakeFace face{wire.Wire(),true};
    if(!face.IsDone()||!BRepCheck_Analyzer{face.Face()}.IsValid())throw std::runtime_error("Cut Shape must be a simple closed loop without self-intersections.");
    const auto tool=BRepPrimAPI_MakePrism{face.Face(),gp_Vec{0,0,z1-z0+2*margin}}.Shape();
    for(int side=0;side<(horizontal?2:1);++side) {
      auto cutter=tool;
      if(side){gp_Trsf mirror;mirror.SetMirror(gp_Ax2{gp_Pnt{0,0,0},gp_Dir{0,1,0}});cutter=BRepBuilderAPI_Transform{tool,mirror,true}.Shape();}
      BRepAlgoAPI_Cut cut{result,cutter,processing.range()};processing.checkpoint();
      if(!cut.IsDone()||cut.HasErrors())throw std::runtime_error("Could not apply Cut Shape. Check the loop for overlaps or self-intersections.");
      BRep_Builder builder;TopoDS_Compound remaining;builder.MakeCompound(remaining);int count=0;
      for(TopExp_Explorer e{cut.Shape(),TopAbs_SOLID};e.More();e.Next()) {
        processing.checkpoint();GProp_GProps volume;BRepGProp::VolumeProperties(e.Current(),volume);
        if(!BRepCheck_Analyzer{e.Current()}.IsValid() || !std::isfinite(volume.Mass()) || volume.Mass()<=1e-9)
          throw std::runtime_error("Cut Shape produced an invalid remaining body.");
        builder.Add(remaining,e.Current());++count;
      }
      if(!count && allowEmpty)return {};
      if(!count)throw std::runtime_error("Cut Shapes remove the entire stabilizer. Reduce or move the cut.");
      result=remaining;
    }
  }
  return result;
}
}

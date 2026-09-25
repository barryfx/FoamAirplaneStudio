#include "geometry/FuselageSymmetry.h"
#include "geometry/FuselageProcessing.h"
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepLib.hxx>
#include <Bnd_Box.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>
#include <cmath>
#include <stdexcept>

namespace designrc::geometry {
bool symmetricFuselageSection(const std::vector<QPointF>& p) {
  if(p.empty())return true; // A point-shaped tip is checked by the caller.
  if(p.size()!=64)return false;
  for(std::size_t i=0;i<p.size();++i) {
    const auto other=p[(p.size()-i)%p.size()];
    if(std::abs(p[i].x()+other.x())>1e-7||std::abs(p[i].y()-other.y())>1e-7)return false;
    if(i<=32&&p[i].x()<-1e-7)return false;
  }
  return true;
}
std::vector<QPointF> rightFuselageSection(const std::vector<QPointF>& p) {
  if(!symmetricFuselageSection(p)||p.empty())
    throw std::runtime_error("A mirrored fuselage section must be symmetric about its centre plane.");
  std::vector<QPointF> result(p.begin(),p.begin()+33);
  result.front().setX(0);result.back().setX(0);
  return result;
}
TopoDS_Shape reflectFuselage(const TopoDS_Shape& shape,const ProcessingControl& processing) {
  processing.checkpoint();gp_Trsf reflection;reflection.SetMirror(gp_Ax2{gp_Pnt{0,0,0},gp_Dir{0,1,0}});
  const auto result=BRepBuilderAPI_Transform{shape,reflection,true}.Shape();
  processing.checkpoint();return result;
}
TopoDS_Shape fuselagePair(const TopoDS_Shape& right,const ProcessingControl& processing) {
  BRep_Builder builder;TopoDS_Compound pair;builder.MakeCompound(pair);
  builder.Add(pair,right);builder.Add(pair,reflectFuselage(right,processing));return pair;
}
TopoDS_Shape joinMirroredFuselage(const TopoDS_Shape& half,const ProcessingControl& processing) {
  processing.checkpoint();
  const auto reflected=reflectFuselage(half,processing);
  BRepBuilderAPI_Sewing sewing{1e-7};
  for(const auto& side:{half,reflected})for(TopExp_Explorer e{side,TopAbs_FACE};e.More();e.Next()) {
    processing.checkpoint();
    Bnd_Box box;fuselageBounds(e.Current(),box,processing);
    double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    if(std::abs(y0)<=1e-7&&std::abs(y1)<=1e-7)continue;
    sewing.Add(e.Current());
  }
  {auto range=processing.range();sewing.Perform(range);}processing.checkpoint();
  TopExp_Explorer shells{sewing.SewedShape(),TopAbs_SHELL};
  if(!shells.More())throw std::runtime_error("Mirrored fuselage skin could not be joined.");
  const auto shell=TopoDS::Shell(shells.Current());shells.Next();
  if(shells.More())throw std::runtime_error("Mirrored fuselage skin contains disconnected shells.");
  auto solid=BRepBuilderAPI_MakeSolid{shell}.Solid();
  if(!BRepLib::OrientClosedSolid(solid)||!fuselageValid(solid,processing))
    throw std::runtime_error("Mirrored fuselage skin is not a valid closed solid.");
  return solid;
}
}

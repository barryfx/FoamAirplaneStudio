#include "geometry/StabilizerSolidBuilder.h"
#include "gui/StabilizerOutlinePanel.h"
#include "gui/SketchBoundary.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Builder.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace designrc::geometry {
namespace {
TopoDS_Wire section(const std::vector<domain::Point2>& foil, double leading, double trailing,
                    double span, double retention) {
  const double chord = trailing - leading;
  auto point = [&](std::size_t i) {
    double z = foil[i].y * chord;
    if (i == 0) z = std::max(z, chord * 1e-6);
    if (i + 1 == foil.size()) z = std::min(z, -chord * 1e-6);
    const double other = foil[foil.size()-1-i].y * chord;
    const double center = (z + other) / 2;
    z = center + (z - center) * retention;
    if (i == 0) z = std::max(z, center + 1e-5);
    if (i + 1 == foil.size()) z = std::min(z, center - 1e-5);
    return gp_Pnt{leading + foil[i].x * chord, span, z};
  };
  BRepBuilderAPI_MakeWire wire;
  const auto mid = foil.size() / 2;
  for (auto range : {std::pair<std::size_t,std::size_t>{0,mid}, {mid,foil.size()-1}}) {
    Handle(TColgp_HArray1OfPnt) nodes = new TColgp_HArray1OfPnt(1, static_cast<int>(range.second-range.first+1));
    for (auto i = range.first; i <= range.second; ++i) nodes->SetValue(static_cast<int>(i-range.first+1), point(i));
    GeomAPI_Interpolate fit{nodes,false,1e-8}; fit.Perform();
    if (!fit.IsDone()) throw std::runtime_error("Stabilizer airfoil interpolation failed.");
    wire.Add(BRepBuilderAPI_MakeEdge{fit.Curve()}.Edge());
  }
  wire.Add(BRepBuilderAPI_MakeEdge{point(foil.size()-1),point(0)}.Edge());
  if (!wire.IsDone()) throw std::runtime_error("Stabilizer section could not be closed.");
  return wire.Wire();
}
}
TopoDS_Shape buildStabilizerSolid(const StabilizerSolidInput& input,
    const std::function<void(const char*)>& progress, const ProcessingControl& control) {
  const auto report = [&](const char* message) { control.checkpoint(); if(progress) progress(message); };
  report("Stabilizer: sampling outline...");
  if (!gui::stabilizerOutlineDefined(input.outline))
    throw std::runtime_error("Define one open stabilizer outline with aligned endpoints, a selected leading-edge endpoint and nonzero area.");
  if (!std::isfinite(input.millimetersPerSceneUnit) || input.millimetersPerSceneUnit <= 0)
    throw std::runtime_error("Stabilizer reference scale must be positive.");
  std::vector<int> degree(input.outline.points.size());
  for (const auto& curve : input.outline.curves)
    for (std::size_t i=1;i<curve.points.size();++i) {++degree[curve.points[i-1]];++degree[curve.points[i]];}
  std::vector<std::size_t> ends;
  for (std::size_t i=0;i<degree.size();++i) if(degree[i]==1) ends.push_back(i);
  auto a=input.outline.points[ends[0]], b=input.outline.points[ends[1]];
  if (*input.outline.leadingEdge == ends[1]) std::swap(a,b);
  const auto chord=(b-a)/std::hypot(b.x()-a.x(),b.y()-a.y());
  QPointF span{-chord.y(),chord.x()};
  auto closed=input.outline; closed.curves.push_back({gui::SketchTool::Line,{ends[0],ends[1]}});
  const auto boundary=*gui::closedSketchBoundary(closed);
  double signedExtent=0;
  for (auto p:boundary) {
    const double y=QPointF::dotProduct(p-a,span);
    if(std::abs(y)>std::abs(signedExtent)) signedExtent=y;
  }
  if(signedExtent<0) span=-span;
  const auto map=[&](QPointF p){p-=a;return QPointF{QPointF::dotProduct(p,chord)*input.millimetersPerSceneUnit,
      QPointF::dotProduct(p,span)*input.millimetersPerSceneUnit};};
  std::vector<QPointF> polygon; double tip=0;
  for(auto p:boundary) {auto q=map(p);polygon.push_back(q);tip=std::max(tip,q.y());}
  if(tip<=1e-6) throw std::runtime_error("Stabilizer has no span.");
  for(auto p:polygon) if(p.y() < -tip*1e-6)
    throw std::runtime_error("The stabilizer outline must lie on one side of its root line.");
  auto sectionBounds=[&](double y) {
    std::vector<double> hits;
    for(std::size_t i=1;i<polygon.size();++i) {
      const auto p=polygon[i-1], q=polygon[i];
      if(std::abs(q.y()-p.y())<1e-10) {
        if(std::abs(y-p.y())<1e-8) {hits.push_back(p.x());hits.push_back(q.x());}
      } else if(y>=std::min(p.y(),q.y())-1e-9 && y<=std::max(p.y(),q.y())+1e-9) {
        const double f=std::clamp((y-p.y())/(q.y()-p.y()),0.,1.);hits.push_back(p.x()+f*(q.x()-p.x()));
      }
    }
    std::sort(hits.begin(),hits.end());
    hits.erase(std::unique(hits.begin(),hits.end(),[](double x,double y){return std::abs(x-y)<1e-7;}),hits.end());
    if(hits.empty() || hits.size()>2) throw std::runtime_error("The stabilizer outline has ambiguous or disconnected chord sections.");
    return std::pair{hits.front(),hits.back()};
  };
  const auto foil=input.airfoil.resampled(25);
  double thickness=0;
  for(std::size_t i=0;i<foil.size();++i) thickness=std::max(thickness,std::abs(foil[i].y-foil[foil.size()-1-i].y));
  if(!std::isfinite(thickness)||thickness<1e-9) throw std::runtime_error("Stabilizer airfoil has no thickness.");
  std::vector<double> sections;
  for(int i=0;i<=32;++i) sections.push_back(tip*(1-std::cos(std::numbers::pi*i/32))*.5);
  for(auto p:input.outline.points) {auto q=map(p);if(q.y()>0 && q.y()<tip)sections.push_back(q.y());}
  std::sort(sections.begin(),sections.end());
  sections.erase(std::unique(sections.begin(),sections.end(),[&](double x,double y){return std::abs(x-y)<tip*1e-8;}),sections.end());
  report("Stabilizer: constructing airfoil sections and rounded tip...");
  BRepOffsetAPI_ThruSections loft{true,true,1e-7}; loft.CheckCompatibility(false);
  for(double y:sections) {
    control.checkpoint();const auto [le,te]=sectionBounds(y);
    if(te-le<1e-6) {
      if(y<tip-1e-6)throw std::runtime_error("The stabilizer outline pinches before its tip.");
      loft.AddVertex(BRepBuilderAPI_MakeVertex{gp_Pnt{le,y,0}}.Vertex());
    } else {
      const double radius=std::max((te-le)*thickness*.5,1e-6);
      const double distance=std::clamp((tip-y)/radius,0.,1.);
      const double retention=std::max(1e-3,std::sqrt(std::max(0.,1-(1-distance)*(1-distance))));
      loft.AddWire(section(foil,le,te,y,retention));
    }
  }
  report("Stabilizer: lofting solid...");loft.Build(control.range());control.checkpoint();
  if(!loft.IsDone())throw std::runtime_error("Stabilizer loft failed.");
  auto shape=loft.Shape();
  report("Stabilizer: validating solid...");
  GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass);
  if(!BRepCheck_Analyzer{shape}.IsValid() || !std::isfinite(mass.Mass()) || mass.Mass()<=1e-9)
    throw std::runtime_error("Stabilizer loft is not a valid positive-volume solid.");
  if(input.horizontal) {
    report("Horizontal stabilizer: mirroring right half...");
    gp_Trsf mirror;mirror.SetMirror(gp_Ax2{gp_Pnt{0,0,0},gp_Dir{0,1,0}});
    BRep_Builder builder;TopoDS_Compound both;builder.MakeCompound(both);builder.Add(both,shape);
    builder.Add(both,BRepBuilderAPI_Transform{shape,mirror,true}.Shape());shape=both;
  } else {
    report("Vertical stabilizer: orienting fin...");
    gp_Trsf rotation;rotation.SetRotation(gp_Ax1{gp_Pnt{0,0,0},gp_Dir{1,0,0}},std::numbers::pi/2);
    shape=BRepBuilderAPI_Transform{shape,rotation,true}.Shape();
  }
  report("Stabilizer: meshing display...");
  IMeshTools_Parameters parameters;parameters.Deflection=.1;parameters.Angle=.25;parameters.InParallel=false;
  BRepMesh_IncrementalMesh mesh{shape,parameters,control.range()};control.checkpoint();
  if(!mesh.IsDone())throw std::runtime_error("Stabilizer display meshing failed.");
  report("Stabilizer: model ready.");return shape;
}
}

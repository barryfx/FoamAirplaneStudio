#include "geometry/ControlSurfaceCut.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
namespace {
int solids(const TopoDS_Shape& shape) {
  int count=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++count;return count;
}
TopoDS_Shape subtract(const TopoDS_Shape& body,const TopoDS_Shape& tool,const ProcessingControl& processing) {
  // Configure parallelism before the single Build call.
  BRepAlgoAPI_Cut cut;NCollection_List<TopoDS_Shape> args,tools;args.Append(body);tools.Append(tool);
  cut.SetArguments(args);cut.SetTools(tools);cut.SetRunParallel(processing.parallel);cut.Build(processing.range());processing.checkpoint();
  if(!cut.IsDone())throw std::runtime_error("OCCT could not cut a control surface.");
  return cut.Shape();
}
void valid(const TopoDS_Shape& shape,const char* message,bool parallel) {
  if(shape.IsNull() || solids(shape)!=1 || !BRepCheck_Analyzer{shape,true,parallel}.IsValid())throw std::runtime_error(message);
}
}
TopoDS_Shape cutControlSurfaces(const TopoDS_Shape& half,
    const std::array<gui::ControlSurface,2>& controls,QPointF chordAxis,
    QPointF spanAxis,double root,double scale,double endClearanceMm,const ProcessingControl& processing) {
  processing.checkpoint();
  if(!std::isfinite(scale) || scale<=0 || !std::isfinite(endClearanceMm) || endClearanceMm<0)
    throw std::runtime_error("Invalid control surface scale or clearance.");
  if(gui::controlSurfacesOverlap(controls))
    throw std::runtime_error("Aileron and flap rectangles must not overlap.");
  auto model=[&](QPointF p,double z) {return gp_Pnt{QPointF::dotProduct(p,chordAxis)*scale,
      (QPointF::dotProduct(p,spanAxis)-root)*scale,z};};
  Bnd_Box bounds;BRepBndLib::Add(half,bounds);
  double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
  const double margin=2*std::max({x1-x0,y1-y0,z1-z0,1.0})+std::max(std::abs(z0),std::abs(z1))+1;
  TopoDS_Shape fixed=half;std::vector<TopoDS_Shape> moving;
  for(const auto& control:controls) {
    if(!control.enabled || !control.rectangle)continue;
    const auto rect=control.rectangle->normalized();
    if(rect.width()*scale<1e-5 || rect.height()*scale<1e-5)
      throw std::runtime_error("The control surface rectangle is too small.");
    QPointF start,end,inward;
    // Rectangles follow scene axes. Choose the spanwise side facing the LE.
    if(std::abs(chordAxis.y())>=std::abs(chordAxis.x())) {
      const bool forward=chordAxis.y()>0;
      start=forward?rect.topLeft():rect.bottomLeft();end=forward?rect.topRight():rect.bottomRight();
      inward={0,forward?1.0:-1.0};
    } else {
      const bool forward=chordAxis.x()>0;
      start=forward?rect.topLeft():rect.topRight();end=forward?rect.bottomLeft():rect.bottomRight();
      inward={forward?1.0:-1.0,0};
    }
    const double depth=std::abs(inward.x())>0?rect.width():rect.height();
    const auto origin=model(start,0);const gp_Vec tangent{origin,model(end,0)};
    const gp_Vec normal{origin,model(start+inward,0)};
    const gp_Vec direction=normal.Normalized();
    IntCurvesFace_ShapeIntersector ray;ray.Load(half,1e-7);
    auto heights=[&](QPointF p)->std::optional<std::pair<double,double>> {
      processing.checkpoint();ray.Perform(gp_Lin{model(p,-margin),gp_Dir{0,0,1}},0,2*margin);
      if(!ray.IsDone() || ray.NbPnt()<2)return {};
      double low=margin,high=-margin;
      for(int i=1;i<=ray.NbPnt();++i){low=std::min(low,ray.Pnt(i).Z());high=std::max(high,ray.Pnt(i).Z());}
      if(high-low<1e-7)return {};return std::pair{low,high};
    };
    struct Sample {double t,anchor;};std::vector<Sample> samples;
    for(int i=0;i<=32;++i) {
      const double t=i/32.0;const auto p=start+t*(end-start);const auto h=heights(p);
      if(!h)continue;
      if(heights(p+depth*inward))
        throw std::runtime_error("Extend the control surface rectangle beyond the trailing edge of the wing.");
      samples.push_back({t,control.hinge==gui::HingeCut::Tape?h->second:(h->first+h->second)*0.5});
    }
    if(samples.size()<2)throw std::runtime_error("Place the rectangle hinge edge inside the wing, with the rectangle extending past its trailing edge.");
    // Continue sampled hinge heights over the overhanging ends of the rectangle.
    if(samples.front().t>0)samples.insert(samples.begin(),{0,samples.front().anchor});
    if(samples.back().t<1)samples.push_back({1,samples.back().anchor});
    for(std::size_t i=1;i+1<samples.size();) {
      const auto a=samples[i-1],b=samples[i],c=samples[i+1];
      const double linear=a.anchor+(c.anchor-a.anchor)*(b.t-a.t)/(c.t-a.t);
      if(std::abs(linear-b.anchor)<1e-6)samples.erase(samples.begin()+i);else ++i;
    }
    // Keep the full opening in the fixed wing, but inset the moving body's
    // two spanwise ends. This creates physical clearance without extra Booleans.
    auto inset=rect;
    const double gap=endClearanceMm/scale;
    if(std::abs(inward.y())>0)inset.adjust(gap,0,-gap,0);
    else inset.adjust(0,gap,0,-gap);
    if(inset.width()*scale<1e-5 || inset.height()*scale<1e-5)
      throw std::runtime_error("The control surface is too narrow for the end clearances.");
    auto makePrism=[&](const QRectF& area) {
      BRepBuilderAPI_MakePolygon polygon;
      for(auto p:{area.topLeft(),area.topRight(),area.bottomRight(),area.bottomLeft()})polygon.Add(model(p,-margin));
      polygon.Close();
      return BRepPrimAPI_MakePrism{BRepBuilderAPI_MakeFace{polygon.Wire()}.Face(),gp_Vec{0,0,2*margin}}.Shape();
    };
    const auto prism=makePrism(rect);
    const auto movingPrism=endClearanceMm==0?prism:makePrism(inset);
    BRepAlgoAPI_Common separate;NCollection_List<TopoDS_Shape> args,tools;args.Append(fixed);tools.Append(movingPrism);
    separate.SetArguments(args);separate.SetTools(tools);separate.SetRunParallel(processing.parallel);separate.Build(processing.range());processing.checkpoint();
    if(!separate.IsDone())throw std::runtime_error("OCCT could not separate the control surface.");
    auto part=separate.Shape();valid(part,"The rectangle must select one connected control surface.",processing.parallel);
    fixed=subtract(fixed,prism,processing);valid(fixed,"The rectangle must leave one connected main wing body.",processing.parallel);
    auto bevel=[&](bool upper) {
      BRepOffsetAPI_ThruSections loft{true,true,1e-7};loft.CheckCompatibility(false);
      for(const auto sample:samples) {
        const auto at=origin.Translated(tangent*sample.t);
        auto point=[&](double d,double z) {return at.Translated(direction*d+gp_Vec{0,0,z});};
        BRepBuilderAPI_MakePolygon wire;
        if(!upper) {
          wire.Add(point(-margin,-margin));wire.Add(point(sample.anchor+margin,-margin));
          wire.Add(point(0,sample.anchor));wire.Add(point(-margin,sample.anchor));
        } else {
          wire.Add(point(-margin,sample.anchor));wire.Add(point(0,sample.anchor));
          wire.Add(point(margin-sample.anchor,margin));wire.Add(point(-margin,margin));
        }
        wire.Close();loft.AddWire(wire.Wire());
      }
      loft.Build(processing.range());processing.checkpoint();if(!loft.IsDone())throw std::runtime_error("OCCT could not form the hinge bevel.");
      part=subtract(part,loft.Shape(),processing);
    };
    bevel(false);if(control.hinge==gui::HingeCut::Standard)bevel(true);
    valid(part,"The hinge bevel consumes or disconnects the control surface; use a deeper rectangle.",processing.parallel);
    moving.push_back(part);
  }
  if(moving.empty())return half;
  BRep_Builder builder;TopoDS_Compound result;builder.MakeCompound(result);builder.Add(result,fixed);
  for(const auto& part:moving)builder.Add(result,part);
  return result;
}
}

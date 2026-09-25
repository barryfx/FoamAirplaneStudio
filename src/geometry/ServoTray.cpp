#include "geometry/ServoTray.h"
#include "geometry/FuselageProcessing.h"
#include "geometry/FuselageSymmetry.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Trsf.hxx>
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
namespace {
std::array<double,6> bounds(const TopoDS_Shape& shape,const ProcessingControl& processing){Bnd_Box b;fuselageBounds(shape,b,processing);std::array<double,6> v;b.Get(v[0],v[1],v[2],v[3],v[4],v[5]);return v;}
template<class Operation> TopoDS_Shape booleanOp(const TopoDS_Shape& a,const TopoDS_Shape& b,const char* failure,const ProcessingControl& processing) {
  processing.checkpoint();Operation op;NCollection_List<TopoDS_Shape> args,tools;args.Append(a);tools.Append(b);
  op.SetArguments(args);op.SetTools(tools);op.SetNonDestructive(true);op.SetRunParallel(processing.parallel);op.SetFuzzyValue(1e-7);
  {auto range=processing.range();op.Build(range);}processing.checkpoint();
  if(!op.IsDone()||op.HasErrors()||op.Shape().IsNull())throw std::runtime_error(failure);
  return op.Shape();
}
TopoDS_Shape oneSolid(const TopoDS_Shape& shape,const char* failure,const ProcessingControl& processing) {
  TopExp_Explorer solids{shape,TopAbs_SOLID};if(!solids.More())throw std::runtime_error(failure);
  const auto solid=solids.Current();solids.Next();if(solids.More()||!fuselageValid(solid,processing))throw std::runtime_error(failure);
  GProp_GProps mass;fuselageVolumeProperties(solid,mass,processing);if(mass.Mass()<=1e-8)throw std::runtime_error(failure);return solid;
}
TopoDS_Shape translated(const TopoDS_Shape& shape,double y){gp_Trsf t;t.SetTranslation(gp_Vec{0,y,0});return BRepBuilderAPI_Transform{shape,t,true}.Shape();}
}
ServoTrayGeometry addServoTray(const TopoDS_Shape& body,const TopoDS_Shape& cavity,const QRectF& r,
    const std::function<void(const char*)>& progress,const ProcessingControl& processing,bool rightHalfOnly) {
  processing.checkpoint();
  if(cavity.IsNull())throw std::runtime_error("Enter Thicken before generating a servo tray; the tray needs inner fuselage walls.");
  if(!std::isfinite(r.x())||!std::isfinite(r.y())||!std::isfinite(r.width())||!std::isfinite(r.height())||r.width()<=0||r.height()<=0)
    throw std::runtime_error("Draw a nonzero Servo Tray rectangle on Side View.");
  const auto cb=bounds(cavity,processing);const double margin=std::max(1.,cb[4]-cb[1]);
  auto slab=[&](double bottom,double height){return BRepPrimAPI_MakeBox{gp_Pnt{r.left(),cb[1]-margin,bottom},r.width(),cb[4]-cb[1]+2*margin,height}.Shape();};
  if(progress)progress("Fuselage: fitting the servo tray to the inner walls...");
  auto tray=oneSolid(booleanOp<BRepAlgoAPI_Common>(cavity,slab(r.top(),r.height()),"Could not fit the servo tray to the cavity.",processing),
      "The servo tray must fit inside one connected cavity. Move or resize the rectangle.",processing);
  const auto tb=bounds(tray,processing);
  if(std::abs(tb[0]-r.left())>1e-5||std::abs(tb[3]-r.right())>1e-5||std::abs(tb[2]-r.top())>1e-5||std::abs(tb[5]-r.bottom())>1e-5)
    throw std::runtime_error("The Servo Tray rectangle extends outside the inner cavity. Move or resize it.");
  if(progress)progress(rightHalfOnly?"Fuselage: adding the right 5 mm inward, 5 mm high servo-tray support...":"Fuselage: adding 5 mm inward, 5 mm high servo-tray supports...");
  auto pocket=oneSolid(booleanOp<BRepAlgoAPI_Common>(cavity,slab(r.top()-5,5),"Could not locate the servo-tray support region.",processing),
      "There is no connected space for supports below the servo tray. Move the tray upward.",processing);
  const auto pb=bounds(pocket,processing);
  if(std::abs(pb[0]-r.left())>1e-5||std::abs(pb[3]-r.right())>1e-5||std::abs(pb[2]-(r.top()-5))>1e-5||std::abs(pb[5]-r.top())>1e-5)
    throw std::runtime_error("The servo tray needs 5 mm of cavity height below it for the support ledges.");
  // Subtract horizontally shifted copies of the cavity slice. The residual
  // bands extend exactly 5 mm inward from each inner side, following taper.
  auto right=booleanOp<BRepAlgoAPI_Cut>(pocket,translated(pocket,-5),"Could not construct the right servo-tray support.",processing);
  oneSolid(right,"Right servo-tray support is disconnected.",processing);
  BRep_Builder builder;TopoDS_Compound supports;builder.MakeCompound(supports);builder.Add(supports,right);
  if(!rightHalfOnly) {
    auto left=booleanOp<BRepAlgoAPI_Cut>(pocket,translated(pocket,5),"Could not construct the left servo-tray support.",processing);
    oneSolid(left,"Left servo-tray support is disconnected.",processing);builder.Add(supports,left);
  }
  auto supported=oneSolid(booleanOp<BRepAlgoAPI_Fuse>(body,supports,"Could not join the servo-tray supports to the fuselage.",processing),
      "Servo-tray supports did not join the fuselage walls.",processing);
  // Keep the exact horizontal upper faces, including their wires, for the
  // future laser-outline exporter. Never fuse the removable tray into the body.
  TopoDS_Compound top;builder.MakeCompound(top);int faces=0;
  for(TopExp_Explorer e{tray,TopAbs_FACE};e.More();e.Next()) {
    const auto fb=bounds(e.Current(),processing);if(std::abs(fb[2]-r.bottom())<1e-6&&std::abs(fb[5]-r.bottom())<1e-6){builder.Add(top,e.Current());++faces;}
  }
  if(!faces)throw std::runtime_error("Could not retain a planar top outline for the servo tray.");
  processing.checkpoint();return {supported,tray,top};
}
}

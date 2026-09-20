#include "geometry/Formers.h"
#include "gui/FormerEditor.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <cmath>
#include <numbers>
#include <gp_Ax1.hxx>
#include <stdexcept>
namespace designrc::geometry {
namespace {
void validateAngles(const std::vector<QRectF>& rectangles,const std::vector<double>& angles) {
  if(!angles.empty()&&angles.size()!=rectangles.size())throw std::runtime_error("Former rotation count does not match former count.");
  for(double angle:angles)if(!std::isfinite(angle)||std::abs(angle)>360)throw std::runtime_error("Former rotation must be between -360 and 360 degrees.");
}
TopoDS_Shape rotated(const TopoDS_Shape& shape,const QRectF& r,double degrees) {
  if(degrees==0)return shape;
  // Model Z is the negative of scene Y. Positive rotation around +Y is
  // clockwise in Side View, matching the editor's screen-coordinate transform.
  gp_Trsf t;t.SetRotation(gp_Ax1{gp_Pnt{r.center().x(),0,r.center().y()},gp_Dir{0,1,0}},degrees*std::numbers::pi/180.);
  return BRepBuilderAPI_Transform{shape,t,true}.Shape();
}
template<class Operation> TopoDS_Shape booleanOp(const TopoDS_Shape& a,const TopoDS_Shape& b,const ProcessingControl& processing){
  processing.checkpoint();Operation op;NCollection_List<TopoDS_Shape> args,tools;args.Append(a);tools.Append(b);op.SetArguments(args);op.SetTools(tools);op.SetNonDestructive(true);op.SetRunParallel(processing.parallel);op.SetFuzzyValue(1e-7);
  {auto range=processing.range();op.Build(range);}processing.checkpoint();
  if(!op.IsDone()||op.HasErrors()||op.Shape().IsNull())throw std::runtime_error("Could not fit a former to the inner cavity.");return op.Shape();
}
}
TopoDS_Shape addFormerRetainers(const TopoDS_Shape& body,const TopoDS_Shape& cavity,
    const std::vector<QRectF>& rectangles,const std::vector<TopoDS_Shape>& inserts,
    const std::function<void(const char*)>& progress,const ProcessingControl& processing,const std::vector<double>& rotationDegrees) {
  validateAngles(rectangles,rotationDegrees);if(rectangles.empty())return body;
  if(cavity.IsNull())throw std::runtime_error("Former retainers require an inner cavity.");
  if(progress)progress("Fuselage: adding 4 mm by 3 mm former retaining rails on both sides...");
  Bnd_Box box;BRepBndLib::AddOptimal(cavity,box,false,false);
  double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
  std::vector<Bnd_Box> insertBounds;
  for(const auto& insert:inserts) {
    Bnd_Box bounds;if(!insert.IsNull())BRepBndLib::AddOptimal(insert,bounds,false,false);
    bounds.SetGap(1e-7);insertBounds.push_back(bounds);
  }
  NCollection_List<TopoDS_Shape> rails;
  std::size_t formerIndex=0;
  for(const auto& r:rectangles) {
    const double angle=gui::formerAngle(rotationDegrees,formerIndex++);
    const auto status="Fuselage: constructing retaining rails for former "+std::to_string(formerIndex)+"...";
    if(progress)progress(status.c_str());
    for(double x:{r.left()-4.,r.right()}) {
      processing.checkpoint();
      // Extend in the former's local height direction far enough to cover the
      // cavity after rotation. Keep the zero-angle path exactly as before.
      const double radius=std::hypot(std::max(std::abs(x0-r.center().x()),std::abs(x1-r.center().x())),std::max(std::abs(z0-r.center().y()),std::abs(z1-r.center().y())))+1;
      const double bottom=angle==0?z0-1:r.center().y()-radius,top=angle==0?z1+1:r.center().y()+radius;
      const auto slice=[&](double padding) {
        return rotated(BRepPrimAPI_MakeBox{gp_Pnt{x-padding,y0-1,bottom},4.+2*padding,y1-y0+2,top-bottom}.Shape(),r,angle);
      };
      const auto slab=slice(0);
      const auto pocket=booleanOp<BRepAlgoAPI_Common>(cavity,slab,processing);
      if(!TopExp_Explorer{pocket,TopAbs_SOLID}.More())continue;
      // A translated copy of pocket has coincident end caps. On faceted lofts
      // OCCT can classify the central overlap as retained material without an
      // error or invalid topology. Extend only the cutter along local X so its
      // caps lie outside the actual 4 mm rail, preserving the cavity contour.
      const auto clearancePocket=booleanOp<BRepAlgoAPI_Common>(cavity,slice(.1),processing);
      for(double offset:{-3.,3.}) {
        // Translation in Y removes the centre of the pocket, leaving a band
        // following the corresponding inner side, including taper and curvature.
        gp_Trsf move;move.SetTranslation(gp_Vec{0,offset,0});
        const auto clearance=BRepBuilderAPI_Transform{clearancePocket,move,true}.Shape();
        auto rail=booleanOp<BRepAlgoAPI_Cut>(pocket,clearance,processing);
        // Topological validity alone cannot detect a retained cavity plug.
        // Reject any residual solid whose interior mass centre is also inside
        // the cutter. The first classification excludes concave-solid centres
        // that fall outside their own material.
        for(TopExp_Explorer solid{rail,TopAbs_SOLID};solid.More();solid.Next()) {
          processing.checkpoint();GProp_GProps mass;BRepGProp::VolumeProperties(solid.Current(),mass);
          const auto center=mass.CentreOfMass();
          if(BRepClass3d_SolidClassifier{solid.Current(),center,1e-7}.State()==TopAbs_IN&&
              BRepClass3d_SolidClassifier{clearance,center,1e-7}.State()==TopAbs_IN)
            throw std::runtime_error("Former retaining rail subtraction left material inside its clearance.");
        }
        Bnd_Box railBounds;BRepBndLib::AddOptimal(rail,railBounds,false,false);railBounds.SetGap(1e-7);
        // Conservative bounds only skip certainly disjoint inserts. Keep the
        // original bounds after clipping: they still enclose every residual rail.
        for(std::size_t i=0;i<inserts.size();++i)
          if(!inserts[i].IsNull()&&!railBounds.IsOut(insertBounds[i])&&TopExp_Explorer{rail,TopAbs_SOLID}.More())
            rail=booleanOp<BRepAlgoAPI_Cut>(rail,inserts[i],processing);
        if(!TopExp_Explorer{rail,TopAbs_SOLID}.More())continue;
        rails.Append(rail);
      }
    }
  }
  auto result=body;
  if(!rails.IsEmpty()) {
    if(progress)progress("Fuselage: joining all retaining rails to the walls...");
    // Intersect the large shell once, rather than once per rail. Separate tool
    // shapes let OCCT resolve intersecting rails from closely spaced formers.
    BRepAlgoAPI_Fuse fuse;NCollection_List<TopoDS_Shape> args;args.Append(body);
    fuse.SetArguments(args);fuse.SetTools(rails);fuse.SetNonDestructive(true);fuse.SetRunParallel(processing.parallel);fuse.SetFuzzyValue(1e-7);
    {auto range=processing.range();fuse.Build(range);}processing.checkpoint();
    if(!fuse.IsDone()||fuse.HasErrors()||fuse.Shape().IsNull())throw std::runtime_error("Could not join former retaining rails to the fuselage.");
    result=fuse.Shape();
  }
  int count=0;for(TopExp_Explorer e{result,TopAbs_SOLID};e.More();e.Next())++count;
  GProp_GProps mass;BRepGProp::VolumeProperties(result,mass);
  if(count!=1||mass.Mass()<=1e-8||!BRepCheck_Analyzer{result,true,processing.parallel}.IsValid())
    throw std::runtime_error("Former retaining rails could not be joined to the fuselage walls.");
  return result;
}
std::vector<TopoDS_Shape> buildFormers(const TopoDS_Shape& cavity,const TopoDS_Shape& supportedBody,
    const std::vector<QRectF>& rectangles,const std::optional<QRectF>& tray,const std::function<void(const char*)>& progress,const ProcessingControl& processing,const std::vector<double>& rotationDegrees){
  processing.checkpoint();validateAngles(rectangles,rotationDegrees);if(rectangles.empty())return {};
  if(cavity.IsNull())throw std::runtime_error("Enter Thicken before generating formers; formers need inner fuselage walls.");
  for(std::size_t i=0;i<rectangles.size();++i){const auto& r=rectangles[i];
    if(!std::isfinite(r.x())||!std::isfinite(r.y())||!std::isfinite(r.width())||!std::isfinite(r.height())||r.width()<=0||r.height()<=0)throw std::runtime_error("Former rectangles need positive thickness and height.");
    for(std::size_t j=0;j<i;++j)if(gui::formerMasksOverlap(r,-gui::formerAngle(rotationDegrees,i),rectangles[j],-gui::formerAngle(rotationDegrees,j)))throw std::runtime_error("Formers cannot overlap each other.");
    if(tray&&gui::formerMasksOverlap(r,-gui::formerAngle(rotationDegrees,i),*tray))throw std::runtime_error("A former overlaps the servo tray. Move or resize it before generating.");
  }
  Bnd_Box box;BRepBndLib::AddOptimal(cavity,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
  std::vector<TopoDS_Shape> result;
  for(std::size_t i=0;i<rectangles.size();++i){
    const auto status="Fuselage: fitting former "+std::to_string(i+1)+" to the inner cavity...";if(progress)progress(status.c_str());
    const auto& r=rectangles[i];const auto slab=rotated(BRepPrimAPI_MakeBox{gp_Pnt{r.left(),y0-1,r.top()},r.width(),y1-y0+2,r.height()}.Shape(),r,gui::formerAngle(rotationDegrees,i));
    auto shape=booleanOp<BRepAlgoAPI_Common>(cavity,slab,processing);
    // The cavity excludes the skin; subtract the supported body as well so partial
    // formers beneath the tray cannot occupy its integral support ledges.
    if(!supportedBody.IsNull())shape=booleanOp<BRepAlgoAPI_Cut>(shape,supportedBody,processing);
    TopExp_Explorer solids{shape,TopAbs_SOLID};
    if(!solids.More())throw std::runtime_error("Former "+std::to_string(i+1)+" misses the inner cavity. Move or resize it.");
    auto solid=solids.Current();solids.Next();GProp_GProps mass;BRepGProp::VolumeProperties(solid,mass);
    if(solids.More()||!BRepCheck_Analyzer{solid,true,processing.parallel}.IsValid()||mass.Mass()<=1e-8)throw std::runtime_error("Former "+std::to_string(i+1)+" must fit one connected inner section. Move or resize it.");
    result.push_back(solid);
  }
  return result;
}
}

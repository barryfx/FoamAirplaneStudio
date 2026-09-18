#include "geometry/Formers.h"
#include "gui/FormerEditor.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
namespace {
template<class Operation> TopoDS_Shape booleanOp(const TopoDS_Shape& a,const TopoDS_Shape& b,const ProcessingControl& processing){
  processing.checkpoint();Operation op;NCollection_List<TopoDS_Shape> args,tools;args.Append(a);tools.Append(b);op.SetArguments(args);op.SetTools(tools);op.SetNonDestructive(true);op.SetFuzzyValue(1e-7);
  {auto range=processing.range();op.Build(range);}processing.checkpoint();
  if(!op.IsDone()||op.HasErrors()||op.Shape().IsNull())throw std::runtime_error("Could not fit a former to the inner cavity.");return op.Shape();
}
}
TopoDS_Shape addFormerRetainers(const TopoDS_Shape& body,const TopoDS_Shape& cavity,
    const std::vector<QRectF>& rectangles,const std::vector<TopoDS_Shape>& inserts,
    const std::function<void(const char*)>& progress,const ProcessingControl& processing) {
  if(rectangles.empty())return body;
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
    const auto status="Fuselage: constructing retaining rails for former "+std::to_string(++formerIndex)+"...";
    if(progress)progress(status.c_str());
    for(double x:{r.left()-4.,r.right()}) {
      processing.checkpoint();
      const auto slab=BRepPrimAPI_MakeBox{gp_Pnt{x,y0-1,z0-1},4.,y1-y0+2,z1-z0+2}.Shape();
      const auto pocket=booleanOp<BRepAlgoAPI_Common>(cavity,slab,processing);
      if(!TopExp_Explorer{pocket,TopAbs_SOLID}.More())continue;
      for(double offset:{-3.,3.}) {
        // Translation in Y removes the centre of the pocket, leaving a band
        // following the corresponding inner side, including taper and curvature.
        gp_Trsf move;move.SetTranslation(gp_Vec{0,offset,0});
        auto rail=booleanOp<BRepAlgoAPI_Cut>(pocket,BRepBuilderAPI_Transform{pocket,move,true}.Shape(),processing);
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
    fuse.SetArguments(args);fuse.SetTools(rails);fuse.SetNonDestructive(true);fuse.SetFuzzyValue(1e-7);
    {auto range=processing.range();fuse.Build(range);}processing.checkpoint();
    if(!fuse.IsDone()||fuse.HasErrors()||fuse.Shape().IsNull())throw std::runtime_error("Could not join former retaining rails to the fuselage.");
    result=fuse.Shape();
  }
  int count=0;for(TopExp_Explorer e{result,TopAbs_SOLID};e.More();e.Next())++count;
  GProp_GProps mass;BRepGProp::VolumeProperties(result,mass);
  if(count!=1||mass.Mass()<=1e-8||!BRepCheck_Analyzer{result}.IsValid())
    throw std::runtime_error("Former retaining rails could not be joined to the fuselage walls.");
  return result;
}
std::vector<TopoDS_Shape> buildFormers(const TopoDS_Shape& cavity,const TopoDS_Shape& supportedBody,
    const std::vector<QRectF>& rectangles,const std::optional<QRectF>& tray,const std::function<void(const char*)>& progress,const ProcessingControl& processing){
  processing.checkpoint();if(rectangles.empty())return {};
  if(cavity.IsNull())throw std::runtime_error("Enter Thicken before generating formers; formers need inner fuselage walls.");
  for(std::size_t i=0;i<rectangles.size();++i){const auto& r=rectangles[i];
    if(!std::isfinite(r.x())||!std::isfinite(r.y())||!std::isfinite(r.width())||!std::isfinite(r.height())||r.width()<=0||r.height()<=0)throw std::runtime_error("Former rectangles need positive thickness and height.");
    for(std::size_t j=0;j<i;++j)if(gui::rectanglesOverlap(r,rectangles[j]))throw std::runtime_error("Formers cannot overlap each other.");
    if(tray&&gui::rectanglesOverlap(r,*tray))throw std::runtime_error("A former overlaps the servo tray. Move or resize it before generating.");
  }
  Bnd_Box box;BRepBndLib::AddOptimal(cavity,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
  std::vector<TopoDS_Shape> result;
  for(std::size_t i=0;i<rectangles.size();++i){
    const auto status="Fuselage: fitting former "+std::to_string(i+1)+" to the inner cavity...";if(progress)progress(status.c_str());
    const auto& r=rectangles[i];const auto slab=BRepPrimAPI_MakeBox{gp_Pnt{r.left(),y0-1,r.top()},r.width(),y1-y0+2,r.height()}.Shape();
    auto shape=booleanOp<BRepAlgoAPI_Common>(cavity,slab,processing);
    // The cavity excludes the skin; subtract the supported body as well so partial
    // formers beneath the tray cannot occupy its integral support ledges.
    if(!supportedBody.IsNull())shape=booleanOp<BRepAlgoAPI_Cut>(shape,supportedBody,processing);
    TopExp_Explorer solids{shape,TopAbs_SOLID};
    if(!solids.More())throw std::runtime_error("Former "+std::to_string(i+1)+" misses the inner cavity. Move or resize it.");
    auto solid=solids.Current();solids.Next();GProp_GProps mass;BRepGProp::VolumeProperties(solid,mass);
    if(solids.More()||!BRepCheck_Analyzer{solid}.IsValid()||mass.Mass()<=1e-8)throw std::runtime_error("Former "+std::to_string(i+1)+" must fit one connected inner section. Move or resize it.");
    result.push_back(solid);
  }
  return result;
}
}

#include "geometry/Formers.h"
#include "gui/FormerEditor.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
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

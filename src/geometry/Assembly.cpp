#include "geometry/Assembly.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Trsf.hxx>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <numbers>

namespace designrc::geometry {
namespace {
std::array<double,6> bounds(const TopoDS_Shape& shape) {
  if(shape.IsNull())throw std::runtime_error("Generate all four components before assembly.");
  Bnd_Box box;BRepBndLib::AddOptimal(shape,box,false,false);
  if(box.IsVoid())throw std::runtime_error("An assembly component has no geometry.");
  std::array<double,6> b;box.Get(b[0],b[1],b[2],b[3],b[4],b[5]);return b;
}
TopoDS_Shape compound(const std::vector<TopoDS_Shape>& shapes) {
  BRep_Builder builder;TopoDS_Compound result;builder.MakeCompound(result);
  for(const auto& shape:shapes)if(!shape.IsNull())builder.Add(result,shape);
  return result;
}
double volume(const TopoDS_Shape& shape) {
  GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass,1e-7);return std::abs(mass.Mass());
}
TopoDS_Shape copy(const TopoDS_Shape& shape) {
  return shape.IsNull()?TopoDS_Shape{}:BRepBuilderAPI_Copy{shape,true,true}.Shape();
}
TopoDS_Shape move(const TopoDS_Shape& shape,const gp_Trsf& placement) {
  if(shape.IsNull())return {};
  return shape.Moved(TopLoc_Location{placement});
}
bool overlaps(const TopoDS_Shape& a,const TopoDS_Shape& b,const ProcessingControl& control) {
  control.checkpoint();if(a.IsNull()||b.IsNull())return false;
  Bnd_Box ba,bb;BRepBndLib::Add(a,ba);BRepBndLib::Add(b,bb);if(ba.IsOut(bb))return false;
  BRepAlgoAPI_Common common;TopTools_ListOfShape args,tools;args.Append(a);tools.Append(b);
  common.SetArguments(args);common.SetTools(tools);common.SetNonDestructive(true);common.Build(control.range());
  control.checkpoint();if(!common.IsDone()||common.HasErrors())throw std::runtime_error("Could not verify assembly collisions.");
  // Face contact is allowed; only positive material overlap blocks assembly.
  return volume(common.Shape())>1e-6;
}
TopoDS_Shape subtract(const TopoDS_Shape& body,const std::vector<TopoDS_Shape>& cutters,
                      const char* name,const ProcessingControl& control) {
  BRep_Builder builder;TopoDS_Compound result;builder.MakeCompound(result);
  // Keep manufacturing parts separate and reject consuming any complete part.
  for(TopExp_Explorer part{body,TopAbs_SOLID};part.More();part.Next()) {
    auto shape=part.Current();
    for(const auto& tool:cutters) {
      control.checkpoint();if(tool.IsNull())continue;
      BRepAlgoAPI_Cut cut;TopTools_ListOfShape args,tools;args.Append(shape);tools.Append(tool);
      cut.SetArguments(args);cut.SetTools(tools);cut.SetNonDestructive(true);cut.Build(control.range());
      control.checkpoint();if(!cut.IsDone()||cut.HasErrors())throw std::runtime_error(std::string{"Could not cut "}+name+".");
      shape=cut.Shape();
    }
    int count=0;
    for(TopExp_Explorer remaining{shape,TopAbs_SOLID};remaining.More();remaining.Next()) {
      if(!BRepCheck_Analyzer{remaining.Current()}.IsValid()||volume(remaining.Current())<=1e-9)
        throw std::runtime_error(std::string{"Invalid remaining "}+name+" part.");
      builder.Add(result,remaining.Current());++count;
    }
    if(!count)throw std::runtime_error(std::string{"Intersections consume an entire "}+name+" part. Reposition the components.");
  }
  return result;
}
}
gui::AssemblyState initialAssemblyPlacement(const AssemblyParts& parts) {
  const auto f=bounds(parts.fuselage),w=bounds(parts.wing);
  const auto h=bounds(compound({parts.horizontal,parts.elevator}));
  const auto v=bounds(compound({parts.vertical,parts.rudder}));
  const double middle=(f[0]+f[3])/2,level=(f[2]+f[5])/2,gap=std::max(5.,(f[3]-f[0])*.03);
  gui::AssemblyState result;result.positioned=true;
  result.offsets={QPointF{middle-(w[0]+w[3])/2,f[5]+gap-w[2]},
                  QPointF{f[3]-h[3],level-(h[2]+h[5])/2},
                  QPointF{f[3]-v[3],f[5]+gap-v[2]}};
  return result;
}
gp_Trsf assemblyComponentPlacement(const AssemblyParts& p,const gui::AssemblyState& s,std::size_t component) {
  gp_Trsf rotation,translation;
  rotation.SetRotation(gp_Ax1{p.rootCenters.at(component),gp_Dir{0,1,0}},s.rotationDegrees.at(component)*std::numbers::pi/180.);
  const auto offset=s.offsets.at(component);translation.SetTranslation(gp_Vec{offset.x(),0,offset.y()});
  return translation*rotation; // Rotate about the root, then translate that root.
}
AssemblyParts placeAssembly(const AssemblyParts& p,const gui::AssemblyState& s) {
  const auto w=assemblyComponentPlacement(p,s,0),h=assemblyComponentPlacement(p,s,1),v=assemblyComponentPlacement(p,s,2);
  AssemblyParts result{p.fuselage,move(p.wing,w),move(p.horizontal,h),
      move(p.vertical,v),move(p.elevator,h),move(p.rudder,v),p.fuselageParts,p.inserts};
  result.sparMaterials=p.sparMaterials;for(auto& spar:result.sparMaterials)if(!spar.fuselage)spar.center.Transform(w);
  for(std::size_t i=0;i<3;++i)result.rootCenters[i]=p.rootCenters[i].Transformed(assemblyComponentPlacement(p,s,i));
  return result;
}
TopoDS_Shape assemblyShape(const AssemblyParts& p) {
  std::vector<TopoDS_Shape> shapes{p.fuselage,p.wing,p.horizontal,p.vertical,p.elevator,p.rudder};
  for(const auto& insert:p.inserts)shapes.push_back(insert.shape);
  return compound(shapes); // Display aggregation only; no union of touching parts.
}
AssemblyCutResult cutAssemblyIntersections(const AssemblyParts& placed,
    const std::function<void(const char*)>& progress,const ProcessingControl& control) {
  auto report=[&](const char* text){control.checkpoint();if(progress)progress(text);};
  report("Assembly: checking rudder and elevator clearance...");
  AssemblyCutResult result;
  result.parts={placed.fuselageParts.empty()?copy(placed.fuselage):TopoDS_Shape{},
      copy(placed.wing),copy(placed.horizontal),copy(placed.vertical),copy(placed.elevator),copy(placed.rudder)};
  auto& p=result.parts;
  p.rootCenters=placed.rootCenters;p.sparMaterials=placed.sparMaterials;
  // Inserts remain independent parts. Only formers receive wing seats below;
  // the tray continues to share its immutable source geometry.
  p.inserts=placed.inserts;
  if(overlaps(p.elevator,p.rudder,control))result.collisions.emplace_back("Elevator intersects Rudder");
  if(!result.collisions.empty())return result;
  report("Assembly: cutting wing and stabilizer seats in fuselage...");
  if(placed.fuselageParts.empty()) {
    p.fuselage=subtract(p.fuselage,{p.wing,p.horizontal,p.vertical},"Fuselage",control);
  } else {
    // All three fixed lifting surfaces cut the body manufacturing pieces.
    std::vector<TopoDS_Shape> shapes;
    for(const auto& source:placed.fuselageParts) {
      auto part=source;
      part.shape=subtract(copy(source.shape),{p.wing,p.horizontal,p.vertical},source.name.c_str(),control);
      shapes.push_back(part.shape);p.fuselageParts.push_back(std::move(part));
    }
    p.fuselage=compound(shapes);
  }
  report("Assembly: cutting wing seats in formers...");
  for(auto& insert:p.inserts) {
    control.checkpoint();
    // The manufacturing plane identifies formers even after the user renames them.
    if(insert.formerPlane&&!insert.shape.IsNull()&&!p.wing.IsNull())
      insert.shape=subtract(copy(insert.shape),{p.wing},insert.name.c_str(),control);
  }
  report("Assembly: cutting fin slot in horizontal stabilizer...");
  p.horizontal=subtract(p.horizontal,{p.vertical},"Horiz Stab",control);
  report("Assembly: meshing cut parts...");
  IMeshTools_Parameters parameters;parameters.Deflection=.1;parameters.Angle=.25;parameters.InParallel=false;
  std::vector<TopoDS_Shape> meshShapes{p.fuselage,p.horizontal};
  for(const auto& insert:p.inserts)
    if(insert.formerPlane&&!insert.shape.IsNull()&&!p.wing.IsNull())meshShapes.push_back(insert.shape);
  for(const auto& shape:meshShapes) {
    BRepMesh_IncrementalMesh mesh{shape,parameters,control.range()};control.checkpoint();
    if(!mesh.IsDone())throw std::runtime_error("Assembly display meshing failed.");
  }
  return result;
}
}

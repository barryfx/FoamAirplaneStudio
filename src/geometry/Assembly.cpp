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
TopoDS_Shape move(const TopoDS_Shape& shape,QPointF offset) {
  if(shape.IsNull())return {};
  gp_Trsf translation;translation.SetTranslation(gp_Vec{offset.x(),0,offset.y()});
  return shape.Moved(TopLoc_Location{translation});
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
AssemblyParts placeAssembly(const AssemblyParts& p,const gui::AssemblyState& s) {
  return {p.fuselage,move(p.wing,s.offsets[0]),move(p.horizontal,s.offsets[1]),
      move(p.vertical,s.offsets[2]),move(p.elevator,s.offsets[1]),move(p.rudder,s.offsets[2]),p.fuselageParts,p.inserts};
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
  // Inserts bypass every Boolean and meshing operation. Sharing these immutable
  // handles retains their exact geometry and independent component identities.
  p.inserts=placed.inserts;
  if(overlaps(p.elevator,p.rudder,control))result.collisions.emplace_back("Elevator intersects Rudder");
  if(!result.collisions.empty())return result;
  report("Assembly: cutting wing and stabilizer seats in fuselage...");
  if(placed.fuselageParts.empty()) {
    p.fuselage=subtract(p.fuselage,{p.wing,p.horizontal,p.vertical},"Fuselage",control);
  } else {
    // Seat cuts apply only to fuselage body pieces, never removable inserts.
    std::vector<TopoDS_Shape> shapes;
    for(const auto& source:placed.fuselageParts) {
      auto part=source;
      part.shape=subtract(copy(source.shape),{p.wing,p.horizontal,p.vertical},source.name.c_str(),control);
      shapes.push_back(part.shape);p.fuselageParts.push_back(std::move(part));
    }
    p.fuselage=compound(shapes);
  }
  report("Assembly: cutting fin slot in horizontal stabilizer...");
  p.horizontal=subtract(p.horizontal,{p.vertical},"Horiz Stab",control);
  report("Assembly: meshing cut parts...");
  IMeshTools_Parameters parameters;parameters.Deflection=.1;parameters.Angle=.25;parameters.InParallel=false;
  for(const auto& shape:{p.fuselage,p.horizontal}) {
    BRepMesh_IncrementalMesh mesh{shape,parameters,control.range()};control.checkpoint();
    if(!mesh.IsDone())throw std::runtime_error("Assembly display meshing failed.");
  }
  return result;
}
}

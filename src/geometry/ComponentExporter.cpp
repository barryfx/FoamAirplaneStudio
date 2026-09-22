#include "domain/DxfExporter.h"
#include "geometry/StepExporter.h"
#include "geometry/ComponentExporter.h"
#include "gui/ComponentNames.h"
#include <QSet>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <StlAPI_Writer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <stdexcept>

namespace designrc::geometry {
namespace {
void appendSolids(std::vector<ExportPart>& parts,const std::string& name,const TopoDS_Shape& shape,const std::string& id={}) {
  std::vector<TopoDS_Shape> solids;
  for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())solids.push_back(e.Current());
  for(std::size_t i=0;i<solids.size();++i)
    parts.push_back({name+(solids.size()>1?" "+std::to_string(i+1):""),solids[i],{},(id.empty()?name:id)+"/solid/"+std::to_string(i)});
}
void writeStl(const TopoDS_Shape& source,const std::filesystem::path& path) {
  // Meshing must never change a cached or displayed Assembly shape.
  auto shape=BRepBuilderAPI_Copy{source,true,false}.Shape();
  BRepTools::Clean(shape);
  BRepMesh_IncrementalMesh mesh{shape,.05,false,.15,true};
  if(!mesh.IsDone())throw std::runtime_error("Could not mesh the selected part for STL.");
  StlAPI_Writer writer;writer.ASCIIMode()=false;
  const auto filename=path.u8string();
  if(!writer.Write(shape,reinterpret_cast<const char*>(filename.c_str())))throw std::runtime_error("Could not write STL file.");
}
}
std::vector<ExportPart> assemblyExportParts(const AssemblyParts& a) {
  std::vector<ExportPart> parts;
  for(const auto& p:a.inserts)if(p.formerPlane)parts.push_back({p.name,p.shape,p.formerPlane,p.id.empty()?p.name:p.id});
  if(a.fuselageParts.empty()) {
    appendSolids(parts,"Fuselage",a.fuselage);
    for(auto& p:parts)if(p.id.starts_with("Fuselage/solid/"))
      p.id="Fuselage/"+p.id.substr(std::string{"Fuselage/solid/"}.size())+"/solid/0";
  }
  else for(const auto& p:a.fuselageParts)appendSolids(parts,p.name,p.shape,p.id);
  for(const auto& p:a.inserts)if(!p.formerPlane)appendSolids(parts,p.name,p.shape,p.id);
  appendSolids(parts,"Wing",a.wing);
  appendSolids(parts,"Horizontal Stabilizer",a.horizontal);
  appendSolids(parts,"Elevator",a.elevator);
  appendSolids(parts,"Vertical Stabilizer",a.vertical);
  appendSolids(parts,"Rudder",a.rudder);
  return parts;
}
domain::PartDrawing formerDrawing(const ExportPart& part) {
  if(!part.formerPlane)throw std::invalid_argument("DXF requires a former plane.");
  BRepAlgoAPI_Section section{part.shape,*part.formerPlane,false};
  section.SetNonDestructive(true);section.Build();
  if(!section.IsDone()||section.HasErrors())throw std::runtime_error("Could not section "+part.name);
  Handle(TopTools_HSequenceOfShape) edges=new TopTools_HSequenceOfShape;
  Handle(TopTools_HSequenceOfShape) wires=new TopTools_HSequenceOfShape;
  for(TopExp_Explorer e{section.Shape(),TopAbs_EDGE};e.More();e.Next())edges->Append(e.Current());
  ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edges,1e-6,false,wires);
  domain::PartDrawing result;result.label=part.name;
  const auto axes=part.formerPlane->Position();
  for(int i=1;i<=wires->Length();++i) {
    const auto wire=TopoDS::Wire(wires->Value(i));
    if(!wire.Closed())throw std::runtime_error("Open DXF contour in "+part.name);
    domain::PartDrawingPath path;path.layer="FORMER_OUTLINE";
    for(BRepTools_WireExplorer e{wire};e.More();e.Next()) {
      BRepAdaptor_Curve curve{e.Current()};
      GCPnts_QuasiUniformDeflection samples{curve,.02};
      if(!samples.IsDone()||samples.NbPoints()<2)throw std::runtime_error("Could not sample "+part.name);
      const bool reverse=e.Current().Orientation()==TopAbs_REVERSED;
      for(int j=0;j<samples.NbPoints()-1;++j) {
        const auto point=samples.Value(reverse?samples.NbPoints()-j:j+1);
        const gp_Vec delta{axes.Location(),point};
        path.points.push_back({delta.Dot(gp_Vec{axes.XDirection()}),delta.Dot(gp_Vec{axes.YDirection()})});
      }
    }
    if(path.points.size()<3)throw std::runtime_error("Empty DXF contour in "+part.name);
    result.paths.push_back(std::move(path));
  }
  if(result.paths.empty())throw std::runtime_error("No mid-plane outline in "+part.name);
  return result;
}
std::vector<std::string> exportFileNames(const std::vector<ExportPart>& selected,
    FormerExportFormat formers,ComponentExportFormat components,const std::string& projectName) {
  std::vector<std::string> names;bool step=false;QSet<QString> partNames;
  for(const auto& p:selected) {
    if(!gui::validComponentName(QString::fromStdString(p.name)))throw std::invalid_argument("Invalid component filename: "+p.name);
    const auto partKey=QString::fromStdString(p.name).toCaseFolded();
    if(partNames.contains(partKey))throw std::invalid_argument("Duplicate component name: "+p.name);
    partNames.insert(partKey);
    if(p.formerPlane) {
      if(formers==FormerExportFormat::Step)step=true;
      else names.push_back(p.name+(formers==FormerExportFormat::Dxf?".dxf":".stl"));
    }
    else if(components==ComponentExportFormat::Stl)names.push_back(p.name+".stl");
    else step=true;
  }
  if(step)names.push_back(gui::exportProjectStem(QString::fromStdString(projectName)).toStdString()+".step");
  QSet<QString> unique;
  for(const auto& name:names) {const auto key=QString::fromStdString(name).toCaseFolded();if(unique.contains(key))throw std::invalid_argument("Duplicate export filename: "+name);unique.insert(key);}
  return names;
}
void writeComponentExports(const std::vector<ExportPart>& selected,
    FormerExportFormat formers,ComponentExportFormat components,const std::filesystem::path& directory,const std::string& projectName) {
  exportFileNames(selected,formers,components,projectName);
  if(selected.empty())throw std::invalid_argument("Select at least one part to export.");
  std::vector<NamedPartShape> step;
  for(const auto& p:selected) {
    if(p.shape.IsNull())throw std::invalid_argument("No Assembly geometry for "+p.name);
    if(p.formerPlane&&formers==FormerExportFormat::Dxf)
      domain::exportPartsDxf({formerDrawing(p)},directory/std::filesystem::u8path(p.name+".dxf"));
    else if(p.formerPlane?formers==FormerExportFormat::Stl:components==ComponentExportFormat::Stl)
      writeStl(p.shape,directory/std::filesystem::u8path(p.name+".stl"));
    else step.push_back({p.name,p.shape,PartMaterial::Wood,false});
  }
  if(!step.empty())exportStepAssembly(step,directory/std::filesystem::u8path(gui::exportProjectStem(QString::fromStdString(projectName)).toStdString()+".step"),projectName,StepAssemblyLayout::Aircraft);
}
}

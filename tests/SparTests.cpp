#include "geometry/SparCut.h"
#include "gui/ProjectDocument.h"
#include "geometry/WingSolidBuilder.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Standard_Failure.hxx>
#include <iostream>

#include <stdexcept>
#include <cmath>
using namespace designrc;
#define CHECK(c) do{if(!(c))throw std::runtime_error(std::string{#c}+" line "+std::to_string(__LINE__));}while(false)
std::vector<TopoDS_Solid> bodies(const TopoDS_Shape& shape) {
  CHECK(BRepCheck_Analyzer{shape}.IsValid());std::vector<TopoDS_Solid> r;
  for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())r.push_back(TopoDS::Solid(e.Current()));return r;
}
bool inside(const TopoDS_Solid& s,double x,double y,double z) {
  BRepClass3d_SolidClassifier c{s,gp_Pnt{x,y,z},1e-6};return c.State()==TopAbs_IN || c.State()==TopAbs_ON;
}
double volume(const TopoDS_Shape& s){GProp_GProps p;BRepGProp::VolumeProperties(s,p);return p.Mass();}
int main(int argc,char** argv) {
  try {
    // Optional read-only reproduction against a user's saved project.
    if(argc>1) {
      QString error;auto project=gui::readProject(QString::fromLocal8Bit(argv[1]),error);
      if(!project)throw std::runtime_error(error.toStdString());
      geometry::WingSolidInput input{project->wing.layers,project->stations.lines,project->airfoils.entries,
        project->reference.toScale?std::nullopt:project->reference.wingspanMm,
        project->dihedralDegrees,project->controls.panels,project->spars,project->lightening};
      if(argc>2 && std::string{argv[2]}=="--surface-only")for(auto& panel:input.spars)panel[2].enabled=false;
      const auto shape=geometry::buildWingSolid(input,[](const char* p){std::cout<<p<<std::endl;});
      std::cout<<"Valid project solids: "<<bodies(shape).size()<<std::endl;return 0;
    }
    const auto box=BRepPrimAPI_MakeBox{gp_Pnt{0,0,-10},200,500,20}.Shape();
    auto chord=[](double){return std::pair{0.0,200.0};};gui::SparState spars;
    spars[0]={true,gui::SparShape::Round,30,60,4,1};
    std::cout<<"Top round groove"<<std::endl;
    std::vector<geometry::SparMaterial> materials;
    auto shape=geometry::cutSpars(box,spars,500,chord,{},0,0,false,0,{}, {},&materials);
    CHECK(materials.size()==1);CHECK(std::abs(materials[0].volumeMm3-3.141592653589793*4*300)<.001);
    CHECK(materials[0].center.Distance(gp_Pnt{60,150,10})<.001);auto parts=bodies(shape);CHECK(parts.size()==1);
    CHECK(!inside(parts[0],60,150,9));CHECK(inside(parts[0],60,150,7.9));CHECK(inside(parts[0],60,301,9));
    CHECK(std::abs(volume(shape)-(2000000-0.5*3.141592653589793*4*300))<0.1);
    std::cout<<"Bottom strip groove"<<std::endl;
    spars[0].enabled=false;spars[1]={true,gui::SparShape::Strip,40,50,6,2};
    materials.clear();parts=bodies(geometry::cutSpars(box,spars,500,chord,{},0,0,false,0,{}, {},&materials));CHECK(parts.size()==1);
    { double expectedVolume=0;for(int i=0;i<2;++i)if(spars[i].enabled)expectedVolume+=500*spars[i].lengthPercent/100*(spars[i].shape==gui::SparShape::Round?3.141592653589793*spars[i].sizeMm*spars[i].sizeMm/4:spars[i].sizeMm*spars[i].heightMm);
    double actualVolume=0;for(const auto& material:materials)actualVolume+=material.volumeMm3;CHECK(std::abs(actualVolume-expectedVolume)<.01); }
    CHECK(!inside(parts[0],80,100,-9));CHECK(inside(parts[0],80,100,-7.9));CHECK(inside(parts[0],83.1,100,-9));
    CHECK(inside(parts[0],80,251,-9));
    std::cout<<"Combined surface grooves"<<std::endl;
    spars[0]={true,gui::SparShape::Strip,20,50,5,2};spars[1].shape=gui::SparShape::Round;
    materials.clear();parts=bodies(geometry::cutSpars(box,spars,500,chord,{},0,0,false,0,{}, {},&materials));CHECK(parts.size()==1);
    { double expectedVolume=0;for(int i=0;i<2;++i)if(spars[i].enabled)expectedVolume+=500*spars[i].lengthPercent/100*(spars[i].shape==gui::SparShape::Round?3.141592653589793*spars[i].sizeMm*spars[i].sizeMm/4:spars[i].sizeMm*spars[i].heightMm);
    double actualVolume=0;for(const auto& material:materials)actualVolume+=material.volumeMm3;CHECK(std::abs(actualVolume-expectedVolume)<.01); }
    CHECK(!inside(parts[0],40,100,9));CHECK(!inside(parts[0],80,100,-9));
    std::cout<<"Mid split and tabs"<<std::endl;
    spars={};spars[2]={true,gui::SparShape::Round,30,60,4,1};
    spars[2].insideDiameterMm=3;materials.clear();
    shape=geometry::cutSpars(box,spars,500,chord,{},0,0,false,0,{}, {},&materials);parts=bodies(shape);
    CHECK(materials.size()==1);CHECK(std::abs(materials[0].volumeMm3-3.141592653589793*(16-9)/4*300)<.001);
    CHECK(materials[0].center.Distance(gp_Pnt{60,150,0})<.001);CHECK(parts.size()==2);
    CHECK(!inside(parts[0],60,150,1));CHECK(!inside(parts[1],60,150,-1));
    CHECK(inside(parts[0],60,301,1));CHECK(inside(parts[1],60,301,-1));
    for(double y:{100.0,400.0})for(double x:{53.5,66.5}) {
      CHECK(inside(parts[1],x,y,1.9));CHECK(!inside(parts[1],x,y,2.1));
      CHECK(!inside(parts[0],x,y,2.05));CHECK(inside(parts[0],x,y,2.2));
      CHECK(!inside(parts[0],x+1.55,y,1));CHECK(!inside(parts[1],x+1.55,y,1));
      CHECK(inside(parts[0],x+1.65,y,1));
    }
    const double fitLoss=4*(3.141592653589793*1.6*1.6*2.1-3.141592653589793*1.5*1.5*2);
    CHECK(std::abs(volume(shape)-(2000000-3.141592653589793*4*300-fitLoss))<0.1);
    spars[2].sizeMm=30;bool rejected=false;
    try{geometry::cutSpars(box,spars,500,chord);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    // A flat-bottom airfoil splits at half thickness, not at the old Z=0 plane.
    const auto flatBottom=BRepPrimAPI_MakeBox{gp_Pnt{0,0,0},200,500,16}.Shape();
    gui::SparState flatSpars;flatSpars[0]={true,gui::SparShape::Round,30,70,2.54,1};
    flatSpars[1]={true,gui::SparShape::Strip,30,70,6.35,2.54};
    CHECK(bodies(geometry::cutSpars(flatBottom,flatSpars,500,chord)).size()==1);
    flatSpars[2]={true,gui::SparShape::Round,30,60,5.08,1};
    const auto cambered=geometry::cutSpars(flatBottom,flatSpars,500,chord);
    const auto camberedParts=bodies(cambered);CHECK(camberedParts.size()==2);
    CHECK(!inside(camberedParts[0],60,150,8.1));CHECK(!inside(camberedParts[1],60,150,7.9));
    CHECK(inside(camberedParts[0],100,150,8.1));CHECK(!inside(camberedParts[0],100,150,7.9));
    CHECK(inside(camberedParts[1],100,150,7.9));
    CHECK(inside(camberedParts[1],53.96,100,9.9)); // Alignment tab follows split at Z=8.
    // Sloped upper skin proves the split follows height and selected chord,
    // including beyond the end of the spar hole.
    BRepOffsetAPI_ThruSections wedge{true,true};
    for(double y:{0.0,500.0}) {
      const double factor=1-y/1000;
      BRepBuilderAPI_MakePolygon wire;
      for(const gp_Pnt p:{gp_Pnt{0,y,0},gp_Pnt{200,y,0},gp_Pnt{200,y,12*factor},gp_Pnt{0,y,20*factor}})wire.Add(p);
      wire.Close();wedge.AddWire(wire.Wire());
    }
    wedge.Build();gui::SparState varying;varying[2]={true,gui::SparShape::Round,30,50,3,1};
    const auto varied=bodies(geometry::cutSpars(wedge.Shape(),varying,500,chord));CHECK(varied.size()==2);
    CHECK(!inside(varied[0],60,100,7.92) && !inside(varied[1],60,100,7.92));
    for(const auto [y,z]:{std::pair{100.0,7.92},std::pair{400.0,6.6}}) {
      CHECK(inside(varied[0],120,y,z+.01));CHECK(!inside(varied[0],120,y,z-.01));
      CHECK(inside(varied[1],120,y,z-.01));
    }
    for(int i=0;i<2;++i) {
      gui::SparState oversized;oversized[i]={true,gui::SparShape::Strip,30,70,3,25};rejected=false;
      try{geometry::cutSpars(box,oversized,500,chord);}catch(const std::exception& e) {
        const std::string message=e.what();rejected=message.starts_with(i==0?"Top spar:":"Bottom spar:") && message.find("local wing thickness is 20.000 mm")!=std::string::npos;
      }CHECK(rejected);
    }
    // Integration: tapered planform and physical span scaling, then mirroring.
    geometry::WingSolidInput wing;
    wing.panels={{{{0,0},{500,20},{500,180},{0,200}},
      {{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}}};
    wing.stations={{{0,0,0,{0,0}},{0,2,1,{0,200}},gui::LineAlignment::Vertical,0},
      {{0,0,1,{500,20}},{0,2,0,{500,180}},gui::LineAlignment::Vertical,0}};
    wing.airfoils.push_back({"NACA0012",domain::AirfoilProfile::nacaSymmetric(.12),{}, {}});
    wing.wingspanMm=1000;wing.spars[0][2]={true,gui::SparShape::Round,30,60,3,1};
    wing.controls[0][0]={true,gui::HingeCut::Tape,QRectF{100,140,300,100}};
    wing.spars[0][2].insideDiameterMm=2;
    auto model=geometry::buildWingModel(wing,[](const char* p){std::cout<<p<<std::endl;});parts=bodies(model.shape);CHECK(parts.size()==6);
    CHECK(model.spars.size()==2);CHECK(std::abs(model.spars[0].volumeMm3-3.141592653589793*5/4*300)<.01);
    CHECK(model.spars[0].center.X()==model.spars[1].center.X());CHECK(model.spars[0].center.Y()==-model.spars[1].center.Y());
    // At y=250 the local LE/TE are 10/190, so the hole is at x=64.
    for(const auto& p:parts) {CHECK(!inside(p,64,250,0.5));CHECK(!inside(p,64,-250,0.5));}
    // Independent panel settings and intact controls within each panel.
    wing.panels={
      {{{0,0},{250,10},{0,200},{250,190}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{2,3}}}},
      {{{250,10},{500,20},{500,180},{250,190}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}}};
    wing.controls.resize(2);wing.controls[0][0].rectangle=QRectF{50,140,150,100};
    wing.stations={{{0,0,0,{0,0}},{0,1,0,{0,200}},gui::LineAlignment::Vertical,0},
      {{0,0,1,{250,10}},{0,1,1,{250,190}},gui::LineAlignment::Vertical,0},
      {{1,0,0,{250,10}},{1,2,1,{250,190}},gui::LineAlignment::Vertical,0},
      {{1,0,1,{500,20}},{1,2,0,{500,180}},gui::LineAlignment::Vertical,0}};
    wing.spars.resize(2);wing.spars[0][2].chordPercent=30;
    wing.spars[1][2]={true,gui::SparShape::Round,50,50,3,1};
    wing.dihedralDegrees={3,5};model=geometry::buildWingModel(wing);parts=bodies(model.shape);CHECK(parts.size()==10);
    CHECK(model.spars.size()==4);CHECK(model.spars[1].center.Z()>model.spars[0].center.Z());
    CHECK(model.spars[1].center.Y()==-model.spars[3].center.Y());
    // Restore flat geometry for the existing fixed-coordinate control assertions.
    wing.dihedralDegrees={};parts=bodies(geometry::buildWingSolid(wing));CHECK(parts.size()==10);
    // The one aileron per half stays whole rather than receiving the Mid split.
    int crossing=0;
    for(const auto& p:parts)if(inside(p,160,100,0) && inside(p,160,150,0))++crossing;
    CHECK(crossing==1);
    std::cout<<"Spar sizes, lengths, groove shapes, split plane, alignment fit, tapered placement and mirroring passed.\n";
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

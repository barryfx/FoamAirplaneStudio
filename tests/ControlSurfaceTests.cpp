#include "geometry/ControlSurfaceCut.h"
#include "geometry/WingSolidBuilder.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <Standard_Failure.hxx>
using namespace designrc;
#define CHECK(c) do {if(!(c))throw std::runtime_error(#c);} while(false)
std::vector<TopoDS_Solid> bodies(const TopoDS_Shape& shape) {
  CHECK(BRepCheck_Analyzer{shape}.IsValid());std::vector<TopoDS_Solid> result;
  for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())result.push_back(TopoDS::Solid(e.Current()));
  return result;
}
bool inside(const TopoDS_Solid& solid,double x,double y,double z) {
  BRepClass3d_SolidClassifier c{solid,gp_Pnt{x,y,z},1e-6};return c.State()==TopAbs_IN || c.State()==TopAbs_ON;
}
double volume(const TopoDS_Shape& shape) {GProp_GProps p;BRepGProp::VolumeProperties(shape,p);return p.Mass();}
int main() {
  try {
    constexpr double gap=25.4/16.0;
    const auto box=BRepPrimAPI_MakeBox{gp_Pnt{0,0,-10},200,500,20}.Shape();
    std::array<gui::ControlSurface,2> controls;
    controls[0]={true,gui::HingeCut::Tape,QRectF{100,140,300,100}};
    const auto tape=geometry::cutControlSurfaces(box,controls,{0,1},{1,0},0,1);
    auto parts=bodies(tape);CHECK(parts.size()==2);
    CHECK(inside(parts[0],139,250,0));CHECK(!inside(parts[0],141,250,0));
    CHECK(inside(parts[1],140,250,10)); // Tape contact at the top.
    CHECK(inside(parts[1],141,250,9.5));CHECK(!inside(parts[1],141,250,8));
    CHECK(!inside(parts[1],159,250,-9.5));CHECK(inside(parts[1],161,250,-9.5));
    CHECK(std::abs(volume(tape)-(200*500*20-0.5*20*20*300-2000*gap))<0.01);
    for(double end:{100.0,400.0}) {
      const double direction=end==100?1:-1;
      CHECK(!inside(parts[0],180,end+direction*gap/2,0));
      CHECK(!inside(parts[1],180,end+direction*gap/2,0));
      CHECK(inside(parts[1],180,end+direction*(gap+0.01),0));
      CHECK(!inside(parts[1],180,end+direction*(gap-0.01),0));
      CHECK(inside(parts[0],180,end-direction*0.01,0));
    }
    // Scene scale and axis choice cannot change the physical gap.
    auto scaledControls=controls;scaledControls[0].rectangle=QRectF{50,70,150,50};
    CHECK(std::abs(volume(geometry::cutControlSurfaces(box,scaledControls,{0,1},{1,0},0,2))-volume(tape))<0.01);
    scaledControls[0].rectangle=QRectF{140,100,100,300};
    CHECK(std::abs(volume(geometry::cutControlSurfaces(box,scaledControls,{1,0},{0,1},0,1))-volume(tape))<0.01);
    scaledControls[0].rectangle=QRectF{100,140,2*gap-0.01,100};
    bool tooNarrow=false;
    try{geometry::cutControlSurfaces(box,scaledControls,{0,1},{1,0},0,1);}catch(const std::exception&){tooNarrow=true;}
    CHECK(tooNarrow);
    controls[0].hinge=gui::HingeCut::Standard;
    const auto standard=geometry::cutControlSurfaces(box,controls,{0,1},{1,0},0,1);
    parts=bodies(standard);CHECK(parts.size()==2);
    CHECK(inside(parts[1],140,250,0)); // Standard center contact, equal 45 degree gaps.
    CHECK(inside(parts[1],141,250,0.5));CHECK(inside(parts[1],141,250,-0.5));
    CHECK(!inside(parts[1],141,250,2));CHECK(!inside(parts[1],141,250,-2));
    CHECK(std::abs(volume(standard)-(200*500*20-10*10*300-2200*gap))<0.01);
    for(double y:{100+gap/2,400-gap/2}) {
      CHECK(!inside(parts[0],180,y,0));CHECK(!inside(parts[1],180,y,0));
    }
    controls[0].rectangle=QRectF{250,140,200,100};
    controls[1]={true,gui::HingeCut::Tape,QRectF{20,150,180,100}};
    const auto both=geometry::cutControlSurfaces(box,controls,{0,1},{1,0},0,1);
    parts=bodies(both);CHECK(parts.size()==3);
    for(double y:{20+gap/2,200-gap/2,250+gap/2,450-gap/2})
      for(const auto& part:parts)CHECK(!inside(part,190,y,0));
    controls[1].rectangle=QRectF{300,130,150,100};bool rejected=false;
    try{geometry::cutControlSurfaces(box,controls,{0,1},{1,0},0,1);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    controls[1].enabled=false;controls[0].rectangle=QRectF{100,100,300,40};rejected=false;
    try{geometry::cutControlSurfaces(box,controls,{0,1},{1,0},0,1);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    // Integrated airfoil loft: a rectangle produces mirrored separate bodies.
    geometry::WingSolidInput wing;
    wing.panels={{{{0,0},{500,0},{500,200},{0,200}},
      {{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}}};
    wing.stations={{{0,0,0,{0,0}},{0,2,1,{0,200}},gui::LineAlignment::Vertical,0},
      {{0,0,1,{500,0}},{0,2,0,{500,200}},gui::LineAlignment::Vertical,0}};
    wing.airfoils.push_back({"NACA0012",domain::AirfoilProfile::nacaSymmetric(0.12),{}, {}});
    wing.controls[0][0]={true,gui::HingeCut::Tape,QRectF{100,140,300,100}};
    CHECK(bodies(geometry::buildWingSolid(wing)).size()==4);
    wing.controls[0][0].hinge=gui::HingeCut::Standard;
    CHECK(bodies(geometry::buildWingSolid(wing)).size()==4);
    std::cout<<"Control surface separation, mirrored bodies, hinge contact, 45-degree hinge gaps, 1/16-inch end clearances, scale invariance, volumes and invalid rectangles passed.\n";
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

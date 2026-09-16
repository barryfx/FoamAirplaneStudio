#include "geometry/SparCut.h"
#include "geometry/WingSolidBuilder.h"
#include "gui/LighteningPanel.h"
#include "gui/ProjectDocument.h"
#include <QApplication>
#include <QScreen>
#include <QEventLoop>
#include <QTimer>
#include <BRepPrimAPI_MakeBox.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <stdexcept>
using namespace designrc;
#define CHECK(c) do{if(!(c))throw std::runtime_error(std::string{#c}+" line "+std::to_string(__LINE__));}while(false)
bool inside(const TopoDS_Shape& s,double x,double y,double z) {
  // Query material intervals along a known vertical ray. The generic solid
  // classifier can choose a ray through coincident stepped-pocket edges.
  for(TopExp_Explorer e{s,TopAbs_SOLID};e.More();e.Next()) {
    IntCurvesFace_ShapeIntersector ray;ray.Load(e.Current(),1e-7);
    ray.Perform(gp_Lin{gp_Pnt{x,y,-1000},gp_Dir{0,0,1}},0,2000);
    std::vector<double> hits;for(int i=1;i<=ray.NbPnt();++i)hits.push_back(ray.Pnt(i).Z());
    std::sort(hits.begin(),hits.end());hits.erase(std::unique(hits.begin(),hits.end(),[](double a,double b){return std::abs(a-b)<1e-7;}),hits.end());
    for(double v:hits)if(std::abs(v-z)<1e-7)return true;
    for(std::size_t i=0;i+1<hits.size();i+=2)if(z>hits[i] && z<hits[i+1])return true;
  }return false;
}
int count(const TopoDS_Shape& s){CHECK(BRepCheck_Analyzer{s}.IsValid());int n=0;for(TopExp_Explorer e{s,TopAbs_SOLID};e.More();e.Next())++n;return n;}
int main(int argc,char** argv) {
  QApplication app{argc,argv};
  try {
    if(qEnvironmentVariableIsSet("FOAM_LIGHTENING_CAPTURE_ONLY")) {
      const auto path=qEnvironmentVariable("FOAM_LIGHTENING_CAPTURE");BRep_Builder builder;TopoDS_Shape lower;
      CHECK(BRepTools::Read(lower,(path+".brep").toLocal8Bit().constData(),builder));
      for(double y:{40.,62.5,90.,115.,140.,167.5,200.})std::cout<<"Lower material at "<<y<<": "<<inside(lower,30,y,-1)<<std::endl;
      if(!qEnvironmentVariableIsSet("FOAM_LIGHTENING_KEEP_MESH")){BRepTools::Clean(lower);BRepMesh_IncrementalMesh rebuild{lower,.1,false,.25,true};}
      Bnd_Box bounds;BRepBndLib::AddOptimal(lower,bounds,false,false);double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
      std::cout<<"Captured half Z bounds: "<<z0<<" to "<<z1<<std::endl;
      gui::OcctViewport view;view.resize(1100,700);view.show();app.processEvents();view.displayShape(lower);view.setCameraView(qEnvironmentVariableIsSet("FOAM_LIGHTENING_BOTTOM")?gui::CameraView::Bottom:gui::CameraView::Reset);app.processEvents();app.processEvents();
      if(const auto camera=view.cameraState())std::cout<<"Camera eye Z "<<camera->eye[2]<<std::endl;
      view.raise();view.activateWindow();QEventLoop settle;QTimer::singleShot(300,&settle,&QEventLoop::quit);settle.exec();
      CHECK(view.screen()->grabWindow(view.winId()).save(path));return 0;
    }
    gui::LighteningPanel panel;int changes=0;panel.changed=[&]{++changes;};
    CHECK(!panel.state().enabled);panel.findChild<QCheckBox*>("lighteningEnabled")->click();
    auto* wall=panel.findChild<QLineEdit*>("lighteningWall");wall->setText(".125 in");QMetaObject::invokeMethod(wall,"editingFinished");
    CHECK(std::abs(panel.state().wallMm-3.175)<1e-9);panel.setUnits(gui::ProjectUnits::Inches);CHECK(wall->text()==".125 in");
    wall->setText("invalid");QMetaObject::invokeMethod(wall,"editingFinished");CHECK(wall->text()==".125 in");
    panel.findChild<QSpinBox*>("lighteningCrossmembers")->setValue(7);CHECK(changes==3);
    gui::ProjectDocument p;p.lightening=panel.state();const auto json=gui::encodeProject(p);const auto restored=gui::decodeProject(json);
    CHECK(restored.lightening.enabled && restored.lightening.crossmembers==7 && restored.lightening.wallMm==3.175);
    CHECK(restored.lightening.text[0]==".125 in");auto legacy=json;legacy["version"]=7;legacy.remove("lightening");CHECK(!gui::decodeProject(legacy).lightening.enabled);
    auto malformed=json;auto l=malformed["lightening"].toObject();l["wallMm"]=-1;malformed["lightening"]=l;bool rejected=false;
    try{gui::decodeProject(malformed);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    const auto box=BRepPrimAPI_MakeBox{gp_Pnt{0,0,-10},200,500,20}.Shape();
    auto chord=[](double){return std::pair{0.0,200.0};};gui::SparState spars;
    spars[2].chordPercent=95; // Disabled saved Mid settings cannot move the access split.
    std::vector<std::pair<double,double>> bays{{25,245},{255,475}};
    std::cout<<"Plain wing: hollowing, retained skins, ends, rib, access split"<<std::endl;
    auto shape=geometry::cutSpars(box,spars,500,chord,{},0,0,false,2,bays);CHECK(count(shape)==2);
    CHECK(!inside(shape,100,100,5));CHECK(inside(shape,100,100,8.1));CHECK(inside(shape,100,100,-8.1));
    CHECK(inside(shape,1.9,100,5));CHECK(inside(shape,198.1,100,5));CHECK(inside(shape,100,20,5));CHECK(inside(shape,100,480,5));CHECK(inside(shape,100,250,5));
    const auto joint=geometry::cutSpars(box,spars,500,chord,{},0,0,false,2,{{-100,150}});
    CHECK(!inside(joint,100,10,5));CHECK(inside(joint,100,1.9,5));
    std::cout<<"Mid spar, groove and alignment support clearance"<<std::endl;
    spars[0]={true,gui::SparShape::Strip,60,60,6,2};spars[2]={true,gui::SparShape::Round,30,60,4,1};
    shape=geometry::cutSpars(box,spars,500,chord,{},0,0,false,2,bays);CHECK(count(shape)==2);
    CHECK(!inside(shape,60,150,1));CHECK(inside(shape,60,150,3.9));CHECK(!inside(shape,100,150,4.2));
    CHECK(inside(shape,120,150,6.1));CHECK(!inside(shape,100,150,5.8));
    for(double x:{53.5,66.5})for(double y:{100.0,400.0}) {
      CHECK(inside(shape,x+3.5,y,5));CHECK(!inside(shape,100,y,5));
      CHECK(inside(shape,x,y,1.9));
      // A vertical intersection avoids ambiguous classifier rays through the
      // many coincident stepped-pocket edges. The real socket gap is 2..2.1.
      IntCurvesFace_ShapeIntersector ray;ray.Load(shape,1e-7);ray.Perform(gp_Lin{gp_Pnt{x,y,-20},gp_Dir{0,0,1}},0,40);
      bool pegTop=false,holeTop=false;for(int i=1;i<=ray.NbPnt();++i) {
        const double z=ray.Pnt(i).Z();pegTop|=std::abs(z-2)<1e-6;holeTop|=std::abs(z-2.1)<1e-6;
        CHECK(z<2.000001 || z>2.099999);
      }CHECK(pegTop && holeTop);
    }
    std::cout<<"Appended control body remains solid and unsplit"<<std::endl;
    BRep_Builder builder;TopoDS_Compound compound;builder.MakeCompound(compound);builder.Add(compound,box);
    builder.Add(compound,BRepPrimAPI_MakeBox{gp_Pnt{210,50,-5},20,100,10}.Shape());
    spars={};shape=geometry::cutSpars(compound,spars,500,chord,{},0,0,false,2,bays);CHECK(count(shape)==3);CHECK(inside(shape,220,100,0));
    std::cout<<"Two-panel wing: global ribs, root/final-station setbacks, solid joints and tip"<<std::endl;
    geometry::WingSolidInput input;
    input.panels={
      {{{0,0},{125,0},{125,100},{0,100}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{2,3}}}},
      {{{125,0},{250,0},{250,100},{125,100}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}}};
    input.controls.resize(2);input.spars.resize(2);
    input.stations={{{0,0,0,{0,0}},{0,1,1,{0,100}},gui::LineAlignment::Vertical,0},
      {{0,0,1,{125,0}},{0,1,0,{125,100}},gui::LineAlignment::Vertical,0},
      {{1,0,0,{125,0}},{1,2,1,{125,100}},gui::LineAlignment::Vertical,0},
      {{1,0,.92,{240,0}},{1,2,.08,{240,100}},gui::LineAlignment::Vertical,0}};
    input.airfoils.push_back({"NACA0012",domain::AirfoilProfile::nacaSymmetric(.12),{}, {}});
    input.lightening.enabled=true;input.lightening.wallMm=1;input.lightening.ribMm=2;
    input.lightening.startMm=10;input.lightening.stopMm=20;input.lightening.crossmembers=3;
    shape=geometry::buildWingSolid(input,[](const char* text){std::cout<<text<<std::endl;});CHECK(count(shape)==8);
    for(double y:{5.0,62.5,115.0,124.5,167.5,230.0,245.0})CHECK(inside(shape,30,y,1));
    for(double y:{40.0,90.0,140.0,200.0}){std::cout<<"Checking cavity at span "<<y<<std::endl;CHECK(!inside(shape,30,y,1));CHECK(!inside(shape,30,-y,1));}
    // No extra partitions or zero-thickness caps between construction slices.
    IntCurvesFace_ShapeIntersector spanRay;spanRay.Load(shape,1e-7);
    spanRay.Perform(gp_Lin{gp_Pnt{30,0,1},gp_Dir{0,1,0}},.1,249);
    for(int i=1;i<=spanRay.NbPnt();++i) {
      const double y=spanRay.Pnt(i).Y();
      for(auto [a,b]:std::vector<std::pair<double,double>>{{10.1,61.4},{63.6,113.9},{116.1,123.9},{126.1,166.4},{168.6,219.9}})CHECK(y<=a || y>=b);
    }
    const auto capture=qEnvironmentVariable("FOAM_LIGHTENING_CAPTURE");
    if(!capture.isEmpty()) {
      TopoDS_Compound lower;builder.MakeCompound(lower);int index=0;
      for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next(),++index)if(index==1 || index==3)builder.Add(lower,e.Current());
      CHECK(BRepTools::Write(lower,(capture+".brep").toLocal8Bit().constData()));
      gui::OcctViewport view;view.resize(1100,700);view.show();app.processEvents();view.displayShape(lower);view.setCameraView(gui::CameraView::Reset);app.processEvents();app.processEvents();
      view.raise();view.activateWindow();QEventLoop settle;QTimer::singleShot(300,&settle,&QEventLoop::quit);settle.exec();
      CHECK(view.screen()->grabWindow(view.winId()).save(capture));
      panel.resize(410,820);panel.show();app.processEvents();CHECK(panel.grab().save(capture+".panel.png"));
    }
    input.lightening.startMm=230;rejected=false;try{geometry::buildWingSolid(input);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    input.lightening.startMm=10;input.lightening.ribMm=100;rejected=false;try{geometry::buildWingSolid(input);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    std::cout<<"Lightening tests passed"<<std::endl;
  } catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

#include "geometry/FuselageSolidBuilder.h"
#include "geometry/FuselageSymmetry.h"
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Standard_Failure.hxx>
#include <array>
#include <algorithm>
#include <set>
#include <memory>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace designrc;
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" line "+std::to_string(__LINE__));}while(false)
gui::SketchLayer polygon(std::vector<QPointF> points) {
  gui::SketchLayer p{points,{}};
  for(std::size_t i=0;i<points.size();++i)p.curves.push_back({gui::SketchTool::Line,{i,(i+1)%points.size()}});
  return p;
}
gui::SketchLayer rectangle(double x,double y,double w,double h) {
  return polygon({{x,y},{x+w,y},{x+w,y+h},{x,y+h}});
}
double volume(const TopoDS_Shape& shape) {GProp_GProps p;BRepGProp::VolumeProperties(shape,p);return p.Mass();}
std::array<double,6> bounds(const TopoDS_Shape& shape) {
  Bnd_Box box;BRepBndLib::AddOptimal(shape,box,false,false);std::array<double,6> b;
  box.Get(b[0],b[1],b[2],b[3],b[4],b[5]);return b;
}
int count(const TopoDS_Shape& shape) {
  int result=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next()) {
    CHECK(BRepCheck_Analyzer{e.Current()}.IsValid());CHECK(volume(e.Current())>0);++result;
  }
  return result;
}
void compare(const TopoDS_Shape& a,const TopoDS_Shape& b) {
  std::cout<<"  solid counts: "<<count(a)<<" / "<<count(b)<<std::endl;CHECK(count(a)==count(b));CHECK(std::abs(volume(a)-volume(b))<std::abs(volume(a))*1e-6);
  GProp_GProps am,bm;BRepGProp::VolumeProperties(a,am);BRepGProp::VolumeProperties(b,bm);
  CHECK(am.CentreOfMass().Distance(bm.CentreOfMass())<1e-5);
  const auto ab=bounds(a),bb=bounds(b);
  for(std::size_t i=0;i<ab.size();++i)CHECK(std::abs(ab[i]-bb[i])<1e-5);
  std::array<std::vector<std::unique_ptr<BRepClass3d_SolidClassifier>>,2> classifiers;
  const std::array<TopoDS_Shape,2> shapes{a,b};
  for(std::size_t side=0;side<2;++side)for(TopExp_Explorer e{shapes[side],TopAbs_SOLID};e.More();e.Next())
    classifiers[side].push_back(std::make_unique<BRepClass3d_SolidClassifier>(e.Current()));
  auto inside=[&](std::size_t side,const gp_Pnt& point) {
    for(auto& classifier:classifiers[side]) {classifier->Perform(point,1e-7);if(classifier->State()==TopAbs_IN)return true;}
    return false;
  };
  // Probe material throughout the volume, away from the pin/socket centreline.
  for(int i=0;i<13;++i)for(int j=0;j<9;++j)for(int k=0;k<7;++k) {
    gp_Pnt p{ab[0]+(ab[3]-ab[0])*(i+.37)/13,
      ab[1]+(ab[4]-ab[1])*(j+.29)/9,ab[2]+(ab[5]-ab[2])*(k+.41)/7};
    CHECK(inside(0,p)==inside(1,p));
  }
}
geometry::FuselageSolidInput fixture() {
  geometry::FuselageSolidInput input;
  input.outlines={rectangle(0,0,120,40),rectangle(0,100,120,30)};
  // Deliberately unequal left/right roof and side slopes in the drawn profile.
  input.profiles={polygon({{0,0},{10,1},{20,0},{20,30},{1,28}})};
  gui::ConstrainedLine station;
  station.first.position={60,100};station.second.position={60,130};
  station.profile=0;station.thicknessMm=6;input.stations={station};
  return input;
}
void parity(geometry::FuselageSolidInput input,const char* name) {
  std::cout<<"Case: "<<name<<std::endl;
  std::cout<<"  full symmetric / serial"<<std::endl;input.mirrorConstruction=false;const auto full=geometry::buildFuselageModel(input,[](const char* status){std::cout<<status<<std::endl;},geometry::ProcessingControl{{},false});
  std::cout<<"  half construction / parallel"<<std::endl;input.mirrorConstruction=true;const auto mirrored=geometry::buildFuselageModel(input,[](const char* status){std::cout<<status<<std::endl;});
  compare(full.shape,mirrored.shape);
  CHECK(full.stiffeners.size()==mirrored.stiffeners.size());
  for(const auto& material:full.stiffeners) {
    auto found=std::find_if(mirrored.stiffeners.begin(),mirrored.stiffeners.end(),[&](const auto& other){return other.name==material.name;});
    CHECK(found!=mirrored.stiffeners.end());CHECK(std::abs(found->volumeMm3-material.volumeMm3)<1e-5);
    CHECK(found->center.Distance(material.center)<1e-5);
  }
  CHECK(full.formers.size()==mirrored.formers.size());
  for(std::size_t i=0;i<full.formers.size();++i)compare(full.formers[i],mirrored.formers[i]);
  if(!full.servoTray.IsNull())compare(full.servoTray,mirrored.servoTray);
}
int main(int argc,char** argv) {
  try {
    const std::string filter=argc>1?argv[1]:"";
    if(filter=="--cancel-stages") {
      std::set<std::string> covered;
      for(int configuration=0;configuration<2;++configuration) {
        auto input=fixture();input.thicken=true;
        if(configuration==0) {
          input.stiffeners.count=1;
          input.servoTray=QRectF{40,108,20,2};input.formers={QRectF{20,98,3,34},QRectF{85,98,3,34}};
          input.formerRotationDegrees={12,-8};
        } else {
          input.cuts.resize(2);input.cuts[1]=polygon({{40,99},{40,105},{80,105},{80,99}});input.cuts[1].curves.pop_back();
          input.holes.resize(4);input.holes[3]=rectangle(90,110,8,8);
        }
        std::vector<std::string> stages;
        geometry::buildFuselageModel(input,[&](const char* stage){if(covered.insert(stage).second)stages.emplace_back(stage);});
        for(const auto& target:stages) {
          std::stop_source stop;bool reached=false,cancelled=false;
          try {
            geometry::buildFuselageModel(input,[&](const char* stage) {
              if(target==stage){reached=true;stop.request_stop();}
            },{stop.get_token()});
          }catch(const geometry::ProcessingCancelled&){cancelled=true;}
          CHECK(reached&&cancelled);std::cout<<"Cancelled: "<<target<<std::endl;
        }
      }
      std::cout<<"Cancelled every reported fuselage stage: "<<covered.size()<<std::endl;return 0;
    }
    const auto check=[&](geometry::FuselageSolidInput input,const char* name) {
      if(filter.empty()||std::string{name}.find(filter)!=std::string::npos)parity(input,name);
    };
    auto input=fixture();check(input,"solid asymmetric profile becomes symmetric");
    input.thicken=true;check(input,"closed cavity");
    auto stiffened=input;stiffened.stiffeners.count=2;check(stiffened,"strip stiffeners before reflection");
    stiffened.stiffeners.shape=gui::SparShape::Round;stiffened.stiffeners.diameterMm=2;
    check(stiffened,"round stiffeners before reflection");
    input.stations[0].first.position.setX(0);input.stations[0].second.position.setX(0);
    auto tail=input.stations.front();tail.first.position.setX(120);tail.second.position.setX(120);
    tail.thicknessMm=5;input.stations.push_back(tail);check(input,"open ends and varying walls");
    input=fixture();input.thicken=true;
    input.servoTray=QRectF{40,108,20,2};input.formers={QRectF{20,98,3,34},QRectF{85,98,3,34}};input.formerRotationDegrees={12,-8};
    check(input,"tray, parallel rotated formers and retaining rails");
    input=fixture();input.thicken=true;
    input.cuts.resize(2);input.cuts[1]=polygon({{40,99},{40,105},{80,105},{80,99}});
    input.cuts[1].curves.pop_back();
    input.holes.resize(4);input.holes[3]=rectangle(90,110,8,8);
    check(input,"whole hatch and right-wall-only hole");
    input=fixture();input.thicken=true;input.cuts.resize(2);
    input.cuts[0]=polygon({{40,-1},{90,41}});input.cuts[0].curves.resize(1);
    check(input,"oblique top cut keeps the largest whole component");
    input=fixture();input.outlines={polygon({{0,20},{60,0},{120,20},{60,40}}),
      polygon({{0,115},{60,100},{120,115},{60,130}})};
    check(input,"pointed ends");
    std::stop_source stop;stop.request_stop();bool cancelled=false;
    try{geometry::buildFuselageModel(input,{}, {stop.get_token()});}
    catch(const geometry::ProcessingCancelled&){cancelled=true;}CHECK(cancelled);
    std::cout<<"Mirrored/full symmetric geometry parity passed\n";return 0;
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

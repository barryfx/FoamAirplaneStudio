#include "geometry/FuselageSolidBuilder.h"
#include "TestCheck.h"
#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <QCoreApplication>
#include <chrono>
#include <iostream>
using namespace designrc;
static gui::SketchLayer rect(double x,double y,double w,double h) {
  return {{{x,y},{x+w,y},{x+w,y+h},{x,y+h}},
    {{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},
     {gui::SketchTool::Line,{2,3}},{gui::SketchTool::Line,{3,0}}}};
}
static void compare(const TopoDS_Shape& a,const TopoDS_Shape& b) {
  TEST_CHECK(BRepCheck_Analyzer{a}.IsValid()&&BRepCheck_Analyzer{b}.IsValid());
  GProp_GProps x,y;BRepGProp::VolumeProperties(a,x);BRepGProp::VolumeProperties(b,y);
  TEST_CHECK(std::abs(x.Mass()-y.Mass())<1e-5);
  TEST_CHECK(x.CentreOfMass().Distance(y.CentreOfMass())<1e-6);
  int ac=0,bc=0;for(TopExp_Explorer e{a,TopAbs_SOLID};e.More();e.Next())++ac;
  for(TopExp_Explorer e{b,TopAbs_SOLID};e.More();e.Next())++bc;TEST_CHECK(ac==bc);
  Bnd_Box ab,bb;BRepBndLib::AddOptimal(a,ab,false,false);BRepBndLib::AddOptimal(b,bb,false,false);
  TEST_CHECK(ab.CornerMin().Distance(bb.CornerMin())<1e-6&&ab.CornerMax().Distance(bb.CornerMax())<1e-6);
}
int main(int argc,char** argv) {
  QCoreApplication app{argc,argv};
  try {
    geometry::FuselageSolidInput input;
    input.outlines={rect(0,0,200,40),rect(0,0,200,30)};input.profiles={rect(0,0,40,30)};
    input.lengthMm=200;input.thicken=true;
    gui::ConstrainedLine station;station.first.position={100,0};station.profile=0;station.thicknessMm=5;input.stations={station};
    input.formers={QRectF{25,-2,3,34}};input.formerRotationDegrees={0};input.servoTray=QRectF{50,10,20,2};
    auto base=geometry::buildFuselageModel(input);TEST_CHECK(base.checkpoint);
    auto reused=[&](const auto& changed) {
      bool hit=false,loft=false;
      auto result=geometry::buildFuselageModel(changed,[&](const char* stage) {
        const std::string text{stage};hit|=text.find("reusing cached")!=text.npos;loft|=text.find("lofting")!=text.npos;
      },{},base.checkpoint);
      TEST_CHECK(hit&&!loft&&result.checkpoint==base.checkpoint);return result;
    };
    for(int feature=0;feature<3;++feature) {
      auto changed=input;
      if(feature==0)changed.stiffeners.count=1;
      if(feature==1){changed.holes.resize(4);changed.holes[3]=rect(130,10,10,8);}
      if(feature==2){changed.cuts.resize(4);changed.cuts[3]=rect(120,10,30,8);}
      const auto start=std::chrono::steady_clock::now();auto resumed=reused(changed);
      const auto middle=std::chrono::steady_clock::now();auto fresh=geometry::buildFuselageModel(changed);
      const auto end=std::chrono::steady_clock::now();
      std::cout<<"feature="<<feature<<",cached_ms="<<std::chrono::duration<double,std::milli>(middle-start).count()
        <<",fresh_ms="<<std::chrono::duration<double,std::milli>(end-middle).count()<<std::endl;
      compare(resumed.shape,fresh.shape);compare(resumed.body,fresh.body);
      compare(resumed.servoTray,fresh.servoTray);compare(resumed.formers.at(0),fresh.formers.at(0));
      TEST_CHECK(resumed.stiffeners.size()==fresh.stiffeners.size());
      for(std::size_t i=0;i<resumed.stiffeners.size();++i) {
        TEST_CHECK(std::abs(resumed.stiffeners[i].volumeMm3-fresh.stiffeners[i].volumeMm3)<1e-6);
        TEST_CHECK(resumed.stiffeners[i].center.Distance(fresh.stiffeners[i].center)<1e-6);
      }
    }
    // Check every upstream dependency without paying for a new loft each time.
    // A miss reports the alignment stage before the cancelled rebuild stops.
    for(int field=0;field<12;++field) {
      auto changed=input;
      switch(field) {
        case 0:changed.outlines[0].points[1].rx()+=1;break;
        case 1:changed.profiles[0].points[1].rx()+=1;break;
        case 2:changed.stations[0].first.position.rx()+=1;break;
        case 3:changed.stations[0].thicknessMm=6;break;
        case 4:changed.lengthMm=210;break;
        case 5:changed.noseOpen=true;break;
        case 6:changed.tailOpen=true;break;
        case 7:changed.formers[0].translate(1,0);break;
        case 8:changed.formerRotationDegrees[0]=2;break;
        case 9:changed.servoTray->translate(1,0);break;
        case 10:changed.mirrorConstruction=false;break;
        case 11:changed.thicken=false;changed.formers.clear();changed.servoTray.reset();break;
      }
      std::stop_source stop;bool hit=false,miss=false,failed=false;
      try {geometry::buildFuselageModel(changed,[&](const char* stage) {
        std::string text{stage};hit|=text.find("reusing cached")!=text.npos;
        miss|=text.find("aligning Top")!=text.npos;stop.request_stop();
      },{stop.get_token()},base.checkpoint);}catch(const std::exception&){failed=true;}
      TEST_CHECK(failed&&miss&&!hit);
    }
    std::stop_source stop;bool cancelled=false;
    try {geometry::buildFuselageModel(input,[&](const char*){stop.request_stop();},{stop.get_token()},base.checkpoint);}
    catch(const std::exception&){cancelled=true;}TEST_CHECK(cancelled);
    auto invalid=input;invalid.stiffeners.count=1;invalid.stiffeners.stopPercent=0;bool failed=false;
    try{reused(invalid);}catch(const std::exception&){failed=true;}TEST_CHECK(failed);
    // Neither failed/cancelled work nor previous meshing/cuts contaminate reuse.
    compare(reused(input).shape,base.shape);
    // Adding the first wall hole also works when there were no inserts or cuts
    // to request a cavity during the original generation.
    input.formers.clear();input.servoTray.reset();base=geometry::buildFuselageModel(input);
    input.holes.resize(4);input.holes[3]=rect(130,10,10,8);
    compare(reused(input).shape,geometry::buildFuselageModel(input).shape);
    std::cout<<"Fuselage checkpoint reuse, invalidation, isolation and parity passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

#include "geometry/FuselageSolidBuilder.h"
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>
using namespace designrc;
using Clock=std::chrono::steady_clock;
static gui::SketchLayer rectangle(double w,double h) {
  gui::SketchLayer layer{{{0,0},{w,0},{w,h},{0,h}}, {}};
  for(std::size_t i=0;i<4;++i)layer.curves.push_back({gui::SketchTool::Line,{i,(i+1)%4}});
  return layer;
}
int main(int argc,char** argv) {
  try {
    const int repetitions=argc>1?std::stoi(argv[1]):5;
    geometry::FuselageSolidInput input{{rectangle(200,40),
      {{{0,0},{100,-5},{200,0},{200,30},{100,25},{0,30}},
       {{gui::SketchTool::Spline,{0,1,2}},{gui::SketchTool::Line,{2,3}},
        {gui::SketchTool::Spline,{3,4,5}},{gui::SketchTool::Line,{5,0}}}}},
      {},{rectangle(40,30)},200,true};
    gui::ConstrainedLine first,last;first.first.position={50,0};first.profile=0;first.thicknessMm=5;
    last=first;last.first.position={150,0};input.stations={first,last};
    input.stiffeners.count=1;input.stiffeners.diameterMm=2;
    std::cout<<"shape,trial,groove_ms,total_ms,volume_mm3,cx,cy,cz,solids,carbon_mm3,carbon_x,carbon_z\n"<<std::setprecision(12);
    for(auto shape:{gui::SparShape::Strip,gui::SparShape::Round}) {
      input.stiffeners.shape=shape;
      for(int trial=-1;trial<repetitions;++trial) {
        auto begin=Clock::now(),grooveBegin=begin,grooveEnd=begin;
        const auto result=geometry::buildFuselageModel(input,[&](const char* message){
          const std::string_view stage{message};
          if(stage=="Fuselage: cutting carbon fiber stiffener grooves...")grooveBegin=Clock::now();
          if(stage=="Fuselage: reflecting the completed right half as a separate left part...")grooveEnd=Clock::now();
        });
        const auto end=Clock::now();
        if(!BRepCheck_Analyzer{result.body}.IsValid()||result.stiffeners.size()!=2||grooveEnd<=grooveBegin)
          throw std::runtime_error("Invalid benchmark result");
        GProp_GProps mass;BRepGProp::VolumeProperties(result.body,mass,1e-7);
        int solids=0;for(TopExp_Explorer e{result.body,TopAbs_SOLID};e.More();e.Next())++solids;
        double carbon=0,x=0,z=0;for(const auto& stock:result.stiffeners){carbon+=stock.volumeMm3;x+=stock.volumeMm3*stock.center.X();z+=stock.volumeMm3*stock.center.Z();}
        if(trial<0)continue; // Warm each shape's kernel paths before measurement.
        const auto center=mass.CentreOfMass();
        std::cout<<(shape==gui::SparShape::Strip?"strip":"round")<<','<<trial<<','
          <<std::chrono::duration<double,std::milli>(grooveEnd-grooveBegin).count()<<','
          <<std::chrono::duration<double,std::milli>(end-begin).count()<<','
          <<mass.Mass()<<','<<center.X()<<','<<center.Y()<<','<<center.Z()<<','<<solids<<','
          <<carbon<<','<<x/carbon<<','<<z/carbon<<std::endl;
      }
    }
  }catch(const Standard_Failure& error){std::cerr<<error.what()<<'\n';return 1;}
   catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

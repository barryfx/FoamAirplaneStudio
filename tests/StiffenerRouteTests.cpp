#include "TestCheck.h"
#include "geometry/FuselageStiffeners.h"
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <TopExp_Explorer.hxx>
#include <fstream>
#include <iostream>
#include <numbers>
using namespace designrc;
int main(int argc,char** argv) {
  try {
    TEST_CHECK(argc==2);std::ifstream input{argv[1]};TEST_CHECK(input.good());
    std::vector<gp_Pnt> route;double x,y,z;
    while(input>>x>>y>>z)route.emplace_back(x,y,z);
    TEST_CHECK(route.size()==65);
    // The original route made a smooth cutter double back and swing >50 mm
    // from its input samples. A simple matching body isolates route construction
    // from the full project's much slower hollowing and support-rail stages.
    BRepOffsetAPI_ThruSections loft{true,true,1e-7};loft.CheckCompatibility(false);
    std::vector<geometry::FuselageWallSection> walls;
    for(const auto& p:route) {
      BRepBuilderAPI_MakePolygon wire;
      for(const auto& q:{gp_Pnt{p.X(),0,p.Z()-20},gp_Pnt{p.X(),p.Y(),p.Z()-20},
          gp_Pnt{p.X(),p.Y(),p.Z()+20},gp_Pnt{p.X(),0,p.Z()+20}})wire.Add(q);
      wire.Close();loft.AddWire(wire.Wire());
      walls.push_back({p.X(),8,{{-p.Y(),p.Z()-20},{p.Y(),p.Z()-20},{p.Y(),p.Z()+20},{-p.Y(),p.Z()+20}}});
    }
    loft.Build();TEST_CHECK(loft.IsDone());const auto body=loft.Shape();
    TEST_CHECK(BRepCheck_Analyzer{body}.IsValid());
    const double length=654.0478357447936,span=route.back().X()-route.front().X();
    gui::StiffenerState settings;settings.count=1;settings.startPercent=20;settings.stopPercent=75;
    settings.widthMm=3;settings.heightMm=1;settings.diameterMm=2;
    for(auto shape:{gui::SparShape::Strip,gui::SparShape::Round}) {
      settings.shape=shape;std::vector<geometry::SparMaterial> stock;
      std::cout<<(shape==gui::SparShape::Strip?"Strip":"Round")<<" bounded route..."<<std::endl;
      const auto cut=geometry::cutFuselageStiffeners(body,walls,length,settings,stock,{},true);
      TEST_CHECK(BRepCheck_Analyzer{cut}.IsValid());
      int solids=0;for(TopExp_Explorer e{cut,TopAbs_SOLID};e.More();e.Next())++solids;
      TEST_CHECK(solids==1&&stock.size()==1);
      const double area=shape==gui::SparShape::Strip?3:std::numbers::pi;
      TEST_CHECK(std::abs(stock[0].volumeMm3-span*area)<.01);
      BRepClass3d_SolidClassifier classifier{cut};
      const auto check=[&](const gp_Pnt& p,TopAbs_State expected){classifier.Perform(p,1e-6);TEST_CHECK(classifier.State()==expected);};
      // Check the finished cut *between* every pair of samples, including the
      // first span that failed in BabyBuzzard. Foam below and beside the groove
      // must remain, while the shallow groove must actually be open.
      for(std::size_t i=1;i<route.size();++i)for(double t:{.25,.5,.75}) {
        const gp_Pnt p{route[i-1].XYZ()*(1-t)+route[i].XYZ()*t};
        check({p.X(),p.Y()-.5,p.Z()},TopAbs_OUT);
        check({p.X(),p.Y()-1.1,p.Z()},TopAbs_IN);
        check({p.X(),p.Y()-.5,p.Z()-2},TopAbs_IN);
        check({p.X(),p.Y()-.5,p.Z()+2},TopAbs_IN);
      }
    }
    std::cout<<"Finished strip and round cuts preserve depth and location between all route samples.\n";
    return 0;
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

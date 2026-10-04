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
static TopoDS_Shape makeBody(const std::vector<geometry::FuselageWallSection>& walls) {
  BRepOffsetAPI_ThruSections loft{true,true,1e-7};loft.CheckCompatibility(false);
  for(const auto& s:walls) {
    const auto a=s.perimeter[1],b=s.perimeter[2];BRepBuilderAPI_MakePolygon wire;
    for(auto p:{gp_Pnt{s.x,0,a.y()},gp_Pnt{s.x,a.x(),a.y()},gp_Pnt{s.x,b.x(),b.y()},gp_Pnt{s.x,0,b.y()}})wire.Add(p);
    wire.Close();loft.AddWire(wire.Wire());
  }
  loft.Build();TEST_CHECK(loft.IsDone());TEST_CHECK(BRepCheck_Analyzer{loft.Shape()}.IsValid());return loft.Shape();
}
static void changingProfileDepth() {
  std::vector<geometry::FuselageWallSection> walls{
    {0,8,{{-10,-10},{10,-10},{20,10},{-20,10}}},
    {50,8,{{-10,-30},{10,-30},{90,170},{-90,170}}},
    {100,8,{{-20,-10},{20,-10},{20,10},{-20,10}}}};
  gui::StiffenerState settings;settings.count=1;settings.shape=gui::SparShape::Strip;
  settings.startPercent=0;settings.stopPercent=100;settings.widthMm=3;settings.heightMm=2;
  std::vector<geometry::SparMaterial> stock;
  auto cut=geometry::cutFuselageStiffeners(makeBody(walls),walls,100,settings,stock,{},true);
  TEST_CHECK(stock.size()==1&&std::abs(stock[0].center.Z())<1e-7);
  BRepClass3d_SolidClassifier classifier{cut};
  // The skin at Z=0 is nonlinear even though each wall face is ruled. Check
  // depth against its analytic intersection, not the production route samples.
  for(double x=.5;x<100;x+=.5) {
    const bool front=x<50;const double t=front?x/50:(x-50)/50;
    const double lowY=front?10:10+10*t,highY=front?20+70*t:90-70*t;
    const double lowZ=front?-10-20*t:-30+20*t,highZ=front?10+160*t:170-160*t;
    const double skin=lowY+(highY-lowY)*(-lowZ)/(highZ-lowZ);
    classifier.Perform({x,skin-1.985,0},1e-6);TEST_CHECK(classifier.State()==TopAbs_OUT);
    classifier.Perform({x,skin-2.015,0},1e-6);TEST_CHECK(classifier.State()==TopAbs_IN);
  }
  // A line joining valid endpoints can leave the side outline in the middle.
  walls[1]={50,8,{{-20,70},{20,70},{20,90},{-20,90}}};
  bool rejected=false;const auto before=stock.size();
  try{geometry::cutFuselageStiffeners(makeBody(walls),walls,100,settings,stock,{},true);}
  catch(const std::exception& e){const std::string message=e.what();rejected=message.find("% from the nose")!=message.npos&&message.find("does not fit on the side")!=message.npos;}
  TEST_CHECK(rejected&&stock.size()==before);
}
int main(int argc,char** argv) {
  try {
    TEST_CHECK(argc==2);std::ifstream input{argv[1]};TEST_CHECK(input.good());
    std::vector<gp_Pnt> route;double x,y,z;
    while(input>>x>>y>>z)route.emplace_back(x,y,z);
    TEST_CHECK(route.size()==65);
    // The original route made a smooth cutter double back and swing >50 mm
    // from its input samples. A simple matching body isolates route construction
    // from the full project's much slower hollowing and support-rail stages.
    std::vector<geometry::FuselageWallSection> walls;
    for(const auto& p:route)walls.push_back({p.X(),8,{{-p.Y(),p.Z()-20},{p.Y(),p.Z()-20},{p.Y(),p.Z()+20},{-p.Y(),p.Z()+20}}});
    const auto body=makeBody(walls);
    const double length=654.0478357447936,span=route.back().X()-route.front().X();
    gui::StiffenerState settings;settings.count=1;settings.startPercent=20;settings.stopPercent=75;
    settings.widthMm=3;settings.heightMm=1;settings.diameterMm=2;
    for(int count:{1,3})for(auto shape:{gui::SparShape::Strip,gui::SparShape::Round}) {
      settings.count=count;
      settings.shape=shape;std::vector<geometry::SparMaterial> stock;
      std::cout<<(shape==gui::SparShape::Strip?"Strip":"Round")<<" straight side-view routes, count "<<count<<"..."<<std::endl;
      const auto cut=geometry::cutFuselageStiffeners(body,walls,length,settings,stock,{},true);
      TEST_CHECK(BRepCheck_Analyzer{cut}.IsValid());
      int solids=0;for(TopExp_Explorer e{cut,TopAbs_SOLID};e.More();e.Next())++solids;
      TEST_CHECK(solids==1&&stock.size()==static_cast<std::size_t>(count));
      const double area=shape==gui::SparShape::Strip?3:std::numbers::pi;
      for(int n=0;n<count;++n){
        TEST_CHECK(std::abs(stock[n].volumeMm3-span*area)<.01);
        TEST_CHECK(std::abs(stock[n].center.Z()-((route.front().Z()+route.back().Z())/2-20+40.*(n+1)/(count+1)))<1e-6);
      }
      BRepClass3d_SolidClassifier classifier{cut};
      const auto check=[&](const gp_Pnt& p,TopAbs_State expected){classifier.Perform(p,1e-6);TEST_CHECK(classifier.State()==expected);};
      // Check the finished cut *between* every pair of samples, including the
      // first span that failed in BabyBuzzard. Foam below and beside the groove
      // must remain, while the shallow groove must actually be open.
      for(int n=0;n<count;++n)for(std::size_t i=1;i<route.size();++i)for(double t:{.25,.5,.75}) {
        gp_Pnt p{route[i-1].XYZ()*(1-t)+route[i].XYZ()*t};
        // Z depends only on the two endpoint heights, whereas Y still follows
        // the actual curved skin. The former curved-in-side-view code fails here.
        p.SetZ(route.front().Z()+(route.back().Z()-route.front().Z())*(p.X()-route.front().X())/span-20+40.*(n+1)/(count+1));
        check({p.X(),p.Y()-.5,p.Z()},TopAbs_OUT);
        check({p.X(),p.Y()-1.1,p.Z()},TopAbs_IN);
        check({p.X(),p.Y()-.5,p.Z()-2},TopAbs_IN);
        check({p.X(),p.Y()-.5,p.Z()+2},TopAbs_IN);
      }
    }
    changingProfileDepth();
    std::cout<<"Strip and round cuts stay straight in side view at constant inward depth, with evenly spaced endpoint heights.\n";
    return 0;
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

#include "geometry/FuselageSolidBuilder.h"
#include "TestCheck.h"
#include <QCoreApplication>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <TopExp_Explorer.hxx>
#include <iostream>
using namespace designrc;
static gui::SketchLayer rect(double drift=0) {
  return {{{0,0},{200,0},{200,40},{drift,40}},
    {{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}},{gui::SketchTool::Line,{3,0}}}};
}
static bool material(const TopoDS_Shape& shape,gp_Pnt p) {
  for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())
    if(BRepClass3d_SolidClassifier{e.Current(),p,1e-6}.State()==TopAbs_IN)return true;
  return false;
}
int main(int argc,char** argv) {
  QCoreApplication app{argc,argv};
  try {
    geometry::FuselageSolidInput input;
    input.outlines={rect(),rect(2)};input.profiles={rect()};input.thicken=true;
    gui::ConstrainedLine station;station.first.position={20,0};station.second.position={20,40};station.profile=0;station.thicknessMm=4;
    input.stations={station};input.noseOpen=false;input.tailOpen=false;
    auto closed=geometry::buildFuselageModel(input,{},{{},false});
    // The slanted nose occupies Z=-40..0 (origin is the upper nose tip).
    TEST_CHECK(material(closed.body,{3,5,-20}));TEST_CHECK(material(closed.body,{198,5,-20}));
    input.noseOpen=true;
    auto nose=geometry::buildFuselageModel(input,{},{{},false});
    TEST_CHECK(BRepCheck_Analyzer{nose.body}.IsValid());
    TEST_CHECK(!material(nose.body,{3,5,-20}));TEST_CHECK(material(nose.body,{198,5,-20}));
    TEST_CHECK(!material(nose.body,{.5,19,-20}));TEST_CHECK(material(nose.body,{3,19,-20}));
    input.tailOpen=true;
    auto both=geometry::buildFuselageModel(input,{},{{},false});
    TEST_CHECK(!material(both.body,{198,5,-20}));TEST_CHECK(!material(both.body,{3,5,-20}));
    input.noseOpen=false;
    auto tail=geometry::buildFuselageModel(input,{},{{},false});
    TEST_CHECK(material(tail.body,{3,5,-20}));TEST_CHECK(!material(tail.body,{198,5,-20}));
    input.outlines[1].points[1].setX(198);
    auto slantedTail=geometry::buildFuselageModel(input,{},{{},false});
    TEST_CHECK(!material(slantedTail.body,{199.5,19,-20}));
    TEST_CHECK(material(slantedTail.body,{197,19,-20}));
    TEST_CHECK(!material(slantedTail.body,{197,5,-20}));
    std::cout<<"Independent explicit ends and slanted opening material checks passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

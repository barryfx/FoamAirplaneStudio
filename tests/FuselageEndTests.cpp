#include "geometry/FuselageEndRegistration.h"
#include "geometry/FuselageSolidBuilder.h"
#include "gui/ProjectDocument.h"
#include <QCoreApplication>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace designrc;
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__));}while(false)
static gui::SketchLayer outline(double drift=.25,double height=30) {
  return {{{0,0},{199.9,0},{200,height},{drift,height}},
    {{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}},{gui::SketchTool::Line,{3,0}}}};
}
static std::pair<double,double> endSpan(const geometry::RegisteredFuselageOutline& shape,bool nose) {
  const double x=nose?shape.noseX:shape.tailX;double lo=1e100,hi=-1e100;
  for(auto p:shape.boundary)if(std::abs(p.x()-x)<1e-8){lo=std::min(lo,p.y());hi=std::max(hi,p.y());}
  return {lo,hi};
}
int main(int argc,char** argv) {
  QCoreApplication app{argc,argv};
  try {
    auto sketch=outline();const auto original=sketch.points;
    auto registered=geometry::registerFuselageEnds(sketch);
    CHECK(registered.flatNose&&registered.flatTail);CHECK(sketch.points==original);
    CHECK(endSpan(registered,true)==std::make_pair(0.,30.));CHECK(endSpan(registered,false)==std::make_pair(0.,30.));
    CHECK(registered.noseX==0&&registered.tailX==200);
    CHECK(registered.noseStation(.25)&&registered.tailStation(199.9));
    CHECK(!registered.noseStation(.501)&&!registered.tailStation(199.499));
    const auto transform=geometry::fuselageSideTransform(sketch,{});
    CHECK(transform.left==0&&transform.verticalOrigin==15&&transform.scale==1);
    auto scaled=geometry::registerFuselageEnds(sketch,400);
    CHECK(scaled.flatNose&&scaled.flatTail&&scaled.scale==2);
    CHECK(scaled.noseStation(.25)&&!scaled.noseStation(.251));
    CHECK(!geometry::registerFuselageEnds(sketch,401).flatNose);
    CHECK(!geometry::registerFuselageEnds(outline(.6)).flatNose);
    CHECK(!geometry::registerFuselageEnds(outline(.4,4)).flatNose);
    for(auto& curve:sketch.curves)std::reverse(curve.points.begin(),curve.points.end());
    CHECK(geometry::registerFuselageEnds(sketch).flatNose);
    auto vertical=geometry::registerFuselageEnds(outline(0));CHECK(vertical.flatNose&&vertical.noseStation(.1));
    gui::SketchLayer pointed{{{0,15},{200,0},{200,30}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,0}}}};
    const auto tip=geometry::registerFuselageEnds(pointed);CHECK(!tip.flatNose&&!tip.noseStation(.01));
    auto curved=outline();curved.curves.back().type=gui::SketchTool::Spline;
    CHECK(!geometry::registerFuselageEnds(curved).flatNose);
    if(argc>1) {
      QString error;const auto project=gui::readProject(QString::fromLocal8Bit(argv[1]),error);CHECK(project);
      const auto before=gui::encodeProject(*project,false);
      const auto side=geometry::registerFuselageEnds(project->fuselage.layers[1]);
      const auto top=geometry::registerFuselageEnds(project->fuselage.layers[0],side.tailX-side.noseX);
      const auto station=std::min_element(project->fuselageStations.lines.begin(),project->fuselageStations.lines.end(),[](const auto& a,const auto& b){return a.first.position.x()<b.first.position.x();});
      CHECK(station!=project->fuselageStations.lines.end());CHECK(side.flatNose&&top.flatNose);
      CHECK(side.noseStation(station->first.position.x()));
      CHECK(endSpan(side,true).second-endSpan(side,true).first>1);
      CHECK(endSpan(top,true).second-endSpan(top,true).first>1);
      CHECK(gui::encodeProject(*project,false)==before);
      std::cout<<"Project nose: both end edges registered; foremost station opens nose; inputs unchanged\n";
      std::cout<<"Side end height "<<endSpan(side,true).second-endSpan(side,true).first<<"; top end width "<<endSpan(top,true).second-endSpan(top,true).first<<'\n';
    }
    std::cout<<"End registration, physical tolerances, alignment, inward stations and pointed/sloping end checks passed; no solid generation\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

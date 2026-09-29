#include "geometry/FuselageSolidBuilder.h"
#include "gui/ProjectDocument.h"
#include "gui/OcctViewport.h"
#include <QApplication>
#include <QScreen>
#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace designrc;
#define CHECK(c) do {if(!(c))throw std::runtime_error(#c);}while(false)
gui::SketchLayer polygon(std::vector<QPointF> points) {
  gui::SketchLayer result{points,{}};
  for(std::size_t i=0;i<points.size();++i)result.curves.push_back({gui::SketchTool::Line,{i,(i+1)%points.size()}});
  return result;
}
void checkUp(const gui::SketchLayer& profile) {
  const auto samples=geometry::sampleFuselageProfile(profile);
  CHECK(samples.size()==64);
  CHECK(std::abs(samples[0].x()-.5)<.015&&samples[0].y()<.1);
  CHECK(samples[16].x()>.9&&std::abs(samples[16].y()-.5)<.015);
  CHECK(std::abs(samples[32].x()-.5)<.015&&samples[32].y()>.9);
  CHECK(samples[48].x()<.1&&std::abs(samples[48].y()-.5)<.015);
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};
  try {
    // Opposite small roof slopes previously chose opposite top corners as seams.
    const auto front=polygon({{0,.03},{0,1},{1,1},{1,0}});
    const auto back=polygon({{0,0},{1,.03},{1,1},{0,1}});
    checkUp(front);checkUp(back);
    const auto first=geometry::sampleFuselageProfile(front),second=geometry::sampleFuselageProfile(back);
    for(std::size_t i=0;i<first.size();++i)CHECK((QLineF{first[i],second[i]}.length()<.05));
    auto reversed=front;std::reverse(reversed.curves.begin(),reversed.curves.end());
    for(auto& curve:reversed.curves)std::reverse(curve.points.begin(),curve.points.end());
    std::rotate(reversed.curves.begin(),reversed.curves.begin()+1,reversed.curves.end());
    const auto alternate=geometry::sampleFuselageProfile(reversed);
    for(std::size_t i=0;i<first.size();++i)CHECK((QLineF{first[i],alternate[i]}.length()<1e-8));
    geometry::FuselageSolidInput input;
    input.outlines={polygon({{0,0},{100,0},{100,10},{0,10}}),polygon({{0,0},{100,0},{100,10},{0,10}})};
    input.profiles={front,back};input.lengthMm=100;
    gui::ConstrainedLine a,b;a.first.position={10,0};a.profile=0;b.first.position={90,0};b.profile=1;input.stations={a,b};
    const auto shape=geometry::buildFuselageSolid(input);GProp_GProps volume;BRepGProp::VolumeProperties(shape,volume);
    CHECK(volume.Mass()>9800&&volume.Mass()<10001); // No twisted, narrowed intermediate sections.
    // A shoulder narrower than the old 1/32 spacing must still guide the skin.
    input.outlines[0]=polygon({{0,0},{100,0},{100,10},{40.4,10},{40.3,20},{40.2,10},{0,10}});
    const auto guided=geometry::buildFuselageSolid(input);
    Bnd_Box guideBox;BRepBndLib::AddOptimal(guided,guideBox,false,false);
    double xmin,ymin,zmin,xmax,ymax,zmax;guideBox.Get(xmin,ymin,zmin,xmax,ymax,zmax);
    // The 20 mm guide width is centered about Y=0 by mirrored construction.
    CHECK(std::abs(ymin+10)<1e-5&&std::abs(ymax-10)<1e-5);
    if(argc>1) {
      QString error;const auto project=gui::readProject(QString::fromLocal8Bit(argv[1]),error);
      if(!project)throw std::runtime_error(error.toStdString());
      for(const auto& station:project->fuselageStations.lines)checkUp(project->fuselageProfiles.layers.at(*station.profile));
      geometry::FuselageSolidInput actual{project->fuselage.layers,project->fuselageStations.lines,project->fuselageProfiles.layers,
        project->reference.toScale?std::nullopt:project->reference.fuselageLengthMm};
      const auto solid=geometry::buildFuselageSolid(actual,[](const char* message){std::cout<<message<<std::endl;});
      gui::OcctViewport view;view.resize(1200,800);view.show();app.processEvents();view.displayShape(solid);app.processEvents();
      if(argc>2)CHECK(view.screen()->grabWindow(view.winId()).save(QString::fromLocal8Bit(argv[2])));
    }
    std::cout<<"Fuselage drawn-up registration, reversed curves, roof slopes and solid volume passed\n";
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

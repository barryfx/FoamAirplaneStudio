#include "gui/MainWindow.h"
#include "gui/FuselageProfilePanel.h"
#include "gui/SketchBoundary.h"
#include "geometry/FuselageSolidBuilder.h"
#include "WaitForModel.h"
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <QAction>
#include <QApplication>
#include <QJsonArray>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPushButton>
#include <QSettings>
#include <QScreen>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolBar>
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace designrc::gui;
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at line "+std::to_string(__LINE__));}while(false)
SketchLayer rectangle(double x,double y,double width,double height) {
  return {{{x,y},{x+width,y},{x+width,y+height},{x,y+height}},
    {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QTemporaryDir dir;
  QCoreApplication::setOrganizationName("FoamFuselageProfilesTests");QCoreApplication::setApplicationName("FoamFuselageProfilesTests");
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
  try {
    ProjectDocument p;p.reference.wingspanMm=1000;p.reference.fuselageLengthMm=700;p.wingspanText="1000 mm";p.fuselageText="700 mm";
    p.wing.layers[0]=rectangle(10,10,90,70);p.wing.layers[0].curves.pop_back();
    p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},{{0,0,1,{100,10}},{0,2,0,{100,80}},LineAlignment::Vertical,0}};
    p.airfoils.entries.push_back({"NACA",designrc::domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
    p.fuselage.layers={rectangle(200,150,400,100),rectangle(200,350,400,80)};
    p.fuselageStations.lines={{{1,0,.25,{300,350}},{1,2,.75,{300,430}},LineAlignment::Vertical},{{1,0,.75,{500,350}},{1,2,.25,{500,430}},LineAlignment::Vertical}};
    p.workspace=2;p.tool="Edit Profiles";
    QString error;const auto path=dir.filePath("profiles.foam");CHECK(writeProject(path,p,error));
    MainWindow window;window.show();app.processEvents();CHECK(window.openProjectFile(path,error));
    auto* tabs=window.findChild<QTabWidget*>("viewportTabs");auto* view=static_cast<PlanViewport*>(tabs->widget(0));
    auto* toolbar=window.findChild<QToolBar*>("componentToolBar");auto& stations=view->fuselageSketchEditor().stationEditor();auto& sketch=view->fuselageProfileEditor();
    CHECK(toolbar->actions()[2]->isEnabled()&&toolbar->actions()[2]->isChecked());CHECK(!toolbar->actions()[3]->isEnabled());
    auto* line=window.findChild<QPushButton*>("fuselageProfileLine");auto* spline=window.findChild<QPushButton*>("fuselageProfileSpline");
    auto mouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton b,Qt::MouseButtons bs){const auto local=view->mapFromScene(point);QMouseEvent event{type,QPointF{local},QPointF{view->viewport()->mapToGlobal(local)},b,bs,Qt::NoModifier};QApplication::sendEvent(view->viewport(),&event);};
    auto click=[&](QPointF point){mouse(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);};
    auto key=[&](int value){QKeyEvent event{QEvent::KeyPress,value,Qt::NoModifier};QApplication::sendEvent(view,&event);};
    CHECK(line->isVisible()&&!window.projectModified());CHECK(stations.selectedLine()==0);
    line->click();for(int i=0;i<4;++i){const auto r=rectangle(680,180,100,100);click(r.points[i]);click(r.points[(i+1)%4]);}
    CHECK(stations.lines()[0].profile);CHECK(closedSketchBoundary(sketch.layers()[*stations.lines()[0].profile]));CHECK(!toolbar->actions()[3]->isEnabled());
    line->click();click({500,390});CHECK(stations.selectedLine()==1);CHECK(!stations.lines()[1].profile);
    spline->click();click({720,180});click({770,230});click({720,280});click({670,230});click({720,180});spline->click();
    CHECK(stations.lines()[1].profile);const auto slot=stations.lines()[1].profile;CHECK(slot!=stations.lines()[0].profile);
    for(int i=3;i<7;++i)CHECK(toolbar->actions()[i]->isEnabled());
    CHECK(window.saveProjectFile(path,error));CHECK(!window.projectModified());click({300,390});CHECK(!window.projectModified());
    CHECK(sketch.activeLayer()==*stations.lines()[0].profile);click({500,390});
    const auto capture=qEnvironmentVariable("FOAM_EDIT_PROFILES_CAPTURE");if(!capture.isEmpty()){view->fitAll();app.processEvents();CHECK(window.grab().save(capture));}
    CHECK(window.saveProjectFile(path,error));CHECK(window.openProjectFile(path,error));CHECK(stations.lines()[1].profile==slot&&toolbar->actions()[2]->isChecked());CHECK(!window.projectModified());
    toolbar->actions()[1]->trigger();click(stations.lines()[1].first.position);mouse(QEvent::MouseMove,{530,350},Qt::NoButton,Qt::NoButton);key(Qt::Key_Escape);
    CHECK(stations.lines()[1].profile==slot);CHECK(std::abs(stations.lines()[1].first.position.x()-530)<2);
    toolbar->actions()[2]->trigger();
    tabs->setCurrentIndex(1);waitForModel(window);
    auto* viewport=tabs->widget(1);if(!viewport->property("fuselageModelReady").toBool())std::cerr<<window.statusBar()->currentMessage().toStdString()<<'\n';
    CHECK(viewport->property("fuselageModelReady").toBool());CHECK(!sketch.state().editing);
    const auto revision=viewport->property("fuselageModelRevision").toInt();CHECK(revision==1);CHECK(viewport->property("wingModelRevision").toInt()==0);
    const auto solidCapture=qEnvironmentVariable("FOAM_FUSELAGE_SOLID_CAPTURE");if(!solidCapture.isEmpty()){app.processEvents();CHECK(window.screen()->grabWindow(window.winId()).save(solidCapture));}
    tabs->setCurrentIndex(0);tabs->setCurrentIndex(1);waitForModel(window);CHECK(viewport->property("fuselageModelRevision").toInt()==revision);
    tabs->setCurrentIndex(0);click({530,390});window.findChild<QPushButton*>("deleteFuselageProfile")->click();CHECK(!stations.lines()[1].profile);CHECK(!toolbar->actions()[3]->isEnabled());
    CHECK(stations.lines()[0].profile);
    spline->click();click({680,180});click({720,160});CHECK(sketch.state().pending.size()==2);
    CHECK(window.saveProjectFile(path,error));CHECK(window.openProjectFile(path,error));
    CHECK(sketch.state().pending.size()==2&&spline->isChecked()&&!window.projectModified());
    key(Qt::Key_Escape);window.findChild<QPushButton*>("deleteFuselageProfile")->click();
    CHECK(window.saveProjectFile(path,error));
    auto encoded=encodeProject(window.projectDocument());CHECK(encoded["version"]==15);
    auto old=encoded;old["version"]=10;old.remove("fuselageProfiles");CHECK(!decodeProject(old).fuselageStations.lines[0].profile);
    auto invalid=encoded;auto fs=invalid["fuselageStations"].toObject();auto records=fs["lines"].toArray();auto record=records[0].toObject();record["profile"]=100000;records[0]=record;fs["lines"]=records;invalid["fuselageStations"]=fs;
    bool rejected=false;try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    // Geometry: differently positioned/scaled views register at a shared nose.
    designrc::geometry::FuselageSolidInput input{{rectangle(100,200,100,20),rectangle(600,800,200,60)},p.fuselageStations.lines,{rectangle(0,0,30,10)},500};
    input.stations.resize(1);input.stations[0].first.position={700,800};input.stations[0].second.position={700,860};input.stations[0].profile=0;
    auto shape=designrc::geometry::buildFuselageSolid(input);Bnd_Box box;BRepBndLib::AddOptimal(shape,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    CHECK(std::abs(x0)<1e-5&&std::abs(x1-500)<1e-5);CHECK(std::abs(y0+50)<1e-5&&std::abs(y1-50)<1e-5);CHECK(std::abs(z0+75)<1e-5&&std::abs(z1-75)<1e-5);
    GProp_GProps volume;BRepGProp::VolumeProperties(shape,volume);CHECK(std::abs(std::abs(volume.Mass())-500*100*150)<1);
    // Pointed ends and a single section still produce a closed, positive-volume body.
    input.outlines={{{{0,10},{50,0},{100,10},{50,20}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}},
      {{{0,30},{100,0},{200,30},{100,60}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}}};
    input.stations[0].first.position={100,0};input.stations[0].second.position={100,60};input.lengthMm.reset();
    shape=designrc::geometry::buildFuselageSolid(input);Bnd_Box pointed;BRepBndLib::AddOptimal(shape,pointed,false,false);pointed.Get(x0,y0,z0,x1,y1,z1);CHECK(std::abs(x1-200)<1e-5);
    BRepGProp::VolumeProperties(shape,volume);CHECK(std::abs(volume.Mass())>1);
    std::stop_source stop;stop.request_stop();rejected=false;try{designrc::geometry::buildFuselageSolid(input,{}, {stop.get_token()});}catch(const designrc::geometry::ProcessingCancelled&){rejected=true;}CHECK(rejected);
    std::cout<<"Fuselage profile interaction, station ownership, persistence, independent job/cache, solid dimensions and cancellation passed\n";
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

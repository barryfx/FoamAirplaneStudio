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
#include <QPainter>
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
    if(app.arguments().contains("--recovery-only")) {
      auto* circle=window.findChild<QPushButton*>("fuselageProfileCircle");
      circle->click();click({720,230});click({770,230});circle->click();app.processEvents();
      const auto slot=*stations.lines()[0].profile;const auto original=sketch.layers()[slot];
      toolbar->actions()[1]->trigger();app.processEvents();
      click({300,350});mouse(QEvent::MouseMove,{340,350},Qt::NoButton,Qt::NoButton);
      CHECK(stations.lines().size()==2&&stations.lines()[0].profile==slot);
      mouse(QEvent::MouseMove,{900,600},Qt::NoButton,Qt::NoButton);key(Qt::Key_Escape);app.processEvents();
      CHECK(stations.lines().size()==2&&stations.lines()[0].profile==slot);
      const auto moved=stations.lines()[0];click((moved.first.position+moved.second.position)/2);key(Qt::Key_Delete);app.processEvents();
      CHECK(stations.lines().size()==1);CHECK(sketch.layers()[slot].points==original.points);
      click({350,350});app.processEvents();CHECK(stations.lines().size()==2&&!stations.lines()[1].profile);
      toolbar->actions()[2]->trigger();click({350,390});app.processEvents();CHECK(stations.selectedLine()==1);
      CHECK(window.saveProjectFile(path,error));
      auto shortcut=[&](int code){QKeyEvent event{QEvent::KeyPress,code,Qt::ControlModifier};QApplication::sendEvent(view,&event);app.processEvents();};
      // Selection alone is clean; orphan deletion preserves stations and is undoable.
      click({720,230});app.processEvents();CHECK(!stations.lines()[1].profile&&!window.projectModified());
      QImage highlight{1000,600,QImage::Format_RGB32};highlight.fill(Qt::black);
      {QPainter painter{&highlight};sketch.paint(painter);}
      bool orange=false;for(int y=228;y<=232;++y)for(int x=668;x<=672;++x)orange=orange||highlight.pixelColor(x,y)==QColor(255,140,0);
      CHECK(orange);
      if(const auto capture=qEnvironmentVariable("FOAM_PROFILE_ORPHAN_CAPTURE");!capture.isEmpty())CHECK(highlight.save(capture));
      auto* remove=window.findChild<QPushButton*>("deleteFuselageProfile");CHECK(remove->isVisible()&&remove->isEnabled());
      key(Qt::Key_Delete);app.processEvents();CHECK(sketch.layers()[slot].curves.empty()&&stations.lines().size()==2);
      shortcut(Qt::Key_Z);CHECK(sketch.layers()[slot].points==original.points&&!window.projectModified());
      shortcut(Qt::Key_Y);CHECK(sketch.layers()[slot].curves.empty());shortcut(Qt::Key_Z);
      click({720,230});remove->click();app.processEvents();CHECK(sketch.layers()[slot].curves.empty());shortcut(Qt::Key_Z);
      // Empty space does not assign. Double-click explicitly recovers a profile.
      click({900,600});app.processEvents();CHECK(!stations.lines()[1].profile);
      click({720,230});mouse(QEvent::MouseButtonDblClick,{720,230},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,{720,230},Qt::LeftButton,Qt::NoButton);app.processEvents();CHECK(stations.lines()[1].profile==slot);
      CHECK(sketch.layers().size()==1&&sketch.layers()[slot].points==original.points);
      CHECK(sketch.activeLayer()==slot&&window.projectModified());

      shortcut(Qt::Key_Z);CHECK(!stations.lines()[1].profile&&!window.projectModified());
      shortcut(Qt::Key_Y);CHECK(stations.lines()[1].profile==slot&&window.projectModified());
      // An occupied profile cannot be stolen by another unassigned station.
      click({500,390});CHECK(stations.selectedLine()==0);click({720,230});app.processEvents();
      CHECK(!stations.lines()[0].profile&&stations.lines()[1].profile==slot);
      CHECK(window.saveProjectFile(path,error));CHECK(window.openProjectFile(path,error));
      CHECK(stations.lines()[1].profile==slot&&sketch.layers()[slot].points==original.points);
      toolbar->actions()[1]->trigger();click({500,390});key(Qt::Key_Delete);click({350,390});key(Qt::Key_Delete);app.processEvents();
      CHECK(stations.lines().empty()&&toolbar->actions()[2]->isEnabled());
      toolbar->actions()[2]->trigger();click({720,230});app.processEvents();CHECK(remove->isVisible()&&remove->isEnabled());
      remove->click();app.processEvents();CHECK(sketch.layers()[slot].curves.empty());
      shortcut(Qt::Key_Z);CHECK(stations.lines().empty()&&sketch.layers()[slot].points==original.points);
      CHECK(!window.property("modelProcessing").toBool());
      std::cout<<"Orphan profile recovery, move retention, undo/redo and persistence passed\n";
      return 0;
    }
    if(app.arguments().contains("--operations-only")) {
      auto* copy=window.findChild<QPushButton*>("copyFuselageProfile");
      auto* paste=window.findChild<QPushButton*>("pasteFuselageProfile");
      auto* move=window.findChild<QPushButton*>("moveFuselageProfile");
      auto* circle=window.findChild<QPushButton*>("fuselageProfileCircle");
      CHECK(copy&&paste&&move&&!copy->isEnabled()&&!paste->isEnabled()&&!move->isEnabled());
      circle->click();click({720,230});click({770,230});circle->click();
      const auto sourceSlot=*stations.lines()[0].profile;const auto original=sketch.layers()[sourceSlot];
      auto profileColor=[&](QPointF point,QColor color) {
        QImage image{4000,1000,QImage::Format_RGB32};image.fill(Qt::black);
        QPainter painter{&image};sketch.paint(painter);painter.end();
        const auto pixel=point.toPoint();
        for(int y=-2;y<=2;++y)for(int x=-2;x<=2;++x)
          if(image.rect().contains(pixel+QPoint{x,y})&&image.pixelColor(pixel+QPoint{x,y})==color)return true;
        return false;
      };
      const auto sourceEdge=original.points[0]-QPointF{QLineF{original.points[0],original.points[1]}.length(),0};
      CHECK(profileColor(sourceEdge,QColor{255,140,0}));
      CHECK(window.saveProjectFile(path,error));copy->click();CHECK(copy->isChecked()&&!circle->isChecked());
      click(original.points[0]);CHECK(!copy->isChecked()&&paste->isEnabled()&&!window.projectModified());
      click({500,390});CHECK(stations.selectedLine()==1);const auto occupied=view->sceneRect();paste->click();
      const auto destinationSlot=*stations.lines()[1].profile;CHECK(destinationSlot!=sourceSlot);
      auto destination=sketch.layers()[destinationSlot];CHECK(destination.curves[0].type==SketchTool::Circle);
      CHECK(profileColor(sourceEdge,QColor{80,200,255}));
      CHECK(profileColor(destination.points[0]-QPointF{QLineF{destination.points[0],destination.points[1]}.length(),0},QColor{255,140,0}));
      CHECK(sketch.layers()[sourceSlot].points==original.points&&window.projectModified());
      const auto delta=destination.points[0]-original.points[0];
      for(std::size_t i=0;i<original.points.size();++i)CHECK((QLineF{destination.points[i],original.points[i]+delta}.length()<1e-8));
      CHECK((destination.points[0].x()-QLineF{destination.points[0],destination.points[1]}.length()>occupied.right()));
      CHECK(view->mapToScene(view->viewport()->rect()).boundingRect().contains(destination.points[0]));
      CHECK(static_cast<FuselageProfilePanel*>(window.findChild<QWidget*>("fuselageProfilePanel"))->allProfilesClosed());
      auto drag=[&](QPointF from,QPointF to){mouse(QEvent::MouseButtonPress,from,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,to,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,to,Qt::LeftButton,Qt::NoButton);};
      CHECK(window.saveProjectFile(path,error));move->click();CHECK(!window.projectModified());
      drag(destination.points[0],destination.points[0]+QPointF{35,20});
      auto moved=sketch.layers()[destinationSlot];const auto movement=moved.points[0]-destination.points[0];CHECK((QLineF{movement,QPointF{35,20}}.length()<2));
      for(std::size_t i=0;i<moved.points.size();++i)CHECK((QLineF{moved.points[i],destination.points[i]+movement}.length()<1e-8));
      CHECK(sketch.layers()[sourceSlot].points==original.points&&stations.lines()[1].profile==destinationSlot);
      mouse(QEvent::MouseMove,moved.points[0]+QPointF{100,100},Qt::NoButton,Qt::NoButton);CHECK(sketch.layers()[destinationSlot].points==moved.points);
      view->fitAll();app.processEvents();drag(original.points[0],original.points[0]+QPointF{0,30});
      CHECK(sketch.layers()[sourceSlot].points!=original.points&&stations.lines()[0].profile==sourceSlot);
      CHECK(sketch.layers()[destinationSlot].points==moved.points);key(Qt::Key_Escape);CHECK(!move->isChecked());
      // Clipboard is a snapshot, independent of later edits/moves of its source.
      paste->click();destination=sketch.layers()[destinationSlot];
      CHECK(std::abs(QLineF{destination.points[0],destination.points[1]}.length()-QLineF{original.points[0],original.points[1]}.length())<1e-8);
      CHECK(window.saveProjectFile(path,error));CHECK(window.openProjectFile(path,error));
      CHECK(stations.lines()[1].profile==destinationSlot&&sketch.layers()[destinationSlot].points==destination.points&&!window.projectModified());
      CHECK(view->sceneRect().contains(destination.points[0])&&view->sceneRect().contains(destination.points[1]));
      // Exercise multi-curve Line/Spline profiles and shared-slot independence.
      auto state=sketch.state();auto mixed=rectangle(680,180,100,100);mixed.curves[1].type=SketchTool::Spline;
      state.layers[sourceSlot]=mixed;sketch.restoreState(state);
      auto assigned=stations.state();assigned.lines[1].profile=sourceSlot;stations.restoreState(assigned);
      copy->click();click({730,230});paste->click();
      const auto independent=*stations.lines()[1].profile;CHECK(independent!=sourceSlot);
      CHECK(sketch.layers()[sourceSlot].points==mixed.points&&sketch.layers()[independent].curves.size()==4);
      CHECK(sketch.layers()[independent].curves[1].type==SketchTool::Spline);
      CHECK(window.saveProjectFile(path,error));CHECK(window.openProjectFile(path,error));
      const auto capture=qEnvironmentVariable("FOAM_PROFILE_OPERATIONS_CAPTURE");
      if(!capture.isEmpty()){view->fitAll();app.processEvents();CHECK(window.grab().save(capture));}
      move->click();line->click();CHECK(!move->isChecked()&&line->isChecked());line->click();
      copy->click();toolbar->actions()[1]->trigger();CHECK(!copy->isChecked());
      const auto& pasted=sketch.layers()[independent];const auto edge=(pasted.points[0]+pasted.points[1])/2;
      CHECK(profileColor(edge,QColor{80,200,255}));toolbar->actions()[2]->trigger();
      const auto& selectedProfile=sketch.layers()[*stations.lines()[stations.selectedLine()].profile];
      CHECK(profileColor((selectedProfile.points[0]+selectedProfile.points[1])/2,QColor{255,140,0}));
      CHECK(!window.property("modelProcessing").toBool());
      std::cout<<"Profile copy/paste, independent assignment, open placement, whole-profile drag and persistence passed\n";
      return 0;
    }
    if(app.arguments().contains("--circle-only")) {
      auto* circle=window.findChild<QPushButton*>("fuselageProfileCircle");CHECK(circle&&circle->isVisible());
      CHECK(circle->mapToGlobal(QPoint{0,0}).y()>line->mapToGlobal(QPoint{0,line->height()}).y());
      auto legacy=encodeProject(p);legacy["version"]=27;CHECK(decodeProject(legacy).fuselageProfiles.layers[0].curves.empty());
      circle->click();CHECK(circle->isChecked()&&!line->isChecked()&&!spline->isChecked());
      line->click();CHECK(line->isChecked()&&!circle->isChecked());
      spline->click();CHECK(spline->isChecked()&&!line->isChecked()&&!circle->isChecked());
      circle->click();click({720,230});click({720,230});
      CHECK(sketch.state().pending.size()==1&&sketch.layers()[sketch.activeLayer()].curves.empty());
      key(Qt::Key_Escape);CHECK(sketch.state().pending.empty());
      click({720,230});line->click();CHECK(sketch.state().pending.empty()&&sketch.layers()[sketch.activeLayer()].curves.empty());
      circle->click();click({720,230});
      mouse(QEvent::MouseMove,{780,230},Qt::NoButton,Qt::NoButton);
      CHECK(sketch.state().pending.size()==1);CHECK(window.saveProjectFile(path,error));
      CHECK(window.openProjectFile(path,error));CHECK(sketch.tool()==SketchTool::Circle&&circle->isChecked()&&sketch.state().pending.size()==1);
      click({780,230});CHECK(sketch.state().pending.empty());
      const auto slot=*stations.lines()[0].profile;auto layer=sketch.layers()[slot];
      CHECK(layer.curves.size()==1&&layer.curves[0].type==SketchTool::Circle&&layer.points.size()==2);
      const auto boundary=closedSketchBoundary(layer);CHECK(boundary&&boundary->size()>=256);
      const double radius=QLineF{layer.points[0],layer.points[1]}.length();
      CHECK(std::abs(radius-60)<2);
      for(auto point:*boundary)CHECK(std::abs(QLineF{point,layer.points[0]}.length()-radius)<1e-7);
      CHECK(!isSketchEndpoint(layer,0)&&!isSketchEndpoint(layer,1));
      circle->click();CHECK(sketch.tool()==SketchTool::None);
      auto drag=[&](QPointF a,QPointF b){mouse(QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);};
      drag(layer.points[0],layer.points[0]+QPointF{20,20});layer=sketch.layers()[slot];
      CHECK(std::abs(QLineF{layer.points[0],layer.points[1]}.length()-radius)<1e-7);
      drag(layer.points[1],layer.points[0]+QPointF{85,0});layer=sketch.layers()[slot];
      CHECK(std::abs(QLineF{layer.points[0],layer.points[1]}.length()-85)<2);
      auto json=encodeProject(window.projectDocument());CHECK(json["version"]==28);
      CHECK(decodeProject(json).fuselageProfiles.layers[slot].points==layer.points);
      auto invalid=window.projectDocument();invalid.fuselageProfiles.layers[slot].points[1]=layer.points[0];
      bool rejected=false;try{decodeProject(encodeProject(invalid));}catch(const std::exception&){rejected=true;}CHECK(rejected);
      auto oldCircle=json;oldCircle["version"]=27;rejected=false;
      try{decodeProject(oldCircle);}catch(const std::exception&){rejected=true;}CHECK(rejected);
      invalid=window.projectDocument();invalid.fuselage.layers[0]=layer;rejected=false;
      try{decodeProject(encodeProject(invalid));}catch(const std::exception&){rejected=true;}CHECK(rejected);
      CHECK(window.saveProjectFile(path,error));CHECK(window.openProjectFile(path,error));
      CHECK(sketch.layers()[slot].points==layer.points&&!window.projectModified());
      const auto capture=qEnvironmentVariable("FOAM_CIRCLE_CAPTURE");
      if(!capture.isEmpty()){view->fitAll();app.processEvents();CHECK(window.grab().save(capture));}
      click(layer.points[0]+QPointF{0,QLineF{layer.points[0],layer.points[1]}.length()});key(Qt::Key_Delete);
      CHECK(sketch.layers()[slot].curves.empty());
      CHECK(!window.property("modelProcessing").toBool());
      std::cout<<"Circle profile tools, drawing, move/resize, delete, boundary and persistence passed\n";
      return 0;
    }
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
    auto encoded=encodeProject(window.projectDocument());CHECK(encoded["version"]==28);
    auto old=encoded;old["version"]=10;old.remove("fuselageProfiles");CHECK(!decodeProject(old).fuselageStations.lines[0].profile);
    auto invalid=encoded;auto fs=invalid["fuselageStations"].toObject();auto records=fs["lines"].toArray();auto record=records[0].toObject();record["profile"]=100000;records[0]=record;fs["lines"]=records;invalid["fuselageStations"]=fs;
    bool rejected=false;try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    // Geometry: differently positioned/scaled views register at a shared nose.
    designrc::geometry::FuselageSolidInput input{{rectangle(100,200,100,20),rectangle(600,800,200,60)},p.fuselageStations.lines,{rectangle(0,0,30,10)},500};
    input.stations.resize(1);input.stations[0].first.position={700,800};input.stations[0].second.position={700,860};input.stations[0].profile=0;
    auto shape=designrc::geometry::buildFuselageSolid(input);Bnd_Box box;BRepBndLib::AddOptimal(shape,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    CHECK(std::abs(x0)<1e-5&&std::abs(x1-500)<1e-5);CHECK(std::abs(y0+50)<1e-5&&std::abs(y1-50)<1e-5);CHECK(std::abs(z0+75)<1e-5&&std::abs(z1-75)<1e-5);
    // Four diameter-4 sockets each remove 0.5 mm more material than their pin adds.
    GProp_GProps volume;BRepGProp::VolumeProperties(shape,volume);CHECK(std::abs(std::abs(volume.Mass())-(500*100*150-8*std::acos(-1.)))<1);
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

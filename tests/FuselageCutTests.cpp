#include "gui/MainWindow.h"
#include "gui/FuselageCutPanel.h"
#include "geometry/FuselageCut.h"
#include "WaitForModel.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepGProp.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <QApplication>
#include <QAction>
#include <QPushButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolBar>
#include <QJsonArray>
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace designrc;
using namespace designrc::gui;
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__));}while(false)
SketchLayer rectangle(double x,double y,double w,double h){return {{{x,y},{x+w,y},{x+w,y+h},{x,y+h}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};}
int count(const TopoDS_Shape& shape){int n=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++n;return n;}
double volume(const TopoDS_Shape& shape){GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass);return mass.Mass();}
void geometryTests() {
  auto box=BRepPrimAPI_MakeBox{gp_Pnt{0,-20,-15},200,40,30}.Shape();
  const std::array<geometry::FuselageCutProjection,2> projections{{{100,2,210},{500,1,615}}};
  std::vector<SketchLayer> cuts(2);
  cuts[0]={{{150,195},{150,210},{155,220},{150,225}},{{SketchTool::Line,{0,1}},{SketchTool::Spline,{1,2,3}}}};
  auto split=geometry::cutFuselage(box,cuts,projections);CHECK(count(split)==2);CHECK(std::abs(volume(split)-volume(box))<.01);
  // Simultaneous Top and Side paths use independent drawing registrations.
  cuts[1]={{{490,615},{710,615}},{{SketchTool::Line,{0,1}}}};
  split=geometry::cutFuselage(box,cuts,projections);CHECK(count(split)==4);
  CHECK(std::abs(volume(split)-volume(box))<.01);
  cuts[0]=rectangle(140,205,20,10);cuts[1]={};
  split=geometry::cutFuselage(box,cuts,projections);CHECK(count(split)==2);
  // Hollow-body hatches preserve the cavity and all material.
  auto pocket=BRepPrimAPI_MakeBox{gp_Pnt{5,-15,-10},190,30,20}.Shape();
  auto hollow=BRepAlgoAPI_Cut{box,pocket}.Shape();
  cuts[0]={};cuts[1]={{{490,615},{710,615}},{{SketchTool::Line,{0,1}}}};
  split=geometry::cutFuselage(hollow,cuts,projections);CHECK(count(split)==2);
  CHECK(std::abs(volume(split)-volume(hollow))<.01);CHECK(BRepCheck_Analyzer{split}.IsValid());
  cuts[1]={{{550,615},{650,615}},{{SketchTool::Line,{0,1}}}};
  bool rejected=false;try{geometry::cutFuselage(box,cuts,projections);}catch(const std::exception& e){rejected=std::string{e.what()}.find("did not separate")!=std::string::npos;}CHECK(rejected);
  cuts[1]={{{490,615},{600,615},{710,615},{600,620}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{1,3}}}};
  rejected=false;try{geometry::cutFuselage(box,cuts,projections);}catch(const std::exception& e){rejected=std::string{e.what()}.find("branch")!=std::string::npos;}CHECK(rejected);
  std::stop_source stop;stop.request_stop();rejected=false;
  try{geometry::cutFuselage(box,cuts,projections,{}, {stop.get_token()});}catch(const geometry::ProcessingCancelled&){rejected=true;}CHECK(rejected);
  std::cout<<"Cut geometry: Top/Side registration, mixed Line/Spline, closed path, hollow bodies, volume, errors and cancellation passed"<<std::endl;
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QTemporaryDir dir;
  QCoreApplication::setOrganizationName("FoamCutTests");QCoreApplication::setApplicationName("FoamCutTests");
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
  try {
    geometryTests();
    ProjectDocument p;p.reference.wingspanMm=1000;p.reference.fuselageLengthMm=400;p.wingspanText="1000 mm";p.fuselageText="400 mm";
    p.wing.layers[0]=rectangle(10,10,90,70);p.wing.layers[0].curves.pop_back();
    p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},{{0,0,1,{100,10}},{0,2,0,{100,80}},LineAlignment::Vertical,0}};
    p.airfoils.entries.push_back({"NACA",domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
    p.fuselage.layers={rectangle(200,150,400,100),rectangle(200,350,400,80)};
    p.fuselageStations.lines={{{1,0,.25,{300,350}},{1,2,.75,{300,430}},LineAlignment::Vertical},{{1,0,.75,{500,350}},{1,2,.25,{500,430}},LineAlignment::Vertical}};
    p.fuselageProfiles.layers={rectangle(680,140,100,80),rectangle(680,300,100,80)};
    p.fuselageStations.lines[0].profile=0;p.fuselageStations.lines[1].profile=1;p.workspace=2;p.tool="Cut";
    QString error;const auto file=dir.filePath("cuts.foam");CHECK(writeProject(file,p,error));
    MainWindow window;window.show();app.processEvents();CHECK(window.openProjectFile(file,error));
    auto* tabs=window.findChild<QTabWidget*>("viewportTabs");auto* view=static_cast<PlanViewport*>(tabs->widget(0));
    view->fitInView(QRectF{0,0,900,500},Qt::KeepAspectRatio);app.processEvents();
    auto& editor=view->fuselageCutEditor();auto* toolbar=window.findChild<QToolBar*>("componentToolBar");
    auto* line=window.findChild<QPushButton*>("fuselageCutLine");auto* spline=window.findChild<QPushButton*>("fuselageCutSpline");
    CHECK(toolbar->actions()[4]->isChecked()&&line->isVisible());CHECK(!window.projectModified());
    auto mouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton b,Qt::MouseButtons bs){const auto local=view->mapFromScene(point);QMouseEvent event{type,QPointF{local},QPointF{view->viewport()->mapToGlobal(local)},b,bs,Qt::NoModifier};QApplication::sendEvent(view->viewport(),&event);};
    auto click=[&](QPointF point){mouse(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);};
    auto key=[&](int value){QKeyEvent event{QEvent::KeyPress,value,Qt::NoModifier};QApplication::sendEvent(view,&event);};
    line->click();click({400,140});click({400,200});
    spline->click();click({400,200});click({410,225});click({400,260});key(Qt::Key_Escape);spline->click();
    CHECK(editor.layers()[0].curves.size()==2);CHECK(editor.layers()[0].curves[0].points.back()==editor.layers()[0].curves[1].points.front());
    CHECK(window.projectModified());
    // Standard selection/Delete and point movement remain available.
    line->click();click({820,140});click({820,220});line->click();click({820,180});CHECK(editor.selectedCurve()==2);key(Qt::Key_Delete);CHECK(editor.layers()[0].curves.size()==2);
    mouse(QEvent::MouseButtonPress,{400,140},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{395,140},Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,{395,140},Qt::LeftButton,Qt::NoButton);
    CHECK(std::abs(editor.layers()[0].points[0].x()-395)<2);
    window.findChild<QPushButton*>("fuselageCutSide")->click();CHECK(editor.activeLayer()==1);
    spline->click();click({250,390});click({300,385});CHECK(editor.state().pending.size()==2);
    CHECK(window.saveProjectFile(file,error));CHECK(window.openProjectFile(file,error));
    CHECK(editor.activeLayer()==1&&editor.state().pending.size()==2&&spline->isChecked());
    // Finish and delete this draft so the geometry test contains only the Top path.
    key(Qt::Key_Escape);spline->click();const auto path=SketchEditor::fittedPath(editor.layers()[1].points,SketchTool::Spline);click(path.pointAtPercent(.5));key(Qt::Key_Delete);CHECK(editor.layers()[1].curves.empty());
    window.findChild<QPushButton*>("fuselageCutTop")->click();
    CHECK(window.saveProjectFile(file,error));auto encoded=encodeProject(window.projectDocument());CHECK(encoded["version"]==15);
    auto legacy=encoded;legacy["version"]=12;legacy.remove("fuselageCuts");CHECK(decodeProject(legacy).fuselageCuts.layers[0].curves.empty());
    auto bad=encoded;auto cuts=bad["fuselageCuts"].toObject();cuts["layers"]=QJsonArray{};bad["fuselageCuts"]=cuts;bool rejected=false;try{decodeProject(bad);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    toolbar=window.findChild<QToolBar*>("componentToolBar");toolbar->actions()[2]->trigger();CHECK(!editor.state().editing);CHECK(!window.projectModified());
    view->fitInView(QRectF{0,0,900,500},Qt::KeepAspectRatio);app.processEvents();
    const auto pixels=view->viewport()->grab().toImage();const auto midpoint=(editor.layers()[0].points[0]+editor.layers()[0].points[1])*.5;
    const auto local=view->mapFromScene(midpoint);const QPoint at{qRound(local.x()*pixels.devicePixelRatio()),qRound(local.y()*pixels.devicePixelRatio())};bool blue=false;
    for(int y=-3;y<=3;++y)for(int x=-3;x<=3;++x)if(pixels.rect().contains(at+QPoint{x,y})){const auto c=pixels.pixelColor(at+QPoint{x,y});if(c.blue()>c.red()+40&&c.green()>c.red()+30)blue=true;}CHECK(blue);
    toolbar->actions()[4]->trigger();const auto capture=qEnvironmentVariable("FOAM_CUT_CAPTURE");if(!capture.isEmpty()){app.processEvents();CHECK(window.grab().save(capture));}
    tabs->setCurrentIndex(1);waitForModel(window);
    if(!tabs->widget(1)->property("fuselageModelReady").toBool())std::cerr<<window.statusBar()->currentMessage().toStdString()<<std::endl;
    CHECK(tabs->widget(1)->property("fuselageModelReady").toBool());CHECK(tabs->widget(1)->property("fuselageBodyCount").toInt()==2);
    CHECK(tabs->widget(1)->property("wingModelRevision").toInt()==0);const int revision=tabs->widget(1)->property("fuselageModelRevision").toInt();
    tabs->setCurrentIndex(0);tabs->setCurrentIndex(1);waitForModel(window);CHECK(tabs->widget(1)->property("fuselageModelRevision").toInt()==revision);
    std::cout<<"Cut UI: drawing, joins, editing, visibility, draft persistence, migration, body splitting and cache passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

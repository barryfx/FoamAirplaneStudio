#include "LegacyProject.h"
#include "gui/MainWindow.h"
#include "geometry/ServoTray.h"
#include "geometry/FuselageCut.h"
#include "WaitForModel.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepGProp.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepBndLib.hxx>
#include <GProp_GProps.hxx>
#include <Bnd_Box.hxx>
#include <TopExp_Explorer.hxx>
#include <QApplication>
#include <QAction>
#include <QPushButton>
#include <QLineEdit>
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
double volume(const TopoDS_Shape& shape){GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass);return mass.Mass();}
bool inside(const TopoDS_Shape& shape,double x,double y,double z){return BRepClass3d_SolidClassifier{shape,gp_Pnt{x,y,z},1e-7}.State()==TopAbs_IN;}
int count(const TopoDS_Shape& shape){int n=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++n;return n;}
void geometryTests() {
  const auto outer=BRepPrimAPI_MakeBox{gp_Pnt{0,-25,-20},100,50,40}.Shape();
  const auto cavity=BRepPrimAPI_MakeBox{gp_Pnt{5,-20,-15},90,40,30}.Shape();
  const auto shell=BRepAlgoAPI_Cut{outer,cavity}.Shape();
  auto tray=geometry::addServoTray(shell,cavity,{30,0,40,3});
  CHECK(count(tray.body)==1&&count(tray.tray)==1);CHECK(std::abs(volume(tray.tray)-4800)<1e-5);
  CHECK(std::abs(volume(tray.body)-volume(shell)-2000)<1e-5);
  for(double side:{-1.,1.}) {
    CHECK(inside(tray.body,50,side*19.9,-2.5));CHECK(inside(tray.body,50,side*15.1,-2.5));
    CHECK(!inside(tray.body,50,side*14.9,-2.5));CHECK(!inside(tray.body,50,side*17,-5.1));
    CHECK(!inside(tray.body,29,side*17,-2.5));
  }
  CHECK(std::abs(volume(BRepAlgoAPI_Common{tray.body,tray.tray}.Shape()))<1e-7);
  Bnd_Box bounds;BRepBndLib::AddOptimal(tray.tray,bounds,false,false);double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
  CHECK(std::abs(x0-30)<1e-6&&std::abs(x1-70)<1e-6&&std::abs(y0+20)<1e-6&&std::abs(y1-20)<1e-6&&std::abs(z0)<1e-6&&std::abs(z1-3)<1e-6);
  GProp_GProps area;BRepGProp::SurfaceProperties(tray.topFaces,area);CHECK(std::abs(area.Mass()-1600)<1e-5);
  // Tapered inner walls trim the tray's plan and the ledges without collision.
  BRepBuilderAPI_MakePolygon wire;for(auto p:{gp_Pnt{5,-20,-15},gp_Pnt{95,-10,-15},gp_Pnt{95,10,-15},gp_Pnt{5,20,-15}})wire.Add(p);wire.Close();
  const auto taper=BRepPrimAPI_MakePrism{BRepBuilderAPI_MakeFace{wire.Wire()}.Face(),gp_Vec{0,0,30}}.Shape();
  const auto taperedBody=BRepAlgoAPI_Cut{outer,taper}.Shape();
  tray=geometry::addServoTray(taperedBody,taper,{30,0,40,3});
  CHECK(std::abs(volume(tray.body)-volume(taperedBody)-2000)<1e-4);
  CHECK(std::abs(volume(BRepAlgoAPI_Common{tray.body,tray.tray}.Shape()))<1e-6);
  CHECK(std::abs(volume(BRepAlgoAPI_Cut{tray.tray,taper}.Shape()))<1e-6);
  std::vector<SketchLayer> cuts(2);cuts[0]={{{50,-30},{50,30}},{{SketchTool::Line,{0,1}}}};
  auto split=geometry::cutFuselage(tray.body,cuts,{{{0,1,0},{0,1,0}}});CHECK(count(split)==2);CHECK(std::abs(volume(split)-volume(tray.body))<1e-4);
  bool rejected=false;try{geometry::addServoTray(shell,{}, {30,0,40,3});}catch(const std::exception& e){rejected=std::string{e.what()}.find("Thicken")!=std::string::npos;}CHECK(rejected);
  rejected=false;try{geometry::addServoTray(shell,cavity,{30,-14,40,3});}catch(const std::exception&){rejected=true;}CHECK(rejected);
  rejected=false;try{geometry::addServoTray(shell,cavity,{-10,0,40,3});}catch(const std::exception&){rejected=true;}CHECK(rejected);
  std::stop_source stop;stop.request_stop();rejected=false;try{geometry::addServoTray(shell,cavity,{30,0,40,3},{},{stop.get_token()});}catch(const geometry::ProcessingCancelled&){rejected=true;}CHECK(rejected);
  std::cout<<"Servo geometry: 5x5 ledges, separate cavity-fitted tray, tapered walls, planar top, cuts, errors and cancellation passed\n";
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QTemporaryDir dir;QCoreApplication::setOrganizationName("FoamTrayTests");QCoreApplication::setApplicationName("FoamTrayTests");
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
  try {
    if(app.arguments().contains("--geometry-only")){geometryTests();return 0;}
    // A mirrored 90-unit half-span establishes 1 mm per scene unit.
    ProjectDocument p;p.reference.wingspanMm=180;p.reference.fuselageLengthMm=400;p.wingspanText="180 mm";p.fuselageText="400 mm";
    p.wing.layers[0]=rectangle(10,10,90,70);p.wing.layers[0].curves.pop_back();
    p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},{{0,0,1,{100,10}},{0,2,0,{100,80}},LineAlignment::Vertical,0}};
    p.airfoils.entries.push_back({"NACA",domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
    p.fuselage.layers={rectangle(200,150,400,100),rectangle(200,350,400,80)};
    p.fuselageStations.lines={{{1,0,.25,{300,350}},{1,2,.75,{300,430}},LineAlignment::Vertical},{{1,0,.75,{500,350}},{1,2,.25,{500,430}},LineAlignment::Vertical}};
    p.fuselageProfiles.layers={rectangle(680,140,100,80),rectangle(680,300,100,80)};
    p.fuselageStations.lines[0].profile=0;p.fuselageStations.lines[1].profile=1;
    p.fuselageThickening=true;for(auto& s:p.fuselageStations.lines)s.thicknessMm=5;
    p.workspace=2;p.tool="Servo Tray";p.servoTray.drawing=true;
    QString error;const auto file=dir.filePath("tray.foam");CHECK(writeProject(file,p,error));
    MainWindow window;window.show();app.processEvents();CHECK(window.openProjectFile(file,error));
    auto* tabs=window.findChild<QTabWidget*>("viewportTabs");auto* view=static_cast<PlanViewport*>(tabs->widget(0));view->fitInView(QRectF{0,0,900,500},Qt::KeepAspectRatio);app.processEvents();
    auto& editor=view->servoTrayEditor();auto* toolbar=window.findChild<QToolBar*>("componentToolBar");
    auto* place=window.findChild<QPushButton*>("servoTrayPlace");
    auto* width=window.findChild<QLineEdit*>("servoTrayWidth");auto* height=window.findChild<QLineEdit*>("servoTrayHeight");
    CHECK(place&&place->isVisible()&&width&&height);CHECK(!window.findChild<QPushButton*>("servoTrayDraw"));
    auto enter=[&](QLineEdit* field,QString value){field->setText(value);field->setModified(true);QMetaObject::invokeMethod(field,"editingFinished");};
    auto mouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton b,Qt::MouseButtons bs){const auto local=view->mapFromScene(point);QMouseEvent event{type,QPointF{local},QPointF{view->viewport()->mapToGlobal(local)},b,bs,Qt::NoModifier};QApplication::sendEvent(view->viewport(),&event);};
    auto click=[&](QPointF point){mouse(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);};
    auto key=[&](int value){QKeyEvent event{QEvent::KeyPress,value,Qt::NoModifier};QApplication::sendEvent(view,&event);};
    enter(width,"130 mm");enter(height,"0.1968503937007874 in");
    CHECK(editor.state().rectangle);CHECK(window.projectModified());
    CHECK(std::abs(editor.state().rectangle->width()-130)<1e-8);CHECK(std::abs(editor.state().rectangle->height()-5)<1e-8);
    const auto center=editor.state().rectangle->center();enter(width,"140");CHECK(editor.state().rectangle->center()==center);
    enter(width,"invalid");CHECK(std::abs(editor.state().rectangle->width()-140)<1e-8);enter(width,"130 mm");
    auto r=*editor.state().rectangle;
    mouse(QEvent::MouseButtonPress,r.center(),Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,r.center()+QPointF{10,0},Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,r.center()+QPointF{10,0},Qt::LeftButton,Qt::NoButton);
    CHECK(std::abs(editor.state().rectangle->left()-r.left()-10)<2);CHECK(editor.state().rectangle->size()==r.size());r=*editor.state().rectangle;
    mouse(QEvent::MouseButtonPress,r.topRight(),Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,r.topRight()+QPointF{10,4},Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,r.topRight()+QPointF{10,4},Qt::LeftButton,Qt::NoButton);
    CHECK(editor.state().rectangle->size()==r.size());
    click(editor.state().rectangle->center());key(Qt::Key_Delete);CHECK(!editor.state().rectangle);
    place->click();CHECK(editor.state().rectangle);CHECK(std::abs(editor.state().rectangle->width()-130)<1e-8);
    CHECK(window.saveProjectFile(file,error));CHECK(window.openProjectFile(file,error));CHECK(editor.state().rectangle);
    auto encoded=encodeProject(window.projectDocument());CHECK(encoded["version"]==32);
    auto old=encoded;old["version"]=13;legacyCutViews(old);old.remove("servoTray");CHECK(!decodeProject(old).servoTray.rectangle);
    auto bad=encoded;auto tray=bad["servoTray"].toObject();tray["rectangle"]=QJsonArray{1,2,0,5};bad["servoTray"]=tray;bool rejected=false;try{decodeProject(bad);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    toolbar=window.findChild<QToolBar*>("componentToolBar");toolbar->actions()[4]->trigger();CHECK(!window.projectModified());
    view->fitInView(QRectF{0,0,900,500},Qt::KeepAspectRatio);app.processEvents();
    const auto pix=view->viewport()->grab().toImage();r=*editor.state().rectangle;const auto loc=view->mapFromScene({r.center().x(),r.top()});const QPoint at{qRound(loc.x()*pix.devicePixelRatio()),qRound(loc.y()*pix.devicePixelRatio())};bool gold=false;
    for(int y=-3;y<=3;++y)for(int x=-3;x<=3;++x)if(pix.rect().contains(at+QPoint{x,y})){const auto c=pix.pixelColor(at+QPoint{x,y});if(c.red()>200&&c.green()>100&&c.blue()<90)gold=true;}CHECK(gold);
    toolbar->actions()[5]->trigger();const auto capture=qEnvironmentVariable("FOAM_TRAY_CAPTURE");if(!capture.isEmpty()){app.processEvents();CHECK(window.grab().save(capture));}
    tabs->setCurrentIndex(1);waitForModel(window);
    if(!tabs->widget(1)->property("fuselageModelReady").toBool())std::cerr<<window.statusBar()->currentMessage().toStdString()<<std::endl;
    CHECK(tabs->widget(1)->property("fuselageModelReady").toBool());CHECK(tabs->widget(1)->property("servoTrayReady").toBool());CHECK(!window.servoTrayTopFaces().IsNull());
    CHECK(tabs->widget(1)->property("fuselageBodyCount").toInt()==3);CHECK(tabs->widget(1)->property("wingModelRevision").toInt()==0);
    const auto revision=tabs->widget(1)->property("fuselageModelRevision").toInt();tabs->setCurrentIndex(0);tabs->setCurrentIndex(1);waitForModel(window);CHECK(tabs->widget(1)->property("fuselageModelRevision").toInt()==revision);
    std::cout<<"Servo UI: dimension entry, fixed-size movement/delete, persistence, overlay, migration and independent generation/cache passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

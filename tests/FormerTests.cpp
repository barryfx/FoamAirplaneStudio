#include "gui/MainWindow.h"
#include "gui/OcctViewport.h"
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <QScreen>
#include <QTimer>
#include "geometry/ServoTray.h"
#include "geometry/Formers.h"
#include "gui/FormerPanel.h"
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
#include <QDoubleSpinBox>
#include <numbers>
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
  const auto inserts=geometry::buildFormers(cavity,shell,{{20,-30,4,60}});
  const auto retained=geometry::addFormerRetainers(shell,cavity,{{20,-30,4,60}},inserts);
  CHECK(count(retained)==1);CHECK(std::abs(volume(retained)-volume(shell)-4*4*3*30)<1e-5);
  for(double x:{18.,26.})for(double y:{-18.5,18.5})for(double z:{-14.,0.,14.})CHECK(inside(retained,x,y,z));
  CHECK(!inside(retained,18,16,0));CHECK(!inside(retained,15,18.5,0));CHECK(!inside(retained,29,18.5,0));
  CHECK(std::abs(volume(BRepAlgoAPI_Common{retained,inserts[0]}.Shape()))<1e-6);
  // Negative degrees tilt the top toward the nose in model coordinates,
  // matching counter-clockwise rotation in the scene's downward Y axis.
  const std::vector<QRectF> angledMask{{20,-30,4,60}};
  for(double angle:{-30.,30.}) {
    const auto angled=geometry::buildFormers(cavity,shell,angledMask,{},{},{},{angle});
    const double slope=std::tan(angle*std::numbers::pi/180.);
    CHECK(std::abs(volume(angled[0])-4800/std::cos(angle*std::numbers::pi/180.))<1e-4);
    CHECK(inside(angled[0],22+slope*10,0,10));CHECK(!inside(angled[0],22-slope*10,0,10));
    CHECK(std::abs(volume(BRepAlgoAPI_Cut{angled[0],cavity}.Shape()))<1e-6);
    const auto rails=geometry::addFormerRetainers(shell,cavity,angledMask,angled,{},{},{angle});
    CHECK(count(rails)==1);CHECK(std::abs(volume(BRepAlgoAPI_Common{rails,angled[0]}.Shape()))<1e-6);
    for(double side:{-1.,1.})CHECK(inside(rails,22+slope*10+side*4/std::cos(angle*std::numbers::pi/180.),18.5,10));
    for(double z:{-10.,0.,10.})for(double side:{-1.,1.})
      CHECK(!inside(rails,22+slope*z+side*4/std::cos(angle*std::numbers::pi/180.),0,z));
  }
  bool rotatedOverlap=false;try{geometry::buildFormers(cavity,shell,{{20,-30,4,60},{34,-30,4,60}},{},{},{},{-30,30});}catch(const std::exception&){rotatedOverlap=true;}CHECK(rotatedOverlap);
  const auto halves=geometry::splitFuselageMainBody(retained);
  CHECK(count(halves)==2);CHECK(std::abs(volume(halves)-volume(retained))<1e-5);
  const auto supported=geometry::addServoTray(shell,cavity,{35,0,30,3});
  const std::vector<QRectF> closeMasks{{20,-30,4,60},{26,-30,4,60},{31,-30,3,60}};
  auto closeInserts=geometry::buildFormers(cavity,supported.body,closeMasks,QRectF{35,0,30,3});
  closeInserts.push_back(supported.tray);
  const auto closeRetainers=geometry::addFormerRetainers(supported.body,cavity,closeMasks,closeInserts);
  CHECK(count(closeRetainers)==1);
  for(const auto& insert:closeInserts)CHECK(std::abs(volume(BRepAlgoAPI_Common{closeRetainers,insert}.Shape()))<1e-6);
  auto shapes=geometry::buildFormers(cavity,supported.body,{{20,-30,4,60},{40,-12,3,7}},QRectF{35,0,30,3});
  CHECK(shapes.size()==2);CHECK(std::abs(volume(shapes[0])-4800)<1e-5);
  CHECK(std::abs(volume(shapes[1])-840)<1e-5);
  for(const auto& shape:shapes){CHECK(count(shape)==1);CHECK(std::abs(volume(BRepAlgoAPI_Common{shape,supported.body}.Shape()))<1e-6);CHECK(std::abs(volume(BRepAlgoAPI_Cut{shape,cavity}.Shape()))<1e-6);CHECK(std::abs(volume(BRepAlgoAPI_Common{shape,supported.tray}.Shape()))<1e-6);}
  // Beneath the tray, remove ledge volume too: 3*30*5 instead of 3*40*5.
  shapes=geometry::buildFormers(cavity,supported.body,{{40,-5,3,5}},QRectF{35,0,30,3});CHECK(std::abs(volume(shapes[0])-450)<1e-5);
  bool rejected=false;try{geometry::buildFormers(cavity,shell,{{20,-30,4,60},{23,-20,3,40}});}catch(const std::exception&){rejected=true;}CHECK(rejected);
  rejected=false;try{geometry::buildFormers(cavity,shell,{{40,-10,3,20}},QRectF{35,0,30,3});}catch(const std::exception&){rejected=true;}CHECK(rejected);
  rejected=false;try{geometry::buildFormers({},shell,{{20,-30,4,60}});}catch(const std::exception&){rejected=true;}CHECK(rejected);
  rejected=false;try{geometry::buildFormers(cavity,shell,{{120,-30,4,60}});}catch(const std::exception&){rejected=true;}CHECK(rejected);
  rejected=false;try{geometry::buildFormers(cavity,shell,{{20,-30,0,60}});}catch(const std::exception&){rejected=true;}CHECK(rejected);
  rejected=false;try{geometry::addServoTray(shell,cavity,{35,0,30,0});}catch(const std::exception&){rejected=true;}CHECK(rejected);
  std::stop_source stop;stop.request_stop();rejected=false;try{geometry::buildFormers(cavity,shell,{{20,-30,4,60}},{},{},{stop.get_token()});}catch(const geometry::ProcessingCancelled&){rejected=true;}CHECK(rejected);
  BRepBuilderAPI_MakePolygon wire;for(auto p:{gp_Pnt{5,-20,-15},gp_Pnt{95,-10,-15},gp_Pnt{95,10,-15},gp_Pnt{5,20,-15}})wire.Add(p);wire.Close();
  const auto taper=BRepPrimAPI_MakePrism{BRepBuilderAPI_MakeFace{wire.Wire()}.Face(),gp_Vec{0,0,30}}.Shape();
  const auto taperedBody=BRepAlgoAPI_Cut{outer,taper}.Shape();shapes=geometry::buildFormers(taper,taperedBody,{{20,-30,4,60}});
  CHECK(std::abs(volume(BRepAlgoAPI_Common{shapes[0],taperedBody}.Shape()))<1e-6);
  const auto taperedRails=geometry::addFormerRetainers(taperedBody,taper,{{20,-30,4,60}},shapes);
  CHECK(count(taperedRails)==1);
  CHECK(std::abs(volume(taperedRails)-volume(taperedBody)-4*4*3*30)<1e-5);
  for(double x:{16.5,18.,19.5,24.5,26.,27.5})for(double z:{-14.,0.,14.}) {
    const double innerSide=20-(x-5)/9.;
    CHECK(!inside(taperedRails,x,0,z));
    for(double side:{-1.,1.}) {
      CHECK(inside(taperedRails,x,side*(innerSide-1.5),z));
      CHECK(!inside(taperedRails,x,side*(innerSide-3.5),z));
    }
  }
  std::cout<<"Former geometry: full/partial height, cavity fit, ledge clearance, overlaps, taper, invalid placement and cancellation passed\n";
}
#include "FuselageHolesChecks.h"
int main(int argc,char** argv) {
  qInstallMessageHandler([](QtMsgType,const QMessageLogContext&,const QString& text){std::cerr<<text.toStdString()<<std::endl;});
  QApplication app{argc,argv};QTemporaryDir dir;QCoreApplication::setOrganizationName("FoamFormerTests");QCoreApplication::setApplicationName("FoamFormerTests");
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
  try {
    if(app.arguments().contains("--holes")){holeChecks(app,argc>2?QString::fromLocal8Bit(argv[2]):dir.path());return 0;}
    if(app.arguments().contains("--preview-brep")){
      CHECK(argc==4);TopoDS_Shape shape;BRep_Builder builder;CHECK(BRepTools::Read(shape,argv[2],builder));
      OcctViewport viewer;viewer.resize(1200,800);viewer.show();app.processEvents();viewer.displayShape(shape);
      bool captured=false;QTimer::singleShot(1000,&app,[&]{captured=viewer.screen()->grabWindow(viewer.winId()).save(QString::fromLocal8Bit(argv[3]));app.quit();});app.exec();CHECK(captured);return 0;
    }
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
    p.workspace=2;p.tool="Formers";p.servoTray.rectangle=QRectF{350,385,100,5};
    const bool readinessOnly=app.arguments().contains("--readiness-only");
    if(readinessOnly){p.tool="Edit Profiles";p.servoTray.rectangle.reset();p.fuselageThickening=false;for(auto& station:p.fuselageStations.lines)station.thicknessMm.reset();}
    const bool defaultsOnly=app.arguments().contains("--defaults-only");
    if(defaultsOnly){p.servoTray.rectangle.reset();p.fuselageThickening=false;for(auto& station:p.fuselageStations.lines)station.thicknessMm.reset();p.fuselageStations.lines[0].thicknessMm=4.;}
    QString error;const auto file=dir.filePath("formers.foam");CHECK(writeProject(file,p,error));
    MainWindow window;window.show();app.processEvents();CHECK(window.openProjectFile(file,error));
    auto* tabs=window.findChild<QTabWidget*>("viewportTabs");auto* view=static_cast<PlanViewport*>(tabs->widget(0));view->fitInView(QRectF{0,0,900,500},Qt::KeepAspectRatio);app.processEvents();
    auto& editor=view->formerEditor();auto* toolbar=window.findChild<QToolBar*>("componentToolBar");CHECK(toolbar->actions()[6]->text()=="Formers");
    auto* add=window.findChild<QPushButton*>("formerAdd");auto* width=window.findChild<QLineEdit*>("formerWidth");auto* rotation=window.findChild<QDoubleSpinBox*>("formerRotationAngle");CHECK(add&&width&&rotation);CHECK(rotation->value()==0&&!rotation->isEnabled());CHECK(readinessOnly||add->isVisible());
    if(readinessOnly){
      auto* workspaces=window.findChild<QToolBar*>("workspaceToolBar");CHECK(workspaces);
      CHECK(workspaces->actions()[3]->isEnabled()&&workspaces->actions()[4]->isEnabled());
      // Readiness follows definitions, and retracts if a required profile is broken.
      const auto profiles=view->fuselageProfileEditor().state();auto broken=profiles;broken.layers[0].curves.pop_back();view->fuselageProfileEditor().restoreState(broken);
      QMetaObject::invokeMethod(&view->fuselageProfileEditor(),"changed");CHECK(!workspaces->actions()[3]->isEnabled()&&!workspaces->actions()[4]->isEnabled());
      view->fuselageProfileEditor().restoreState(profiles);QMetaObject::invokeMethod(&view->fuselageProfileEditor(),"changed");CHECK(workspaces->actions()[3]->isEnabled()&&workspaces->actions()[4]->isEnabled());
      const auto capture=qEnvironmentVariable("FOAM_READY_CAPTURE");if(!capture.isEmpty()){app.processEvents();CHECK(window.grab().save(capture));}
      CHECK(!window.projectDocument().fuselageThickening);tabs->setCurrentIndex(1);waitForModel(window);
      CHECK(tabs->widget(1)->property("fuselageModelReady").toBool());CHECK(window.projectDocument().fuselageThickening);
      CHECK(tabs->widget(1)->property("fuselageBodyCount").toInt()==2);CHECK(tabs->widget(1)->property("wingModelRevision").toInt()==0);
      workspaces->actions()[3]->trigger();CHECK(window.projectDocument().workspace==3);workspaces->actions()[4]->trigger();CHECK(window.projectDocument().workspace==4);
      std::cout<<"Fuselage generates without optional tab visits; both stabilizers follow readiness\n";return 0;
    }
    if(defaultsOnly){
      toolbar->actions()[5]->trigger();window.findChild<QPushButton*>("servoTrayPlace")->click();
      CHECK(view->servoTrayEditor().state().rectangle);CHECK(std::abs(view->servoTrayEditor().state().rectangle->height()-3)<1e-8);
      toolbar->actions()[6]->trigger();add->click();CHECK(editor.state().rectangles.size()==1);CHECK(std::abs(editor.state().rectangles[0].width()-3)<1e-8);
      CHECK(!window.projectDocument().fuselageThickening);
      tabs->setCurrentIndex(1);waitForModel(window);
      if(!tabs->widget(1)->property("fuselageModelReady").toBool())std::cerr<<window.statusBar()->currentMessage().toStdString()<<std::endl;
      CHECK(tabs->widget(1)->property("fuselageModelReady").toBool());CHECK(tabs->widget(1)->property("fuselageBodyCount").toInt()==4);
      const auto saved=window.projectDocument();CHECK(saved.fuselageThickening);CHECK(saved.fuselageStations.lines[0].thicknessMm==4.);CHECK(saved.fuselageStations.lines[1].thicknessMm.value_or(0)>0);
      CHECK(window.saveProjectFile(file,error));CHECK(readProject(file,error)->fuselageThickening);CHECK(tabs->widget(1)->property("wingModelRevision").toInt()==0);
      std::cout<<"Untouched tray/former defaults regenerate without visiting Thicken; missing walls initialized and explicit walls preserved\n";return 0;
    }
    auto enter=[&](QString value){width->setText(value);width->setModified(true);QMetaObject::invokeMethod(width,"editingFinished");};
    auto mouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton b,Qt::MouseButtons bs){const auto local=view->mapFromScene(point);QMouseEvent event{type,QPointF{local},QPointF{view->viewport()->mapToGlobal(local)},b,bs,Qt::NoModifier};QApplication::sendEvent(view->viewport(),&event);};
    auto click=[&](QPointF point){mouse(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);};
    auto drag=[&](QPointF a,QPointF b){mouse(QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);};
    enter("0.25 in");add->click();CHECK(editor.state().rectangles.size()==1);CHECK(std::abs(editor.state().rectangles[0].width()-6.35)<1e-8);
    CHECK(editor.state().rectangles[0].top()<350&&editor.state().rectangles[0].bottom()>430);CHECK(!rectanglesOverlap(editor.state().rectangles[0],*p.servoTray.rectangle));
    auto r=editor.state().rectangles[0];drag(r.center(),{280,r.center().y()});CHECK(std::abs(editor.state().rectangles[0].center().x()-280)<2);
    r=editor.state().rectangles[0];drag(r.center(),{400,r.center().y()});CHECK(editor.state().rectangles[0]==r);
    enter("8 mm");CHECK(std::abs(editor.state().rectangles[0].width()-8)<1e-8);
    r=editor.state().rectangles[0];drag({r.center().x(),r.top()},{r.center().x(),365});CHECK(std::abs(editor.state().rectangles[0].top()-365)<2);
    r=editor.state().rectangles[0];drag({r.center().x(),r.bottom()},{r.center().x(),415});CHECK(std::abs(editor.state().rectangles[0].bottom()-415)<2);
    r=editor.state().rectangles[0];drag(r.center(),r.center()+QPointF{0,4});CHECK(std::abs(editor.state().rectangles[0].top()-r.top()-4)<2);
    add->click();CHECK(editor.state().rectangles.size()==2);r=editor.state().rectangles[1];drag(r.center(),{510,r.center().y()});r=editor.state().rectangles[1];
    drag(r.center(),editor.state().rectangles[0].center());CHECK(editor.state().rectangles[1]==r);
    // Tray movement and resizing must obey the same collision rule.
    const auto trayBefore=view->servoTrayEditor().state().rectangle;
    view->servoTrayEditor().setDimensions({400,5},{400,390});CHECK(view->servoTrayEditor().state().rectangle==trayBefore);
    click(editor.state().rectangles[1].center());QKeyEvent del{QEvent::KeyPress,Qt::Key_Delete,Qt::NoModifier};QApplication::sendEvent(view,&del);CHECK(editor.state().rectangles.size()==1);
    add->click();CHECK(editor.state().rectangles.size()==2);
    CHECK(window.projectModified());CHECK(window.saveProjectFile(file,error));CHECK(window.openProjectFile(file,error));CHECK(editor.state().rectangles.size()==2);
    auto encoded=encodeProject(window.projectDocument());CHECK(encoded["version"]==28);auto old=encoded;old["version"]=14;old.remove("formers");CHECK(decodeProject(old).formers.rectangles.empty());
    auto v23=encoded;v23["version"]=23;auto legacyFormers=v23["formers"].toObject();legacyFormers.remove("rotationDegrees");v23["formers"]=legacyFormers;CHECK(decodeProject(v23).formers.rotationDegrees==std::vector<double>(2,0));
    auto invalidAngle=encoded;auto angleFields=invalidAngle["formers"].toObject();angleFields["rotationDegrees"]=QJsonArray{400,0};invalidAngle["formers"]=angleFields;
    bool angleRejected=false;try{decodeProject(invalidAngle);}catch(const std::exception&){angleRejected=true;}CHECK(angleRejected);
    auto oldUi=old["ui"].toObject();oldUi["tool"]="Firewall";old["ui"]=oldUi;CHECK(decodeProject(old).tool=="Formers");
    auto bad=encoded;auto formers=bad["formers"].toObject();auto rects=formers["rectangles"].toArray();rects.append(rects[0]);formers["rectangles"]=rects;bad["formers"]=formers;bool rejected=false;try{decodeProject(bad);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    toolbar=window.findChild<QToolBar*>("componentToolBar");toolbar->actions()[4]->trigger();CHECK(!window.projectModified());
    view->fitInView(QRectF{0,0,900,500},Qt::KeepAspectRatio);app.processEvents();
    const auto pix=view->viewport()->grab().toImage();r=editor.state().rectangles[0];const auto loc=view->mapFromScene({r.left(),r.center().y()});const QPoint at{qRound(loc.x()*pix.devicePixelRatio()),qRound(loc.y()*pix.devicePixelRatio())};bool green=false;
    for(int y=-3;y<=3;++y)for(int x=-3;x<=3;++x)if(pix.rect().contains(at+QPoint{x,y})){const auto c=pix.pixelColor(at+QPoint{x,y});if(c.green()>170&&c.red()<130)green=true;}CHECK(green);
    toolbar->actions()[6]->trigger();click(editor.state().rectangles[0].center());CHECK(rotation->isEnabled());
    rotation->setValue(-12.5);CHECK(editor.state().rotationDegrees[0]==-12.5);CHECK(editor.state().rotationDegrees[1]==0);
    auto tilted=editor.state().rectangles[0];const auto transformed=formerTransform(tilted,-12.5);CHECK(transformed.map(QPointF{tilted.center().x(),tilted.top()}).x()<tilted.center().x());
    CHECK(decodeProject(encodeProject(window.projectDocument())).formers.rotationDegrees[0]==-12.5);
    CHECK(window.saveProjectFile(file,error));CHECK(readProject(file,error)->formers.rotationDegrees[0]==-12.5);
    click(editor.state().rectangles[1].center());CHECK(rotation->value()==0);click(tilted.center());CHECK(rotation->value()==-12.5);
    // Drag and resize in the tilted former's own coordinates.
    drag(tilted.center(),tilted.center()+QPointF{-5,0});CHECK(std::abs(editor.state().rectangles[0].center().x()-tilted.center().x()+5)<2);
    tilted=editor.state().rectangles[0];const auto transform=formerTransform(tilted,-12.5);
    drag(transform.map(QPointF{tilted.center().x(),tilted.top()}),transform.map(QPointF{tilted.center().x(),tilted.top()+3}));CHECK(editor.state().rectangles[0].height()<tilted.height()-1);
    const auto capture=qEnvironmentVariable("FOAM_FORMER_CAPTURE");if(!capture.isEmpty()){app.processEvents();CHECK(window.grab().save(capture));}
    if(app.arguments().contains("--editor-only")) {
      CHECK(window.findChild<QLabel*>("formerInstructions")->text().contains("4 mm fore/aft"));
      CHECK(tabs->widget(1)->property("fuselageModelRevision").toInt()==0);
      CHECK(tabs->widget(1)->property("wingModelRevision").toInt()==0);
      std::cout<<"Former editor, units, placement, overlap, persistence, overlay and instructions passed (no generation)\n";return 0;
    }
    tabs->setCurrentIndex(1);waitForModel(window);
    if(!tabs->widget(1)->property("fuselageModelReady").toBool())std::cerr<<window.statusBar()->currentMessage().toStdString()<<std::endl;
    CHECK(tabs->widget(1)->property("fuselageModelReady").toBool());CHECK(tabs->widget(1)->property("formerCount").toInt()==2);
    CHECK(tabs->widget(1)->property("fuselageBodyCount").toInt()==5);CHECK(tabs->widget(1)->property("wingModelRevision").toInt()==0);
    const auto revision=tabs->widget(1)->property("fuselageModelRevision").toInt();tabs->setCurrentIndex(0);tabs->setCurrentIndex(1);waitForModel(window);CHECK(tabs->widget(1)->property("fuselageModelRevision").toInt()==revision);
    tabs->setCurrentIndex(0);click(editor.state().rectangles[0].center());rotation->setValue(-10.25);tabs->setCurrentIndex(1);waitForModel(window);
    CHECK(tabs->widget(1)->property("fuselageModelReady").toBool());CHECK(tabs->widget(1)->property("fuselageModelRevision").toInt()>revision);
    std::cout<<"Former UI: rotation, cache invalidation, add, units, move, partial-height resize, delete, bidirectional overlap checks, persistence, overlay and generation/cache passed\n";
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<std::endl;return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

#include "geometry/Assembly.h"
#include "gui/MainWindow.h"
#include "gui/PlanViewport.h"
#include "WaitForModel.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <GProp_GProps.hxx>
#include <QApplication>
#include <QAction>
#include <QJsonArray>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QPainter>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <QFile>
#include <QJsonDocument>
#include <QScreen>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <iostream>
#include <Standard_Failure.hxx>
#include <cmath>
#define CHECK(c) do { if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__)); } while(false)
using namespace designrc;
using namespace designrc::gui;
double volume(const TopoDS_Shape& s){GProp_GProps mass;BRepGProp::VolumeProperties(s,mass,1e-7);return mass.Mass();}
TopoDS_Shape box(double x,double y,double z,double dx,double dy,double dz) {
  auto shape=BRepPrimAPI_MakeBox{gp_Pnt{x,y,z},dx,dy,dz}.Shape();
  BRepMesh_IncrementalMesh mesh{shape,.1};return shape;
}
geometry::AssemblyParts parts() {
  return {box(0,-10,-10,100,20,20),box(30,-40,5,20,80,10),
      box(80,-25,-2,15,50,4),box(85,-2,-5,10,4,25),
      box(100,-25,-2,5,50,4),box(100,-2,0,5,4,20)};
}
SketchLayer rectangle(double x,double y,double w,double h) {
  return {{{x,y},{x+w,y},{x+w,y+h},{x,y+h}},{{SketchTool::Line,{0,1}},
    {SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};
}
ProjectDocument fixture() {
  ProjectDocument p;p.reference.wingspanMm=1000;p.reference.fuselageLengthMm=100;
  p.wingspanText="1000 mm";p.fuselageText="100 mm";
  p.wing.layers[0]=rectangle(10,10,90,70);p.wing.layers[0].curves.pop_back();
  p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},
      {{0,0,1,{100,10}},{0,2,0,{100,80}},LineAlignment::Vertical,0}};
  p.airfoils.entries.push_back({"NACA",domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
  p.fuselage.layers={rectangle(200,100,400,50),rectangle(200,200,400,50)};
  p.fuselageStations.lines={{{1,0,.25,{300,200}},{1,2,.75,{300,250}},LineAlignment::Vertical}};
  p.fuselageStations.lines[0].profile=0;p.fuselageStations.lines[0].thicknessMm=5;p.fuselageThickening=true;
  p.fuselageProfiles.layers[0]=rectangle(700,100,50,50);
  for(auto& outline:p.stabilizerOutlines){outline.layers[0]=rectangle(800,300,100,60);outline.layers[0].curves.pop_back();outline.layers[0].leadingEdge=0;}
  QImage image{1000,450,QImage::Format_RGB32};image.fill(QColor{250,247,232});
  QPainter painter{&image};painter.setPen(QPen{Qt::red,4});painter.drawRect(QRectF{200,185,400,80});
  painter.drawText(QPointF{210,178},"REFERENCE: NOSE LEFT / TOP");painter.end();
  p.reference.image.pages.push_back({image,{}});
  return p;
}
namespace designrc::gui {
class AssemblyWorkflowTest {
public:
  static void install(MainWindow& w,geometry::AssemblyParts p) {
    // Inject already-generated component caches. This test never calls either
    // Wing or Fuselage generation (AGENTS.md restriction).
    w.wingShape_=p.wing;w.fuselageShape_=p.fuselage;w.fuselageModel_.shape=p.fuselage;
    w.fuselageModel_.body=p.fuselage;
    geometry::AssemblyParts h;h.horizontal=p.horizontal;h.elevator=p.elevator;
    geometry::AssemblyParts v;v.vertical=p.vertical;v.rudder=p.rudder;
    w.stabilizerModels_={geometry::StabilizerBuildResult{geometry::assemblyShape(h),p.horizontal,p.elevator},
                        geometry::StabilizerBuildResult{geometry::assemblyShape(v),p.vertical,p.rudder}};
    w.builtWingFingerprint_=w.wingFingerprint();w.builtFuselageFingerprint_=w.fuselageFingerprint();
    for(int i=0;i<2;++i){w.stabilizerShapes_[i]=w.stabilizerModels_[i].shape;w.builtStabilizerFingerprints_[i]=w.stabilizerFingerprint(i);}
  }
  static void persistenceUi(const QString& directory) {
    // Empty compound handles simulate populated in-memory caches. No geometry
    // generators, Boolean operations, meshing or geometry tests are invoked.
    TopoDS_Compound shape;BRep_Builder{}.MakeCompound(shape);
    geometry::AssemblyParts sample{shape,shape,shape,shape,shape,shape};
    MainWindow w;w.show();w.restoreProject(fixture());install(w,sample);
    w.fuselageModel_.servoTray=shape;w.servoTrayTopFaces_=shape;w.fuselageModel_.formers={shape};
    w.assemblyOriginals_=sample;w.assemblyCutParts_=sample;w.assemblyState_.positioned=true;w.assemblyState_.cuts=true;
    w.assemblyState_.offsets[0]={12,34};w.assemblySourceFingerprint_=w.assemblyFingerprint();
    QString error;const auto file=directory+"/no-models.foam";
    CHECK(w.saveProjectFile(file,error));
    QFile input{file};CHECK(input.open(QIODevice::ReadOnly));
    const auto json=QJsonDocument::fromJson(input.readAll()).object();input.close();
    CHECK(json["version"]==24&&!json.contains("models"));CHECK(!encodeProject(w.projectDocument(),false).contains("models"));
    CHECK(!w.wingShape_.IsNull()&&w.assemblyCutParts_); // Save retains session caches.
    for(const QJsonValue obsolete:{QJsonValue{QJsonObject{{"wing",QJsonObject{{"fingerprint","bad"},{"shapes",QJsonArray{QJsonObject{{"brep","not compressed geometry"},{"sha256","bad"}}}}}}}},QJsonValue{"invalid legacy cache"},QJsonValue{}}) {
      auto legacy=json;legacy["version"]=22;legacy["models"]=obsolete;
      legacy["ui"]=QJsonObject{legacy["ui"].toObject()};auto ui=legacy["ui"].toObject();ui["workspace"]=5;ui["viewport"]=1;legacy["ui"]=ui;
      QFile output{file};CHECK(output.open(QIODevice::WriteOnly));output.write(QJsonDocument{legacy}.toJson());output.close();
      CHECK(w.openProjectFile(file,error));QApplication::processEvents();
      CHECK(w.graphicsTabs_->currentIndex()==0&&!w.property("modelProcessing").toBool());
      CHECK(w.wingShape_.IsNull()&&w.fuselageShape_.IsNull());
      CHECK(w.stabilizerShapes_[0].IsNull()&&w.stabilizerShapes_[1].IsNull());
      CHECK(w.servoTrayTopFaces_.IsNull()&&w.fuselageModel_.formers.empty());
      CHECK(w.assemblyOriginals_.fuselage.IsNull()&&!w.assemblyCutParts_);
      CHECK(w.assemblyState_.cuts&&w.assemblyState_.offsets[0]==QPointF(12,34));
      CHECK(!w.projectModified());if(!w.saveProjectFile(file,error))throw std::runtime_error(error.toStdString());
      CHECK(input.open(QIODevice::ReadOnly));const auto resaved=QJsonDocument::fromJson(input.readAll()).object();input.close();
      CHECK(resaved["version"]==24&&!resaved.contains("models"));
    }
    auto older=json;older["version"]=21;CHECK(decodeProject(older).assembly.cuts);
    auto invalid=json;invalid["assembly"]=false;bool rejected=false;
    try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
  }
  static void run(const QString& directory) {
    MainWindow w;w.show();QApplication::processEvents();
    CHECK(!w.workspaceToolBar_->actions()[5]->isEnabled());
    // Opening any saved viewport stays in 2D and starts no geometry jobs.
    for(int workspace=0;workspace<=5;++workspace)for(int view=0;view<=1;++view) {
      auto saved=fixture();saved.workspace=workspace;saved.viewport=view;
      saved.assembly.positioned=true;saved.assembly.cuts=true;saved.assembly.offsets[0]={12,34};
      QString error;const auto file=directory+"/open-2d.foam";
      CHECK(writeProject(file,saved,error));CHECK(w.openProjectFile(file,error));
      QApplication::processEvents();
      CHECK(w.graphicsTabs_->currentIndex()==0&&w.graphicsTabs_->isTabEnabled(0));
      CHECK(!w.property("modelProcessing").toBool());
      CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.stabilizerProcessing()&&!w.assemblyProcessing());
      CHECK(w.wingShape_.IsNull()&&w.fuselageShape_.IsNull());
      CHECK(w.projectDocument().workspace==(workspace==5?2:workspace));
      CHECK(w.assemblyState_.cuts&&w.assemblyState_.offsets[0]==QPointF(12,34));
      CHECK(!w.projectModified());
    }
    w.restoreProject(fixture());install(w,parts());
    CHECK(w.workspaceToolBar_->actions()[5]->isEnabled());
    // Real cache preparation worker, using all four existing component caches.
    w.workspaceToolBar_->actions()[5]->trigger();waitForModel(w);
    CHECK(w.property("assemblyReady").toBool());CHECK(!w.graphicsTabs_->isTabEnabled(0));
    CHECK(w.viewport_->property("assemblyReferencePages").toInt()==1);
    CHECK(w.graphicsTabs_->currentIndex()==1);CHECK(w.assemblyState_.positioned);
    auto camera=w.viewport_->cameraState();CHECK(camera);CHECK(std::abs(camera->center[0]-50)<1e-6);
    CHECK(camera->eye[1]<camera->center[1]);CHECK(camera->up[2]>.99);
    w.assemblySelect_[0]->click();const auto offset=w.assemblyState_.offsets[0];
    w.activateWindow();w.raise();QApplication::processEvents();
    QKeyEvent press{QEvent::KeyPress,Qt::Key_Right,Qt::NoModifier};QApplication::sendEvent(w.assemblySelect_[0],&press);
    QKeyEvent release{QEvent::KeyRelease,Qt::Key_Right,Qt::NoModifier};QApplication::sendEvent(w.assemblySelect_[0],&release);
    CHECK(w.assemblyState_.offsets[0]==offset+QPointF(1,0));
    w.moveAssembly(Qt::Key_Up,Qt::ShiftModifier);CHECK(w.assemblyState_.offsets[0]==offset+QPointF(1,10));
    w.moveAssembly(Qt::Key_Down,Qt::ControlModifier);CHECK(std::abs(w.assemblyState_.offsets[0].y()-offset.y()-9.9)<1e-8);
    // Position fixture parts into known intersecting seats, with controls clear.
    w.assemblyState_.offsets={};
    // Rudder initially overlaps elevator; report both names and leave originals untouched.
    bool popup=false;QTimer dismiss;QObject::connect(&dismiss,&QTimer::timeout,[&]{
      if(auto* message=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
        popup=message->text().contains("Elevator intersects Rudder");message->accept();
      }});dismiss.start(10);
    w.assemblyCutButton_->click();waitForModel(w);dismiss.stop();CHECK(popup);CHECK(!w.assemblyState_.cuts);
    w.assemblyOriginals_.rudder=box(100,-2,5,5,4,15); // Same fixed fin; control above elevator.
    const double before=volume(w.assemblyOriginals_.fuselage);
    w.assemblyCutButton_->click();waitForModel(w);
    CHECK(w.assemblyState_.cuts);CHECK(w.assemblyCutButton_->text()=="Undo Cuts");
    for(auto* button:w.assemblySelect_)CHECK(!button->isEnabled());
    CHECK(volume(w.assemblyOriginals_.fuselage)==before);CHECK(w.exportAssemblyParts());
    CHECK(volume(w.exportAssemblyParts()->fuselage)<before);
    auto frozen=w.assemblyState_.offsets;w.moveAssembly(Qt::Key_Left,Qt::NoModifier);CHECK(w.assemblyState_.offsets==frozen);
    QString error;CHECK(w.saveProjectFile(directory+"/assembly.foam",error));
    const auto saved=readProject(directory+"/assembly.foam",error);CHECK(saved&&saved->assembly.cuts);
    CHECK(saved->assembly.offsets==frozen);CHECK(saved->workspace==5&&saved->viewport==1);
    QApplication::processEvents();
    if(auto* screen=w.screen())screen->grabWindow(w.winId()).save(directory+"/assembly-cut.png");
    // Cut state survives component navigation, and only the cut snapshot is exported.
    w.workspaceToolBar_->actions()[0]->trigger();CHECK(w.graphicsTabs_->isTabEnabled(0));
    w.workspaceToolBar_->actions()[5]->trigger();waitForModel(w);CHECK(w.assemblyState_.cuts);
    w.assemblyCutButton_->click();CHECK(!w.assemblyState_.cuts);
    CHECK(volume(w.exportAssemblyParts()->fuselage)==before);
    for(auto* button:w.assemblySelect_)CHECK(button->isEnabled());
    // Cancel uses the standard worker control and publishes no result.
    w.assemblyCutButton_->click();w.cancelProcessing_->click();waitForModel(w);CHECK(!w.assemblyState_.cuts);
    // Mutating a component input invalidates export and the derived Assembly cache.
    auto outline=w.planViewport_->stabilizerSketchEditor(0).state();outline.layers[0].points[1].rx()+=1;
    w.restoringProject_=true;w.planViewport_->stabilizerSketchEditor(0).restoreState(outline);w.restoringProject_=false;
    CHECK(!w.exportAssemblyParts());w.invalidateAssembly();CHECK(w.assemblyOriginals_.fuselage.IsNull());
    w.resetProject();CHECK(!w.assemblyState_.positioned&&!w.assemblyState_.cuts);CHECK(!w.exportAssemblyParts());
  }
};
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QApplication::setStyle("Fusion");QTemporaryDir settings;
  QCoreApplication::setOrganizationName("FoamAssemblyTests");QCoreApplication::setApplicationName("FoamAssemblyTests");
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
  try {
    if(argc>1&&QString::fromLocal8Bit(argv[1])=="--persistence-ui") {
      AssemblyWorkflowTest::persistenceUi(argc>2?QString::fromLocal8Bit(argv[2]):settings.path());
      std::cout<<"Input-only saves, ignored legacy caches and 2D restore passed; no geometry tests run.\n";return 0;
    }
    auto p=parts();const double original=volume(p.fuselage);
    auto collision=geometry::cutAssemblyIntersections(p);CHECK(collision.collisions.size()==1);
    CHECK(collision.collisions[0]=="Elevator intersects Rudder");CHECK(volume(p.fuselage)==original);
    p.rudder=box(100,-2,5,5,4,15);
    auto cut=geometry::cutAssemblyIntersections(p);CHECK(cut.collisions.empty());
    // Wing saddle: 2000 mm3; horizontal seat: 1200; fin removes 600
    // additional mm3 after subtracting its 160 mm3 overlap with horizontal.
    CHECK(std::abs(volume(cut.parts.fuselage)-(original-2000-1200-440))<1e-5);
    CHECK(std::abs(volume(cut.parts.horizontal)-(volume(p.horizontal)-160))<1e-5);
    CHECK(BRepCheck_Analyzer{cut.parts.fuselage}.IsValid());CHECK(BRepCheck_Analyzer{cut.parts.horizontal}.IsValid());
    CHECK(volume(p.fuselage)==original);CHECK(volume(cut.parts.wing)==volume(p.wing));
    auto pass=p;pass.wing=box(30,-40,-3,20,80,6);auto through=geometry::cutAssemblyIntersections(pass);
    CHECK(through.collisions.empty());CHECK(volume(through.parts.fuselage)<original);
    // All offending combinations are reported before subtraction.
    auto bad=p;bad.elevator=box(85,-5,-1,10,10,2);bad.rudder=box(86,-2,-1,5,4,5);
    CHECK(geometry::cutAssemblyIntersections(bad).collisions.size()==1);
    std::stop_source stop;stop.request_stop();bool cancelled=false;
    try{geometry::cutAssemblyIntersections(p,{}, {stop.get_token()});}catch(const geometry::ProcessingCancelled&){cancelled=true;}CHECK(cancelled);
    const auto side=rectangle(200,200,400,50);
    const auto mapping=geometry::fuselageSideTransform(side,100.);
    CHECK(mapping.left==200&&mapping.verticalOrigin==225&&mapping.scale==.25);
    const auto physical=geometry::fuselageSideTransform(side,std::nullopt);CHECK(physical.scale==1.);
    auto doc=fixture();doc.assembly=geometry::initialAssemblyPlacement(p);doc.assembly.cuts=true;
    auto json=encodeProject(doc);CHECK(json["version"]==24);CHECK(decodeProject(json).assembly.cuts);
    auto legacy=json;legacy["version"]=20;legacy.remove("assembly");CHECK(!decodeProject(legacy).assembly.positioned);
    auto invalid=json;auto state=invalid["assembly"].toObject();state["offsets"]=QJsonArray{};invalid["assembly"]=state;
    bool rejected=false;try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    AssemblyWorkflowTest::run(argc>1?QString::fromLocal8Bit(argv[1]):settings.path());
    std::cout<<"Assembly geometry, collision, editor, cache, undo and persistence checks passed.\n";return 0;
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

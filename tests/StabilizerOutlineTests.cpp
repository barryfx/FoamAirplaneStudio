#include "gui/MainWindow.h"
#include "gui/StabilizerOutlinePanel.h"
#include "gui/StabilizerAirfoilPanel.h"
#include "geometry/StabilizerSolidBuilder.h"
#include "WaitForModel.h"
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <QStatusBar>
#include <QScreen>
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QAction>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <iostream>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <stdexcept>
using namespace designrc::gui;
#define CHECK(c) do { if (!(c)) throw std::runtime_error(std::string{#c}+" at line "+std::to_string(__LINE__)); } while(false)

SketchLayer rectangle(double x, double y, double w, double h) {
  return {{{x,y},{x+w,y},{x+w,y+h},{x,y+h}},
    {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};
}
ProjectDocument fixture() {
  ProjectDocument p;
  p.reference.wingspanMm=1000; p.reference.fuselageLengthMm=700;
  p.wingspanText="1000 mm"; p.fuselageText="700 mm";
  p.wing.layers[0]=rectangle(10,10,90,70); p.wing.layers[0].curves.pop_back();
  p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},
    {{0,0,1,{100,10}},{0,2,0,{100,80}},LineAlignment::Vertical,0}};
  p.airfoils.entries.push_back({"NACA",designrc::domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
  p.fuselage.layers={rectangle(200,100,400,50),rectangle(200,200,400,50)};
  p.fuselageStations.lines={{{1,0,.25,{300,200}},{1,2,.75,{300,250}},LineAlignment::Vertical}};
  p.fuselageProfiles.layers[0]=rectangle(700,100,50,50); p.fuselageStations.lines[0].profile=0;
  p.workspace=3; p.tool="Outline";
  return p;
}
class EventCounter : public QObject {
public:
 int paints=0,resizes=0;
 bool eventFilter(QObject*,QEvent* event) override {
   if(event->type()==QEvent::Paint)++paints;
   if(event->type()==QEvent::Resize) ++resizes;
   return false;
 }
};
int main(int argc,char** argv) {
  QApplication app{argc,argv}; QTemporaryDir dir;
  QCoreApplication::setOrganizationName("FoamStabilizerTests"); QCoreApplication::setApplicationName("FoamStabilizerTests");
  QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
  try {
    if(app.arguments().contains("--open-only")) {
      CHECK(argc==3);QString error;QElapsedTimer elapsed;elapsed.start();
      std::cout<<"Reading project"<<std::endl;
      const auto document=readProject(QString::fromLocal8Bit(argv[2]),error);CHECK(document);
      CHECK(document->viewport==0); // This diagnostic must never trigger component generation.
      std::cout<<"Read complete: "<<elapsed.elapsed()<<" ms; constructing window"<<std::endl;
      QApplication::setStyle("Fusion");
      MainWindow window;window.showMaximized();app.processEvents();
      EventCounter events;auto* plan=dynamic_cast<PlanViewport*>(window.findChild<QGraphicsView*>());CHECK(plan);plan->viewport()->installEventFilter(&events);
      std::cout<<"Opening project"<<std::endl;elapsed.restart();
      CHECK(window.openProjectFile(QString::fromLocal8Bit(argv[2]),error));
      std::cout<<"Open returned: "<<elapsed.elapsed()<<" ms"<<std::endl;
      int ticks=0;QEventLoop loop;QTimer heartbeat,finish;
      QObject::connect(&heartbeat,&QTimer::timeout,&loop,[&]{++ticks;});
      QObject::connect(&finish,&QTimer::timeout,&loop,&QEventLoop::quit);
      heartbeat.start(25);finish.setSingleShot(true);finish.start(3000);loop.exec();
      std::cout<<"Event loop ended: "<<elapsed.elapsed()<<" ms; paints="<<events.paints<<" resizes="<<events.resizes<<" ticks="<<ticks<<std::endl;
      CHECK(events.resizes<20);CHECK(ticks>=10);CHECK(!window.property("modelProcessing").toBool());CHECK(!window.projectModified());
      const auto capture=qEnvironmentVariable("FOAM_OPEN_CAPTURE");
      if(!capture.isEmpty())CHECK(window.grab().save(capture));
      std::cout<<"Responsive: "<<ticks<<" heartbeats; no model generation"<<std::endl;return 0;
    }
    const SketchLayer chain{{{200,500},{300,350},{500,350},{600,500}},{{SketchTool::Spline,{0,1,2,3}}},0};
    CHECK(stabilizerOutlineWarning(chain).isEmpty());
    // Check both sides of each axis, inclusive boundaries, endpoint reversal,
    // and independence from translation, uniform scale and interior shape.
    for (double axis : {0., 90., 180., 270.})
      for (double offset : {-45., -10.001, -10., -9.999, 0., 9.999, 10., 10.001, 45.}) {
        const double radians = (axis + offset) * std::numbers::pi / 180.;
        auto oriented = chain;
        oriented.points.back() = oriented.points.front() + QPointF{400*std::cos(radians),400*std::sin(radians)};
        const bool passes = std::abs(offset) <= 10.;
        CHECK(stabilizerOutlineWarning(oriented).isEmpty() == passes);
        std::reverse(oriented.curves[0].points.begin(), oriented.curves[0].points.end());
        CHECK(stabilizerOutlineWarning(oriented).isEmpty() == passes);
        for(auto& pt:oriented.points)pt=pt*2+QPointF{1200,700};
        oriented.points[1] += QPointF{10000,-20000};
        CHECK(stabilizerOutlineWarning(oriented).isEmpty() == passes);
      }
    auto coincident=chain;coincident.points.back()=coincident.points.front();
    CHECK(stabilizerOutlineWarning(coincident).contains("coincide"));
    auto joined=chain; joined.curves={{SketchTool::Line,{2,3}},{SketchTool::Spline,{2,1,0}}};
    CHECK(stabilizerOutlineWarning(joined).isEmpty());
    auto invalid=joined; invalid.curves.push_back({SketchTool::Line,{1,3}}); CHECK(!stabilizerOutlineWarning(invalid).isEmpty());
    invalid=joined; invalid.curves.push_back({SketchTool::Line,{3,0}}); CHECK(!stabilizerOutlineWarning(invalid).isEmpty());
    invalid=joined; invalid.points.insert(invalid.points.end(),{{800,400},{850,350},{900,400}});
    invalid.curves.push_back({SketchTool::Spline,{4,5,6,4}}); CHECK(!stabilizerOutlineWarning(invalid).isEmpty());

    auto noLeading=chain;noLeading.leadingEdge.reset();CHECK(!stabilizerOutlineDefined(noLeading));
    auto interiorLeading=chain;interiorLeading.leadingEdge=1;CHECK(!stabilizerOutlineDefined(interiorLeading));
    CHECK(stabilizerOutlineDefined(chain));
    const auto defaultFoil=defaultStabilizerAirfoil();
    bool missingRejected=false;
    try{designrc::geometry::buildStabilizerSolid({noLeading,defaultFoil,1,true});}
    catch(const std::runtime_error&){missingRejected=true;}CHECK(missingRejected);
    const auto volume=[](const TopoDS_Shape& shape){GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass,1e-8);return mass.Mass();};
    const auto bodies=[](const TopoDS_Shape& shape){int n=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++n;return n;};
    const auto fin=designrc::geometry::buildStabilizerSolid({chain,defaultFoil,1,false});
    CHECK(BRepCheck_Analyzer{fin}.IsValid()&&bodies(fin)==1&&volume(fin)>0);
    auto rotated=chain;for(auto& pt:rotated.points)pt={pt.y(),pt.x()};
    const auto horizontal=designrc::geometry::buildStabilizerSolid({rotated,defaultFoil,2,true});
    CHECK(BRepCheck_Analyzer{horizontal}.IsValid()&&bodies(horizontal)==2);
    std::cout<<"Stabilizer volumes: fin="<<volume(fin)<<" horizontal scaled="<<volume(horizontal)<<" ratio="<<volume(horizontal)/(16*volume(fin))<<std::endl;
    CHECK(std::abs(volume(horizontal)/(16*volume(fin))-1)<1e-4);
    Bnd_Box bounds;BRepBndLib::Add(horizontal,bounds);double xmin,ymin,zmin,xmax,ymax,zmax;bounds.Get(xmin,ymin,zmin,xmax,ymax,zmax);
    CHECK(std::abs(ymin+ymax)<1e-5&&ymax>0);
    const SketchLayer squareTip{{{0,0},{0,100},{200,100},{200,0}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}}},0};
    const auto square=designrc::geometry::buildStabilizerSolid({squareTip,defaultFoil,1,false});
    CHECK(BRepCheck_Analyzer{square}.IsValid()&&bodies(square)==1);
    // An asymmetric swept planform makes a reversed root direction measurable.
    auto swept=squareTip;swept.points[1].setX(150);swept.points[2].setX(350);
    const auto forward=designrc::geometry::buildStabilizerSolid({swept,defaultFoil,1,true});
    swept.leadingEdge=3;
    const auto backward=designrc::geometry::buildStabilizerSolid({swept,defaultFoil,1,true});
    CHECK(BRepCheck_Analyzer{forward}.IsValid()&&BRepCheck_Analyzer{backward}.IsValid());
    Bnd_Box forwardBounds,backwardBounds;BRepBndLib::Add(forward,forwardBounds);BRepBndLib::Add(backward,backwardBounds);
    forwardBounds.Get(xmin,ymin,zmin,xmax,ymax,zmax);CHECK(xmax>340&&xmin>-1);
    backwardBounds.Get(xmin,ymin,zmin,xmax,ymax,zmax);CHECK(xmin<-140&&xmax<201);
    std::stop_source stop;stop.request_stop();bool stopped=false;
    try{designrc::geometry::buildStabilizerSolid({chain,defaultFoil,1,true},{},{stop.get_token()});}
    catch(const designrc::geometry::ProcessingCancelled&){stopped=true;}CHECK(stopped);
    auto p=fixture(); auto encoded=encodeProject(p); CHECK(encoded["version"]==18);
    auto legacy=encoded; legacy["version"]=15; legacy.remove("horizontalStabilizerOutline"); legacy.remove("verticalStabilizerOutline");
    CHECK(decodeProject(legacy).stabilizerOutlines[0].layers[0].curves.empty());
    for (const auto& oldTool : {"Airfoils","Airfoil Stations","Edit"}) {
      auto ui=legacy["ui"].toObject(); ui["tool"]=oldTool; legacy["ui"]=ui;
      CHECK(decodeProject(legacy).tool==(QString{oldTool}=="Airfoils"?"Airfoil":"Outline"));
    }
    auto withLeading=p;withLeading.stabilizerOutlines[0].layers[0]=chain;
    const auto selectedJson=encodeProject(withLeading);
    CHECK(decodeProject(selectedJson).stabilizerOutlines[0].layers[0].leadingEdge==0);
    auto oldSelection=selectedJson;oldSelection["version"]=17;
    CHECK(!decodeProject(oldSelection).stabilizerOutlines[0].layers[0].leadingEdge);
    auto reject=[](const QJsonObject& json){bool rejected=false;try{decodeProject(json);}catch(const std::exception&){rejected=true;}CHECK(rejected);};
    for(int invalidIndex:{-1,1,99}) {
      auto malformed=selectedJson;auto outline=malformed["horizontalStabilizerOutline"].toObject();
      auto ls=outline["layers"].toArray();auto l=ls[0].toObject();l["leadingEdge"]=invalidIndex;ls[0]=l;outline["layers"]=ls;
      malformed["horizontalStabilizerOutline"]=outline;reject(malformed);
    }
    auto invalidAirfoil=encoded;invalidAirfoil["stabilizerAirfoils"]=QJsonArray{QJsonValue{}};reject(invalidAirfoil);
    invalidAirfoil["stabilizerAirfoils"]=QJsonArray{QJsonObject{{"name","Flat"},{"coordinates",QJsonArray{QJsonArray{1,0},QJsonArray{.5,0},QJsonArray{0,0},QJsonArray{.5,0},QJsonArray{1,0}}}},QJsonValue{}};reject(invalidAirfoil);
    auto legacy16=encoded;legacy16["version"]=16;legacy16.remove("stabilizerAirfoils");
    CHECK(!decodeProject(legacy16).stabilizerAirfoils[0]&&!decodeProject(legacy16).stabilizerAirfoils[1]);
    auto bad=encoded; auto sketch=bad["horizontalStabilizerOutline"].toObject();
    auto layers=sketch["layers"].toArray(); layers.append(layers[0]); sketch["layers"]=layers; bad["horizontalStabilizerOutline"]=sketch; reject(bad);
    bad=encoded; sketch=bad["verticalStabilizerOutline"].toObject(); sketch["editing"]=true; bad["verticalStabilizerOutline"]=sketch; reject(bad);
    bad=encoded; sketch=bad["horizontalStabilizerOutline"].toObject(); sketch["pending"]=QJsonArray{QJsonArray{200,500}}; bad["horizontalStabilizerOutline"]=sketch; reject(bad);

    QString error; const auto file=dir.filePath("stabilizers.foam"); CHECK(writeProject(file,p,error));
    MainWindow window; window.show(); app.processEvents(); CHECK(window.openProjectFile(file,error)); app.processEvents();
    auto* tabs=window.findChild<QTabWidget*>("viewportTabs"); auto* view=static_cast<PlanViewport*>(tabs->widget(0));
    auto* workspaces=window.findChild<QToolBar*>("workspaceToolBar"); auto* toolbar=window.findChild<QToolBar*>("componentToolBar");
    auto mouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons){
      const auto local=view->mapFromScene(point); QMouseEvent event{type,QPointF{local},QPointF{view->viewport()->mapToGlobal(local)},button,buttons,Qt::NoModifier}; QApplication::sendEvent(view->viewport(),&event);
    };
    auto click=[&](QPointF point){mouse(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);};
    auto key=[&](int code){QKeyEvent event{QEvent::KeyPress,code,Qt::NoModifier};QApplication::sendEvent(view,&event);};
    QString warning, title;
    auto expectWarning=[&]{warning.clear();QTimer::singleShot(0,[&]{auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());if(!box)qFatal("Expected stabilizer warning");warning=box->text();title=box->windowTitle();box->accept();});};
    for(int i=0;i<2;++i) {
      workspaces->actions()[i+3]->trigger(); app.processEvents();
      auto& editor=view->stabilizerSketchEditor(i);
      auto* panel=static_cast<StabilizerOutlinePanel*>(window.findChild<QWidget*>(i==0?"horizontalStabilizerOutlinePanel":"verticalStabilizerOutlinePanel"));
      CHECK(panel->isVisible()&&editor.state().editing&&!view->stabilizerSketchEditor(1-i).state().editing);
      QStringList names; for(auto* action:toolbar->actions())names.append(action->text());
      CHECK(names==QStringList({"Outline","Airfoil","Hinge Line","Cut"})); CHECK(toolbar->actions()[0]->isChecked());
      CHECK(!window.projectModified());
      for(int tool=1;tool<4;++tool)CHECK(!toolbar->actions()[tool]->isEnabled());
      auto* airfoilPanel=static_cast<StabilizerAirfoilPanel*>(window.findChild<QWidget*>(i==0?"horizontalStabilizerAirfoilPanel":"verticalStabilizerAirfoilPanel"));
      CHECK(airfoilPanel->airfoil().name()=="NACA-0009 9.0% smoothed");
      CHECK(!airfoilPanel->selection());
      auto button=[&](const QString& name){for(auto* b:panel->findChildren<QPushButton*>())if(b->text()==name)return b;throw std::runtime_error("Missing button");};
      const auto text=panel->findChild<QLabel*>("stabilizerOutlineInstructions")->text();
      CHECK(text.contains(i==0?"including the elevator":"including the rudder")); CHECK(text.contains("within 10 degrees of horizontal or vertical"));
      button("Spline")->click(); CHECK(button("Spline")->isChecked()&&!window.projectModified());
      for(auto point:chain.points)click(point);
      key(Qt::Key_Escape); CHECK(editor.layers()[0].curves.size()==1&&window.projectModified());
      CHECK(stabilizerOutlineWarning(editor.layers()[0]).isEmpty());
      button("Spline")->click();
      CHECK(!stabilizerOutlineDefined(editor.layers()[0]));
      for(int tool=1;tool<4;++tool)CHECK(!toolbar->actions()[tool]->isEnabled());
      auto* selectLeading=button("Select Leading Edge End Point");
      selectLeading->click();CHECK(editor.selectingLeadingEdge());
      click(editor.layers()[0].points[1]);CHECK(!editor.layers()[0].leadingEdge&&editor.selectingLeadingEdge());
      key(Qt::Key_Escape);CHECK(!editor.selectingLeadingEdge()&&!selectLeading->isChecked());
      selectLeading->click();click(editor.layers()[0].points.back());
      CHECK(editor.layers()[0].leadingEdge==3&&!editor.selectingLeadingEdge()&&!selectLeading->isChecked());
      CHECK(stabilizerOutlineDefined(editor.layers()[0]));
      for(int tool=1;tool<4;++tool)CHECK(toolbar->actions()[tool]->isEnabled());
      // Removing a different curve compacts point IDs without losing the LE.
      auto selectedState=editor.state();auto remappedState=selectedState;
      remappedState.layers[0].curves={{SketchTool::Line,{0,1}},{SketchTool::Spline,{1,2,3}}};
      remappedState.selected=0;editor.restoreState(remappedState);key(Qt::Key_Delete);
      CHECK(editor.layers()[0].leadingEdge==2);
      editor.restoreState(selectedState);
      const auto old=editor.layers()[0].points[1]; mouse(QEvent::MouseButtonPress,old,Qt::LeftButton,Qt::LeftButton);
      mouse(QEvent::MouseMove,old+QPointF{0,-15},Qt::NoButton,Qt::LeftButton); mouse(QEvent::MouseButtonRelease,old+QPointF{0,-15},Qt::LeftButton,Qt::NoButton);
      CHECK(editor.layers()[0].points[1]!=old);
      click(editor.layers()[0].points[1]); key(Qt::Key_Escape);
      CHECK(window.saveProjectFile(file,error)); CHECK(!window.projectModified());
      selectLeading->click();click(editor.layers()[0].points.front());CHECK(window.projectModified());
      selectLeading->click();click(editor.layers()[0].points.back());CHECK(!window.projectModified());
      CHECK(window.openProjectFile(file,error));CHECK(editor.layers()[0].leadingEdge==3);
      // Generate using the default before the Airfoil panel is ever visited.
      tabs->setCurrentIndex(1);CHECK(window.property("modelProcessing").toBool());
      CHECK(window.findChild<QPushButton*>("cancelProcessing")->isVisible());
      CHECK(!workspaces->isEnabled()&&!tabs->isEnabled());
      window.findChild<QPushButton*>("cancelProcessing")->click();waitForModel(window);
      CHECK(window.statusBar()->currentMessage().contains("cancelled"));
      tabs->setCurrentIndex(0);tabs->setCurrentIndex(1);waitForModel(window);
      auto* solidView=tabs->widget(1);
      if(!solidView->property("stabilizerModelReady").toBool())throw std::runtime_error(window.statusBar()->currentMessage().toStdString());
      CHECK(solidView->property("stabilizerBodyCount").toInt()==(i==0?2:1));
      const int revision=solidView->property("stabilizerModelRevision").toInt();
      const auto modelCapture=qEnvironmentVariable("FOAM_STABILIZER_CAPTURE");
      if(!modelCapture.isEmpty()){app.processEvents();CHECK(window.screen()->grabWindow(window.winId()).save(modelCapture+"model"+QString::number(i)+".png"));}
      tabs->setCurrentIndex(0);
      toolbar->actions()[1]->trigger(); CHECK(!editor.state().editing&&toolbar->actions()[1]->isChecked()); CHECK(!window.projectModified());
      CHECK(airfoilPanel->isVisible());CHECK(!window.statusBar()->currentMessage().contains("not implemented"));
      if(!modelCapture.isEmpty()){app.processEvents();CHECK(window.grab().save(modelCapture+"airfoil"+QString::number(i)+".png"));}
      CHECK(airfoilPanel->findChild<QLabel*>("stabilizerAirfoilName")->text()=="NACA-0009 9.0% smoothed");
      CHECK(airfoilPanel->findChild<QPushButton*>("loadStabilizerAirfoil")->text()=="Load Airfoil .dat File");
      toolbar->actions()[0]->trigger(); CHECK(editor.state().editing);
      tabs->setCurrentIndex(1); CHECK(!editor.state().editing&&!panel->isEnabled()); waitForModel(window);CHECK(solidView->property("stabilizerModelRevision").toInt()==revision);tabs->setCurrentIndex(0); CHECK(editor.state().editing);
      // A mismatched endpoint warns on either mode exit or 3D entry without moving it.
      auto state=editor.state();state.layers[0].points.back().setY(650);editor.restoreState(state);
      expectWarning(); if(i==0)toolbar->actions()[2]->trigger();else tabs->setCurrentIndex(1);
      CHECK(warning.contains("within 10 degrees of horizontal or vertical")&&title.contains(i==0?"Horizontal":"Vertical"));
      CHECK(editor.layers()[0].points.back().y()==650&&!editor.state().editing);
      if(i==0)toolbar->actions()[0]->trigger();else tabs->setCurrentIndex(0);
      state=editor.state();state.layers[0]=chain;editor.restoreState(state);
      // A 90-degree rotated outline also leaves Outline without a dialog.
      for(auto& pt:state.layers[0].points)pt={pt.y(),pt.x()};
      editor.restoreState(state);toolbar->actions()[1]->trigger();CHECK(!editor.state().editing);
      toolbar->actions()[0]->trigger();state=editor.state();state.layers[0]=chain;editor.restoreState(state);
      // Select a point along the curve, away from its editing handles, then Delete.
      const auto path=SketchEditor::fittedPath(chain.points,SketchTool::Spline);
      click(path.pointAtPercent(.18));CHECK(editor.selectedCurve()==0);key(Qt::Key_Delete);CHECK(editor.layers()[0].curves.empty());CHECK(!editor.layers()[0].leadingEdge);
      for(int tool=1;tool<4;++tool)CHECK(!toolbar->actions()[tool]->isEnabled());
      button("Line")->click();click(chain.points[0]);click(chain.points[1]);
      button("Spline")->click();click(chain.points[1]);click(chain.points[2]);click(chain.points[3]);key(Qt::Key_Escape);
      CHECK(editor.layers()[0].curves.size()==2&&editor.layers()[0].points.size()==4); CHECK(stabilizerOutlineWarning(editor.layers()[0]).isEmpty());
      // Save/Open must preserve the unfinished spline, active tool, and both components.
      click({600,500});click({650,450}); CHECK(editor.state().pending.size()==2);
      CHECK(window.saveProjectFile(file,error)); const auto saved=encodeProject(window.projectDocument(),false);
      CHECK(window.openProjectFile(file,error)); CHECK(editor.state().pending.size()==2&&editor.state().editing&&button("Spline")->isChecked());
      CHECK(!window.projectModified()); CHECK(encodeProject(window.projectDocument(),false)[i==0?"horizontalStabilizerOutline":"verticalStabilizerOutline"]==saved[i==0?"horizontalStabilizerOutline":"verticalStabilizerOutline"]);
      // Restore a completed valid outline for navigation and the visual capture.
      state=editor.state();state.pending.clear();state.layers[0]=chain;state.tool=SketchTool::None;editor.restoreState(state);panel->restoreControls();
      CHECK(window.projectDocument().wing.layers[0].points==p.wing.layers[0].points);
      CHECK(window.projectDocument().fuselage.layers[0].points==p.fuselage.layers[0].points);
      const auto capture=qEnvironmentVariable("FOAM_STABILIZER_CAPTURE");
      if(!capture.isEmpty()){view->fitAll();app.processEvents();CHECK(window.grab().save(capture+QString::number(i)+".png"));}
      // Imported airfoil names and coordinates survive Save/Open without the source file.
      const auto dat=dir.filePath(QString{"custom%1.dat"}.arg(i));
      {QFile output{dat};CHECK(output.open(QIODevice::WriteOnly));QByteArray contents="Custom stabilizer profile\n";
       for(auto pt:designrc::domain::AirfoilProfile::nacaSymmetric(.15).outline())contents+=QByteArray::number(pt.x,'g',17)+" "+QByteArray::number(pt.y,'g',17)+"\n";
       output.write(contents);}
      CHECK(airfoilPanel->loadFile(dat,error));CHECK(window.projectModified());
      CHECK(airfoilPanel->airfoil().name()=="Custom stabilizer profile");
      const auto selected=airfoilPanel->airfoil().outline().size();
      CHECK(!airfoilPanel->loadFile(dir.filePath("missing.dat"),error));CHECK(airfoilPanel->airfoil().outline().size()==selected);
      CHECK(window.saveProjectFile(file,error));CHECK(QFile::remove(dat));
      CHECK(window.openProjectFile(file,error));CHECK(airfoilPanel->airfoil().name()=="Custom stabilizer profile");CHECK(!window.projectModified());
      tabs->setCurrentIndex(1);waitForModel(window);
      if(!solidView->property("stabilizerModelReady").toBool())throw std::runtime_error(window.statusBar()->currentMessage().toStdString());
      CHECK(window.projectDocument().stabilizerAirfoils[i]);
      CHECK(!solidView->property("wingModelReady").toBool()&&!solidView->property("fuselageModelReady").toBool());
      tabs->setCurrentIndex(0);
      CHECK(window.saveProjectFile(file,error));workspaces->actions()[0]->trigger(); CHECK(!window.projectModified());
    }
    CHECK(!view->stabilizerSketchEditor(0).layers()[0].curves.empty()&&!view->stabilizerSketchEditor(1).layers()[0].curves.empty());
    CHECK(window.saveProjectFile(file,error));
    const auto before=encodeProject(window.projectDocument(),false); const auto invalidFile=dir.filePath("invalid.foam");
    {QFile output{invalidFile};CHECK(output.open(QIODevice::WriteOnly));output.write(QJsonDocument{bad}.toJson());}
    CHECK(!window.openProjectFile(invalidFile,error)); CHECK(encodeProject(window.projectDocument(),false)==before);
    window.findChild<QAction*>("projectNew")->trigger(); CHECK(view->stabilizerSketchEditor(0).layers()[0].curves.empty()&&view->stabilizerSketchEditor(1).layers()[0].curves.empty());
    CHECK(window.openProjectFile(file,error)); CHECK(view->stabilizerSketchEditor(1).layers()[0].curves.size()==1);
    window.findChild<QAction*>("projectClose")->trigger(); CHECK(view->stabilizerSketchEditor(1).layers()[0].curves.empty());
    PlanViewport remap; QImage image{100,100,QImage::Format_RGB32};image.fill(Qt::white);
    remap.setReferenceBackground({{image,QSizeF{200,200}}},false);
    for(int i=0;i<2;++i){SketchState state;state.layers[0]=chain;remap.stabilizerSketchEditor(i).restoreState(state);}
    remap.setReferenceBackground({{image,QSizeF{200,200}}},true);
    for(int i=0;i<2;++i){CHECK(remap.stabilizerSketchEditor(i).layers()[0].points[0]==chain.points[0]*2);CHECK(remap.stabilizerSketchEditor(i).layers()[0].leadingEdge==0); }
    std::cout<<"Stabilizer outlines: drawing, editing, warnings, isolation, migration and lifecycle passed\n";
    return 0;
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}

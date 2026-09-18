#include "gui/MainWindow.h"
#include "gui/FuselageOutlinePanel.h"
#include "gui/SketchBoundary.h"
#include <QApplication>
#include <QAction>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace designrc::gui;
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at line "+std::to_string(__LINE__));}while(false)
ProjectDocument fixture() {
  ProjectDocument p;
  p.reference.wingspanMm=1000;p.reference.fuselageLengthMm=700;
  p.wingspanText="1000 mm";p.fuselageText="700 mm";
  p.wing.layers[0]={{{10,10},{100,10},{100,80},{10,80}},
    {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}}}};
  p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},
    {{0,0,1,{100,10}},{0,2,0,{100,80}},LineAlignment::Vertical,0}};
  p.airfoils.entries.push_back({"NACA",designrc::domain::AirfoilProfile::nacaSymmetric(.12),{}, {}});
  p.workspace=2;p.tool="Outline";
  return p;
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QTemporaryDir dir;
  QCoreApplication::setOrganizationName("FoamFuselageTests");QCoreApplication::setApplicationName("FoamFuselageTests");
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
  try {
    auto p=fixture();auto canonical=encodeProject(p);CHECK(canonical["version"]==18);
    auto legacy=canonical;legacy["version"]=8;legacy.remove("fuselageOutline");
    auto old=decodeProject(legacy);CHECK(old.fuselage.layers.size()==2 && old.fuselageView==-1);
    auto bad=canonical;auto f=bad["fuselageOutline"].toObject();f["layers"]=QJsonArray{};bad["fuselageOutline"]=f;
    bool rejected=false;try{decodeProject(bad);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    SketchLayer closed{{{0,0},{100,0},{100,80},{0,80}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};
    CHECK(closedSketchBoundary(closed));auto branched=closed;branched.curves.push_back({SketchTool::Line,{0,2}});CHECK(!closedSketchBoundary(branched));
    auto multiple=closed;for(auto pt:closed.points)multiple.points.push_back(pt+QPointF{200,0});for(auto curve:closed.curves){for(auto& id:curve.points)id+=4;multiple.curves.push_back(curve);}CHECK(!closedSketchBoundary(multiple));
    QString error;const auto filename=dir.filePath("fuselage.foam");CHECK(writeProject(filename,p,error));
    MainWindow window;window.show();app.processEvents();CHECK(window.openProjectFile(filename,error));app.processEvents();
    auto* view=static_cast<PlanViewport*>(window.findChild<QTabWidget*>("viewportTabs")->widget(0));auto& editor=view->fuselageSketchEditor();
    auto* panel=static_cast<FuselageOutlinePanel*>(window.findChild<QWidget*>("fuselageOutlinePanel"));
    auto* top=window.findChild<QPushButton*>("fuselageTopView");auto* side=window.findChild<QPushButton*>("fuselageSideView");
    auto* tools=window.findChild<QToolBar*>("componentToolBar");auto* workspaces=window.findChild<QToolBar*>("workspaceToolBar");
    CHECK(panel->isVisible() && tools->actions()[0]->isChecked() && !tools->actions()[1]->isEnabled());
    CHECK(top->isEnabled() && side->isEnabled() && !top->isChecked());
    auto button=[&](const QString& text){for(auto* b:panel->findChildren<QPushButton*>())if(b->text()==text&&b->isVisible())return b;throw std::runtime_error("Missing visible button");};
    auto mouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton b,Qt::MouseButtons bs){const auto local=view->mapFromScene(point);QMouseEvent e{type,QPointF{local},QPointF{view->viewport()->mapToGlobal(local)},b,bs,Qt::NoModifier};QApplication::sendEvent(view->viewport(),&e);};
    auto click=[&](QPointF point){mouse(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);};
    auto key=[&](int k){QKeyEvent e{QEvent::KeyPress,k,Qt::NoModifier};QApplication::sendEvent(view,&e);};
    QString warning;
    auto expectWarning=[&]{warning.clear();QTimer::singleShot(0,[&]{auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());if(!box)qFatal("Expected fuselage warning");warning=box->text();box->accept();});};
    top->click();CHECK(top->isChecked()&&!side->isEnabled());CHECK(!window.projectModified());
    button("Line")->click();CHECK(editor.tool()==SketchTool::Line);CHECK(!window.projectModified());
    const std::vector<QPointF> corners{{200,150},{600,150},{600,250},{200,250}};
    for(int i=0;i<4;++i){click(corners[i]);click(corners[(i+1)%4]);}
    CHECK(closedSketchBoundary(editor.layers()[0]) && !panel->outlinesDefined());CHECK(window.projectModified());
    CHECK(view->sketchEditor().layers()[0].points==p.wing.layers[0].points);
    top->click();CHECK(!top->isChecked()&&side->isEnabled());side->click();CHECK(!top->isEnabled());
    button("Spline")->click();click({200,350});click({400,300});click({600,350});click({400,400});click({200,350});
    CHECK(editor.layers()[1].curves.size()==1 && panel->outlinesDefined());
    CHECK(tools->actions()[1]->isEnabled() && tools->actions()[0]->isChecked());
    CHECK(editor.tool()==SketchTool::Spline);button("Spline")->click();
    // Moving a point preserves a closed spline and affects only the selected view.
    const auto oldSide=editor.layers()[1].points.front();
    mouse(QEvent::MouseButtonPress,oldSide,Qt::LeftButton,Qt::LeftButton);
    mouse(QEvent::MouseMove,oldSide+QPointF{15,0},Qt::NoButton,Qt::LeftButton);
    mouse(QEvent::MouseButtonRelease,oldSide+QPointF{15,0},Qt::LeftButton,Qt::NoButton);
    CHECK(editor.layers()[1].points.front()!=oldSide && panel->outlinesDefined());
    CHECK(window.saveProjectFile(filename,error));CHECK(!window.projectModified());
    tools->actions()[1]->trigger();CHECK(tools->actions()[1]->isChecked()&&!editor.state().editing);CHECK(!window.projectModified());
    // One click on either Side boundary places a complete vertical section.
    auto& profiles=editor.stationEditor();CHECK(profiles.enabled());
    CHECK(window.findChild<QWidget*>("fuselageProfileStationsPanel")->isVisible());
    click({400,150});CHECK(profiles.lines().empty()); // Top View cannot receive stations.
    const auto sectionA=profiles.verticalSection(300,1);CHECK(sectionA);
    mouse(QEvent::MouseMove,sectionA->first.position,Qt::NoButton,Qt::NoButton);
    CHECK(profiles.hoverPoint());CHECK(!window.projectModified());
    click(sectionA->first.position);CHECK(profiles.lines().size()==1&&!profiles.state().first);
    const auto sectionB=profiles.verticalSection(500,1);CHECK(sectionB);
    click(sectionB->second.position);CHECK(profiles.lines().size()==2);
    for(const auto& line:profiles.lines()) {
      CHECK(line.alignment==LineAlignment::Vertical);
      CHECK(std::abs(line.first.position.x()-line.second.position.x())<1e-7);
      CHECK(line.first.position.y()<line.second.position.y());
    }
    // Existing endpoint enters move mode, never adds a duplicate; Escape drops it.
    click(profiles.lines()[0].first.position);key(Qt::Key_Escape);CHECK(profiles.lines().size()==2);
    click(profiles.lines()[0].first.position);
    const auto movedSection=profiles.verticalSection(350,1);CHECK(movedSection);
    mouse(QEvent::MouseMove,movedSection->first.position,Qt::NoButton,Qt::NoButton);key(Qt::Key_Escape);
    CHECK(std::abs(profiles.lines()[0].first.position.x()-350)<2);
    CHECK(window.saveProjectFile(filename,error));CHECK(!window.projectModified());
    CHECK(window.openProjectFile(filename,error));CHECK(profiles.enabled()&&profiles.lines().size()==2);
    CHECK(window.projectDocument().tool=="Profile Stations"&&!window.projectModified());
    const auto stationCapture=qEnvironmentVariable("FOAM_PROFILE_STATIONS_CAPTURE");
    if(!stationCapture.isEmpty()){view->fitAll();app.processEvents();CHECK(window.grab().save(stationCapture));}
    click((profiles.lines()[0].first.position+profiles.lines()[0].second.position)/2);
    CHECK(profiles.selectedLine()==0&&!window.projectModified());
    window.findChild<QTabWidget*>("viewportTabs")->setCurrentIndex(1);CHECK(!profiles.enabled());
    window.findChild<QTabWidget*>("viewportTabs")->setCurrentIndex(0);CHECK(profiles.enabled());
    click((profiles.lines()[0].first.position+profiles.lines()[0].second.position)/2);
    key(Qt::Key_Delete);CHECK(profiles.lines().size()==1&&window.projectModified());
    key(Qt::Key_Escape);CHECK(window.saveProjectFile(filename,error));
    auto legacy9=encodeProject(window.projectDocument());legacy9["version"]=9;legacy9.remove("fuselageStations");
    CHECK(decodeProject(legacy9).fuselageStations.lines.empty());
    auto badProfiles=encodeProject(window.projectDocument());auto profileData=badProfiles["fuselageStations"].toObject();
    auto profileLines=profileData["lines"].toArray();auto record=profileLines[0].toObject();
    auto bottom=record["bottom"].toObject();auto position=bottom["position"].toArray();position[0]=position[0].toDouble()+10;
    bottom["position"]=position;record["bottom"]=bottom;profileLines[0]=record;profileData["lines"]=profileLines;badProfiles["fuselageStations"]=profileData;
    rejected=false;try{decodeProject(badProfiles);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    tools->actions()[0]->trigger();CHECK(side->isChecked()&&editor.state().editing&&!profiles.enabled());
    const auto capture=qEnvironmentVariable("FOAM_FUSELAGE_CAPTURE");if(!capture.isEmpty()){view->fitAll();app.processEvents();CHECK(window.grab().save(capture));}
    // Delete the Side spline and verify named warning on exit, with Top retained.
    auto boundary=*closedSketchBoundary(editor.layers()[1]);click(boundary[boundary.size()/3]);key(Qt::Key_Delete);
    CHECK(editor.layers()[1].curves.empty()&&!tools->actions()[1]->isEnabled()&&profiles.lines().empty());
    expectWarning();workspaces->actions()[0]->trigger();CHECK(warning.contains("Side View")&&!warning.contains("Top View"));
    CHECK(!editor.state().editing && closedSketchBoundary(editor.layers()[0]));
    workspaces->actions()[2]->trigger();side->click();side->click(); // Release and reselect.
    button("Spline")->click();click({200,350});click({400,300});
    CHECK(editor.state().pending.size()==2);CHECK(window.saveProjectFile(filename,error));
    CHECK(window.openProjectFile(filename,error));CHECK(editor.state().pending.size()==2 && side->isChecked() && editor.state().editing);CHECK(!window.projectModified());
    key(Qt::Key_Escape);CHECK(editor.state().pending.empty()&&editor.layers()[1].curves.size()==1);
    expectWarning();workspaces->actions()[0]->trigger();CHECK(warning.contains("Side View"));
    CHECK(window.saveProjectFile(filename,error));
    window.findChild<QAction*>("projectNew")->trigger();CHECK(editor.layers().size()==2&&editor.layers()[0].curves.empty()&&editor.layers()[1].curves.empty());
    CHECK(window.openProjectFile(filename,error));CHECK(editor.layers()[0].curves.size()==4);
    // Invalid files cannot replace the current sketches.
    const auto beforeInvalid=encodeProject(window.projectDocument(),false);
    const auto invalidFile=dir.filePath("invalid.foam");
    {QFile file{invalidFile};CHECK(file.open(QIODevice::WriteOnly));file.write(QJsonDocument{bad}.toJson());}
    CHECK(!window.openProjectFile(invalidFile,error));CHECK(encodeProject(window.projectDocument(),false)==beforeInvalid);
    window.findChild<QAction*>("projectClose")->trigger();CHECK(editor.layers()[0].curves.empty()&&editor.layers()[1].curves.empty());
    CHECK(window.openProjectFile(filename,error));CHECK(editor.layers()[0].curves.size()==4);
    // Both invalid views are named on 3D entry, and editing stays locked there.
    workspaces->actions()[2]->trigger();
    auto empty=editor.state();empty.layers=std::vector<SketchLayer>(2);empty.pending.clear();editor.restoreState(empty);
    expectWarning();window.findChild<QTabWidget*>("viewportTabs")->setCurrentIndex(1);
    CHECK(warning.contains("Top View")&&warning.contains("Side View")&&!editor.state().editing);
    window.findChild<QTabWidget*>("viewportTabs")->setCurrentIndex(0);
    auto oneValid=editor.state();oneValid.layers[1]=closed;editor.restoreState(oneValid);
    expectWarning();workspaces->actions()[0]->trigger();CHECK(warning.contains("Top View")&&!warning.contains("Side View"));
    // Reference scale remapping includes both fuselage views, independently of Wing.
    PlanViewport remap;QImage image{100,100,QImage::Format_RGB32};image.fill(Qt::white);
    remap.setReferenceBackground({{image,QSizeF{200,200}}},false);auto state=remap.fuselageSketchEditor().state();state.layers={closed,closed};remap.fuselageSketchEditor().restoreState(state);
    remap.setReferenceBackground({{image,QSizeF{200,200}}},true);CHECK(remap.fuselageSketchEditor().layers()[1].points[1]==QPointF(200,0));
    // Same-curve upper/lower intersections and ambiguous concave sections.
    PlanViewport sections;auto geometry=sections.fuselageSketchEditor().state();
    geometry.layers[1]=closed;sections.fuselageSketchEditor().restoreState(geometry);
    auto& vertical=sections.fuselageSketchEditor().stationEditor();
    CHECK(vertical.verticalSection(50,1));CHECK(!vertical.verticalSection(150,1));
    StationState attached;attached.lines.push_back(*vertical.verticalSection(50,1));vertical.restoreState(attached);
    sections.fuselageSketchEditor().mapPoints([](QPointF point){return QPointF{point.x()*2,point.y()*3};});
    CHECK(vertical.lines().size()==1);
    CHECK(std::abs(vertical.lines()[0].first.position.x()-100)<1e-7);
    CHECK(std::abs(vertical.lines()[0].second.position.y()-240)<1e-7);
    geometry.layers[1]={{{0,0},{100,0},{100,20},{20,20},{20,60},{100,60},{100,80},{0,80}},
      {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,4}},
       {SketchTool::Line,{4,5}},{SketchTool::Line,{5,6}},{SketchTool::Line,{6,7}},{SketchTool::Line,{7,0}}}};
    sections.fuselageSketchEditor().restoreState(geometry);CHECK(!vertical.verticalSection(50,1));
    std::cout<<"Fuselage outlines, interaction, validation, migration and lifecycle passed\n";
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

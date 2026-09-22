#pragma once
#include "geometry/FuselageHoles.h"
#include "gui/FuselageCutPanel.h"
#include "gui/SketchPaths.h"
#include <QComboBox>
#include <QMessageBox>
inline void holeChecks(QApplication& app,const QString& directory) {
  using namespace designrc;
  const auto outer=BRepPrimAPI_MakeBox{gp_Pnt{0,-20,-15},100,40,30}.Shape();
  const auto cavity=BRepPrimAPI_MakeBox{gp_Pnt{2,-17,-12},96,34,24}.Shape();
  const auto body=BRepAlgoAPI_Cut{outer,cavity}.Shape();
  const std::vector<SketchLayer> outlines{rectangle(0,-20,100,40),rectangle(0,-15,100,30)};
  const std::array<geometry::FuselageCutProjection,2> projections{};
  for(int wall=0;wall<4;++wall) {
    std::cout<<"Hole wall "<<wall<<std::endl;
    std::vector<SketchLayer> holes(4);holes[wall]=rectangle(40,2,10,6);
    const auto result=geometry::cutFuselageHoles(body,cavity,holes,outlines,projections);
    CHECK(count(result)==1);CHECK(std::abs(volume(body)-volume(result)-180)<1e-5);
    if(wall<2){const double z=wall==0?13.5:-13.5;CHECK(!inside(result,45,5,z));CHECK(inside(result,45,5,-z));CHECK(inside(result,45,-5,z));}
    else {const double y=wall==2?-18.5:18.5;CHECK(!inside(result,45,y,-5));CHECK(inside(result,45,-y,-5));CHECK(inside(result,45,y,5));}
  }
  std::vector<SketchLayer> holes(4);holes[0]=rectangle(30,-4,10,8);
  auto second=rectangle(60,-4,10,8);for(auto c:second.curves){for(auto& i:c.points)i+=holes[0].points.size();holes[0].curves.push_back(c);}holes[0].points.insert(holes[0].points.end(),second.points.begin(),second.points.end());
  const auto result=geometry::cutFuselageHoles(body,cavity,holes,outlines,projections);
  CHECK(std::abs(volume(body)-volume(result)-480)<1e-5);
  for(int bad=0;bad<3;++bad) {
    holes[0]=bad==0?rectangle(-1,-4,10,8):bad==1?rectangle(.5,-4,3,8):rectangle(40,-4,10,8);
    if(bad==2)holes[0].curves.pop_back();
    bool rejected=false;try{geometry::cutFuselageHoles(body,cavity,holes,outlines,projections);}catch(const std::exception&){rejected=true;}CHECK(rejected);
  }
  holes[0]={{{40,0},{45,-4},{50,0},{45,4}},{{SketchTool::Spline,{0,1,2,3,0}}}};
  const auto spline=geometry::cutFuselageHoles(body,cavity,holes,outlines,projections);
  CHECK(!inside(spline,45,0,13.5)&&inside(spline,45,0,-13.5));
  std::stop_source stop;stop.request_stop();bool cancelled=false;try{geometry::cutFuselageHoles(body,cavity,holes,outlines,projections,{stop.get_token()});}catch(const geometry::ProcessingCancelled&){cancelled=true;}CHECK(cancelled);
  std::cout<<"Hole geometry complete"<<std::endl;
  // Round-trip all wall layers and verify old projects receive empty holes.
  ProjectDocument p;p.fuselageHoles.layers=holes;
  auto encoded=encodeProject(p);CHECK(encoded["version"]==28);
  CHECK(decodeProject(encoded).fuselageHoles.layers[0].curves.size()==1);
  auto legacy=encoded;legacy["version"]=24;legacy.remove("fuselageHoles");CHECK(decodeProject(legacy).fuselageHoles.layers.size()==4&&decodeProject(legacy).fuselageHoles.layers[0].curves.empty());
  std::cout<<"Hole persistence complete"<<std::endl;
  // Real panel controls: whole-path deletion, shared layout, containment and leave warning.
  PlanViewport view;view.show();SketchEditor outline{&view};SketchState os;os.layers=outlines;outline.restoreState(os);
  SketchEditor editor{&view};editor.setLayerCount(4);FuselageCutPanel panel{editor,nullptr,true,&outline};panel.show();panel.setActive(true);
  auto state=editor.state();state.layers[0]=rectangle(30,-4,10,8);editor.restoreState(state);emit editor.changed();
  panel.findChild<QComboBox*>("fuselageHolesPaths")->setCurrentIndex(0);panel.findChild<QPushButton*>("fuselageHolesDelete")->click();CHECK(editor.layers()[0].curves.empty());
  panel.findChild<QPushButton*>("fuselageHolesRight")->click();CHECK(editor.activeLayer()==3);
  int warnings=0;QTimer dismiss;QObject::connect(&dismiss,&QTimer::timeout,[&]{if(auto* m=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){++warnings;m->accept();}});dismiss.start(10);
  state=editor.state();state.layers[3]=rectangle(-1,-4,10,8);editor.restoreState(state);emit editor.changed();CHECK(editor.layers()[3].curves.empty());CHECK(warnings==1);
  state=editor.state();state.layers[3]=rectangle(30,-4,10,8);state.layers[3].curves.pop_back();editor.restoreState(state);emit editor.changed();panel.setActive(false);CHECK(warnings==2);dismiss.stop();
  SketchEditor cuts{&view};cuts.setLayerCount(2);FuselageCutPanel cutPanel{cuts};cutPanel.setActive(true);
  state=cuts.state();state.layers[0]={{{10,0},{20,0},{30,0},{40,0}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{2,3}}}};cuts.restoreState(state);emit cuts.changed();
  auto* list=cutPanel.findChild<QComboBox*>("fuselageCutPaths");CHECK(list->count()==2);list->setCurrentIndex(0);cutPanel.findChild<QPushButton*>("fuselageCutDelete")->click();CHECK(cuts.layers()[0].curves.size()==1&&cuts.layers()[0].points[0].x()==30);
  // Full fuselage generation with a Top hole, without invoking Wing generation.
  p=ProjectDocument{};p.reference.wingspanMm=1000;p.wingspanText="1000 mm";p.reference.fuselageLengthMm=400;p.fuselageText="400 mm";
  p.wing.layers[0]=rectangle(10,10,90,70);p.wing.layers[0].curves.pop_back();
  p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},{{0,0,1,{100,10}},{0,2,0,{100,80}},LineAlignment::Vertical,0}};
  p.airfoils.entries.push_back({"NACA",domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
  p.fuselage.layers={rectangle(200,150,400,100),rectangle(200,350,400,80)};
  p.fuselageStations.lines={{{1,0,.25,{300,350}},{1,2,.75,{300,430}},LineAlignment::Vertical},{{1,0,.75,{500,350}},{1,2,.25,{500,430}},LineAlignment::Vertical}};
  p.fuselageProfiles.layers={rectangle(680,140,100,80),rectangle(680,300,100,80)};
  p.fuselageStations.lines[0].profile=0;p.fuselageStations.lines[1].profile=1;
  for(auto& s:p.fuselageStations.lines)s.thicknessMm=5;
  p.fuselageThickening=true;p.fuselageHoles.layers[0]=rectangle(380,190,20,20);p.workspace=2;p.tool="Holes";
  QString error;const auto file=directory+"/holes.foam";CHECK(writeProject(file,p,error));
  MainWindow window;window.show();CHECK(window.openProjectFile(file,error));app.processEvents();
  auto* tabs=window.findChild<QTabWidget*>("viewportTabs");CHECK(tabs->currentIndex()==0);
  auto* toolbar=window.findChild<QToolBar*>("componentToolBar");CHECK(!toolbar->actions().empty());CHECK(toolbar->actions().back()->text()=="Holes");CHECK(window.findChild<QPushButton*>("fuselageHolesAdd")->isVisible());
  CHECK(window.projectDocument().fuselageHoles.layers[0].curves.size()==4);
  tabs->setCurrentIndex(1);waitForModel(window);CHECK(tabs->widget(1)->property("fuselageModelReady").toBool());CHECK(tabs->widget(1)->property("wingModelRevision").toInt()==0);
  tabs->setCurrentIndex(0);app.processEvents();CHECK(window.grab().save(directory+"/holes-panel.png"));
  CHECK(window.saveProjectFile(file,error));CHECK(window.openProjectFile(file,error));CHECK(tabs->currentIndex()==0&&window.projectDocument().fuselageHoles.layers[0].curves.size()==4);
  std::cout<<"Holes: four walls, opposite-wall preservation, multiple loops, spline, containment, cancellation, UI, persistence and fuselage generation passed."<<std::endl;
}

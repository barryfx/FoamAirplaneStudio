#include "TestCheck.h"
#include "geometry/Fiberglass.h"
#include "gui/FiberglassPanel.h"
#include "gui/SketchPaths.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QJsonArray>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <iostream>
#include <numbers>
using namespace designrc;
using namespace designrc::gui;
static bool closeEnough(double a,double b,double tolerance=1e-5){return std::abs(a-b)<tolerance;}
static SketchLayer loop(std::vector<QPointF> points,bool closed=true) {
  SketchLayer layer;layer.points=std::move(points);
  for(std::size_t i=1;i<layer.points.size();++i)layer.curves.push_back({SketchTool::Line,{i-1,i}});
  if(closed)layer.curves.push_back({SketchTool::Line,{layer.points.size()-1,0}});return layer;
}
static void geometryChecks() {
  std::cout<<"Box surfaces"<<std::endl;
  const auto box=BRepPrimAPI_MakeBox{100,100,10}.Shape();
  QPainterPath outline;outline.addRect(0,0,100,100);
  auto region=geometry::fiberglassRegion(loop({{20,-20},{40,-20},{40,120},{20,120}}),outline);
  geometry::FiberglassProjection projection{[](const gp_Pnt& p){return QPointF{p.X(),p.Y()};},[](const gp_Pnt&){return gp_Dir{0,0,1};}};
  auto top=geometry::measureFiberglass(box,region,projection,false);
  TEST_CHECK(closeEnough(top.areaMm2,2000));TEST_CHECK(closeEnough(top.centroidMm.x(),30));TEST_CHECK(closeEnough(top.centroidMm.y(),10));
  auto wrap=geometry::measureFiberglass(box,region,projection,true);
  TEST_CHECK(closeEnough(wrap.areaMm2,4400));TEST_CHECK(closeEnough(wrap.centroidMm.x(),30));TEST_CHECK(closeEnough(wrap.centroidMm.y(),5));
  projection.outward=[](const gp_Pnt&){return gp_Dir{0,0,-1};};
  auto bottom=geometry::measureFiberglass(box,region,projection,false);TEST_CHECK(closeEnough(bottom.areaMm2,2000));TEST_CHECK(closeEnough(bottom.centroidMm.y(),0));
  const auto hollow=BRepAlgoAPI_Cut{box,BRepPrimAPI_MakeBox{gp_Pnt{1,1,1},98,98,8}.Shape()}.Shape();
  std::cout<<"Hollow shell"<<std::endl;
  auto hollowWrap=geometry::measureFiberglass(hollow,region,projection,true);TEST_CHECK(closeEnough(hollowWrap.areaMm2,4400));
  auto open=geometry::fiberglassRegion(loop({{-10,-10},{-10,50},{110,50},{110,-10}},false),outline);
  TEST_CHECK(open.contains({50,25}));TEST_CHECK(!open.contains({50,75}));
  bool rejected=false;try{geometry::fiberglassRegion(loop({{-10,50},{110,50}},false),outline);}catch(const std::exception&){rejected=true;}TEST_CHECK(rejected);
  rejected=false;try{geometry::fiberglassRegion(loop({{10,10},{20,30},{50,10}},false),outline);}catch(const std::exception&){rejected=true;}TEST_CHECK(rejected);
  SketchLayer circle;circle.points={{50,50},{60,50}};circle.curves={{SketchTool::Circle,{0,1}}};
  std::cout<<"Circle and curved surface"<<std::endl;
  const auto circleRegion=geometry::fiberglassRegion(circle,outline);auto disk=geometry::measureFiberglass(box,circleRegion,projection,false);
  TEST_CHECK(closeEnough(disk.areaMm2,std::numbers::pi*100,.1));
  const auto cylinder=BRepPrimAPI_MakeCylinder{gp_Ax2{gp_Pnt{0,50,0},gp_Dir{1,0,0}},10,100}.Shape();
  auto curved=geometry::measureFiberglass(cylinder,region,projection,true);
  TEST_CHECK(closeEnough(curved.areaMm2,2*std::numbers::pi*10*20,10));TEST_CHECK(closeEnough(curved.centroidMm.x(),30,.05));
  FoamMassProperties mass;wrap.clothGrams=wrap.areaMm2*100*1e-6;wrap.resinVolumeMm3=wrap.areaMm2*.1;mass.fiberglass.push_back(wrap);
  WeightBalanceState state;auto balance=calculateBalance(state,mass);TEST_CHECK(closeEnough(balance.grams,.44+.5192));TEST_CHECK(closeEnough(balance.centerMm.x(),30));
  mass.fiberglass.push_back(wrap);TEST_CHECK(closeEnough(calculateBalance(state,mass).grams,2*balance.grams));
  state.resinDensityKgM3=1000;TEST_CHECK(closeEnough(calculateBalance(state,mass).grams,1.76));
  TEST_CHECK(defaultResinThickness(100)>defaultResinThickness(50));
}
static void persistenceChecks() {
  std::cout<<"Persistence"<<std::endl;
  ProjectDocument p;p.fiberglass[1].sketch.layers[0]=loop({{-10,-10},{-10,50},{110,50},{110,-10}},false);
  auto& patch=p.fiberglass[1].patches[0];patch.name="Side reinforcement";patch.wrap=false;patch.side=CoverSide::Left;patch.clothGm2=80;patch.imperialCloth=true;patch.automaticResin=false;patch.resinThicknessMm=.08;p.weightBalance.resinDensityKgM3=1200;
  const auto json=encodeProject(p);TEST_CHECK(json["version"]==32);auto decoded=decodeProject(json);TEST_CHECK(encodeProject(decoded)==json);
  auto legacy=json;legacy["version"]=31;legacy.remove("fiberglass");auto old=decodeProject(legacy);TEST_CHECK(old.fiberglass[1].sketch.layers[0].curves.empty());TEST_CHECK(old.weightBalance.resinDensityKgM3==1180);
  auto bad=json;auto components=bad["fiberglass"].toArray();auto component=components[1].toObject();component["patches"]=QJsonArray{};components[1]=component;bad["fiberglass"]=components;
  bool rejected=false;try{decodeProject(bad);}catch(const std::exception&){rejected=true;}TEST_CHECK(rejected);
  StatisticsBalance cached;cached.sourceKey=QByteArray(64,'a');cached.materials.fiberglass.push_back({"Patch",2000,.1,100,{20,30}});p.statistics.balance=cached;
  TEST_CHECK(decodeProject(encodeProject(p)).statistics.balance->materials.fiberglass[0].areaMm2==2000);
}
static void placementChecks() {
  std::cout<<"Component projection and placement"<<std::endl;
  ProjectDocument p;p.reference.wingspanMm=200;
  p.wing.layers[0]=loop({{0,0},{100,0},{100,20},{0,20}},false);
  ConstrainedLine station;station.first.position={0,0};station.second.position={0,20};p.stations.lines={station};
  auto& patch=p.fiberglass[0];patch.sketch.layers[0]=loop({{-10,-10},{20,-10},{20,30},{-10,30}});
  geometry::AssemblyParts originals;BRep_Builder builder;TopoDS_Compound wing;builder.MakeCompound(wing);
  builder.Add(wing,BRepPrimAPI_MakeBox{gp_Pnt{0,0,-5},20,100,10}.Shape());builder.Add(wing,BRepPrimAPI_MakeBox{gp_Pnt{0,-100,-5},20,100,10}.Shape());originals.wing=wing;
  p.assembly.offsets[0]={100,40};p.assembly.rotationDegrees[0]=30;
  auto results=geometry::fiberglassMassProperties(p,originals,geometry::placeAssembly(originals,p.assembly));
  TEST_CHECK(results.size()==1);TEST_CHECK(closeEnough(results[0].areaMm2,2400,.01));
  const auto expected=gp_Pnt{10,0,0}.Transformed(geometry::assemblyComponentPlacement(originals,p.assembly,0));
  TEST_CHECK(closeEnough(results[0].centroidMm.x(),expected.X(),.01));TEST_CHECK(closeEnough(results[0].centroidMm.y(),expected.Z(),.01));
  // Dihedral uses the unfolded reference footprint on both wing halves.
  p.assembly={};p.dihedralDegrees={20};gp_Trsf tilt;tilt.SetRotation(gp_Ax1{gp_Pnt{},gp_Dir{1,0,0}},20*std::numbers::pi/180);
  auto half=BRepBuilderAPI_Transform{BRepPrimAPI_MakeBox{gp_Pnt{0,0,-5},20,100,10}.Shape(),tilt,true}.Shape();
  gp_Trsf mirror;mirror.SetMirror(gp_Ax2{gp_Pnt{},gp_Dir{0,1,0}});builder.MakeCompound(wing);builder.Add(wing,half);builder.Add(wing,BRepBuilderAPI_Transform{half,mirror,true}.Shape());originals.wing=wing;
  patch.patches[0].wrap=false;
  patch.sketch.layers[0]=loop({{10,-10},{30,-10},{30,30},{10,30}});
  results=geometry::fiberglassMassProperties(p,originals,geometry::placeAssembly(originals,p.assembly));
  std::cout<<"Dihedral area "<<results[0].areaMm2<<", center "<<results[0].centroidMm.y()<<std::endl;
  TEST_CHECK(closeEnough(results[0].areaMm2,800,.1));
  TEST_CHECK(closeEnough(results[0].centroidMm.y(),20*std::sin(20*std::numbers::pi/180)+5*std::cos(20*std::numbers::pi/180),.1));
  // Horizontal and vertical stabilizer roots can have unrelated scene origins.
  p.fiberglass[0]={};p.dihedralDegrees={0};
  for(int i=0;i<2;++i){p.stabilizerOutlines[i].layers[0]=loop({{200,50},{300,50},{300,70},{200,70}},false);p.stabilizerOutlines[i].layers[0].leadingEdge=0;p.fiberglass[i+2].sketch.layers[0]=loop({{210,40},{230,40},{230,80},{210,80}});p.fiberglass[i+2].patches[0].wrap=false;}
  builder.MakeCompound(wing);builder.Add(wing,BRepPrimAPI_MakeBox{gp_Pnt{0,0,-5},20,100,10}.Shape());builder.Add(wing,BRepPrimAPI_MakeBox{gp_Pnt{0,-100,-5},20,100,10}.Shape());originals.horizontal=wing;
  originals.vertical=BRepPrimAPI_MakeBox{gp_Pnt{0,-5,0},20,10,100}.Shape();
  results=geometry::fiberglassMassProperties(p,originals,geometry::placeAssembly(originals,p.assembly));TEST_CHECK(results.size()==2);TEST_CHECK(closeEnough(results[0].areaMm2,800,.01));TEST_CHECK(closeEnough(results[1].areaMm2,400,.01));TEST_CHECK(closeEnough(results[1].centroidMm.y(),20,.01));
  p.fiberglass[2]={};p.fiberglass[3]={};
  p.fuselage.layers={loop({{0,200},{100,200},{100,240},{0,240}}),loop({{50,80},{250,80},{250,100},{50,100}})};
  originals.fuselage=BRepPrimAPI_MakeBox{gp_Pnt{0,-40,-10},200,80,20}.Shape();
  auto& fuselage=p.fiberglass[1];
  for(int side=0;side<4;++side) {
    fuselage.sketch.layers[0]=side<2?loop({{25,180},{35,180},{35,260},{25,260}}):loop({{100,70},{120,70},{120,110},{100,110}});
    fuselage.patches[0].side=static_cast<CoverSide>(side);fuselage.patches[0].wrap=false;
    results=geometry::fiberglassMassProperties(p,originals,geometry::placeAssembly(originals,p.assembly));
    TEST_CHECK(closeEnough(results[0].areaMm2,side<2?1600:400,.01));TEST_CHECK(closeEnough(results[0].centroidMm.x(),60,.01));
    TEST_CHECK(closeEnough(results[0].centroidMm.y(),side==0?10:side==1?-10:0,.01));
    fuselage.patches[0].wrap=true;results=geometry::fiberglassMassProperties(p,originals,geometry::placeAssembly(originals,p.assembly));TEST_CHECK(closeEnough(results[0].areaMm2,4000,.01));
  }
}
static void guiChecks(QApplication& app,const QString& directory) {
  std::cout<<"Editor GUI"<<std::endl;
  QWidget window;auto* layout=new QHBoxLayout{&window};auto* view=new PlanViewport{&window};auto* panel=new FiberglassPanel{view->fiberglassEditor(1),1,&window};
  panel->setFixedWidth(360);layout->addWidget(panel);layout->addWidget(view,1);window.resize(1300,800);window.show();panel->configure(ProjectUnits::Inches);panel->setActive(true);app.processEvents();
  auto& editor=view->fiberglassEditor(1);
  const auto click=[&](QPointF point){const auto local=view->mapFromScene(point);for(const auto type:{QEvent::MouseButtonPress,QEvent::MouseButtonRelease}){QMouseEvent event{type,QPointF{local},QPointF{view->viewport()->mapToGlobal(local)},Qt::LeftButton,type==QEvent::MouseButtonPress?Qt::LeftButton:Qt::NoButton,Qt::NoModifier};QApplication::sendEvent(view->viewport(),&event);}};
  panel->findChild<QPushButton*>("fiberglassAdd")->click();click({100,100});click({300,100});TEST_CHECK(editor.layers()[0].curves.size()==1);
  auto* name=panel->findChild<QLineEdit*>("fiberglassName");name->setText("Nose reinforcement");QMetaObject::invokeMethod(name,"editingFinished");TEST_CHECK(panel->state().patches[0].name=="Nose reinforcement");
  panel->findChild<QRadioButton*>("fiberglassOneSided")->click();panel->findChild<QComboBox*>("fiberglassSide")->setCurrentIndex(2);TEST_CHECK(!panel->state().patches[0].wrap);TEST_CHECK(panel->state().patches[0].side==CoverSide::Left);
  auto* units=panel->findChild<QComboBox*>("fiberglassClothUnits");TEST_CHECK(units->currentIndex()==1);units->setCurrentIndex(0);panel->findChild<QDoubleSpinBox*>("fiberglassClothWeight")->setValue(100);units->setCurrentIndex(1);TEST_CHECK(closeEnough(panel->state().patches[0].clothGm2,100));
  panel->findChild<QCheckBox*>("fiberglassAutomaticResin")->setChecked(false);panel->findChild<QDoubleSpinBox*>("fiberglassResinThickness")->setValue(.004);TEST_CHECK(closeEnough(panel->state().patches[0].resinThicknessMm,.1016));
  panel->findChild<QPushButton*>("fiberglassAdd")->click();TEST_CHECK(editor.layers().size()==2);
  for(auto* button:panel->findChildren<QPushButton*>())if(button->text()=="Circle")button->click();click({400,250});click({450,250});TEST_CHECK(editor.layers()[1].curves[0].type==SketchTool::Circle);
  const auto saved=panel->state();panel->restore(saved);TEST_CHECK(panel->state().patches[0].name=="Nose reinforcement");
  panel->setActive(false);TEST_CHECK(!editor.state().editing);panel->setActive(true);app.processEvents();
  if(!directory.isEmpty())TEST_CHECK(window.grab().save(directory+"/fiberglass-editor.png"));
  panel->findChild<QPushButton*>("fiberglassDelete")->click();TEST_CHECK(editor.layers().size()==1);TEST_CHECK(panel->state().patches[0].name=="Nose reinforcement");
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};app.setStyle("Fusion");
  try{geometryChecks();placementChecks();persistenceChecks();guiChecks(app,argc>1?QString::fromLocal8Bit(argv[1]):QString{});std::cout<<"Fiberglass area, mass, persistence and GUI checks passed\n";return 0;}
  catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

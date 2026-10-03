#include "TestCheck.h"
#include "geometry/FuselageStiffeners.h"
#include "geometry/FuselageSolidBuilder.h"
#include "geometry/FuselageSymmetry.h"
#include "geometry/WeightBalance.h"
#include "gui/StiffenerPanel.h"
#include "gui/ProjectDocument.h"
#include "gui/OcctViewport.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <QApplication>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QScreen>
#include <iostream>
#include <algorithm>
#include <numbers>
using namespace designrc;using namespace designrc::gui;
static bool closeEnough(double a,double b,double epsilon=.01){return std::abs(a-b)<epsilon;}
static double volume(const TopoDS_Shape& s){GProp_GProps p;BRepGProp::VolumeProperties(s,p,1e-7);return p.Mass();}
int main(int argc,char** argv) {
  QApplication app{argc,argv};app.setStyle("Fusion");
  try {
    std::vector<geometry::FuselageWallSection> sections{{0,5,{{-10,-10},{10,-10},{10,10},{-10,10}}},{200,5,{{-10,-10},{10,-10},{10,10},{-10,10}}}};
    BRep_Builder builder;TopoDS_Compound body;builder.MakeCompound(body);
    builder.Add(body,BRepPrimAPI_MakeBox{gp_Pnt{0,-10,-10},200,10,20}.Shape());builder.Add(body,BRepPrimAPI_MakeBox{gp_Pnt{0,0,-10},200,10,20}.Shape());
    StiffenerState s;s.count=1;s.startPercent=20;s.stopPercent=80;
    std::vector<geometry::SparMaterial> stock;const auto strip=geometry::cutFuselageStiffeners(body,sections,200,s,stock);
    TEST_CHECK(stock.size()==2&&BRepCheck_Analyzer{strip}.IsValid());TEST_CHECK(closeEnough(volume(body)-volume(strip),2*120*3));
    for(const auto& material:stock){TEST_CHECK(closeEnough(material.volumeMm3,360));TEST_CHECK(closeEnough(material.center.X(),100));TEST_CHECK(closeEnough(material.center.Z(),0));TEST_CHECK(material.fuselage);}
    s.count=3;stock.clear();const auto multiple=geometry::cutFuselageStiffeners(body,sections,200,s,stock);
    TEST_CHECK(stock.size()==6&&closeEnough(volume(body)-volume(multiple),6*360));
    TEST_CHECK(closeEnough(stock[0].center.Z(),-5)&&closeEnough(stock[2].center.Z(),0)&&closeEnough(stock[4].center.Z(),5));
    const auto right=BRepPrimAPI_MakeBox{gp_Pnt{0,0,-10},200,10,20}.Shape();
    std::vector<geometry::SparMaterial> rightStock;
    const auto groovedRight=geometry::cutFuselageStiffeners(right,sections,200,s,rightStock,{},true);
    TEST_CHECK(rightStock.size()==3&&closeEnough(volume(right)-volume(groovedRight),3*360));
    const auto mirroredGrooves=geometry::fuselagePair(groovedRight);
    TEST_CHECK(BRepCheck_Analyzer{mirroredGrooves}.IsValid()&&closeEnough(volume(mirroredGrooves),volume(multiple)));
    geometry::AssemblyParts parts;parts.fuselage=multiple;parts.sparMaterials=stock;AssemblyState placement;placement.offsets[0]={25,30};placement.rotationDegrees[0]=20;
    const auto placed=geometry::placeAssembly(parts,placement);TEST_CHECK(placed.sparMaterials[0].center.Distance(stock[0].center)<1e-8);
    const auto mass=geometry::foamMassProperties(placed);TEST_CHECK(closeEnough(mass.carbonFiberVolumeMm3,2160));TEST_CHECK(closeEnough(mass.carbonFiberCentroidMm.x(),100));
    s.count=1;s.shape=SparShape::Round;s.diameterMm=2;stock.clear();const auto round=geometry::cutFuselageStiffeners(body,sections,200,s,stock);
    TEST_CHECK(stock.size()==2&&closeEnough(stock[0].volumeMm3,120*std::numbers::pi));TEST_CHECK(closeEnough(volume(body)-volume(round),120*std::numbers::pi));
    const auto cylinder=BRepPrimAPI_MakeCylinder{gp_Ax2{gp_Pnt{},gp_Dir{1,0,0}},10,200}.Shape();
    auto curvedSections=sections;for(auto& section:curvedSections){section.perimeter.clear();for(int i=0;i<64;++i){double angle=i*2*std::numbers::pi/64;section.perimeter.push_back({10*std::cos(angle),10*std::sin(angle)});}}
    stock.clear();const auto curved=geometry::cutFuselageStiffeners(cylinder,curvedSections,200,s,stock);TEST_CHECK(BRepCheck_Analyzer{curved}.IsValid()&&volume(curved)<volume(cylinder));
    auto invalid=s;invalid.diameterMm=12;bool rejected=false;try{geometry::cutFuselageStiffeners(body,sections,200,invalid,stock);}catch(const std::exception&){rejected=true;}TEST_CHECK(rejected);
    invalid=s;invalid.stopPercent=invalid.startPercent;rejected=false;try{validateStiffeners(invalid);}catch(const std::exception&){rejected=true;}TEST_CHECK(rejected);
    auto disabled=s;disabled.count=0;stock.clear();TEST_CHECK(geometry::cutFuselageStiffeners(body,sections,200,disabled,stock).IsSame(body)&&stock.empty());
    const auto rectangle=[](double w,double h){SketchLayer layer{{{0,0},{w,0},{w,h},{0,h}}, {}};for(std::size_t i=0;i<4;++i)layer.curves.push_back({SketchTool::Line,{i,(i+1)%4}});return layer;};
    geometry::FuselageSolidInput input{{rectangle(200,40),rectangle(200,30)}, {},{rectangle(40,30)},200,true};
    ConstrainedLine first,last;first.first.position={50,0};first.profile=0;first.thicknessMm=5;last=first;last.first.position={150,0};input.stations={first,last};
    std::cout<<"Generating hollow fuselage with mirrored strip grooves..."<<std::endl;
    input.stiffeners=s;input.stiffeners.shape=SparShape::Strip;input.stiffeners.count=2;
    std::vector<std::string> stages;
    const auto generatedStrip=geometry::buildFuselageModel(input,[&](const char* stage){stages.emplace_back(stage);});
    auto grooveStage=std::find(stages.begin(),stages.end(),"Fuselage: cutting carbon fiber stiffener grooves...");
    auto mirrorStage=std::find(stages.begin(),stages.end(),"Fuselage: reflecting the completed right half as a separate left part...");
    TEST_CHECK(grooveStage!=stages.end()&&mirrorStage!=stages.end()&&grooveStage<mirrorStage);
    TEST_CHECK(generatedStrip.stiffeners.size()==4&&BRepCheck_Analyzer{generatedStrip.body}.IsValid());
    for(const auto& material:generatedStrip.stiffeners)TEST_CHECK(closeEnough(material.volumeMm3,360));
    for(int i=0;i<2;++i) {
      const auto& r=generatedStrip.stiffeners[i];const auto& l=generatedStrip.stiffeners[i+2];
      TEST_CHECK(r.volumeMm3==l.volumeMm3&&r.center.X()==l.center.X()&&r.center.Z()==l.center.Z()&&r.center.Y()==-l.center.Y());
      TEST_CHECK(r.name.ends_with(" / Right")&&l.name.ends_with(" / Left"));
    }
    std::cout<<"Generating hollow fuselage with mirrored round grooves..."<<std::endl;
    input.stiffeners=s;const auto generatedRound=geometry::buildFuselageModel(input);
    TEST_CHECK(generatedRound.stiffeners.size()==2&&BRepCheck_Analyzer{generatedRound.body}.IsValid());
    // A gently curved boom exercises non-collinear routes and smooth tool lofts.
    input.outlines[1]={{{0,0},{100,-5},{200,0},{200,30},{100,25},{0,30}},
      {{SketchTool::Spline,{0,1,2}},{SketchTool::Line,{2,3}},{SketchTool::Spline,{3,4,5}},{SketchTool::Line,{5,0}}}};
    std::cout<<"Generating curved hollow boom with round grooves..."<<std::endl;
    const auto curvedBoom=geometry::buildFuselageModel(input);
    TEST_CHECK(curvedBoom.stiffeners.size()==2&&BRepCheck_Analyzer{curvedBoom.body}.IsValid());
    ProjectDocument p;p.stiffeners=s;auto json=encodeProject(p);TEST_CHECK(json["version"]==33);TEST_CHECK(encodeProject(decodeProject(json))==json);
    auto legacy=json;legacy["version"]=32;legacy.remove("stiffeners");TEST_CHECK(decodeProject(legacy).stiffeners.count==0);
    auto bad=json;auto data=bad["stiffeners"].toObject();data["stopPercent"]=10;bad["stiffeners"]=data;rejected=false;try{decodeProject(bad);}catch(const std::exception&){rejected=true;}TEST_CHECK(rejected);
    QWidget window;auto* layout=new QHBoxLayout{&window};auto* panel=new StiffenerPanel{&window};auto* view=new OcctViewport{&window};layout->addWidget(panel);layout->addWidget(view,1);panel->setFixedWidth(330);window.resize(1200,750);window.show();app.processEvents();
    int edits=0;panel->changed=[&]{++edits;};panel->setUnits(ProjectUnits::Inches);TEST_CHECK(edits==0);panel->findChild<QSpinBox*>("stiffenerCount")->setValue(3);TEST_CHECK(panel->state().count==3);
    auto* width=panel->findChild<QLineEdit*>("stiffenerWidth");width->setText(".125 in");width->setModified(true);QMetaObject::invokeMethod(width,"editingFinished");TEST_CHECK(closeEnough(panel->state().widthMm,3.175));
    width->setText("-2");width->setModified(true);QMetaObject::invokeMethod(width,"editingFinished");TEST_CHECK(closeEnough(panel->state().widthMm,3.175));
    const auto saved=panel->state();panel->setUnits(ProjectUnits::Millimeters);TEST_CHECK(panel->state().widthMm==saved.widthMm);
    panel->findChild<QComboBox*>("stiffenerShape")->setCurrentIndex(1);TEST_CHECK(panel->findChild<QLineEdit*>("stiffenerDiameter")->isVisible()&&!width->isVisible());
    panel->restore(saved);TEST_CHECK(width->isVisible());view->displayShape(multiple);view->setCameraView(CameraView::Reset);view->fitAll();app.processEvents();
    auto displayed=s;displayed.shape=SparShape::Strip;displayed.count=3;panel->restore(displayed);app.processEvents();
    if(argc>1)TEST_CHECK(window.screen()->grabWindow(window.winId()).save(QString::fromLocal8Bit(argv[1])+"/stiffeners.png"));
    std::cout<<"Stiffener geometry, full fuselage generation, carbon mass, placement, persistence and GUI checks passed\n";return 0;
  } catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

#include "gui/MainWindow.h"
#include "gui/ReferencePanel.h"
#include "gui/WeightBalancePanel.h"
#include "geometry/WeightBalance.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <iostream>
#include <cmath>
#include <stdexcept>
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__));}while(false)
using namespace designrc;
using namespace designrc::gui;
static bool closeEnough(double a,double b,double tolerance=1e-6){return std::abs(a-b)<tolerance;}
static SketchLayer rectangle(double x,double y,double w,double h) {
  return {{{x,y},{x+w,y},{x+w,y+h},{x,y+h}},{{SketchTool::Line,{0,1}},
    {SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};
}
static ProjectDocument fixture() {
  ProjectDocument p;p.reference.wingspanMm=1000;p.reference.fuselageLengthMm=500;
  p.wingspanText="1000 mm";p.fuselageText="500 mm";
  p.wing.layers[0]=rectangle(10,10,100,70);p.wing.layers[0].curves.pop_back();
  p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},
      {{0,0,1,{110,10}},{0,2,0,{110,80}},LineAlignment::Vertical,0}};
  p.airfoils.entries.push_back({"NACA",domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
  p.fuselage.layers={rectangle(200,100,400,50),rectangle(200,230,400,100)};
  return p;
}
static TopoDS_Shape box(double x,double dx,double dy,double dz) {
  return BRepPrimAPI_MakeBox{gp_Pnt{x,0,0},dx,dy,dz}.Shape();
}
static geometry::AssemblyParts parts() {
  geometry::AssemblyParts p;
  p.fuselage=box(0,100,100,100); // 1e6 mm3; center X=50.
  p.wing=box(100,100,100,100); // 1e6 mm3; center X=150.
  p.inserts.push_back({"Former 1",box(200,10,100,100),{}}); // 1e5 mm3; center X=205.
  p.inserts.push_back({"Servo Tray",box(300,10,100,100),{}}); // 1e5 mm3; center X=305.
  p.fuselageParts.push_back({"Fuselage",p.fuselage,{}}); // Must not double count.
  return p;
}
static void mathematics() {
  auto mass=geometry::foamMassProperties(parts());
  CHECK(closeEnough(mass.volumeMm3,2e6,.001));CHECK(closeEnough(mass.centroidMm.x(),100));
  CHECK(closeEnough(mass.plywoodVolumeMm3,2e5,.001));CHECK(closeEnough(mass.plywoodCentroidMm.x(),255));
  WeightBalanceState state;auto result=calculateBalance(state,mass);
  CHECK(closeEnough(result.grams,65+136));CHECK(closeEnough(result.centerMm.x(),(65.*100+136.*255)/201));
  state.parts.push_back({"Battery",20,30,50,100,{0,20},false});result=calculateBalance(state,mass);
  CHECK(closeEnough(result.grams,301));CHECK(closeEnough(result.centerMm.x(),(65.*100+136.*255)/301));
  state.parts[0].centerMm.setX(400);auto moved=calculateBalance(state,mass);
  CHECK(closeEnough(moved.grams,result.grams));CHECK(closeEnough(moved.centerMm.x()-result.centerMm.x(),40000./301));
  state.densityKgM3=65;state.plywoodDensityKgM3=700;CHECK(closeEnough(calculateBalance(state,mass).grams,370));
  CHECK(calculateBalance({},{}).grams==0);
  auto p=fixture();CHECK(closeEnough(geometry::wingRootLeadingEdgeX(p.wing.layers,p.stations.lines,1000),50));
  CHECK(closeEnough(geometry::wingRootLeadingEdgeX(p.wing.layers,p.stations.lines,{}),10));
  // Reverse the drawing chord direction; root LE remains the user-defined edge.
  for(auto& station:p.stations.lines)std::swap(station.first,station.second);
  CHECK(closeEnough(geometry::wingRootLeadingEdgeX(p.wing.layers,p.stations.lines,1000),-400));
  // Swept outboard leading edges must not redefine the root datum.
  p=fixture();p.wing.layers[0].points[1].setY(-100);
  CHECK(closeEnough(geometry::wingRootLeadingEdgeX(p.wing.layers,p.stations.lines,1000),50));
}
static void persistence() {
  auto p=fixture();p.workspace=7;p.weightBalance.parts.push_back({"Motor",30,40,50,gramsPerOunce,{123,-45},true});
  p.weightBalance.plywoodDensityKgM3=725;p.weightBalance.densityKgM3=35;
  auto json=encodeProject(p);CHECK(json["version"]==28);auto restored=decodeProject(json);
  CHECK(restored.workspace==7);CHECK(restored.weightBalance.parts[0].centerMm==QPointF(123,-45));
  CHECK(restored.weightBalance.parts[0].ounces);CHECK(closeEnough(restored.weightBalance.parts[0].grams,gramsPerOunce));
  CHECK(restored.weightBalance.plywoodDensityKgM3==725);CHECK(restored.weightBalance.densityKgM3==35);
  auto old=json;old["version"]=25;old.remove("weightBalance");auto ui=old["ui"].toObject();ui["workspace"]=0;old["ui"]=ui;
  const auto legacy=decodeProject(old);CHECK(legacy.weightBalance.parts.empty());CHECK(legacy.weightBalance.densityKgM3==32.5);
  auto rejects=[](QJsonObject value){try{decodeProject(value);}catch(const std::exception&){return true;}return false;};
  auto bad=json;auto balance=bad["weightBalance"].toObject();balance["densityKgM3"]=-1;bad["weightBalance"]=balance;CHECK(rejects(bad));
  bad=json;balance=bad["weightBalance"].toObject();auto list=balance["parts"].toArray();list.append(list[0]);balance["parts"]=list;bad["weightBalance"]=balance;CHECK(rejects(bad));
  bad=json;balance=bad["weightBalance"].toObject();list=balance["parts"].toArray();auto part=list[0].toObject();part["grams"]=-2;list[0]=part;balance["parts"]=list;bad["weightBalance"]=balance;CHECK(rejects(bad));
}
namespace designrc::gui {
class WeightBalanceTest {
public:
  static void run(const QString& directory) {
    auto project=fixture();
    // Wing and Side View now share one scale; retain the same physical fixture.
    for(auto& layer:project.wing.layers)for(auto& point:layer.points)point*=4;
    for(auto& station:project.stations.lines){station.first.position*=4;station.second.position*=4;}
    MainWindow w;w.resize(1280,900);w.show();w.restoreProject(project);QApplication::processEvents();
    auto* toolbar=w.findChild<QToolBar*>("workspaceToolBar");CHECK(toolbar->actions()[7]->text()=="Weight and Balance");
    CHECK(toolbar->actions()[7]->isEnabled());toolbar->actions()[7]->trigger();QApplication::processEvents();
    auto* panel=w.weightBalancePanel_;CHECK(panel->isVisible());CHECK(w.graphicsTabs_->currentIndex()==0);CHECK(!w.graphicsTabs_->isTabEnabled(1));
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("Total weight: unavailable"));
    CHECK(!panel->findChild<QPushButton*>("balanceEditPart")->isEnabled());
    w.assemblyOriginals_=parts();w.assemblyState_.positioned=true;w.assemblyState_.offsets[0]={25,0};
    int calculations=0;
    QObject::connect(w.statusBar(),&QStatusBar::messageChanged,&w,[&](const QString& message){
      if(message.startsWith("Calculating Weight and Balance")) {
        ++calculations;
        CHECK(QApplication::overrideCursor());CHECK(QApplication::overrideCursor()->shape()==Qt::WaitCursor);
      }
    });
    w.assemblySourceFingerprint_=w.assemblyFingerprint();w.updateWeightBalance();
    CHECK(calculations==1);CHECK(!QApplication::overrideCursor());
    toolbar->actions()[0]->trigger();toolbar->actions()[7]->trigger();
    CHECK(calculations==1); // Unchanged tab round trip reuses measured solids.
    w.assemblyState_.offsets[0].rx()+=10;w.updateWeightBalance();CHECK(calculations==2);
    w.assemblyState_.offsets[0].rx()-=10;w.updateWeightBalance();CHECK(calculations==3);
    w.assemblyOriginals_=parts();w.updateWeightBalance();CHECK(calculations==4); // Rebuilt solids.
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("201.00 g"));
    auto mass=geometry::foamMassProperties(*w.exportAssemblyParts());
    const double expected=calculateBalance(panel->state(),mass).centerMm.x()-75;
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains(QString::number(expected,'f',3)));
    // Add via the real modal dialog. Invalid/duplicate names leave it open.
    QTimer::singleShot(0,&w,[&]{
      auto* d=panel->findChild<QDialog*>("balancePartDialog");CHECK(d);
      auto* buttons=d->findChild<QDialogButtonBox*>();buttons->button(QDialogButtonBox::Ok)->click();CHECK(d->isVisible());
      d->findChild<QLineEdit*>("balancePartName")->setText("Battery");
      d->findChild<QDoubleSpinBox*>("balancePartWidth")->setValue(25);
      d->findChild<QDoubleSpinBox*>("balancePartHeight")->setValue(30);
      d->findChild<QDoubleSpinBox*>("balancePartLength")->setValue(80);
      d->findChild<QComboBox*>("balancePartMassUnit")->setCurrentIndex(1);
      d->findChild<QDoubleSpinBox*>("balancePartWeight")->setValue(2);
      buttons->button(QDialogButtonBox::Ok)->click();
    });
    panel->findChild<QPushButton*>("balanceAddPart")->click();
    CHECK(panel->state().parts.size()==1);CHECK(closeEnough(panel->state().parts[0].grams,2*gramsPerOunce));
    CHECK(closeEnough(panel->state().parts[0].centerMm.x(),250));CHECK(closeEnough(panel->state().parts[0].centerMm.y(),0));
    CHECK(closeEnough(panel->partRectangle(0).width(),64));CHECK(closeEnough(panel->partRectangle(0).height(),24));
    CHECK(panel->selected()==0);CHECK(w.projectModified());
    const auto before=panel->state().parts[0].centerMm;
    const auto start=w.planViewport_->mapFromScene(panel->partRectangle(0).center());const auto end=start+QPoint{80,-20};
    const auto drag=[&](QEvent::Type type,QPoint at,Qt::MouseButton button,Qt::MouseButtons buttons){
      QMouseEvent event{type,QPointF{at},QPointF{w.planViewport_->viewport()->mapToGlobal(at)},button,buttons,Qt::NoModifier};
      QApplication::sendEvent(w.planViewport_->viewport(),&event);
    };
    drag(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);drag(QEvent::MouseMove,end,Qt::NoButton,Qt::LeftButton);drag(QEvent::MouseButtonRelease,end,Qt::LeftButton,Qt::NoButton);
    CHECK(panel->state().parts[0].centerMm.x()>before.x());CHECK(panel->state().parts[0].centerMm.y()>before.y());
    CHECK(closeEnough(panel->state().parts[0].grams,2*gramsPerOunce));
    const auto moved=panel->state().parts[0].centerMm;
    // Cancel does not change data.
    QTimer::singleShot(0,&w,[&]{panel->findChild<QDialog*>("balancePartDialog")->reject();});
    panel->findChild<QPushButton*>("balanceEditPart")->click();CHECK(panel->state().parts[0].centerMm==moved);
    QTimer::singleShot(0,&w,[&]{
      auto* d=panel->findChild<QDialog*>("balancePartDialog");CHECK(d->findChild<QLineEdit*>("balancePartName")->text()=="Battery");
      d->findChild<QLineEdit*>("balancePartName")->setText("Flight battery");d->findChild<QDoubleSpinBox*>("balancePartLength")->setValue(100);
      d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });panel->findChild<QPushButton*>("balanceEditPart")->click();
    CHECK(panel->state().parts[0].name=="Flight battery");CHECK(panel->state().parts[0].centerMm==moved);
    CHECK(closeEnough(panel->partRectangle(0).width(),80));
    QTimer::singleShot(0,&w,[&]{
      auto* d=panel->findChild<QDialog*>("balancePartDialog");
      d->findChild<QLineEdit*>("balancePartName")->setText("flight battery");
      d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();CHECK(d->isVisible());
      d->findChild<QLineEdit*>("balancePartName")->setText("Receiver");
      d->findChild<QDoubleSpinBox*>("balancePartWeight")->setValue(15);
      d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });panel->findChild<QPushButton*>("balanceAddPart")->click();
    CHECK(panel->state().parts.size()==2);
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("272.70 g"));
    panel->findChild<QPushButton*>("balanceDeletePart")->click();CHECK(panel->state().parts.size()==1);
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("257.70 g"));
    panel->findChild<QComboBox*>("balanceParts")->setCurrentIndex(0);
    // Unit changes retain physical placement, dimensions and grams.
    auto reference=w.projectReference();reference.units=ProjectUnits::Inches;
    w.referencePanel_->restoreReference(reference);w.updateWeightBalance();
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains(" in from"));
    panel->setFoam(mass,1000,{});CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("Center of mass: -"));w.updateWeightBalance();
    QTimer::singleShot(0,&w,[&]{
      auto* d=panel->findChild<QDialog*>("balancePartDialog");CHECK(closeEnough(d->findChild<QDoubleSpinBox*>("balancePartLength")->value(),100/25.4,1e-6));d->reject();
    });panel->findChild<QPushButton*>("balanceEditPart")->click();
    // Changed Assembly cuts replace the mass source without counting originals.
    const int beforeCuts=calculations;
    w.assemblyCutParts_=parts();w.assemblyCutParts_->fuselage=box(0,50,100,100);w.assemblyState_.cuts=true;w.updateWeightBalance();
    CHECK(calculations==beforeCuts+1);
    CHECK(closeEnough(calculateBalance(panel->state(),geometry::foamMassProperties(*w.assemblyCutParts_)).grams,48.75+136+2*gramsPerOunce));
    panel->findChild<QDoubleSpinBox*>("balanceDensity")->setValue(40);CHECK(panel->state().densityKgM3==40);
    panel->findChild<QDoubleSpinBox*>("balancePlywoodDensity")->setValue(700);CHECK(panel->state().plywoodDensityKgM3==700);
    w.updateWeightBalance();CHECK(calculations==beforeCuts+1); // Density/part edits reuse volumes.
    QString error;const auto file=directory+"/weight-balance.foam";CHECK(w.saveProjectFile(file,error));CHECK(!w.projectModified());
    panel->findChild<QComboBox*>("balanceParts")->setCurrentIndex(-1);CHECK(!w.projectModified());
    toolbar->actions()[0]->trigger();CHECK(!panel->isVisible());CHECK(!w.balanceStatus_->isVisible());CHECK(w.graphicsTabs_->isTabEnabled(1));
    QImage hidden{1000,700,QImage::Format_ARGB32};hidden.fill(Qt::transparent);auto blank=hidden.copy();
    {QPainter painter{&hidden};panel->paint(painter);}CHECK(hidden==blank);
    CHECK(!w.projectModified());toolbar->actions()[7]->trigger();QApplication::processEvents();
    CHECK(w.planViewport_->viewport()->rect().contains(w.planViewport_->mapFromScene(panel->partRectangle(0).center())));
    CHECK(w.balanceStatus_->height()<40);
    CHECK(w.grab().save(directory+"/weight-balance.png"));
    CHECK(w.openProjectFile(file,error));CHECK(!w.projectModified());CHECK(w.projectDocument().workspace==7);
    CHECK(w.projectDocument().weightBalance.parts[0].centerMm==moved);CHECK(w.projectDocument().weightBalance.plywoodDensityKgM3==700);
    CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_);CHECK(w.wingShape_.IsNull()&&w.fuselageShape_.IsNull());
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("Total weight: unavailable"));
    panel->findChild<QComboBox*>("balanceParts")->setCurrentIndex(0);panel->findChild<QPushButton*>("balanceDeletePart")->click();
    CHECK(panel->state().parts.empty());CHECK(w.projectModified());
    w.assemblyOriginals_=parts();w.assemblySourceFingerprint_=w.assemblyFingerprint();w.updateWeightBalance();
    auto side=w.planViewport_->fuselageSketchEditor().state();side.layers[1].points[1].rx()+=1;
    w.planViewport_->fuselageSketchEditor().restoreState(side);w.updateProjectTitle();
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("Total weight: unavailable"));
    w.resetProject();CHECK(panel->state().parts.empty());CHECK(panel->state().densityKgM3==32.5);CHECK(!toolbar->actions()[7]->isEnabled());
  }
};
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};app.setStyle("Fusion");QTemporaryDir settings;QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());app.setOrganizationName("WeightBalanceTests");app.setApplicationName("WeightBalanceTests");
  try {mathematics();persistence();WeightBalanceTest::run(argc>1?QString::fromLocal8Bit(argv[1]):settings.path());std::cout<<"Weight and Balance checks passed\n";return 0;}
  catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

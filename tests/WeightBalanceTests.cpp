#include "LegacyProject.h"
#include "gui/MainWindow.h"
#include "gui/ReferencePanel.h"
#include "gui/WeightBalancePanel.h"
#include "gui/FiberglassPanel.h"
#include "gui/WingCalibration.h"
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
#include <QTableWidget>
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
  CHECK(geometry::balanceWorkerLimit(0)==1&&geometry::balanceWorkerLimit(1)==1);
  CHECK(geometry::balanceWorkerLimit(2)==1&&geometry::balanceWorkerLimit(4)==3);
  CHECK(geometry::balanceWorkerLimit(8)==7&&geometry::balanceWorkerLimit(16)==14);
  CHECK(geometry::balanceWorkerLimit(20)==18&&geometry::balanceWorkerLimit(64)==57);
  geometry::MaterialMeasurementCache cache;auto cachedParts=parts();
  auto cachedMass=geometry::foamMassProperties(cachedParts,0,&cache);CHECK(cache.integrations==4);
  geometry::foamMassProperties(cachedParts,0,&cache);CHECK(cache.integrations==4);
  cachedParts.inserts[0].name="Renamed former";cachedMass=geometry::foamMassProperties(cachedParts,0,&cache);CHECK(cache.integrations==4&&cachedMass.components[6].name=="Renamed former");
  cachedParts.wing=box(100,50,100,100);cachedMass=geometry::foamMassProperties(cachedParts,0,&cache);CHECK(cache.integrations==5&&closeEnough(cachedMass.volumeMm3,1.5e6));
  AssemblyState movedState;movedState.offsets[0]={20,30};
  cachedMass=geometry::foamMassProperties(geometry::placeAssembly(cachedParts,movedState),0,&cache);CHECK(cache.integrations==6);
  const auto freshMass=geometry::foamMassProperties(geometry::placeAssembly(cachedParts,movedState),1);CHECK(closeEnough(cachedMass.volumeMm3,freshMass.volumeMm3));CHECK(cachedMass.centroidMm==freshMass.centroidMm);
  geometry::foamMassProperties(geometry::placeAssembly(cachedParts,movedState),0,&cache);CHECK(cache.integrations==6);
  std::swap(cachedParts.inserts[0],cachedParts.inserts[1]);geometry::foamMassProperties(geometry::placeAssembly(cachedParts,movedState),0,&cache);CHECK(cache.integrations==6);
  auto mass=geometry::foamMassProperties(parts());
  const auto serial=geometry::foamMassProperties(parts(),1);
  CHECK(closeEnough(serial.volumeMm3,mass.volumeMm3));CHECK(serial.centroidMm==mass.centroidMm);
  CHECK(closeEnough(serial.plywoodVolumeMm3,mass.plywoodVolumeMm3));CHECK(serial.plywoodCentroidMm==mass.plywoodCentroidMm);
  CHECK(serial.components.size()==mass.components.size());
  for(std::size_t i=0;i<serial.components.size();++i){CHECK(serial.components[i].name==mass.components[i].name);CHECK(closeEnough(serial.components[i].volumeMm3,mass.components[i].volumeMm3));}
  CHECK(closeEnough(mass.volumeMm3,2e6,.001));CHECK(closeEnough(mass.centroidMm.x(),100));
  CHECK(closeEnough(mass.plywoodVolumeMm3,2e5,.001));CHECK(closeEnough(mass.plywoodCentroidMm.x(),255));
  CHECK(mass.components.size()==8);double foamSum=0,plywoodSum=0;
  for(const auto& component:mass.components)(component.plywood?plywoodSum:foamSum)+=component.volumeMm3;
  CHECK(closeEnough(foamSum,mass.volumeMm3));CHECK(closeEnough(plywoodSum,mass.plywoodVolumeMm3));
  CHECK(mass.components[0].name=="Fuselage"&&closeEnough(mass.components[0].volumeMm3,1e6,.001));
  CHECK(mass.components[6].name=="Former 1"&&mass.components[6].plywood);
  WeightBalanceState state;auto result=calculateBalance(state,mass);
  CHECK(closeEnough(result.grams,51.26+136));CHECK(closeEnough(result.centerMm.x(),(51.26*100+136.*255)/187.26));
  state.parts.push_back({"Battery",20,30,50,100,{0,20},false});result=calculateBalance(state,mass);
  CHECK(closeEnough(result.grams,287.26));CHECK(closeEnough(result.centerMm.x(),(51.26*100+136.*255)/287.26));
  state.parts[0].centerMm.setX(400);auto moved=calculateBalance(state,mass);
  CHECK(closeEnough(moved.grams,result.grams));CHECK(closeEnough(moved.centerMm.x()-result.centerMm.x(),40000./287.26));
  state.densityKgM3=65;state.plywoodDensityKgM3=700;CHECK(closeEnough(calculateBalance(state,mass).grams,370));
  CHECK(calculateBalance({},{}).grams==0);
  auto carbon=parts();carbon.sparMaterials={{"Mid tube",10000,{60,100,12}},{"Top rod",5000,{80,-100,20}}};
  auto cf=geometry::foamMassProperties(carbon);CHECK(cf.components.size()==10&&cf.components.back().carbonFiber);
  CHECK(closeEnough(cf.carbonFiberVolumeMm3,15000));CHECK(closeEnough(cf.carbonFiberCentroidMm.x(),200./3));
  WeightBalanceState cfState;auto withCf=calculateBalance(cfState,cf);
  CHECK(closeEnough(withCf.grams,187.26+23.1));CHECK(closeEnough(withCf.centerMm.x(),(51.26*100+136*255+23.1*200/3)/(187.26+23.1)));
  AssemblyState placement;placement.offsets[0]={20,30};placement.rotationDegrees[0]=90;
  auto placed=geometry::placeAssembly(carbon,placement);CHECK(closeEnough(placed.sparMaterials[0].volumeMm3,10000));
  CHECK(placed.sparMaterials[0].center.Distance(gp_Pnt{32,100,-30})<1e-6);
  const auto cut=geometry::cutAssemblyIntersections(placed);CHECK(cut.parts.sparMaterials.size()==2);
  CHECK(cut.parts.sparMaterials[0].center.Distance(placed.sparMaterials[0].center)<1e-6);

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
  auto p=fixture();p.spars[0][2].sizeMm=6.35;p.spars[0][2].sizeText=".25 in";
  p.spars[0][2].insideDiameterMm=5.08;p.spars[0][2].insideDiameterText=".2 in";
  p.weightBalance.carbonFiberDensityKgM3=1600;p.workspace=7;p.weightBalance.parts.push_back({"Motor",30,40,50,gramsPerOunce,{123,-45},true});
  p.weightBalance.plywoodDensityKgM3=725;p.weightBalance.densityKgM3=35;
  auto json=encodeProject(p);CHECK(json["version"]==33);auto restored=decodeProject(json);
  CHECK(restored.weightBalance.carbonFiberDensityKgM3==1600);
  CHECK(restored.spars[0][2].insideDiameterMm==5.08&&restored.spars[0][2].insideDiameterText==".2 in");
  auto previous=json;previous["version"]=28;legacyCutViews(previous);auto earlier=decodeProject(previous);
  CHECK(earlier.weightBalance.carbonFiberDensityKgM3==1540);
  CHECK(closeEnough(earlier.spars[0][2].insideDiameterMm,5.35)&&earlier.spars[0][2].sizeMm==6.35);
  CHECK(restored.workspace==7);CHECK(restored.weightBalance.parts[0].centerMm==QPointF(123,-45));
  CHECK(restored.weightBalance.parts[0].ounces);CHECK(closeEnough(restored.weightBalance.parts[0].grams,gramsPerOunce));
  CHECK(restored.weightBalance.plywoodDensityKgM3==725);CHECK(restored.weightBalance.densityKgM3==35);
  auto old=json;old["version"]=25;legacyCutViews(old);old.remove("weightBalance");auto ui=old["ui"].toObject();ui["workspace"]=0;old["ui"]=ui;
  const auto legacy=decodeProject(old);CHECK(legacy.weightBalance.parts.empty());CHECK(legacy.weightBalance.densityKgM3==25.63);
  auto rejects=[](QJsonObject value){try{decodeProject(value);}catch(const std::exception&){return true;}return false;};
  auto invalid=json;auto panels=invalid["spars"].toArray();auto spars=panels[0].toArray();auto mid=spars[2].toObject();
  mid["insideDiameterMm"]=6.35;spars[2]=mid;panels[0]=spars;invalid["spars"]=panels;CHECK(rejects(invalid));
  invalid=json;auto invalidBalance=invalid["weightBalance"].toObject();invalidBalance["carbonFiberDensityKgM3"]=0;invalid["weightBalance"]=invalidBalance;CHECK(rejects(invalid));
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
    w.assemblyState_.rotationDegrees[0]=.5;w.updateWeightBalance();CHECK(calculations==5);
    w.updateWeightBalance();CHECK(calculations==5); // Same rotation reuses the cache.
    w.assemblyState_.rotationDegrees[0]=0;w.updateWeightBalance();CHECK(calculations==6);
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("187.26 g"));
    auto* breakdown=panel->findChild<QTableWidget*>("balanceBreakdown");CHECK(breakdown&&breakdown->rowCount()==11);
    CHECK(breakdown->item(0,1)->text()=="1000.00");CHECK(breakdown->item(0,2)->text()=="25.63");
    CHECK(breakdown->item(0,3)->text()=="0.904");CHECK(breakdown->item(2,0)->text().contains("not present"));
    CHECK(breakdown->item(6,2)->text()=="68.00");CHECK(breakdown->item(8,2)->text()=="51.26");
    auto mass=geometry::foamMassProperties(*w.exportAssemblyParts());
    const double expected=calculateBalance(panel->state(),mass).centerMm.x()-75;
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains(QString::number(expected,'f',3)));
    const auto verifyMarker=[&] {
      const auto position=panel->cgScenePosition();CHECK(position);
      const auto transform=geometry::fuselageSideTransform(project.fuselage.layers[1],500);
      const auto root=wingCalibration(project.wing.layers,project.stations.lines,1000);
      const auto profile=normalizedAirfoil(project.airfoils.entries[0]).resampled(201);
      double low=profile.front().y,high=low;for(auto point:profile){low=std::min(low,point.y);high=std::max(high,point.y);}
      const auto currentMass=calculateBalance(panel->state(),geometry::foamMassProperties(*w.exportAssemblyParts()));
      CHECK(closeEnough(position->x(),transform.left+currentMass.centerMm.x()/transform.scale));
      CHECK(closeEnough(position->y(),transform.verticalOrigin-(low+.2*(high-low))*root.rootChordMm/transform.scale));
    };
    verifyMarker();
    // Render the actual bundled symbol at two zoom levels: location transforms,
    // but the small 28-pixel marker stays the same size.
    for(double zoom:{.5,3.}) {
      QImage image{200,200,QImage::Format_ARGB32_Premultiplied};image.fill(Qt::white);
      {QPainter painter{&image};painter.translate(100,100);painter.scale(zoom,zoom);painter.translate(-*panel->cgScenePosition());panel->paint(painter);}
      int colored=0;QRect bounds;
      for(int y=0;y<200;++y)for(int x=0;x<200;++x){const auto color=image.pixelColor(x,y);if(color.red()>150&&color.blue()>100&&color.green()<100){++colored;bounds=bounds.united(QRect{x,y,1,1});}}
      CHECK(colored>80);CHECK(bounds.width()<=28&&bounds.height()<=28);CHECK(bounds.center().manhattanLength()>190&&bounds.center().manhattanLength()<210);
      CHECK(image.save(directory+QString{"/cg-marker-%1.png"}.arg(zoom)));
    }
    // Add via the real modal dialog. Invalid/duplicate names leave it open.
    QTimer::singleShot(0,&w,[&]{
      auto* d=panel->findChild<QDialog*>("balancePartDialog");CHECK(d);
      auto* buttons=d->findChild<QDialogButtonBox*>();buttons->button(QDialogButtonBox::Ok)->click();CHECK(d->isVisible());
      d->findChild<QLineEdit*>("balancePartName")->setText("Battery");
      d->findChild<QLineEdit*>("balancePartWidth")->setText("25");
      d->findChild<QLineEdit*>("balancePartHeight")->setText("30");
      d->findChild<QLineEdit*>("balancePartLength")->setText("80");
      d->findChild<QComboBox*>("balancePartMassUnit")->setCurrentIndex(1);
      d->findChild<QDoubleSpinBox*>("balancePartWeight")->setValue(2);
      buttons->button(QDialogButtonBox::Ok)->click();
    });
    panel->findChild<QPushButton*>("balanceAddPart")->click();
    CHECK(panel->state().parts.size()==1);CHECK(closeEnough(panel->state().parts[0].grams,2*gramsPerOunce));
    CHECK(closeEnough(panel->state().parts[0].centerMm.x(),250));CHECK(closeEnough(panel->state().parts[0].centerMm.y(),0));
    CHECK(closeEnough(panel->partRectangle(0).width(),64));CHECK(closeEnough(panel->partRectangle(0).height(),24));
    CHECK(panel->selected()==0);CHECK(w.projectModified());
    verifyMarker();
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
      d->findChild<QLineEdit*>("balancePartName")->setText("Flight battery");d->findChild<QLineEdit*>("balancePartLength")->setText("100");
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
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("258.96 g"));
    panel->findChild<QPushButton*>("balanceDeletePart")->click();CHECK(panel->state().parts.size()==1);
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("243.96 g"));
    panel->findChild<QComboBox*>("balanceParts")->setCurrentIndex(0);
    // Unit changes retain physical placement, dimensions and grams.
    auto reference=w.projectReference();reference.units=ProjectUnits::Inches;
    w.referencePanel_->restoreReference(reference);w.updateWeightBalance();
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains(" in from"));
    panel->setFoam(mass,1000,{});CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("Center of Gravity: -"));w.updateWeightBalance();
    QTimer::singleShot(0,&w,[&]{
      auto* d=panel->findChild<QDialog*>("balancePartDialog");CHECK(d->findChild<QLineEdit*>("balancePartLength")->text()=="100 mm");d->reject();
    });panel->findChild<QPushButton*>("balanceEditPart")->click();
    // Changed Assembly cuts replace the mass source without counting originals.
    const int beforeCuts=calculations;
    w.assemblyCutParts_=parts();w.assemblyCutParts_->fuselage=box(0,50,100,100);w.assemblyState_.cuts=true;w.updateWeightBalance();
    CHECK(calculations==beforeCuts+1);
    CHECK(closeEnough(calculateBalance(panel->state(),geometry::foamMassProperties(*w.assemblyCutParts_)).grams,38.445+136+2*gramsPerOunce));
    QTimer::singleShot(0,&w,[&]{panel->findChild<QDoubleSpinBox*>("balanceDensity")->setValue(99);panel->findChild<QDialog*>("balanceMaterialsDialog")->reject();});
    panel->findChild<QPushButton*>("balanceMaterials")->click();CHECK(panel->state().densityKgM3==25.63);
    QTimer::singleShot(0,&w,[&]{auto* dialog=panel->findChild<QDialog*>("balanceMaterialsDialog");dialog->findChild<QDoubleSpinBox*>("balanceDensity")->setValue(40);dialog->findChild<QDoubleSpinBox*>("balancePlywoodDensity")->setValue(700);dialog->findChild<QDoubleSpinBox*>("balanceResinDensity")->setValue(1200);CHECK(dialog->grab().save(directory+"/material-densities.png"));dialog->accept();});
    panel->findChild<QPushButton*>("balanceMaterials")->click();CHECK(panel->state().densityKgM3==40);CHECK(panel->state().plywoodDensityKgM3==700);CHECK(panel->state().resinDensityKgM3==1200);
    CHECK(breakdown->item(0,1)->text()=="500.00");CHECK(breakdown->item(0,2)->text()=="20.00");
    CHECK(breakdown->item(6,2)->text()=="70.00");CHECK(breakdown->item(8,2)->text()=="60.00");
    CHECK(breakdown->rowCount()==12);CHECK(breakdown->item(11,1)->text()==QString::fromUtf8("—"));
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
    CHECK(breakdown->rowCount()==0); // Stale geometry must not leave old component rows.
    CHECK(panel->state().parts.empty());CHECK(w.projectModified());
    w.assemblyOriginals_=parts();w.assemblySourceFingerprint_=w.assemblyFingerprint();w.updateWeightBalance();
    auto side=w.planViewport_->fuselageSketchEditor().state();side.layers[1].points[1].rx()+=1;
    w.planViewport_->fuselageSketchEditor().restoreState(side);w.updateProjectTitle();
    CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("Total weight: unavailable"));
    w.assemblyOriginals_=parts();w.assemblyOriginals_.sparMaterials={{"Mid tube",10000,{60,0,10}}};
    w.assemblyState_.cuts=false;w.assemblySourceFingerprint_=w.assemblyFingerprint();w.updateWeightBalance();
    auto* carbonDensity=panel->findChild<QDoubleSpinBox*>("balanceCarbonFiberDensity");CHECK(carbonDensity&&carbonDensity->value()==1540);
    CHECK(breakdown->item(8,0)->text().contains("Carbon Fiber"));CHECK(breakdown->item(8,2)->text()=="15.40");
    const int beforeCarbonDensity=calculations;
    QTimer::singleShot(0,&w,[&]{carbonDensity->setValue(1600);panel->findChild<QDialog*>("balanceMaterialsDialog")->accept();});
    panel->findChild<QPushButton*>("balanceMaterials")->click();w.updateWeightBalance();CHECK(calculations==beforeCarbonDensity);
    CHECK(breakdown->item(8,2)->text()=="16.00");CHECK(panel->findChild<QLabel*>("balanceResults")->text().contains("Carbon Fiber: 16.00 g"));
    CHECK(w.projectDocument().weightBalance.carbonFiberDensityKgM3==1600);
    QApplication::processEvents();CHECK(w.grab().save(directory+"/weight-balance-carbon.png"));
    // Covering edits change mass, but never invalidate the Assembly solids.
    const auto sourceFingerprint=w.assemblySourceFingerprint_;const auto wingShape=w.assemblyOriginals_.wing;
    const int beforeFiberglass=calculations;
    toolbar->actions()[1]->trigger();bool found=false;
    for(auto* action:w.componentToolBar_->actions())if(action->text()=="Fiberglass"){CHECK(action->isEnabled());action->trigger();found=true;}
    CHECK(found&&w.fiberglassPanels_[0]->isVisible()&&w.graphicsTabs_->currentIndex()==0);
    auto covering=w.fiberglassPanels_[0]->state();covering.sketch.layers[0]=rectangle(20,60,160,200);covering.patches[0].name="Root reinforcement";covering.patches[0].wrap=false;
    w.fiberglassPanels_[0]->restore(covering);w.planViewport_->fitInView(QRectF{0,0,600,400},Qt::KeepAspectRatio);QApplication::processEvents();CHECK(w.grab().save(directory+"/fiberglass-wing.png"));
    w.updateProjectTitle();w.updateWeightBalance();CHECK(calculations==beforeFiberglass);
    CHECK(!w.statistics_.weightGrams&&!w.statistics_.cgFromLeadingEdgeMm&&!w.statistics_.wingLoadingGramsPerDm2);
    CHECK(w.fiberglassPanels_[0]->findChild<QLabel*>("airplaneStatistics")->text().count(QString::fromUtf8("—"))==3);
    toolbar->actions()[7]->trigger();w.updateWeightBalance();
    CHECK(w.assemblySourceFingerprint_==sourceFingerprint&&w.assemblyOriginals_.wing.IsEqual(wingShape));
    CHECK(w.balanceMassCache_&&w.balanceMassCache_->fiberglass.size()==1);CHECK(w.balanceMassCache_->fiberglass[0].areaMm2>0);
    const auto measuredArea=w.balanceMassCache_->fiberglass[0].areaMm2;const int beforeClothEdit=calculations;
    covering.patches[0].clothGm2=100;w.fiberglassPanels_[0]->restore(covering);w.updateProjectTitle();
    CHECK(calculations==beforeClothEdit);CHECK(closeEnough(w.balanceMassCache_->fiberglass[0].clothGrams,measuredArea*.0001));
    CHECK(closeEnough(*w.statistics_.weightGrams,calculateBalance(panel->state(),*w.balanceMassCache_).grams));
    QString clothArea,resinArea;for(int row=0;row<breakdown->rowCount();++row){const auto label=breakdown->item(row,0)->text();if(label.contains("Root reinforcement / Fiberglass"))clothArea=label.mid(label.indexOf('('));if(label.contains("Root reinforcement / Resin"))resinArea=label.mid(label.indexOf('('));}CHECK(!clothArea.isEmpty()&&clothArea==resinArea);
    CHECK(w.grab().save(directory+"/weight-balance-fiberglass.png"));
    // An aggregate cache miss must preserve each unaffected measurement.
    const auto volumeIntegrations=w.balanceMaterialCache_.integrations;
    const auto coveringIntegrations=w.balanceFiberglassCache_.integrations;
    auto incremental=covering;auto second=covering.sketch.layers[0];for(auto& point:second.points)point.rx()+=5;
    incremental.sketch.layers.push_back(second);incremental.patches.push_back(covering.patches[0]);incremental.patches[1].name="Second patch";
    w.fiberglassPanels_[0]->restore(incremental);w.updateProjectTitle();
    CHECK(w.balanceMaterialCache_.integrations==volumeIntegrations);
    CHECK(w.balanceFiberglassCache_.integrations==coveringIntegrations+1);
    CHECK(w.balanceMassCache_->fiberglass.size()==2&&closeEnough(w.balanceMassCache_->fiberglass[0].areaMm2,measuredArea));
    incremental.sketch.layers[1].points[0].rx()+=2;w.fiberglassPanels_[0]->restore(incremental);w.updateProjectTitle();
    CHECK(w.balanceMaterialCache_.integrations==volumeIntegrations&&w.balanceFiberglassCache_.integrations==coveringIntegrations+2);
    auto densityState=panel->state();densityState.resinDensityKgM3+=10;panel->restore(densityState);w.updateProjectTitle();
    CHECK(w.balanceMaterialCache_.integrations==volumeIntegrations&&w.balanceFiberglassCache_.integrations==coveringIntegrations+2);
    w.fiberglassPanels_[0]->restore(covering);w.updateProjectTitle();
    CHECK(w.balanceFiberglassCache_.integrations==coveringIntegrations+2&&w.balanceFiberglassCache_.entries.size()==1);
    // Undo/redo includes patch material and sketch inputs.
    w.resetEditHistory();covering.patches[0].name="Renamed covering";w.fiberglassPanels_[0]->restore(covering);w.captureEdit();CHECK(w.undoAction_->isEnabled());
    w.undoAction_->trigger();CHECK(w.projectDocument().fiberglass[0].patches[0].name=="Root reinforcement");w.redoAction_->trigger();CHECK(w.projectDocument().fiberglass[0].patches[0].name=="Renamed covering");
    CHECK(w.saveProjectFile(file,error));CHECK(w.openProjectFile(file,error));CHECK(w.projectDocument().fiberglass[0].patches[0].name=="Renamed covering");CHECK(!w.projectModified());
    CHECK(w.balanceMaterialCache_.entries.empty()&&w.balanceFiberglassCache_.entries.empty());
    CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_);
    w.resetProject();CHECK(panel->state().carbonFiberDensityKgM3==1540);CHECK(panel->state().parts.empty());CHECK(panel->state().densityKgM3==25.63);CHECK(!toolbar->actions()[7]->isEnabled());
  }
};
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};app.setStyle("Fusion");QTemporaryDir settings;QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());app.setOrganizationName("WeightBalanceTests");app.setApplicationName("WeightBalanceTests");
  try {if(!app.arguments().contains("--gui-only")){mathematics();persistence();}WeightBalanceTest::run(argc>1?QString::fromLocal8Bit(argv[1]):settings.path());std::cout<<"Weight and Balance checks passed\n";return 0;}
  catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

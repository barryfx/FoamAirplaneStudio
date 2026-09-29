#include "gui/MainWindow.h"
#include "gui/AirplaneStatistics.h"
#include "gui/AirfoilPanel.h"
#include "gui/ReferencePanel.h"
#include "gui/WeightBalancePanel.h"
#include <QApplication>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <QAction>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolBar>
#include <QVBoxLayout>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace designrc;
using namespace designrc::gui;
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" line "+std::to_string(__LINE__));}while(false)
static bool closeEnough(double a,double b){return std::abs(a-b)<1e-6*std::max(1.,std::abs(b));}
static SketchLayer rectangle(double x,double y,double width,double height,bool closed=false) {
  SketchLayer layer{{{x,y},{x+width,y},{x+width,y+height},{x,y+height}},
    {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}}}};
  if(closed)layer.curves.push_back({SketchTool::Line,{3,0}});return layer;
}
static ProjectDocument fixture() {
  ProjectDocument p;p.reference.wingspanMm=1000;p.wingspanText="1000 mm";
  p.wing.layers[0]=rectangle(0,0,500,200);
  p.stations.lines={{{0,0,0,{0,0}},{0,2,1,{0,200}},LineAlignment::Vertical,0},
    {{0,0,1,{500,0}},{0,2,0,{500,200}},LineAlignment::Vertical,0}};
  p.airfoils.entries.push_back({"NACA",domain::AirfoilProfile::nacaSymmetric(.12),{}, {}});
  p.fuselage.layers={rectangle(0,400,700,100,true),rectangle(0,600,700,100,true)};
  p.stabilizerOutlines[0].layers[0]=rectangle(800,0,150,100);
  p.stabilizerOutlines[1].layers[0]=rectangle(800,200,100,100);
  p.weightBalance.parts.push_back({"Battery",20,20,40,974.37,{100,0}});
  return p;
}
static void measurements() {
  auto p=fixture();auto s=outlineStatistics(p);
  CHECK(s.wingspanMm&&closeEnough(*s.wingspanMm,1000));CHECK(closeEnough(*s.rootChordMm,200));CHECK(closeEnough(*s.wingAreaMm2,200000));
  CHECK(closeEnough(*s.aspectRatio,5));CHECK(closeEnough(*s.fuselageLengthMm,700));CHECK(closeEnough(*s.horizontalAreaMm2,30000));CHECK(closeEnough(*s.verticalAreaMm2,10000));
  CHECK(!s.weightGrams&&!s.cgFromLeadingEdgeMm&&!s.wingLoadingGramsPerDm2);
  p.reference.wingspanMm=2000;s=outlineStatistics(p);
  CHECK(closeEnough(*s.wingAreaMm2,800000)&&closeEnough(*s.rootChordMm,400)&&closeEnough(*s.aspectRatio,5));
  CHECK(closeEnough(*s.horizontalAreaMm2,120000)&&closeEnough(*s.fuselageLengthMm,1400));
  p.wing.layers[0].curves.pop_back();s=outlineStatistics(p);CHECK(!s.wingAreaMm2&&!s.aspectRatio);
  p=fixture();p.wing.layers={rectangle(0,0,250,200),rectangle(250,0,250,200)};p.wing.layers[0].curves.erase(p.wing.layers[0].curves.begin()+1);
  s=outlineStatistics(p);CHECK(closeEnough(*s.wingAreaMm2,200000));
  p=fixture();p.stations.lines.clear();s=outlineStatistics(p);CHECK(s.wingspanMm&&closeEnough(*s.rootChordMm,200)&&closeEnough(*s.wingAreaMm2,200000));
  p=fixture();p.reference.toScale=true;p.reference.image.physicalSizeMm=QSizeF{1000,1000};
  p.reference.image.pages.push_back({QImage{1,1,QImage::Format_RGB32},QSizeF{1000,1000}});
  s=outlineStatistics(p);CHECK(closeEnough(*s.wingspanMm,1000));CHECK(closeEnough(*s.wingAreaMm2,200000));
  CHECK(statisticsText({},ProjectUnits::Millimeters).isEmpty());
}
namespace designrc::gui {
class AirplaneStatisticsTest {
public:
 static void run(const QString& directory) {
  MainWindow w;w.resize(1280,900);w.show();w.restoreProject(fixture());QApplication::processEvents();
  CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_&&!w.stabilizerProcessing());
  CHECK(w.statistics_.wingspanMm&&closeEnough(*w.statistics_.wingAreaMm2,200000));
  auto* referenceStats=w.referencePanel_->findChild<QLabel*>("airplaneStatistics");CHECK(referenceStats&&referenceStats->isVisible());
  CHECK(referenceStats->text().contains("Wingspan")&&!referenceStats->text().contains("Weight"));
  StatisticsBalance balance;balance.materials.volumeMm3=1e6;balance.materials.centroidMm={100,0};balance.leadingEdgeMm=0;
  balance.sourceKey=w.statisticsMassKey();w.statistics_.balance=balance;w.updateStatistics();
  CHECK(closeEnough(*w.statistics_.weightGrams,1000)&&closeEnough(*w.statistics_.cgFromLeadingEdgeMm,100));
  CHECK(closeEnough(*w.statistics_.wingLoadingGramsPerDm2,50));
  w.selectWorkspace(1);
  for(auto* action:w.componentToolBar_->actions())if(action->text()=="Airfoils")action->trigger();
  QApplication::processEvents();w.updateStatistics();QApplication::processEvents();
  auto* stats=w.airfoilPanel_->findChild<QLabel*>("airplaneStatistics");auto* smooth=w.airfoilPanel_->findChild<QPushButton*>("smoothAirfoil");
  CHECK(stats&&stats->isVisible()&&stats->text().contains("Wing Loading"));
  CHECK(stats->geometry().bottom()<smooth->geometry().top());CHECK(stats->geometry().bottom()<w.airfoilPanel_->height());
  CHECK(w.grab().save(directory+"/airplane-statistics-airfoils.png"));
  for(int i=0;i<w.dataContents_->layout()->count();++i) {
    auto* panel=w.dataContents_->layout()->itemAt(i)->widget();if(!panel||panel==w.weightBalancePanel_||panel==w.assemblyPanel_||panel->objectName()=="exportPanel")continue;
    CHECK(panel->findChild<QLabel*>("airplaneStatistics"));
  }
  for(const auto* name:{"exportPanel"}) {
    auto* panel=w.findChild<QWidget*>(name);CHECK(panel);
    CHECK(!panel->findChild<QLabel*>("airplaneStatistics"));
  }
  for(auto* action:w.componentToolBar_->actions())if(action->text()=="Outline")action->trigger();
  QApplication::processEvents();
  auto* outline=w.findChild<QWidget*>("wingOutlinePanel");CHECK(outline);
  auto* tabs=outline->findChild<QTabWidget*>();auto* outlineStats=outline->findChild<QLabel*>("airplaneStatistics");
  CHECK(tabs&&outlineStats&&outlineStats->isVisible());CHECK(tabs->geometry().bottom()<outlineStats->geometry().top());
  for(auto* action:w.componentToolBar_->actions())if(action->text()=="Airfoils")action->trigger();

  QString error;const auto file=directory+"/airplane-statistics.foam";CHECK(w.saveProjectFile(file,error));CHECK(!w.projectModified());
  auto saved=readProject(file,error);CHECK(saved&&saved->statistics.balance&&closeEnough(*saved->statistics.weightGrams,1000));
  CHECK(w.openProjectFile(file,error));CHECK(w.statistics_.balance&&closeEnough(*w.statistics_.weightGrams,1000));CHECK(!w.projectModified());
  CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_&&!w.stabilizerProcessing());
  auto massState=w.weightBalancePanel_->state();massState.densityKgM3=50;w.weightBalancePanel_->restore(massState);w.updateProjectTitle();
  CHECK(closeEnough(*w.statistics_.weightGrams,1024.37));CHECK(closeEnough(*w.statistics_.wingLoadingGramsPerDm2,51.2185));
  auto units=w.projectDocument();units.reference.units=ProjectUnits::Inches;w.restoreProject(units);
  CHECK(w.statistics_.balance);CHECK(statisticsText(w.statistics_,ProjectUnits::Inches).contains("oz/ft²"));
  w.selectWorkspace(7);QApplication::processEvents();CHECK(!w.weightBalancePanel_->findChild<QLabel*>("airplaneStatistics"));
  w.weightBalancePanel_->setFoam(balance.materials,0,{});w.updateStatistics();
  auto* results=w.weightBalancePanel_->findChild<QLabel*>("balanceResults");CHECK(results->text().contains("Wing Loading:"));CHECK(results->text().contains("oz/ft²"));
  CHECK(results->text().contains(QString::number(1024.37/gramsPerOunce/(200000/(304.8*304.8)),'f',2)));
  CHECK(w.grab().save(directory+"/airplane-statistics-balance.png"));
  // Geometry changes invalidate saved mass immediately, but recompute outline measurements without generation.
  auto changed=w.projectDocument();changed.reference.wingspanMm=2000;changed.wingspanText="2000 mm";changed.workspace=0;changed.viewport=0;w.restoreProject(changed);
  CHECK(!w.statistics_.balance&&!w.statistics_.weightGrams&&!w.statistics_.wingLoadingGramsPerDm2);
  CHECK(closeEnough(*w.statistics_.wingAreaMm2,800000));CHECK(!w.modelJob_&&!w.fuselageJob_);
  auto old=encodeProject(fixture());old.remove("airplaneStatistics");CHECK(!decodeProject(old).statistics.balance);
  auto invalid=encodeProject(*saved);auto cache=invalid["airplaneStatistics"].toObject();cache["wingAreaMm2"]=-1;invalid["airplaneStatistics"]=cache;
  bool rejected=false;try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
  CHECK(!w.assemblyPanel_->findChild<QLabel*>("airplaneStatistics"));
  // Supply only an empty cached shape, never generate a wing or fuselage. A
  // misplaced automatic measurement would reset/replace the sentinel mass cache.
  TopoDS_Compound cached;BRep_Builder builder;builder.MakeCompound(cached);
  w.assemblyOriginals_.fuselage=cached;w.assemblyState_.cuts=false;
  w.assemblySourceFingerprint_=w.assemblyFingerprint();
  FoamMassProperties sentinel;sentinel.volumeMm3=123456;
  w.balanceMassCache_=sentinel;w.balanceMassFingerprint_="unmeasured sentinel";
  balance.sourceKey=w.statisticsMassKey();w.statistics_.balance=balance;
  w.statistics_.weightGrams=1000;w.statistics_.cgFromLeadingEdgeMm=100;w.statistics_.wingLoadingGramsPerDm2=50;
  w.dataPanel_->setProperty("workspaceIndex",5);
  for(int i=0;i<3;++i) {
    w.assemblyState_.offsets[0]+=QPointF{1,0};w.assemblyState_.rotationDegrees[0]+=.5;
    w.updateProjectTitle();w.updateStatistics();
    CHECK(w.balanceMassCache_&&w.balanceMassCache_->volumeMm3==123456);
    CHECK(w.balanceMassFingerprint_=="unmeasured sentinel");
    CHECK(!w.statistics_.balance&&!w.statistics_.weightGrams&&!w.statistics_.cgFromLeadingEdgeMm&&!w.statistics_.wingLoadingGramsPerDm2);
    CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_&&!w.stabilizerProcessing());
  }
  // Leaving Assembly resumes the statistics footer without starting generation.
  w.assemblyOriginals_={};w.dataPanel_->setProperty("workspaceIndex",0);w.updateStatistics();
  CHECK(referenceStats->text().contains("Wingspan"));
  w.resetProject();CHECK(referenceStats->isHidden()&&!w.statistics_.weightGrams);
 }
};
}
int main(int argc,char** argv) {
 QApplication app{argc,argv};app.setStyle("Fusion");QTemporaryDir settings;QSettings::setDefaultFormat(QSettings::IniFormat);
 QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());app.setOrganizationName("StatisticsTests");app.setApplicationName("StatisticsTests");
 try {measurements();AirplaneStatisticsTest::run(argc>1?QString::fromLocal8Bit(argv[1]):settings.path());std::cout<<"Airplane statistics checks passed (no geometry generation)\n";return 0;}
 catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

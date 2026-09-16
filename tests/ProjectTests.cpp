#include "gui/ProjectDocument.h"
#include "WaitForModel.h"
#include <QDoubleSpinBox>
#include <QMenuBar>
#include "gui/MainWindow.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QSettings>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QLineEdit>
#include <QRadioButton>
#include <QScrollBar>
#include <QTabWidget>
#include <QTabBar>
#include <QPushButton>
#include <QScreen>
#include <QToolBar>
#include <QCheckBox>
#include <QStatusBar>
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace designrc::gui;
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at line "+std::to_string(__LINE__));} while(false)
ProjectDocument fixture() {
  ProjectDocument p;
  p.reference.image.path="/missing/source/multipage.pdf";
  for(auto color:{Qt::red,Qt::green}) {
    QImage image{800,1200,QImage::Format_RGB32};image.fill(color);
    p.reference.image.pages.push_back({image,QSizeF{200,300}});
  }
  p.reference.image.physicalSizeMm=QSizeF{200,600};
  p.reference.units=ProjectUnits::Inches;p.reference.wingspanMm=1000;p.reference.fuselageLengthMm=700;
  p.wingspanText="39.37007874";p.fuselageText="27.55905512";
  p.wing.layers[0]={{{10,10},{70,10},{70,90},{10,90}},
    {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}}}};
  p.wing.selected=1;
  p.airfoils.entries.push_back({QString::fromUtf8("Imported foil \xCE\xB1"),designrc::domain::AirfoilProfile::nacaSymmetric(.12),{}, {}});
  SketchLayer foil{{{5,130},{35,120},{65,130},{35,140}},
    {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};
  p.airfoils.entries.push_back({"Traced foil",{},foil,*closedAirfoilBoundary(foil)});
  p.airfoilSketch.layers={foil,SketchLayer{}};p.airfoilSketch.active=1;p.airfoilSketch.tool=SketchTool::Spline;
  p.airfoilSketch.pending={{10,160},{40,150}};p.airfoilSketch.editing=true;
  p.airfoils.sketching=true;p.airfoils.draft=1;p.airfoils.draftName="Unfinished draft";p.airfoils.chosen=1;
  p.stations.lines={{{0,0,1./6,{20,10}},{0,2,5./6,{20,90}},LineAlignment::Vertical,0},
    {{0,0,5./6,{60,10}},{0,2,1./6,{60,90}},LineAlignment::Vertical,1}};
  p.stations.selected=1;p.workspace=1;p.tool="Airfoils";
  p.spars[0][0]={true,SparShape::Strip,27.5,65,6.35,1.5};
  p.spars[0][2]={true,SparShape::Round,40,75,3.175,1};
  p.plan={4.0,{450,1180}};p.splitterSizes={360,1040};
  return p;
}
void response(QMessageBox::StandardButton button) {
  QTimer::singleShot(0,[button]{
    auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
    if(!box)qFatal("Expected an unsaved-project dialog");box->button(button)->click();
  });
}
void chooseFile(const QString& path) {
  QTimer::singleShot(0,[path]{
    auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
    if(!dialog)qFatal("Expected a project file dialog");dialog->selectFile(path);
    QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
  });
}
void panelWorkflow(MainWindow& window,const QString& filename) {
  QString error;auto* workspace=window.findChild<QToolBar*>("workspaceToolBar");
    auto panelsProject=fixture();panelsProject.airfoils.sketching=false;panelsProject.airfoilSketch.pending.clear();
    panelsProject.wing.layers[0].curves={{SketchTool::Line,{0,1}},{SketchTool::Line,{2,3}}};
    panelsProject.wing.layers.push_back({{{70,10},{130,10},{130,90},{70,90}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}}}});
    panelsProject.stations.lines[0].second.curve=panelsProject.stations.lines[1].second.curve=1;
    for(int i=0;i<2;++i){auto station=panelsProject.stations.lines[i];station.first.layer=station.second.layer=1;station.second.curve=2;station.first.position+=QPointF{60,0};station.second.position+=QPointF{60,0};panelsProject.stations.lines.push_back(station);}
    panelsProject.stations.selected=3;panelsProject.selectedStationPanel=1;
    panelsProject.airfoils.panel=1;panelsProject.airfoils.panelChoices={0,1};
    panelsProject.controls.panels.resize(2);panelsProject.controls.panel=1;
    panelsProject.controls.panels[1][1]={true,HingeCut::Standard,QRectF{80,60,30,60}};
    panelsProject.spars=PanelSpars(2);panelsProject.dihedralDegrees={3,5};panelsProject.selectedDihedralPanel=1;panelsProject.tool="Ailerons/Flaps";
    CHECK(writeProject(filename,panelsProject,error));CHECK(window.openProjectFile(filename,error));QApplication::processEvents();
    CHECK(window.projectDocument().dihedralDegrees==panelsProject.dihedralDegrees);
    CHECK(window.findChild<QTabBar*>("dihedralPanelTabs")->currentIndex()==1);
    auto* controlTabs=window.findChild<QTabBar*>("controlPanelTabs");
    auto* stationTabs=window.findChild<QTabBar*>("stationPanelTabs");
    auto* airfoilTabs=window.findChild<QTabBar*>("airfoilPanelTabs");
    CHECK(controlTabs->count()==2 && stationTabs->count()==2 && airfoilTabs->count()==2);
    CHECK(controlTabs->currentIndex()==1 && stationTabs->currentIndex()==1 && airfoilTabs->currentIndex()==1);
    CHECK(window.findChild<QCheckBox*>("addFlaps")->isChecked());
    controlTabs->setCurrentIndex(0);CHECK(!window.findChild<QCheckBox*>("addFlaps")->isChecked());
    controlTabs->setCurrentIndex(1);CHECK(!window.projectModified());
    auto* component=window.findChild<QToolBar*>("componentToolBar");
    component->actions()[1]->trigger();stationTabs->setCurrentIndex(0);CHECK(!window.projectModified());
    component->actions()[2]->trigger();airfoilTabs->setCurrentIndex(0);CHECK(!window.projectModified());
    CHECK(window.saveProjectFile(filename,error));CHECK(window.openProjectFile(filename,error));QApplication::processEvents();
    CHECK(airfoilTabs->currentIndex()==0 && stationTabs->currentIndex()==0 && controlTabs->currentIndex()==1);
    CHECK(!window.projectModified());
    auto incomplete=panelsProject;incomplete.stations.lines.pop_back();incomplete.stations.selected=-1;
    CHECK(writeProject(filename,incomplete,error));CHECK(window.openProjectFile(filename,error));QApplication::processEvents();
    CHECK(component->actions()[1]->isEnabled() && !component->actions()[2]->isEnabled());
    CHECK(!workspace->actions()[2]->isEnabled());
    auto legacyPanels=encodeProject(panelsProject);legacyPanels["version"]=4;
    auto legacyControls=legacyPanels["controlSurfaces"].toObject();legacyControls["surfaces"]=legacyControls["panels"].toArray()[1];legacyPanels["controlSurfaces"]=legacyControls;
    const auto migrated=decodeProject(legacyPanels);CHECK(migrated.controls.panels[0][1].enabled && !migrated.controls.panels[1][1].enabled);
    const auto tabsCapture=qEnvironmentVariable("FOAM_PANEL_EDITORS_CAPTURE");
    if(!tabsCapture.isEmpty()){CHECK(writeProject(filename,panelsProject,error));CHECK(window.openProjectFile(filename,error));QApplication::processEvents();CHECK(window.grab().save(tabsCapture));
      component->actions()[1]->trigger();QApplication::processEvents();CHECK(window.grab().save(tabsCapture+".stations.png"));
      component->actions()[2]->trigger();QApplication::processEvents();CHECK(window.grab().save(tabsCapture+".airfoils.png"));}
}
int main(int argc,char** argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app{argc,argv};
  app.setOrganizationName("FoamProjectTests");app.setApplicationName("Isolated");
  QTemporaryDir dir;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
  try {
    const auto smokeFile=qEnvironmentVariable("FOAM_PROJECT_SMOKE_FILE");
    if(!smokeFile.isEmpty()) {
      MainWindow window;window.show();QApplication::processEvents();QString error;
      QObject::connect(window.statusBar(),&QStatusBar::messageChanged,[](const QString& text){std::cout<<text.toStdString()<<std::endl;});
      CHECK(window.openProjectFile(smokeFile,error));
      window.findChild<QTabWidget*>("viewportTabs")->setCurrentIndex(1);
      QApplication::processEvents();waitForModel(window);
      CHECK(window.findChild<QTabWidget*>("viewportTabs")->widget(1)->property("wingModelReady").toBool());
      const auto capture=qEnvironmentVariable("FOAM_PROJECT_CAPTURE");
      if(!capture.isEmpty()) {
        CHECK(window.screen()->grabWindow(window.winId()).save(capture));
        auto* solid=static_cast<OcctViewport*>(window.findChild<QTabWidget*>("viewportTabs")->widget(1));
        solid->setCameraView(CameraView::Front);QApplication::processEvents();
        CHECK(window.screen()->grabWindow(window.winId()).save(capture+".front.png"));
      }
      CHECK(!window.projectModified());
      std::cout<<"Project opened, generated and displayed in 3D without changing saved inputs.\n";return 0;
    }
    if(qEnvironmentVariableIsSet("FOAM_PANEL_ONLY")){MainWindow window;window.show();QApplication::processEvents();panelWorkflow(window,dir.filePath("panels.foam"));return 0;}
    auto p=fixture();QString error;const auto filename=dir.filePath("complete.foam");
    CHECK(writeProject(filename,p,error));auto loaded=readProject(filename,error);CHECK(loaded);
    CHECK(loaded->reference.image.pages.size()==2);
    CHECK(loaded->reference.image.pages[1].pixels.pixelColor(5,5)==QColor{Qt::green});
    CHECK(loaded->reference.image.path==p.reference.image.path);
    CHECK(loaded->reference.units==ProjectUnits::Inches&&loaded->reference.wingspanMm==p.reference.wingspanMm);
    CHECK(loaded->wing.layers[0].curves.size()==3&&loaded->stations.lines[1].airfoil==1);
    CHECK(loaded->airfoilSketch.pending==p.airfoilSketch.pending&&loaded->airfoils.draftName==p.airfoils.draftName);
    CHECK(loaded->airfoils.entries[0].name==p.airfoils.entries[0].name);
    CHECK(loaded->spars[0][0].enabled && loaded->spars[0][0].shape==SparShape::Strip);
    auto mixed=*loaded;mixed.spars[0][0].sizeText=".25 in";mixed.spars[0][0].heightText="1.5 mm";
    mixed.wingspanText="1000 mm";
    const auto mixedRoundTrip=decodeProject(encodeProject(mixed));
    CHECK(mixedRoundTrip.spars[0][0].sizeText==".25 in");
    CHECK(mixedRoundTrip.spars[0][0].heightText=="1.5 mm");
    CHECK(mixedRoundTrip.wingspanText=="1000 mm");
    CHECK(loaded->spars[0][0].chordPercent==27.5 && loaded->spars[0][0].sizeMm==6.35 && loaded->spars[0][0].heightMm==1.5);
    CHECK(loaded->spars[0][2].enabled && loaded->spars[0][2].lengthPercent==75);
    auto old=encodeProject(p);old["version"]=2;old.remove("spars");{auto c=old["controlSurfaces"].toObject();c["surfaces"]=c["panels"].toArray()[0];old["controlSurfaces"]=c;}CHECK(!decodeProject(old).spars[0][0].enabled);
    auto malformed=encodeProject(p);auto invalidSpars=malformed["spars"].toArray();auto firstSpars=invalidSpars[0].toArray();auto mid=firstSpars[2].toObject();mid["shape"]=1;firstSpars[2]=mid;invalidSpars[0]=firstSpars;malformed["spars"]=invalidSpars;
    bool invalidSpar=false;try{decodeProject(malformed);}catch(const std::exception&){invalidSpar=true;}CHECK(invalidSpar);
    auto v3=encodeProject(p);v3["version"]=3;{auto c=v3["controlSurfaces"].toObject();c["surfaces"]=c["panels"].toArray()[0];v3["controlSurfaces"]=c;}v3["spars"]=v3["spars"].toArray()[0];
    CHECK(decodeProject(v3).spars[0][2].sizeMm==3.175);
    auto multi=p;multi.wing.layers.push_back(multi.wing.layers[0]);multi.spars.resize(2);multi.dihedralDegrees.resize(2);multi.controls.panels.resize(2);multi.airfoils.panelChoices={0,1};
    multi.airfoils.panel=1;multi.selectedStationPanel=1;multi.controls.panel=1;
    multi.controls.panels[1][1]={true,HingeCut::Standard,QRectF{80,100,40,50}};
    multi.spars[1][1]={true,SparShape::Round,60,40,4,1};multi.selectedSparPanel=1;
    auto multiCopy=decodeProject(encodeProject(multi));CHECK(multiCopy.spars.size()==2);
    CHECK(multiCopy.airfoils.panel==1 && multiCopy.selectedStationPanel==1 && multiCopy.airfoils.panelChoices[0]==0);
    CHECK(multiCopy.controls.panel==1 && multiCopy.controls.panels[1][1].enabled && !multiCopy.controls.panels[0][1].enabled);
    CHECK(multiCopy.spars[1][1].chordPercent==60 && multiCopy.selectedSparPanel==1);
    auto wrongCount=encodeProject(multi);wrongCount["spars"]=QJsonArray{};invalidSpar=false;
    try{decodeProject(wrongCount);}catch(const std::exception&){invalidSpar=true;}CHECK(invalidSpar);
    auto angled=*loaded;angled.dihedralDegrees={4.5};angled.tool="Dihedral";angled.airfoils.sketching=false;angled.airfoilSketch.pending.clear();angled.airfoilSketch.editing=false;
    const auto angledRoundTrip=decodeProject(encodeProject(angled));
    CHECK(angledRoundTrip.dihedralDegrees==std::vector<double>{4.5});
    auto oldTip=encodeProject(angled);oldTip["version"]=6;oldTip["wingTip"]=2;oldTip.remove("dihedralDegrees");
    auto oldUi=oldTip["ui"].toObject();oldUi["tool"]="Wing Tip";oldUi.remove("dihedralPanel");oldTip["ui"]=oldUi;
    const auto migrated=decodeProject(oldTip);CHECK(migrated.dihedralDegrees==std::vector<double>{0});CHECK(migrated.tool=="Dihedral");
    auto badAngles=encodeProject(angled);badAngles["dihedralDegrees"]=QJsonArray{0,1};bool badCount=false;
    try{decodeProject(badAngles);}catch(const std::exception&){badCount=true;}CHECK(badCount);
    auto lightProject=*loaded;lightProject.lightening.enabled=true;lightProject.lightening.wallMm=3.175;
    lightProject.lightening.text[0]=".125 in";lightProject.lightening.crossmembers=6;lightProject.lightening.startMm=0;
    const auto lightCopy=decodeProject(encodeProject(lightProject));CHECK(lightCopy.lightening.enabled && lightCopy.lightening.crossmembers==6);
    CHECK(lightCopy.lightening.text[0]==".125 in" && lightCopy.lightening.wallMm==3.175 && lightCopy.lightening.startMm==0);
    auto oldLight=encodeProject(lightProject);oldLight["version"]=7;oldLight.remove("lightening");CHECK(!decodeProject(oldLight).lightening.enabled);
    auto canonical=encodeProject(*loaded);CHECK(decodeProject(canonical).airfoils.sketching);
    // Version 1 opens with controls disabled; version 2 preserves disabled
    // rectangles, hinge settings and an unfinished rectangle's meaning.
    auto legacy=canonical;legacy["version"]=1;legacy.remove("controlSurfaces");
    CHECK(!decodeProject(legacy).controls.panels[0][0].enabled);
    auto controls=p;controls.airfoils.sketching=false;controls.tool="Ailerons/Flaps";
    controls.controls.panels[0][0]={true,HingeCut::Standard,QRectF{30,60,40,50}};
    controls.controls.panels[0][1]={false,HingeCut::Tape,QRectF{10,60,10,50}};
    controls.controls.drawing=0;controls.controls.first=QPointF{31,61};
    auto controlRoundTrip=decodeProject(encodeProject(controls));
    CHECK(controlRoundTrip.controls.panels[0][0].enabled);
    CHECK(controlRoundTrip.controls.panels[0][0].hinge==HingeCut::Standard);
    CHECK(controlRoundTrip.controls.panels[0][1].rectangle==controls.controls.panels[0][1].rectangle);
    CHECK(controlRoundTrip.controls.first==controls.controls.first);
    auto badControls=encodeProject(controls);
    auto badState=badControls["controlSurfaces"].toObject();auto badPanels=badState["panels"].toArray();auto badSurfaces=badPanels[0].toArray();
    auto badSurface=badSurfaces[0].toObject();badSurface["hinge"]=9;badSurfaces[0]=badSurface;
    badPanels[0]=badSurfaces;badState["panels"]=badPanels;badControls["controlSurfaces"]=badState;
    bool badHingeRejected=false;try{decodeProject(badControls);}catch(const std::exception&){badHingeRejected=true;}
    CHECK(badHingeRejected);
    auto invalid=canonical;invalid["version"]=9;bool rejected=false;
    try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    invalid=canonical;auto stationObject=invalid["stations"].toObject();auto lines=stationObject["lines"].toArray();
    auto line=lines[0].toObject();line["airfoil"]=99;lines[0]=line;stationObject["lines"]=lines;invalid["stations"]=stationObject;
    rejected=false;try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    invalid=canonical;auto wing=invalid["wingOutline"].toObject();wing["active"]=999;invalid["wingOutline"]=wing;
    rejected=false;try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    auto actual=p;actual.reference.toScale=true;actual.reference.units=ProjectUnits::Millimeters;
    CHECK(decodeProject(encodeProject(actual)).reference.toScale);
    // Exercise the real Open path with a version-1 file; Save As upgrades it.
    {QFile old{filename};CHECK(old.open(QIODevice::WriteOnly));old.write(QJsonDocument{legacy}.toJson());}
    // UI round trip preserves active airfoil draft, selected station, tools and zoom.
    MainWindow window;window.show();QApplication::processEvents();waitForModel(window);
    CHECK(!window.projectModified());
    chooseFile(filename);window.findChild<QAction*>("projectOpen")->trigger();QApplication::processEvents();waitForModel(window);
    auto restored=window.projectDocument();
    CHECK(restored.workspace==1&&restored.tool=="Airfoils"&&restored.viewport==0&&restored.dihedralDegrees==std::vector<double>{0});
    CHECK(restored.stations.selected==1&&restored.stations.lines[0].airfoil==0);
    CHECK(restored.airfoils.sketching&&restored.airfoilSketch.pending==p.airfoilSketch.pending);
    CHECK(window.findChild<QPushButton*>("sketchAirfoil")->isChecked());
    CHECK(!window.findChild<QRadioButton*>("airfoilChoice0")->isEnabled());
    CHECK(std::abs(restored.plan.zoom-p.plan.zoom)<1e-10);CHECK(!window.projectModified());
    auto* planView=static_cast<PlanViewport*>(window.findChild<QGraphicsView*>());
    CHECK((QLineF{planView->mapToScene(planView->viewport()->rect().center()),p.plan.center}.length()<1));
    const auto second=dir.filePath("copy.foam");chooseFile(second);window.findChild<QAction*>("projectSaveAs")->trigger();
    CHECK(QFile::exists(second)&&!window.projectModified());
    // Invalid Open is transactional and does not discard the current draft.
    const auto badFile=dir.filePath("bad.foam");{QFile file{badFile};CHECK(file.open(QIODevice::WriteOnly));file.write("{broken");}
    CHECK(!window.openProjectFile(badFile,error));CHECK(window.projectDocument().airfoils.sketching);
    window.findChild<QLineEdit*>("referenceWingspan")->setText("40");CHECK(window.projectModified());
    response(QMessageBox::Cancel);window.findChild<QAction*>("projectNew")->trigger();CHECK(window.projectDocument().airfoils.sketching);
    response(QMessageBox::Cancel);CHECK(!window.close());CHECK(window.isVisible());
    // Save failure leaves the old file and dirty state intact.
    CHECK(!window.saveProjectFile(dir.filePath("missing/subdir/project.foam"),error));CHECK(window.projectModified());
    CHECK(!QApplication::overrideCursor());
    CHECK(window.statusBar()->currentMessage().startsWith("Save failed:"));
    response(QMessageBox::Save);window.findChild<QAction*>("projectClose")->trigger();
    CHECK(!window.findChild<QAction*>("projectSave")->isEnabled());CHECK(!window.centralWidget()->isEnabled());
    CHECK(readProject(second,error)->reference.wingspanMm==40*25.4);
    window.findChild<QAction*>("projectNew")->trigger();QApplication::processEvents();CHECK(window.centralWidget()->isEnabled());
    CHECK(window.projectDocument().airfoils.entries.empty());
    window.findChild<QLineEdit*>("referenceWingspan")->setText("123");
    QApplication::processEvents();CHECK(window.projectModified());
    QTimer::singleShot(0,[]{
      auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());if(!box)qFatal("Expected save prompt");
      QTimer::singleShot(0,[]{
        auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());if(!dialog)qFatal("Expected Save As");dialog->reject();
      });
      box->button(QMessageBox::Save)->click();
    });
    window.findChild<QAction*>("projectClose")->trigger();
    CHECK(window.centralWidget()->isEnabled()&&window.projectDocument().wingspanText=="123");
    response(QMessageBox::Discard);window.findChild<QAction*>("projectNew")->trigger();QApplication::processEvents();
    // Lightening lifecycle and dirty tracking, without unnecessary geometry work.
    auto* lightCheck=window.findChild<QCheckBox*>("lighteningEnabled");
    lightCheck->setChecked(true);auto* lightWall=window.findChild<QLineEdit*>("lighteningWall");
    lightWall->setText(".1 in");QMetaObject::invokeMethod(lightWall,"editingFinished");
    CHECK(window.projectModified() && window.projectDocument().lightening.enabled);
    CHECK(window.saveProjectFile(filename,error));CHECK(!window.projectModified());
    window.findChild<QAction*>("projectNew")->trigger();CHECK(!window.projectDocument().lightening.enabled && !window.projectModified());
    CHECK(window.openProjectFile(filename,error));CHECK(lightCheck->isChecked() && lightWall->text()==".1 in");CHECK(!window.projectModified());
    // Restore a 3D view and its camera after automatic model regeneration.
    p=fixture();p.airfoils.sketching=false;p.airfoilSketch.pending.clear();p.airfoilSketch.editing=false;
    p.tool="Dihedral";p.viewport=1;p.camera=CameraState{{300,-400,200},{100,0,0},{0,0,1},800,45,0};
    CHECK(writeProject(filename,p,error));if(window.projectModified())response(QMessageBox::Discard);
    CHECK(window.openProjectFile(filename,error));QApplication::processEvents();waitForModel(window);
    restored=window.projectDocument();CHECK(restored.viewport==1&&restored.camera);
    CHECK(std::abs(restored.camera->scale-800)<1e-7);CHECK(std::abs(restored.camera->eye[0]-300)<1e-7);
    CHECK(!window.projectModified());
    CHECK(window.findChild<QTabWidget*>("viewportTabs")->widget(1)->property("wingModelReady").toBool());
    // Cancellation leaves source edits and the displayed model/camera intact,
    // while every GUI control except Cancel is disabled, including shortcuts.
    const int beforeCancel=window.findChild<QTabWidget*>("viewportTabs")->widget(1)->property("wingModelRevision").toInt();
    auto* angle=window.findChild<QDoubleSpinBox*>("rootDihedral");angle->setValue(2);
    QApplication::processEvents();CHECK(window.property("modelProcessing").toBool());
    auto* cancel=window.findChild<QPushButton*>("cancelProcessing");CHECK(cancel->isVisible() && cancel->isEnabled());
    CHECK(!angle->isEnabled() && !window.menuBar()->isEnabled());
    CHECK(!window.findChild<QTabWidget*>("viewportTabs")->isEnabled());
    for(auto* toolbar:window.findChildren<QToolBar*>())CHECK(!toolbar->isEnabled());
    for(auto* action:window.findChildren<QAction*>())CHECK(!action->isEnabled());
    const auto busyCapture=qEnvironmentVariable("FOAM_PROCESSING_CAPTURE");if(!busyCapture.isEmpty())CHECK(window.grab().save(busyCapture));
    cancel->click();waitForModel(window);
    CHECK(window.statusBar()->currentMessage().contains("cancelled"));CHECK(!cancel->isVisible());
    CHECK(window.menuBar()->isEnabled() && angle->isEnabled());CHECK(!QApplication::overrideCursor());
    CHECK(window.projectDocument().dihedralDegrees[0]==2 && window.projectModified());
    CHECK(window.findChild<QTabWidget*>("viewportTabs")->widget(1)->property("wingModelRevision").toInt()==beforeCancel);
    CHECK(std::abs(window.projectDocument().camera->scale-800)<1e-7);
    auto* cancelledTools=window.findChild<QToolBar*>("componentToolBar");
    cancelledTools->actions()[0]->trigger();QApplication::processEvents();
    CHECK(!window.property("modelProcessing").toBool());
    cancelledTools->actions()[3]->trigger();QApplication::processEvents();
    CHECK(!window.property("modelProcessing").toBool());
    // Regeneration can be retried after cancellation and preserves the camera.
    angle->setValue(0);waitForModel(window);CHECK(window.findChild<QTabWidget*>("viewportTabs")->widget(1)->property("wingModelReady").toBool());
    const auto capture=qEnvironmentVariable("FOAM_PROJECT_CAPTURE");
    if(!capture.isEmpty())CHECK(window.screen()->grabWindow(window.winId()).save(capture));
    auto* viewportTabs=window.findChild<QTabWidget*>("viewportTabs");
    viewportTabs->setCurrentIndex(0);QApplication::processEvents();
    CHECK(std::abs(window.projectDocument().plan.zoom-p.plan.zoom)<1e-10);
    CHECK((QLineF{planView->mapToScene(planView->viewport()->rect().center()),p.plan.center}.length()<1));
    CHECK(window.saveProjectFile(filename,error));
    // A camera saved while 2D is selected survives the deferred first 3D build.
    p.viewport=0;CHECK(writeProject(filename,p,error));CHECK(window.openProjectFile(filename,error));
    QApplication::processEvents();waitForModel(window);CHECK(!window.projectModified());
    window.findChild<QTabWidget*>("viewportTabs")->setCurrentIndex(1);waitForModel(window);
    restored=window.projectDocument();CHECK(restored.camera&&std::abs(restored.camera->scale-800)<1e-7);
    CHECK(!window.projectModified()); // Entering 3D did not change project data.
    window.findChild<QAction*>("projectNew")->trigger();QApplication::processEvents();waitForModel(window);
    auto* workspace = window.findChild<QToolBar*>("workspaceToolBar");
    CHECK(workspace->actions().front()->isEnabled());
    CHECK(workspace->actions().front()->isChecked());
    for(int i=1;i<workspace->actions().size();++i) CHECK(!workspace->actions()[i]->isEnabled());
    CHECK(!QApplication::overrideCursor());
    CHECK(!window.statusBar()->currentMessage().contains("under development"));
    CHECK(window.projectDocument().plan.zoom!=p.plan.zoom);CHECK(!window.projectModified());
    CHECK(writeProject(filename,controls,error));CHECK(window.openProjectFile(filename,error));QApplication::processEvents();
    CHECK(window.findChild<QWidget*>("controlSurfacePanel")->isVisible());
    CHECK(window.findChild<QCheckBox*>("addAilerons")->isChecked());
    CHECK(!window.findChild<QCheckBox*>("addFlaps")->isChecked());
    CHECK(window.findChild<QRadioButton*>("AileronsStandardHinge")->isChecked());
    CHECK(window.projectDocument().controls.first==controls.controls.first);
    CHECK(window.findChild<QCheckBox*>("sparTop")->isChecked());
    CHECK(window.projectDocument().spars[0][0].sizeMm==6.35);
    CHECK(!window.projectModified());
    const auto controlCapture=qEnvironmentVariable("FOAM_CONTROLS_CAPTURE");
    if(!controlCapture.isEmpty())CHECK(window.screen()->grabWindow(window.winId()).save(controlCapture));
    const auto controlFile=dir.filePath("controls.foam");chooseFile(controlFile);
    window.findChild<QAction*>("projectSaveAs")->trigger();
    CHECK(readProject(controlFile,error)->controls.panels[0][0].rectangle==controls.controls.panels[0][0].rectangle);
    window.findChild<QAction*>("projectClose")->trigger();
    window.findChild<QAction*>("projectNew")->trigger();
    CHECK(!window.projectDocument().controls.panels[0][0].enabled);
    CHECK(!window.projectDocument().controls.panels[0][0].rectangle);
    CHECK(!window.projectDocument().spars[0][0].enabled && !window.projectDocument().spars[0][2].enabled);
    CHECK(window.close());
    {
      MainWindow closing;closing.show();p.viewport=1;
      CHECK(writeProject(filename,p,error));CHECK(closing.openProjectFile(filename,error));
      CHECK(closing.property("modelProcessing").toBool());CHECK(!closing.close());
      waitForModel(closing);CHECK(!closing.isVisible());CHECK(!QApplication::overrideCursor());
    }
    {
      MainWindow replacement;replacement.show();CHECK(replacement.openProjectFile(filename,error));
      CHECK(replacement.property("modelProcessing").toBool());
      auto empty=ProjectDocument{};CHECK(writeProject(filename,empty,error));CHECK(replacement.openProjectFile(filename,error));
      waitForModel(replacement);CHECK(!replacement.projectModified());
      CHECK(!replacement.findChild<QTabWidget*>("viewportTabs")->widget(1)->property("wingModelReady").toBool());
      CHECK(replacement.findChild<QToolBar*>("workspaceToolBar")->actions().front()->isEnabled());
      CHECK(!replacement.findChild<QToolBar*>("workspaceToolBar")->actions()[1]->isEnabled());
    }
    {
      MainWindow failed;failed.show();p.lightening.enabled=true;p.lightening.startMm=10000;
      CHECK(writeProject(filename,p,error));CHECK(failed.openProjectFile(filename,error));waitForModel(failed);
      CHECK(failed.statusBar()->currentMessage().startsWith("Wing generation failed:"));
      CHECK(failed.menuBar()->isEnabled());CHECK(!failed.findChild<QPushButton*>("cancelProcessing")->isVisible());
      CHECK(!failed.projectModified() && !QApplication::overrideCursor());
    }
    panelWorkflow(window,filename);
    std::cout<<"Project data, embedded images, drafts, UI/camera restoration, file actions, cancellation and failures passed.\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}


#include "gui/MainWindow.h"
#include "gui/SparPanel.h"
#include "gui/DihedralPanel.h"
#include "gui/LighteningPanel.h"
#include "gui/ProcessingScope.h"

#include "gui/OcctViewport.h"
#include "gui/PlanViewport.h"
#include "gui/ReferencePanel.h"
#include "gui/ReferenceWorkflow.h"
#include "gui/WingOutlinePanel.h"
#include "gui/FuselageOutlinePanel.h"
#include "gui/FuselageProfilePanel.h"
#include "gui/FuselageThickenPanel.h"
#include "gui/FuselageCutPanel.h"
#include "gui/ServoTrayPanel.h"
#include "gui/FormerPanel.h"
#include "gui/SketchBoundary.h"
#include <TopExp_Explorer.hxx>
#include <QLineF>
#include <limits>
#include "geometry/FuselageSolidBuilder.h"
#include "gui/AirfoilPanel.h"
#include "gui/ControlSurfacePanel.h"
#include "geometry/WingSolidBuilder.h"
#include "gui/FileSelectionDialog.h"
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSignalBlocker>
#include <Standard_Failure.hxx>
#include <QButtonGroup>
#include <QPushButton>
#include <QTimer>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QKeySequence>
#include <QLineEdit>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTabBar>
#include <QTextEdit>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>

#include <array>

namespace designrc::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow{parent} {
  setWindowTitle("FoamAirplaneStudio");
  resize(1400, 860);

  auto* splitter = new QSplitter{this};
  dataPanel_ = new QWidget{splitter};
  dataPanel_->setObjectName("dataPanel");
  dataPanel_->setMinimumWidth(240);
  auto* panelLayout=new QVBoxLayout{dataPanel_};panelLayout->setContentsMargins(0,0,0,0);
  dataContents_=new QWidget{dataPanel_};panelLayout->addWidget(dataContents_,1);
  auto* dataLayout = new QVBoxLayout{dataContents_};
  dataLayout->setContentsMargins(8, 8, 8, 8);

  graphicsTabs_ = new QTabWidget{splitter};
  graphicsTabs_->setObjectName("viewportTabs");
  planViewport_ = new PlanViewport{graphicsTabs_};
  viewport_ = new OcctViewport{graphicsTabs_};
  graphicsTabs_->addTab(planViewport_, "2D View");
  graphicsTabs_->addTab(viewport_, "3D View");
  referencePanel_ = new ReferencePanel{dataPanel_};
  dataLayout->addWidget(referencePanel_);
  wingOutlinePanel_ = new WingOutlinePanel{planViewport_->sketchEditor(), dataPanel_};
  dataLayout->addWidget(wingOutlinePanel_);
  fuselageOutlinePanel_=new FuselageOutlinePanel{planViewport_->fuselageSketchEditor(),dataPanel_};
  dataLayout->addWidget(fuselageOutlinePanel_);
  fuselageOutlinePanel_->drawingRequested=[this]{graphicsTabs_->setCurrentWidget(planViewport_);};
  fuselageStationPanel_=new QWidget{dataPanel_};
  fuselageStationPanel_->setObjectName("fuselageProfileStationsPanel");
  auto* fuselageStationLayout=new QVBoxLayout{fuselageStationPanel_};
  fuselageStationLayout->setContentsMargins(0,0,0,0);
  auto* fuselageStationText=new QLabel{
      "Place fuselage cross-section profiles along the Side View outline. Hover near its top or bottom edge "
      "to see a green floating point and a vertical preview between the two edges. Left-click once to place "
      "the profile station, then move to the next location. Start near the nose and add stations where the shape changes. "
      "Only vertical stations with two distinct boundary intersections can be placed; ambiguous or zero-height locations are ignored. "
      "Click a station line to select it; Delete removes it. Click an endpoint, move along the outline, then "
      "left-click or press Escape to drop it. Escape clears selection. The outlines remain locked in this mode.",fuselageStationPanel_};
  fuselageStationText->setObjectName("fuselageProfileStationInstructions");
  fuselageStationText->setWordWrap(true);fuselageStationLayout->addWidget(fuselageStationText);fuselageStationLayout->addStretch();
  dataLayout->addWidget(fuselageStationPanel_);
  fuselageProfilePanel_=new FuselageProfilePanel{*planViewport_,planViewport_->fuselageSketchEditor(),planViewport_->fuselageProfileEditor(),dataPanel_};
  dataLayout->addWidget(fuselageProfilePanel_);
  formerPanel_=new FormerPanel{planViewport_->formerEditor(),dataPanel_};
  dataLayout->addWidget(formerPanel_);
  connect(&planViewport_->formerEditor(),&FormerEditor::changed,this,[this]{if(!restoringProject_)QTimer::singleShot(0,this,[this]{updateFuselageModel();});});
  connect(&planViewport_->formerEditor(),&FormerEditor::message,this,[this](const QString& text){if(!text.isEmpty())statusBar()->showMessage(text);});
  connect(&planViewport_->servoTrayEditor(),&ServoTrayEditor::message,this,[this](const QString& text){if(!text.isEmpty())statusBar()->showMessage(text);});
  servoTrayPanel_=new ServoTrayPanel{planViewport_->servoTrayEditor(),dataPanel_};
  dataLayout->addWidget(servoTrayPanel_);
  connect(&planViewport_->servoTrayEditor(),&ServoTrayEditor::changed,this,[this]{
    if(!restoringProject_)QTimer::singleShot(0,this,[this]{updateFuselageModel();});
  });
  fuselageCutPanel_=new FuselageCutPanel{planViewport_->fuselageCutEditor(),dataPanel_};
  dataLayout->addWidget(fuselageCutPanel_);
  connect(&planViewport_->fuselageCutEditor(),&SketchEditor::changed,this,[this]{
    if(!restoringProject_)QTimer::singleShot(0,this,[this]{updateFuselageModel();});
  });
  fuselageThickenPanel_=new FuselageThickenPanel{planViewport_->fuselageSketchEditor(),dataPanel_};
  dataLayout->addWidget(fuselageThickenPanel_);
  fuselageThickenPanel_->changed=[this]{
    if(restoringProject_)return;
    statusBar()->showMessage("Fuselage wall thickness updated; open 3D View to regenerate the hollow body.");
    QTimer::singleShot(0,this,[this]{updateFuselageModel();});
  };
  stationPanel_ = new QWidget{dataPanel_};
  stationPanel_->setObjectName("airfoilStationsPanel");
  auto* stationLayout = new QVBoxLayout{stationPanel_};
  stationLayout->setContentsMargins(0, 0, 0, 0);
  stationTabs_=new QTabBar{stationPanel_};stationTabs_->setObjectName("stationPanelTabs");stationTabs_->addTab("1");stationLayout->addWidget(stationTabs_);
  connect(stationTabs_,&QTabBar::currentChanged,this,[this](int panel){if(panel>=0)planViewport_->sketchEditor().stationEditor().setActivePanel(panel);});
  auto* stationText = new QLabel{"Enter lines between the leading edge and trailing edge for the location of the airfoil profiles.  These will be used to interpolate the shape of the wing.  They usually only need to go at the root and tip rib locations.  Click on the LE first, then the TE to locate the station.", stationPanel_};
  stationText->setWordWrap(true); stationLayout->addWidget(stationText);
  auto* stationHint = new QLabel{"Click a station to select it; Delete removes it. Click an endpoint, move along its outline, then left-click or press Escape to drop it. Escape also cancels incomplete placement and clears selection.", stationPanel_};
  stationHint->setWordWrap(true); stationLayout->addWidget(stationHint); stationLayout->addStretch();
  dataLayout->addWidget(stationPanel_);
  airfoilPanel_ = new AirfoilPanel{*planViewport_, dataPanel_};
  dataLayout->addWidget(airfoilPanel_);
  dihedralPanel_=new DihedralPanel{dataPanel_};dataLayout->addWidget(dihedralPanel_);
  dihedralPanel_->changed=[this]{invalidateWing();};
  controlSurfacePanel_=new ControlSurfacePanel{planViewport_->controlSurfaceEditor(),dataPanel_};
  dataLayout->addWidget(controlSurfacePanel_);
  sparPanel_=new SparPanel{dataPanel_};dataLayout->addWidget(sparPanel_);
  sparPanel_->changed=[this]{invalidateWing();};
  lighteningPanel_=new LighteningPanel{dataPanel_};dataLayout->addWidget(lighteningPanel_);
  lighteningPanel_->changed=[this]{invalidateWing();};
  planViewport_->controlSurfaceEditor().changed=[this]{invalidateWing();};
  planViewport_->controlSurfaceEditor().drawingRequested=[this]{graphicsTabs_->setCurrentWidget(planViewport_);};
  connect(referencePanel_, &ReferencePanel::referenceChanged, this, [this] {
    if(restoringProject_) return;
    const auto& reference = projectReference();
    sparPanel_->setUnits(reference.units);lighteningPanel_->setUnits(reference.units);
    fuselageThickenPanel_->setUnits(reference.units);
    // Unscaled images retain pixel coordinates until outlines can be calibrated
    // against the aircraft dimensions. Page width is not aircraft wingspan.
    planViewport_->setReferenceBackground(reference.image.pages, reference.toScale);
    updateWorkspaceAvailability();
    invalidateWing();
  });
  splitter->addWidget(dataPanel_);
  splitter->addWidget(graphicsTabs_);
  splitter->setChildrenCollapsible(false);
  splitter->setSizes({360, 1040});
  splitter->setStretchFactor(1, 1);
  setCentralWidget(splitter);

  buildMenus();
  buildToolBars();
  updateWorkspaceAvailability();
  const auto updateWingProgress = [this] {
    if(restoringProject_) return;
    invalidateWing();
    updatePanelCounts();
    const bool defined = wingOutlinePanel_->outlinesDefined();
    const bool stationsDefined = defined &&
        planViewport_->sketchEditor().stationEditor().allPanelsDefined();
    const bool airfoilsDefined = stationsDefined && airfoilPanel_->allStationsAssigned();
    if (wingDefinitions_.outlineDefined == defined && wingDefinitions_.stationsDefined == stationsDefined &&
        wingDefinitions_.airfoilsDefined == airfoilsDefined) return;
    wingDefinitions_.outlineDefined = defined;
    wingDefinitions_.stationsDefined = stationsDefined;
    wingDefinitions_.airfoilsDefined = airfoilsDefined;
    wingDefinitions_.dihedralDefined = airfoilsDefined; // Zero degrees is a complete default.
    updateWorkspaceAvailability();
    if (dataPanel_->property("workspaceIndex").toInt() == 1) {
      const QString current = dataPanel_->property("activeTool").toString();
      const int next = applyWingWorkflow(*componentToolBar_, wingDefinitions_);
      QString selected = componentToolBar_->actions().at(next)->text();
      for (auto* action : componentToolBar_->actions())
        if (action->text() == current && action->isEnabled()) {
          action->setChecked(true); selected = current; break;
        }
      dataPanel_->setProperty("activeTool", selected);
      updateEditorVisibility();
    }
  };
  connect(&planViewport_->sketchEditor(), &SketchEditor::changed, this, updateWingProgress);
  connect(&planViewport_->sketchEditor(), &SketchEditor::stationsChanged, this, updateWingProgress);
  connect(airfoilPanel_, &AirfoilPanel::libraryChanged, this, updateWingProgress);
  connect(&planViewport_->fuselageSketchEditor(),&SketchEditor::changed,this,[this]{
    if(!restoringProject_)updateFuselageProgress();
  });
  connect(&planViewport_->fuselageSketchEditor(),&SketchEditor::stationsChanged,this,[this]{if(!restoringProject_)updateFuselageProgress();});
  connect(&planViewport_->fuselageProfileEditor(),&SketchEditor::changed,this,[this]{if(!restoringProject_)updateFuselageProgress();});
  connect(graphicsTabs_, &QTabWidget::currentChanged, this, [this](int index) {
    const bool fuselageOutline=dataPanel_->property("workspaceIndex").toInt()==2 && dataPanel_->property("activeTool").toString()=="Outline";
    fuselageOutlinePanel_->setActive(fuselageOutline&&index==0,!restoringProject_);
    updateFuselageStationMode();
    if(index==1 && !restoringProject_){planViewport_->controlSurfaceEditor().finishEditing();wingDirty_=true;}
    statusBar()->showMessage(index == 0
        ? "Wheel: zoom  |  Scrollbars: scroll"
        : "Left drag: orbit  |  Right drag: pan  |  Wheel: zoom");
    updateWingModel();
  });
  cancelProcessing_=new QPushButton{"Cancel",dataPanel_};
  cancelProcessing_->setObjectName("cancelProcessing");panelLayout->addWidget(cancelProcessing_);
  cancelProcessing_->hide();
  connect(cancelProcessing_,&QPushButton::clicked,this,[this] {
    if(!modelJob_&&!fuselageJob_)return;
    if(modelJob_)modelJob_->cancel();if(fuselageJob_)fuselageJob_->cancel();cancelProcessing_->setText("Cancelling...");cancelProcessing_->setEnabled(false);
    statusBar()->showMessage("Cancelling processing at the next safe point...");
  });
  auto* jobTimer=new QTimer{this};jobTimer->setInterval(40);
  connect(jobTimer,&QTimer::timeout,this,[this]{pollModelJob();});jobTimer->start();
  selectWorkspace(0);
  savedFingerprint_=projectFingerprint();
  auto* modifiedTimer=new QTimer{this}; modifiedTimer->setInterval(400);
  connect(modifiedTimer,&QTimer::timeout,this,[this]{if(!restoringProject_)updateProjectTitle();});
  modifiedTimer->start();
}

void MainWindow::buildToolBars() {
  workspaceToolBar_ = addToolBar("Design");
  workspaceToolBar_->setObjectName("workspaceToolBar");
  workspaceToolBar_->setMovable(false);
  auto* group = new QActionGroup{this};
  const std::array<const char*, 7> names{
      "Reference", "Wing", "Fuselage", "Horiz Stab", "Vert Stab", "Assembly", "Export"};
  for (int index = 0; index < static_cast<int>(names.size()); ++index) {
    auto* action = workspaceToolBar_->addAction(names[index]);
    action->setCheckable(true);
    group->addAction(action);
    if (index == 0) action->setChecked(true);
    action->setEnabled(index == 0);
    connect(action, &QAction::triggered, this, [this, index] { selectWorkspace(index); });
  }
  addToolBarBreak();
  componentToolBar_ = addToolBar("Component");
  componentToolBar_->setObjectName("componentToolBar");
  componentToolBar_->setMovable(false);
}

void MainWindow::selectWorkspace(int index) {
  componentToolBar_->clear();
  const std::array<QStringList, 7> tools{{
      {},
      {"Outline", "Airfoil Stations", "Airfoils", "Dihedral", "Ailerons/Flaps", "Spars", "Lightening"},
      {"Outline", "Profile Stations", "Edit Profiles", "Thicken", "Cut", "Servo Tray", "Formers"},
      {"Outline", "Airfoil Stations", "Airfoils", "Edit", "Hinge Line", "Cut"},
      {"Outline", "Airfoil Stations", "Airfoils", "Edit", "Hinge Line", "Cut"},
      {"Wing Location", "Horiz Stab Location", "Vert Stab Location"},
      {}
  }};
  referencePanel_->setVisible(index == 0);
  if (index == 0) graphicsTabs_->setCurrentWidget(planViewport_);
  dataPanel_->setProperty("workspaceIndex", index);
  dataPanel_->setProperty("activeTool", QString{});
  auto* fuselageGroup = index==2 ? new QActionGroup{componentToolBar_} : nullptr;
  for (const auto& name : tools.at(index)) {
    auto* action = componentToolBar_->addAction(name);
    if(fuselageGroup){action->setCheckable(true);fuselageGroup->addAction(action);action->setEnabled(name=="Outline" || (name=="Profile Stations"&&fuselageOutlinePanel_->outlinesDefined()));}
    connect(action, &QAction::triggered, this, [this, name] {
      dataPanel_->setProperty("activeTool", name);
      if(dataPanel_->property("workspaceIndex").toInt()==2&&(name=="Outline"||name=="Profile Stations"||name=="Edit Profiles"||name=="Cut"||name=="Servo Tray"||name=="Formers"))graphicsTabs_->setCurrentWidget(planViewport_);
      if(dataPanel_->property("workspaceIndex").toInt()==2 && name=="Thicken")fuselageThickenPanel_->enter(fuselageWingLeadingEdge());
      const bool wingWorkspace=dataPanel_->property("workspaceIndex").toInt()==1;
      statusBar()->showMessage(wingWorkspace && name=="Outline"
          ? "Select Line or Spline to sketch; turn both off to move points"
          : wingWorkspace && name=="Airfoil Stations" ? "Click LE then TE; Left-click or Escape drops a move; Delete removes selected station"
          : wingWorkspace && name=="Airfoils" ? "Choose an airfoil and click a station to assign it"
          : wingWorkspace && name=="Ailerons/Flaps" ? "Draw opposite rectangle corners; when idle, click a rectangle to select; Delete removes it; Escape cancels drawing or selection"
          : wingWorkspace && name=="Dihedral" ? "Set panel root dihedral angles; open 3D View to see the wing"
          : wingWorkspace && name=="Spars" ? "Choose spar locations, percentages and sizes; open 3D to generate grooves and the mid-plane split"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Outline" ? "Choose Top View or Side View and trace a closed fuselage outline"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Profile Stations" ? "Hover on Side View top/bottom; left-click once to place a vertical profile station"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Edit Profiles" ? "Select a station; draw a closed section using Line or Spline; open 3D to generate the fuselage"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Thicken" ? "Set station wall thickness in Reference units, or enter mm/in; 3D generation now hollows the fuselage"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Cut" ? "Choose Top or Side View; draw a connected cut path, then open 3D to split the fuselage"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Formers" ? "Enter former thickness, Add Former, then drag its position or top/bottom edges; overlapping placements are blocked"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Servo Tray" ? "Enter tray width and height, then drag the rectangle into position on Side View; Thicken provides inner walls; supports extend 5 mm inward and down"
          : name + ": editor not implemented yet");
      updateEditorVisibility();
    });
  }
  if (index == 1) {
    const int selected = applyWingWorkflow(*componentToolBar_, wingDefinitions_);
    dataPanel_->setProperty("activeTool", tools.at(index).at(selected));
  }
  if(index==2) {
    componentToolBar_->actions().front()->setChecked(true);
    dataPanel_->setProperty("activeTool","Outline");
    if(!restoringProject_)graphicsTabs_->setCurrentWidget(planViewport_);
  }
  if(index==2)updateFuselageProgress();
  componentToolBar_->setVisible(!tools.at(index).empty());
  statusBar()->showMessage(index == 0 ? "Set the project reference and dimensions" : index == 1 ? "Wing workspace ready" : index==2 ? "Fuselage workspace ready" : "Stabilizer workspace available; modeling tools are not implemented yet");
  updateEditorVisibility();
}

void MainWindow::updatePanelCounts() {
  const int count=static_cast<int>(planViewport_->sketchEditor().layers().size());
  QSignalBlocker block{stationTabs_};
  while(stationTabs_->count()>count)stationTabs_->removeTab(stationTabs_->count()-1);
  while(stationTabs_->count()<count)stationTabs_->addTab(QString::number(stationTabs_->count()+1));
  if(stationTabs_->currentIndex()<0)stationTabs_->setCurrentIndex(0);
  airfoilPanel_->setPanelCount(count);sparPanel_->setPanelCount(count);dihedralPanel_->setPanelCount(count);
  planViewport_->controlSurfaceEditor().setPanelCount(count);
}
double MainWindow::fuselageWingLeadingEdge() const {
  // Prefer the Side View wing seat: a straight upper edge matching the root
  // chord. Plan-view span coordinates do not locate the wing on the fuselage.
  double chord=0,fallback=0;
  for(const auto& station:planViewport_->sketchEditor().stationEditor().lines())if(station.first.layer==0) {
    const double size=QLineF{station.first.position,station.second.position}.length();
    if(size>chord){chord=size;fallback=station.first.position.x();}
  }
  auto& source=planViewport_->fuselageSketchEditor();const auto& side=source.layers()[1];
  double best=.2,leading=fallback;
  for(const auto& curve:side.curves)if(curve.type==SketchTool::Line&&curve.points.size()==2&&chord>0) {
    const auto a=side.points[curve.points[0]],b=side.points[curve.points[1]];
    const double dx=std::abs(b.x()-a.x());if(dx<1e-8||std::abs(b.y()-a.y())/dx>.1)continue;
    const auto section=source.stationEditor().verticalSection((a.x()+b.x())*.5,1);
    if(!section||std::abs(section->first.position.y()-(a.y()+b.y())*.5)>1e-4)continue;
    const double difference=std::abs(dx-chord)/chord;
    if(difference<best){best=difference;leading=std::min(a.x(),b.x());}
  }
  if(best<.2) {
    // A traced station can sit just beside the wing-seat corner. Register the
    // nearest station within 2% of the chord as the inclusive LE boundary.
    double nearest=.02*chord;
    const double seatLeading=leading;
    for(const auto& station:source.stationEditor().lines()) {
      const double x=station.first.position.x(),distance=std::abs(x-seatLeading);
      if(distance<=nearest){nearest=distance;leading=x;}
    }
  }
  return leading;
}
void MainWindow::updateFuselageProgress() {
  fuselageThickenPanel_->synchronize(fuselageWingLeadingEdge());
  updateWorkspaceAvailability();
  if(dataPanel_->property("workspaceIndex").toInt()!=2)return;
  const bool ready=fuselageOutlinePanel_->outlinesDefined();
  const bool hasStations=ready&&!planViewport_->fuselageSketchEditor().stationEditor().lines().empty();
  const bool complete=hasStations&&fuselageProfilePanel_->allProfilesClosed();
  for(auto* action:componentToolBar_->actions()) {
    const bool enabled=action->text()=="Outline" || (action->text()=="Profile Stations"?ready:action->text()=="Edit Profiles"?hasStations:complete);
    action->setEnabled(enabled);
    if(!enabled&&action->isChecked()) {
      componentToolBar_->actions().front()->setChecked(true);
      dataPanel_->setProperty("activeTool","Outline");
      QTimer::singleShot(0,this,[this]{updateEditorVisibility();});
    }
  }
}
void MainWindow::updateFuselageStationMode() {
  const bool tray=dataPanel_->property("workspaceIndex").toInt()==2 && dataPanel_->property("activeTool").toString()=="Servo Tray";
  const auto reference=projectReference();
  const auto& layers=planViewport_->fuselageSketchEditor().layers();
  if(layers.size()>1)if(const auto boundary=closedSketchBoundary(layers[1])) {
    double left=boundary->front().x(),right=left,top=boundary->front().y(),bottom=top;
    for(auto point:*boundary){left=std::min(left,point.x());right=std::max(right,point.x());top=std::min(top,point.y());bottom=std::max(bottom,point.y());}
    const double scale=!reference.toScale&&reference.fuselageLengthMm&&right>left?*reference.fuselageLengthMm/(right-left):1.;
    servoTrayPanel_->configure(reference.units,scale,{(left+right)/2,(top+bottom)/2});
    formerPanel_->configure(reference.units,scale,QRectF{QPointF{left,top},QPointF{right,bottom}});
  }
  const bool formers=dataPanel_->property("workspaceIndex").toInt()==2&&dataPanel_->property("activeTool").toString()=="Formers";
  formerPanel_->setVisible(formers);formerPanel_->setEnabled(graphicsTabs_->currentIndex()==0);formerPanel_->setActive(formers&&graphicsTabs_->currentIndex()==0);
  servoTrayPanel_->setVisible(tray);servoTrayPanel_->setEnabled(graphicsTabs_->currentIndex()==0);
  servoTrayPanel_->setActive(tray&&graphicsTabs_->currentIndex()==0);
  const bool cut=dataPanel_->property("workspaceIndex").toInt()==2 && dataPanel_->property("activeTool").toString()=="Cut";
  fuselageCutPanel_->setVisible(cut);
  fuselageCutPanel_->setEnabled(graphicsTabs_->currentIndex()==0);
  fuselageCutPanel_->setActive(cut&&graphicsTabs_->currentIndex()==0);
  const bool active=dataPanel_->property("workspaceIndex").toInt()==2 &&
      dataPanel_->property("activeTool").toString()=="Profile Stations";
  fuselageStationPanel_->setVisible(active);
  fuselageThickenPanel_->setVisible(dataPanel_->property("workspaceIndex").toInt()==2 && dataPanel_->property("activeTool").toString()=="Thicken");
  auto& stations=planViewport_->fuselageSketchEditor().stationEditor();
  stations.setActivePanel(1);
  const bool editProfiles=dataPanel_->property("workspaceIndex").toInt()==2 && dataPanel_->property("activeTool").toString()=="Edit Profiles";
  fuselageProfilePanel_->setVisible(editProfiles);
  fuselageProfilePanel_->setEnabled(graphicsTabs_->currentIndex()==0);
  fuselageProfilePanel_->setActive(editProfiles&&graphicsTabs_->currentIndex()==0);
  stations.setEnabled(active&&graphicsTabs_->currentIndex()==0&&fuselageOutlinePanel_->outlinesDefined());
}
void MainWindow::updateEditorVisibility() {
  const bool fuselageOutline=dataPanel_->property("workspaceIndex").toInt()==2 && dataPanel_->property("activeTool").toString()=="Outline";
  // Finish the outgoing fuselage editor before activating another controller.
  if(!fuselageOutline)fuselageOutlinePanel_->setActive(false,!restoringProject_);
  const bool outline = dataPanel_->property("workspaceIndex").toInt() == 1 &&
      dataPanel_->property("activeTool").toString() == "Outline";
  const bool stations = dataPanel_->property("workspaceIndex").toInt() == 1 &&
      dataPanel_->property("activeTool").toString() == "Airfoil Stations";
  const bool airfoils = dataPanel_->property("workspaceIndex").toInt() == 1 &&
      dataPanel_->property("activeTool").toString() == "Airfoils";
  dihedralPanel_->setVisible(dataPanel_->property("workspaceIndex").toInt() == 1 &&
      dataPanel_->property("activeTool").toString() == "Dihedral");
  wingOutlinePanel_->setVisible(outline);
  stationPanel_->setVisible(stations);
  planViewport_->sketchEditor().setEditing(outline);
  if(stations)planViewport_->sketchEditor().stationEditor().setActivePanel(stationTabs_->currentIndex());
  planViewport_->sketchEditor().stationEditor().setEnabled(stations);
  airfoilPanel_->setActive(airfoils);
  const bool controls=dataPanel_->property("workspaceIndex").toInt()==1 &&
      dataPanel_->property("activeTool").toString()=="Ailerons/Flaps";
  controlSurfacePanel_->setVisible(controls);
  sparPanel_->setVisible(dataPanel_->property("workspaceIndex").toInt()==1 && dataPanel_->property("activeTool").toString()=="Spars");
  lighteningPanel_->setVisible(dataPanel_->property("workspaceIndex").toInt()==1 && dataPanel_->property("activeTool").toString()=="Lightening");
  planViewport_->controlSurfaceEditor().setEditing(controls);
  fuselageOutlinePanel_->setVisible(fuselageOutline);
  if(fuselageOutline)fuselageOutlinePanel_->setActive(graphicsTabs_->currentIndex()==0,!restoringProject_);
  updateFuselageStationMode();
  updateWingModel();
}

void MainWindow::invalidateWing() {
  if(restoringProject_) return;
  wingDirty_ = true;
  // Coalesce nested assignment/progress signals and read a completed edit snapshot.
  QTimer::singleShot(0, this, [this] { updateWingModel(); });
}

MainWindow::~MainWindow() {
  // No worker refers to this window. Destruction still joins before GUI-owned
  // snapshots/OCCT runtime resources are released.
  if(modelJob_){modelJob_->cancel();modelJob_.reset();QApplication::restoreOverrideCursor();}
  if(fuselageJob_){fuselageJob_->cancel();fuselageJob_.reset();QApplication::restoreOverrideCursor();}
}

void MainWindow::setModelProcessing(bool active) {
  setProperty("modelProcessing",active);
  if(active) {
    processingActions_.clear();
    for(auto* action:findChildren<QAction*>()){processingActions_.emplace_back(action,action->isEnabled());action->setEnabled(false);}
  } else {
    for(auto& [action,enabled]:processingActions_)if(action)action->setEnabled(enabled);
    processingActions_.clear();
  }
  dataContents_->setEnabled(!active);graphicsTabs_->setEnabled(!active);
  menuBar()->setEnabled(!active);workspaceToolBar_->setEnabled(!active);componentToolBar_->setEnabled(!active);
  cancelProcessing_->setText("Cancel");cancelProcessing_->setEnabled(true);cancelProcessing_->setVisible(active);
  if(active)QApplication::setOverrideCursor(Qt::WaitCursor);else QApplication::restoreOverrideCursor();
}

void MainWindow::updateWingModel() {
  if(restoringProject_ || modelJob_ || fuselageJob_)return;
  if(dataPanel_->property("workspaceIndex").toInt()==2){updateFuselageModel();return;}
  if(graphicsTabs_->currentWidget()==viewport_ && dataPanel_->property("workspaceIndex").toInt()==1 && displayedComponent_!=1) {
    if(!wingShape_.IsNull())viewport_->displayShape(wingShape_,true);else viewport_->clearShape();
    displayedComponent_=1;wingDirty_=true;
  }
  if (!wingDirty_ || !graphicsTabs_ || graphicsTabs_->currentWidget() != viewport_ ||
      dataPanel_->property("workspaceIndex").toInt() != 1) return;
  wingDirty_ = false;
  const auto fingerprint = wingFingerprint();
  if (fingerprint == builtWingFingerprint_) return;
  builtWingFingerprint_ = fingerprint; // Failed/incomplete attempts wait for changed inputs.
  if (!wingDefinitions_.airfoilsDefined) {
    viewport_->clearShape();wingShape_.Nullify();viewport_->setProperty("wingModelReady",false);
    statusBar()->showMessage("Complete the outline and assign every airfoil station to generate the wing.");return;
  }
  geometry::WingSolidInput input{planViewport_->sketchEditor().layers(),
      planViewport_->sketchEditor().stationEditor().lines(),airfoilPanel_->library().entries(),
      projectReference().toScale?std::nullopt:projectReference().wingspanMm,
      dihedralPanel_->values(),planViewport_->controlSurfaceEditor().state().panels,sparPanel_->state(),lighteningPanel_->state()};
  try {
    modelJob_=std::make_unique<processing::BackgroundJob<TopoDS_Shape>>(
      [input=std::move(input)](std::stop_token stop,const auto& progress) {
        return geometry::buildWingSolid(input,progress,{{stop}});
      });
    jobEpoch_=projectEpoch_;jobFingerprint_=fingerprint;setModelProcessing(true);
    viewport_->setProperty("wingModelReady",false);
    statusBar()->showMessage("Preparing wing geometry...");
  } catch(const std::exception& error) {statusBar()->showMessage("Could not start wing generation: "+QString::fromUtf8(error.what()));}
}

void MainWindow::pollModelJob() {
  if(fuselageJob_){pollFuselageJob();return;}
  if(!modelJob_)return;
  for(const auto& message:modelJob_->messages())
    if(!modelJob_->cancelled())statusBar()->showMessage(QString::fromStdString(message));
  if(!modelJob_->ready())return;
  const bool cancelled=modelJob_->cancelled();
  const bool obsolete=jobEpoch_!=projectEpoch_ || jobFingerprint_!=wingFingerprint();
  TopoDS_Shape shape;QString error;
  try {shape=modelJob_->take();}
  catch(const Standard_Failure& failure){error=QString::fromUtf8(failure.what());}
  catch(const std::exception& failure){error=QString::fromUtf8(failure.what());}
  catch(...){error="Unknown geometry processing failure.";}
  modelJob_.reset();setModelProcessing(false);
  if(!obsolete) {
    if(cancelled) {
      builtWingFingerprint_.clear();wingDirty_=false;
      statusBar()->showMessage("Wing generation cancelled. The previous display is retained. Re-enter 3D View to retry.");
    } else if(!error.isEmpty()) {
      viewport_->clearShape();wingShape_.Nullify();viewport_->setProperty("wingModelReady",false);
      statusBar()->showMessage("Wing generation failed: "+error);
    } else {
      try {
        ProcessingScope processing{this,"Displaying wing model..."};
        const auto camera=restoredWingCamera_?restoredWingCamera_:wingHasView_?viewport_->cameraState():std::nullopt;
        wingShape_=shape;displayedComponent_=1;
        viewport_->displayShape(shape,!camera.has_value());if(camera)viewport_->restoreCamera(camera);
        restoredWingCamera_.reset();wingHasView_=true;
        viewport_->setProperty("wingModelReady",true);
        viewport_->setProperty("wingModelRevision",viewport_->property("wingModelRevision").toInt()+1);
        statusBar()->showMessage("Wing model updated  |  Left drag: orbit  |  Right drag: pan  |  Wheel: zoom");
      } catch(const Standard_Failure& failure){statusBar()->showMessage("Wing display failed: "+QString::fromUtf8(failure.what()));}
        catch(const std::exception& failure){statusBar()->showMessage("Wing display failed: "+QString::fromUtf8(failure.what()));}
    }
  }
  if(closingAfterProcessing_){closingAfterProcessing_=false;close();}
  else if(obsolete) {
    updateWorkspaceAvailability();updateProjectTitle();wingDirty_=true;updateWingModel();
  }
}

QByteArray MainWindow::fuselageFingerprint() const {
  const auto p=encodeProject(projectDocument(),false);
  return QJsonDocument{QJsonObject{{"outlines",p["fuselageOutline"].toObject()["layers"]},
      {"stations",p["fuselageStations"].toObject()["lines"]},
      {"profiles",p["fuselageProfiles"].toObject()["layers"]},
      {"formers",p["formers"].toObject()["rectangles"]},{"tray",p["servoTray"].toObject()["rectangle"]},{"thicken",p["fuselageThickening"]},{"cuts",p["fuselageCuts"].toObject()["layers"]},
      {"length",projectReference().toScale?QJsonValue{}:p["reference"].toObject()["fuselageLengthMm"]}}}.toJson(QJsonDocument::Compact);
}
void MainWindow::updateFuselageModel() {
  if(restoringProject_||modelJob_||fuselageJob_||graphicsTabs_->currentWidget()!=viewport_||dataPanel_->property("workspaceIndex").toInt()!=2)return;
  // A complete fuselage needs no optional tab visits. Initialize missing wall
  // defaults before taking the model snapshot, preserving explicit values.
  if(fuselageOutlinePanel_->outlinesDefined()&&fuselageProfilePanel_->allProfilesClosed()) {
    if(!fuselageThickenPanel_->enabled())fuselageThickenPanel_->enter(fuselageWingLeadingEdge());
    else fuselageThickenPanel_->synchronize(fuselageWingLeadingEdge());
  }
  const auto fingerprint=fuselageFingerprint();
  if(displayedComponent_!=2) {
    if(!fuselageShape_.IsNull())viewport_->displayShape(fuselageShape_,true);else viewport_->clearShape();
    displayedComponent_=2;
  }
  if(fingerprint==builtFuselageFingerprint_)return;
  builtFuselageFingerprint_=fingerprint;
  viewport_->setProperty("fuselageModelReady",false);
  viewport_->setProperty("servoTrayReady",false);viewport_->setProperty("formerCount",0);servoTrayTopFaces_.Nullify();
  if(!fuselageOutlinePanel_->outlinesDefined()||!fuselageProfilePanel_->allProfilesClosed()) {
    viewport_->clearShape();fuselageShape_.Nullify();
    statusBar()->showMessage("Fuselage: close both outlines and assign a closed profile to every station before opening 3D.");return;
  }
  geometry::FuselageSolidInput input{planViewport_->fuselageSketchEditor().layers(),
      planViewport_->fuselageSketchEditor().stationEditor().lines(),planViewport_->fuselageProfileEditor().layers(),
      projectReference().toScale?std::nullopt:projectReference().fuselageLengthMm,fuselageThickenPanel_->enabled(),planViewport_->fuselageCutEditor().layers(),planViewport_->servoTrayEditor().state().rectangle,planViewport_->formerEditor().state().rectangles};
  try {
    // A separate owned worker and immutable snapshot; no Wing state is read.
    fuselageJob_=std::make_unique<processing::BackgroundJob<geometry::FuselageBuildResult>>(
      [input=std::move(input)](std::stop_token stop,const auto& progress){return geometry::buildFuselageModel(input,progress,{stop});});
    fuselageJobEpoch_=projectEpoch_;fuselageJobFingerprint_=fingerprint;setModelProcessing(true);
    statusBar()->showMessage("Preparing fuselage geometry...");
  } catch(const std::exception& error){statusBar()->showMessage("Could not start fuselage generation: "+QString::fromUtf8(error.what()));}
}
void MainWindow::pollFuselageJob() {
  for(const auto& message:fuselageJob_->messages())if(!fuselageJob_->cancelled())statusBar()->showMessage(QString::fromStdString(message));
  if(!fuselageJob_->ready())return;
  const bool cancelled=fuselageJob_->cancelled();
  const bool obsolete=fuselageJobEpoch_!=projectEpoch_||fuselageJobFingerprint_!=fuselageFingerprint();
  geometry::FuselageBuildResult result;QString error;
  try{result=fuselageJob_->take();}
  catch(const Standard_Failure& failure){error=QString::fromUtf8(failure.what());}
  catch(const std::exception& failure){error=QString::fromUtf8(failure.what());}
  catch(...){error="Unknown geometry processing failure.";}
  fuselageJob_.reset();setModelProcessing(false);
  if(!obsolete) {
    if(cancelled){builtFuselageFingerprint_.clear();statusBar()->showMessage("Fuselage generation cancelled. Re-enter 3D View to retry.");}
    else if(!error.isEmpty()) {
      fuselageShape_.Nullify();viewport_->clearShape();statusBar()->showMessage("Fuselage generation failed: "+error);
    } else try {
      const auto& shape=result.shape;servoTrayTopFaces_=result.servoTrayTopFaces;
      viewport_->setProperty("servoTrayReady",!result.servoTray.IsNull());
      viewport_->setProperty("formerCount",static_cast<int>(result.formers.size()));
      const auto camera=restoredFuselageCamera_?restoredFuselageCamera_:fuselageShape_.IsNull()?std::nullopt:viewport_->cameraState();
      fuselageShape_=shape;displayedComponent_=2;viewport_->displayShape(shape,!camera.has_value());if(camera)viewport_->restoreCamera(camera);
      restoredFuselageCamera_.reset();
      viewport_->setProperty("fuselageModelReady",true);
      viewport_->setProperty("fuselageModelRevision",viewport_->property("fuselageModelRevision").toInt()+1);
      int bodies=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++bodies;
      viewport_->setProperty("fuselageBodyCount",bodies);
      statusBar()->showMessage(!result.formers.empty()?QString{"Fuselage updated: %1 formers%2; %3 bodies"}.arg(result.formers.size()).arg(result.servoTray.IsNull()?QString{}:QString{" and servo tray"}).arg(bodies):!result.servoTray.IsNull()?QString{"Fuselage updated: servo tray and 5 mm support ledges; %1 bodies"}.arg(bodies):
          bodies>1?QString{"Fuselage updated: %1 separate bodies after cuts"}.arg(bodies):
          fuselageThickenPanel_->enabled()?"Fuselage hollow body updated with station wall thicknesses":"Fuselage solid updated  |  Left drag: orbit  |  Right drag: pan  |  Wheel: zoom");
    } catch(const Standard_Failure& failure){statusBar()->showMessage("Fuselage display failed: "+QString::fromUtf8(failure.what()));}
      catch(const std::exception& failure){statusBar()->showMessage("Fuselage display failed: "+QString::fromUtf8(failure.what()));}
  }
  if(closingAfterProcessing_){closingAfterProcessing_=false;close();}
  else if(obsolete){updateWorkspaceAvailability();updateProjectTitle();updateWingModel();}
}

void MainWindow::updateWorkspaceAvailability() {
  if (!workspaceToolBar_) return;
  applyReferenceWorkflow(*workspaceToolBar_, projectReference(), wingDefinitions_.outlineDefined && wingDefinitions_.stationsDefined && wingDefinitions_.airfoilsDefined);
  const bool fuselageReady=workspaceToolBar_->actions().at(2)->isEnabled()&&fuselageOutlinePanel_->outlinesDefined()&&fuselageProfilePanel_->allProfilesClosed();
  workspaceToolBar_->actions().at(3)->setEnabled(fuselageReady);
  workspaceToolBar_->actions().at(4)->setEnabled(fuselageReady);
  const int current = dataPanel_->property("workspaceIndex").toInt();
  if (!workspaceToolBar_->actions().at(current)->isEnabled()) {
    const int fallback = referenceReady(projectReference()) ? 1 : 0;
    workspaceToolBar_->actions().at(fallback)->setChecked(true);
    selectWorkspace(fallback);
  }
}

void MainWindow::setWingDefinitionState(const WingDefinitionState& state) {
  wingDefinitions_ = state;
  updateWorkspaceAvailability();
  invalidateWing();
  if (dataPanel_->property("workspaceIndex").toInt() == 1) {
    const int selected = applyWingWorkflow(*componentToolBar_, wingDefinitions_);
    dataPanel_->setProperty("activeTool", componentToolBar_->actions().at(selected)->text());
    updateEditorVisibility();
  }
}

void MainWindow::buildMenus() {
  auto* file = menuBar()->addMenu("&File");
  auto* newAction = file->addAction("&New");
  newAction->setShortcut(QKeySequence::New);
  newAction->setObjectName("projectNew");
  connect(newAction, &QAction::triggered, this, [this] { newProject(); });
  auto* open=file->addAction("&Open...");open->setObjectName("projectOpen");open->setShortcut(QKeySequence::Open);
  connect(open,&QAction::triggered,this,[this]{openProject();});
  closeAction_=file->addAction("&Close Project");closeAction_->setObjectName("projectClose");closeAction_->setShortcut(QKeySequence::Close);
  connect(closeAction_,&QAction::triggered,this,[this]{closeProject();});
  saveAction_=file->addAction("&Save");saveAction_->setObjectName("projectSave");saveAction_->setShortcut(QKeySequence::Save);
  connect(saveAction_,&QAction::triggered,this,[this]{saveProject();});
  saveAsAction_=file->addAction("Save &As...");saveAsAction_->setObjectName("projectSaveAs");saveAsAction_->setShortcut(QKeySequence::SaveAs);
  connect(saveAsAction_,&QAction::triggered,this,[this]{saveProject(true);});
  file->addSeparator();
  auto* exitAction = file->addAction("E&xit");
  exitAction->setShortcut(QKeySequence::Quit);
  connect(exitAction, &QAction::triggered, this, &QWidget::close);
  auto* edit = menuBar()->addMenu("&Edit");
  auto* copyAction = edit->addAction("&Copy");
  copyAction->setShortcut(QKeySequence::Copy);
  connect(copyAction, &QAction::triggered, this, [this] { copyFocusedText(); });
  auto* pasteAction = edit->addAction("&Paste");
  pasteAction->setShortcut(QKeySequence::Paste);
  connect(pasteAction, &QAction::triggered, this, [this] { pasteFocusedText(); });

  auto* view = menuBar()->addMenu("&View");
  auto* fit = view->addAction("Fit View");
  connect(fit, &QAction::triggered, this, [this] {
    if (graphicsTabs_->currentIndex() == 0) planViewport_->fitAll();
    else viewport_->fitAll();
  });
  const std::array<const char*, 7> names{"Reset", "Top", "Bottom", "Front", "Back", "Left", "Right"};
  const std::array<CameraView, 7> cameras{CameraView::Reset, CameraView::Top,
      CameraView::Bottom, CameraView::Front, CameraView::Back, CameraView::Left, CameraView::Right};
  for (std::size_t index = 0; index < names.size(); ++index) {
    auto* action = view->addAction(names[index]);
    connect(action, &QAction::triggered, this, [this, camera = cameras[index]] { setCameraView(camera); });
  }
  auto* help = menuBar()->addMenu("&Help");
  auto* helpAction = help->addAction("&Help");
  helpAction->setShortcut(QKeySequence::HelpContents);
  connect(helpAction, &QAction::triggered, this, [this] { openHelp(); });
  auto* about = help->addAction("&About");
  connect(about, &QAction::triggered, this, [this] { showAbout(); });
}

void MainWindow::setCameraView(CameraView cameraView) {
  graphicsTabs_->setCurrentWidget(viewport_);
  viewport_->setCameraView(cameraView);
}

void MainWindow::resetProject() {
  ++projectEpoch_;if(modelJob_)modelJob_->cancel();if(fuselageJob_)fuselageJob_->cancel();
  ProcessingScope processing{this, "Creating empty project..."};
  restoringProject_=true;projectOpen_=true;centralWidget()->setEnabled(true);
  workspaceToolBar_->setEnabled(!modelJob_&&!fuselageJob_);componentToolBar_->setEnabled(!modelJob_&&!fuselageJob_);
  projectPath_.clear();
  builtWingFingerprint_.clear();builtFuselageFingerprint_.clear();
  wingShape_.Nullify();fuselageShape_.Nullify();servoTrayTopFaces_.Nullify();displayedComponent_=-1;
  viewport_->setProperty("fuselageModelReady",false);
  viewport_->setProperty("servoTrayReady",false);viewport_->setProperty("formerCount",0);servoTrayTopFaces_.Nullify();
  wingHasView_ = false;
  restoredWingCamera_.reset();restoredFuselageCamera_.reset();
  dihedralPanel_->restore({0});
  lighteningPanel_->restore({},ProjectUnits::Millimeters);
  wingDirty_ = true;
  wingDefinitions_ = {};
  airfoilPanel_->reset();
  wingOutlinePanel_->reset();
  referencePanel_->reset();
  sparPanel_->restore({},ProjectUnits::Millimeters);
  updatePanelCounts();stationTabs_->setCurrentIndex(0);
  viewport_->clearShape();
  viewport_->setProperty("wingModelReady", false);
  viewport_->resetCamera();
  fuselageThickenPanel_->restore(false);
  fuselageThickenPanel_->setUnits(ProjectUnits::Millimeters);
  planViewport_->clearPlan();
  fuselageProfilePanel_->setActive(false);
  fuselageCutPanel_->setActive(false);fuselageCutPanel_->restoreControls();
  servoTrayPanel_->setActive(false);
  formerPanel_->setActive(false);
  fuselageOutlinePanel_->reset();
  controlSurfacePanel_->restoreControls();
  graphicsTabs_->setCurrentWidget(planViewport_);
  workspaceToolBar_->actions().front()->setChecked(true);
  selectWorkspace(0);
  updateWorkspaceAvailability();
  statusBar()->showMessage("New project ready; specify the Reference dimensions");
  restoringProject_=false;
  savedFingerprint_=projectFingerprint();updateProjectTitle();
}

ProjectDocument MainWindow::projectDocument() const {
  ProjectDocument p;
  p.reference=projectReference();
  p.wingspanText=findChild<QLineEdit*>("referenceWingspan")->text();
  p.fuselageText=findChild<QLineEdit*>("referenceFuselageLength")->text();
  p.wing=planViewport_->sketchEditor().state();p.airfoilSketch=planViewport_->airfoilSketchEditor().state();
  p.fuselageThickening=fuselageThickenPanel_->enabled();
  p.fuselageProfiles=planViewport_->fuselageProfileEditor().state();
  p.fuselageCuts=planViewport_->fuselageCutEditor().state();
  p.servoTray=planViewport_->servoTrayEditor().state();
  p.formers=planViewport_->formerEditor().state();
  p.fuselageStations=planViewport_->fuselageSketchEditor().stationEditor().state();
  p.fuselage=planViewport_->fuselageSketchEditor().state();p.fuselageView=fuselageOutlinePanel_->selectedView();
  p.stations=planViewport_->sketchEditor().stationEditor().state();p.airfoils=airfoilPanel_->state();
  p.workspace=dataPanel_->property("workspaceIndex").toInt();p.tool=dataPanel_->property("activeTool").toString();
  p.controls=planViewport_->controlSurfaceEditor().state();p.spars=sparPanel_->state();p.selectedSparPanel=sparPanel_->selectedPanel();p.selectedStationPanel=stationTabs_->currentIndex();
  p.lightening=lighteningPanel_->state();
  p.dihedralDegrees=dihedralPanel_->values();p.selectedDihedralPanel=dihedralPanel_->selectedPanel();p.viewport=graphicsTabs_->currentIndex();p.plan=planViewport_->viewState();p.camera=viewport_->cameraState();
  for(int size:static_cast<QSplitter*>(centralWidget())->sizes())p.splitterSizes.push_back(size);
  return p;
}
QByteArray MainWindow::projectFingerprint() const {
  auto snapshot = encodeProject(projectDocument(),false);
  auto controls=snapshot["controlSurfaces"].toObject();
  if(controls["first"].isNull()){controls.remove("drawing");controls.remove("panel");}
  snapshot["controlSurfaces"]=controls;
  auto tray=snapshot["servoTray"].toObject();if(tray["first"].isNull())tray.remove("drawing");snapshot["servoTray"]=tray;
  snapshot.remove("ui"); // Still saved/restored, but navigation is not a document edit.
  for (const char* key : {"wingOutline", "airfoilSketches", "fuselageOutline", "fuselageProfiles", "fuselageCuts"}) {
    auto sketch = snapshot[key].toObject();
    sketch.remove("selected"); sketch.remove("editing");
    // A pending point's tool/layer gives it meaning; idle tool/tab choices do not.
    if (sketch["pending"].toArray().isEmpty()) {sketch.remove("tool");sketch.remove("active");}
    snapshot[key] = sketch;
  }
  auto fuselageStations=snapshot["fuselageStations"].toObject();fuselageStations.remove("selected");snapshot["fuselageStations"]=fuselageStations;
  auto stations = snapshot["stations"].toObject(); stations.remove("selected"); snapshot["stations"] = stations;
  auto airfoils = snapshot["airfoils"].toObject();
  if (!airfoils["sketching"].toBool()) airfoils.remove("draftName");
  airfoils.remove("panel");airfoils.remove("panelChoices");airfoils.remove("chosen"); airfoils.remove("sketching"); airfoils.remove("draft");
  snapshot["airfoils"] = airfoils;
  return QJsonDocument{snapshot}.toJson(QJsonDocument::Compact);
}
QByteArray MainWindow::wingFingerprint() const {
  const auto snapshot = encodeProject(projectDocument(),false);
  const auto reference = snapshot["reference"].toObject();
  QJsonArray physicalSpars;
  for(const auto panel:snapshot["spars"].toArray()) {
    QJsonArray entries;
    for(const auto value:panel.toArray()) {
      auto spar=value.toObject();spar.remove("sizeText");spar.remove("heightText");entries.append(spar);
    }
    physicalSpars.append(entries);
  }
  auto physicalLightening=snapshot["lightening"].toObject();physicalLightening.remove("text");
  const QJsonObject inputs{
    {"lightening",physicalLightening},
    {"outline",snapshot["wingOutline"].toObject()["layers"]},
    {"stations",snapshot["stations"].toObject()["lines"]},
    {"airfoils",snapshot["airfoils"].toObject()["entries"]},
    {"wingspan",reference["toScale"].toBool() ? QJsonValue{} : reference["wingspanMm"]},
    {"controls",snapshot["controlSurfaces"].toObject()["panels"]},{"spars",physicalSpars},
    {"dihedral",snapshot["dihedralDegrees"]}, {"ready",wingDefinitions_.airfoilsDefined}};
  return QJsonDocument{inputs}.toJson(QJsonDocument::Compact);
}
bool MainWindow::projectModified() const {return projectOpen_ && projectFingerprint()!=savedFingerprint_;}
void MainWindow::updateProjectTitle() {
  setWindowModified(projectModified());
  setWindowTitle(projectOpen_?(projectPath_.isEmpty()?"Untitled":QFileInfo{projectPath_}.fileName())+"[*] - FoamAirplaneStudio":"FoamAirplaneStudio");
  if(!modelJob_&&!fuselageJob_){saveAction_->setEnabled(projectOpen_);saveAsAction_->setEnabled(projectOpen_);closeAction_->setEnabled(projectOpen_);}
}
bool MainWindow::saveProjectFile(const QString& path,QString& error) {
  if(!projectOpen_) {error="No project is open.";return false;}
  ProcessingScope processing{this, "Saving project..."};
  if(!writeProject(path,projectDocument(),error)){processing.update("Save failed: " + error);return false;}
  projectPath_=QFileInfo{path}.absoluteFilePath();savedFingerprint_=projectFingerprint();updateProjectTitle();
  statusBar()->showMessage("Saved " + projectPath_);return true;
}
bool MainWindow::saveProject(bool saveAs) {
  QString path=projectPath_;
  if(saveAs||path.isEmpty()) {
    FileSelectionDialog dialog{this,"projectFile","Save Project As",QFileDialog::AnyFile,QFileDialog::AcceptSave};
    dialog.setNameFilter("FoamAirplaneStudio projects (*.foam)");dialog.setDefaultSuffix("foam");
    if(dialog.exec()!=QDialog::Accepted)return false;path=dialog.selectedFiles().front();
  }
  QString error;if(saveProjectFile(path,error))return true;
  QMessageBox::warning(this,"Save Project",error);return false;
}
bool MainWindow::maybeSaveProject() {
  if(!projectModified())return true;
  const auto choice=QMessageBox::warning(this,"Unsaved Project","Save changes to the current project?",
      QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
  if(choice==QMessageBox::Discard)return true;
  if(choice==QMessageBox::Save)return saveProject();
  return false;
}
void MainWindow::newProject() {if(maybeSaveProject())resetProject();}
void MainWindow::closeProject() {
  if(!maybeSaveProject())return;
  resetProject();projectOpen_=false;centralWidget()->setEnabled(false);
  workspaceToolBar_->setEnabled(false);componentToolBar_->setEnabled(false);updateProjectTitle();
  statusBar()->showMessage("Project closed");
}
void MainWindow::closeEvent(QCloseEvent* event) {
  if(modelJob_||fuselageJob_) {
    closingAfterProcessing_=true;if(modelJob_)modelJob_->cancel();if(fuselageJob_)fuselageJob_->cancel();cancelProcessing_->setText("Cancelling...");cancelProcessing_->setEnabled(false);
    statusBar()->showMessage("Cancelling processing before closing...");event->ignore();return;
  }if(maybeSaveProject())event->accept();else event->ignore();}
void MainWindow::openProject() {
  FileSelectionDialog dialog{this,"projectFile","Open Project"};dialog.setNameFilter("FoamAirplaneStudio projects (*.foam)");
  if(dialog.exec()!=QDialog::Accepted)return;
  QString error;if(!openProjectFile(dialog.selectedFiles().front(),error)&&!error.isEmpty())QMessageBox::warning(this,"Open Project",error);
}
bool MainWindow::openProjectFile(const QString& path,QString& error) {
  std::optional<ProjectDocument> p;
  {
    ProcessingScope processing{this, "Reading and validating project..."};
    p=readProject(path,error);
    processing.update(p ? "Project read successfully" : "Open failed: " + error);
  }
  if(!p)return false; // Validate before any current-state changes or prompt.
  if(!maybeSaveProject())return false;
  ProcessingScope processing{this, "Restoring project..."};
  restoreProject(*p);projectPath_=QFileInfo{path}.absoluteFilePath();updateProjectTitle();
  processing.update("Opened " + projectPath_ + (graphicsTabs_->currentIndex()==1 && !viewport_->property("wingModelReady").toBool() ? " | " + statusBar()->currentMessage() : QString{}));
  return true;
}
void MainWindow::restoreProject(const ProjectDocument& p) {
  resetProject();restoringProject_=true;
  statusBar()->showMessage("Restoring reference, sketches and project settings...");
  statusBar()->repaint();
  referencePanel_->restoreReference(p.reference);
  planViewport_->setReferenceBackground(p.reference.image.pages,p.reference.toScale);
  {
    auto* span=findChild<QLineEdit*>("referenceWingspan");auto* length=findChild<QLineEdit*>("referenceFuselageLength");
    QSignalBlocker a{span},b{length};span->setText(p.wingspanText);length->setText(p.fuselageText);
  }
  planViewport_->sketchEditor().restoreState(p.wing);
  planViewport_->airfoilSketchEditor().restoreState(p.airfoilSketch);
  planViewport_->sketchEditor().stationEditor().restoreState(p.stations);
  wingOutlinePanel_->restoreControls();
  planViewport_->fuselageSketchEditor().restoreState(p.fuselage);
  fuselageOutlinePanel_->restoreControls(p.fuselageView);
  planViewport_->fuselageProfileEditor().restoreState(p.fuselageProfiles);
  planViewport_->fuselageSketchEditor().stationEditor().restoreState(p.fuselageStations);
  // Install the library before enabling Airfoils so selection callbacks cannot
  // accidentally apply an entry from the previous project.
  updatePanelCounts();{QSignalBlocker block{stationTabs_};stationTabs_->setCurrentIndex(p.selectedStationPanel);}
  auto libraryOnly=p.airfoils;libraryOnly.sketching=false;airfoilPanel_->restoreState(libraryOnly);
  wingDefinitions_.outlineDefined=wingOutlinePanel_->outlinesDefined();
  wingDefinitions_.stationsDefined=wingDefinitions_.outlineDefined&&planViewport_->sketchEditor().stationEditor().allPanelsDefined();
  wingDefinitions_.airfoilsDefined=wingDefinitions_.stationsDefined&&airfoilPanel_->allStationsAssigned();
  wingDefinitions_.dihedralDefined=wingDefinitions_.airfoilsDefined;
  dihedralPanel_->restore(p.dihedralDegrees,p.selectedDihedralPanel);
  updateWorkspaceAvailability();
  const int workspace=workspaceToolBar_->actions()[p.workspace]->isEnabled()?p.workspace:0;
  workspaceToolBar_->actions()[workspace]->setChecked(true);selectWorkspace(workspace);
  for(auto* action:componentToolBar_->actions())if(action->text()==p.tool&&action->isEnabled()) {
    action->setChecked(true);dataPanel_->setProperty("activeTool",p.tool);break;
  }
  updateEditorVisibility();
  // Mode activation may finish sketches or select a default station. Restore
  // the saved editing state last, including unfinished lines/splines and drafts.
  planViewport_->sketchEditor().restoreState(p.wing);
  planViewport_->airfoilSketchEditor().restoreState(p.airfoilSketch);
  airfoilPanel_->restoreState(p.airfoils);
  planViewport_->sketchEditor().stationEditor().setSelectionEnabled(workspace==1&&p.tool=="Airfoils"&&!p.airfoils.sketching);
  planViewport_->sketchEditor().stationEditor().setActivePanel(p.tool=="Airfoils"?p.airfoils.panel:p.selectedStationPanel);
  planViewport_->sketchEditor().stationEditor().restoreState(p.stations);
  if(p.splitterSizes.size()==2)static_cast<QSplitter*>(centralWidget())->setSizes({p.splitterSizes[0],p.splitterSizes[1]});
  graphicsTabs_->setCurrentIndex(p.viewport);planViewport_->restoreView(p.plan);
  planViewport_->controlSurfaceEditor().restore(p.controls);
  sparPanel_->restore(p.spars,p.reference.units,p.selectedSparPanel);
  lighteningPanel_->restore(p.lightening,p.reference.units);
  controlSurfacePanel_->restoreControls();
  auto fuselage=p.fuselage;
  fuselage.editing=workspace==2&&p.tool=="Outline"&&p.viewport==0&&p.fuselageView>=0;
  planViewport_->fuselageSketchEditor().restoreState(fuselage);
  fuselageOutlinePanel_->restoreControls(p.fuselageView);
  fuselageThickenPanel_->setUnits(p.reference.units);
  fuselageThickenPanel_->restore(p.fuselageThickening);
  updateFuselageProgress();
  updateFuselageStationMode();
  planViewport_->fuselageSketchEditor().stationEditor().restoreState(p.fuselageStations);
  fuselageProfilePanel_->selectStation();
  auto profileState=p.fuselageProfiles;profileState.editing=workspace==2&&p.tool=="Edit Profiles"&&p.viewport==0&&profileState.editing;
  planViewport_->fuselageProfileEditor().restoreState(profileState);
  fuselageProfilePanel_->restoreControls();
  auto cuts=p.fuselageCuts;cuts.editing=workspace==2&&p.tool=="Cut"&&p.viewport==0;
  planViewport_->fuselageCutEditor().restoreState(cuts);fuselageCutPanel_->restoreControls();
  planViewport_->servoTrayEditor().restore(p.servoTray);
  planViewport_->formerEditor().restore(p.formers);
  updateFuselageProgress();
  if(workspace==2)restoredFuselageCamera_=p.camera;else restoredWingCamera_=p.camera;
  restoringProject_=false;wingDirty_=true;updateWingModel();viewport_->restoreCamera(p.camera);
  savedFingerprint_=projectFingerprint();updateProjectTitle();
}

const ProjectReference& MainWindow::projectReference() const {
  return referencePanel_->projectReference();
}

void MainWindow::openHelp() {
  const QString path = QDir{QApplication::applicationDirPath()}
      .filePath("help/index.html");
  if (!QFileInfo::exists(path)) {
    QMessageBox::critical(this, "Help unavailable",
        QString{"The FoamAirplaneStudio help document was not found at:\n%1"}.arg(path));
    return;
  }
  if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
    QMessageBox::critical(this, "Help unavailable",
        "The system could not open the FoamAirplaneStudio help document.");
  }
}

void MainWindow::showAbout() {
  const QString licensesPath = QDir::toNativeSeparators(
      QDir{QApplication::applicationDirPath()}.filePath("licenses"));
  QMessageBox::about(this, "About FoamAirplaneStudio",
      QString{"<h2>FoamAirplaneStudio</h2>"
              "<p>Version %1: application shell derived from DesignRC.</p>"
              "<p>The foam-airplane workflow is under development.</p>"
              "<p>Copyright &copy; 2026 Barry Foust. GNU GPL version 3 only; "
              "absolutely no warranty.</p>"
              "<p>Uses Qt 6, Open CASCADE Technology, and FreeType. "
              "License texts and notices: <code>%2</code></p>"}
          .arg(QApplication::applicationVersion(), licensesPath.toHtmlEscaped()));
}


void MainWindow::copyFocusedText() {
  if (auto* line = qobject_cast<QLineEdit*>(QApplication::focusWidget())) line->copy();
  else if (auto* text = qobject_cast<QTextEdit*>(QApplication::focusWidget())) text->copy();
}
void MainWindow::pasteFocusedText() {
  if (auto* line = qobject_cast<QLineEdit*>(QApplication::focusWidget())) line->paste();
  else if (auto* text = qobject_cast<QTextEdit*>(QApplication::focusWidget())) text->paste();
}

} // namespace designrc::gui







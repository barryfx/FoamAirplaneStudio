#include "gui/InspectPanel.h"
#include "gui/WingCalibration.h"
#include "gui/WeightBalancePanel.h"
#include "gui/FiberglassPanel.h"
#include "geometry/Fiberglass.h"
#include "geometry/WeightBalance.h"
#include "gui/MainWindow.h"
#include <QCryptographicHash>
#include "gui/SparPanel.h"
#include "gui/DihedralPanel.h"
#include "gui/LighteningPanel.h"
#include "gui/ProcessingScope.h"
#include "gui/ExportPanel.h"

#include "gui/OcctViewport.h"
#include "gui/PlanViewport.h"
#include "gui/ReferencePanel.h"
#include "gui/ReferenceWorkflow.h"
#include "gui/WingOutlinePanel.h"
#include "gui/FuselageOutlinePanel.h"
#include "geometry/FuselageEndRegistration.h"
#include "gui/StabilizerOutlinePanel.h"
#include "gui/StabilizerHingePanel.h"
#include "gui/StabilizerCutPanel.h"
#include "gui/StabilizerAirfoilPanel.h"
#include "geometry/StabilizerSolidBuilder.h"
#include "geometry/StabilizerCut.h"
#include "gui/FuselageProfilePanel.h"
#include "gui/FuselageThickenPanel.h"
#include "gui/FuselageCutPanel.h"
#include "gui/ServoTrayPanel.h"
#include "gui/FormerPanel.h"
#include "gui/SketchBoundary.h"
#include <TopExp_Explorer.hxx>
#include <QLineF>
#include <limits>
#include <algorithm>
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
#include <QFileInfo>
#include <QKeySequence>
#include <QKeyEvent>
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
  fuselageOutlinePanel_->endsChanged=[this]{
    if(restoringProject_)return;
    updateProjectTitle();captureEdit();
    QTimer::singleShot(0,this,[this]{updateFuselageModel();});
  };
  fuselageOutlinePanel_->drawingRequested=[this]{graphicsTabs_->setCurrentWidget(planViewport_);};
  for (int i = 0; i < 2; ++i) {
    stabilizerOutlinePanels_[i] = new StabilizerOutlinePanel{planViewport_->stabilizerSketchEditor(i), i == 0, dataPanel_};
    dataLayout->addWidget(stabilizerOutlinePanels_[i]);
    stabilizerCutPanels_[i]=new StabilizerCutPanel{planViewport_->stabilizerCutEditor(i),i==0,dataPanel_};
    dataLayout->addWidget(stabilizerCutPanels_[i]);
    connect(&planViewport_->stabilizerCutEditor(i),&SketchEditor::changed,this,[this]{if(!restoringProject_)QTimer::singleShot(0,this,[this]{updateWingModel();});});
    stabilizerHingePanels_[i]=new StabilizerHingePanel{planViewport_->stabilizerHingeEditor(i),i==0,dataPanel_};
    dataLayout->addWidget(stabilizerHingePanels_[i]);
    stabilizerHingePanels_[i]->changed=[this]{if(!restoringProject_)updateWingModel();};
    connect(&planViewport_->stabilizerHingeEditor(i),&SketchEditor::changed,this,[this]{if(!restoringProject_)QTimer::singleShot(0,this,[this]{updateWingModel();});});
    stabilizerAirfoilPanels_[i] = new StabilizerAirfoilPanel{i == 0, dataPanel_};
    dataLayout->addWidget(stabilizerAirfoilPanels_[i]);
    stabilizerAirfoilPanels_[i]->changed = [this] { if (!restoringProject_) updateWingModel(); };
    connect(&planViewport_->stabilizerSketchEditor(i), &SketchEditor::changed, this, [this] {
      if (!restoringProject_) {updateStabilizerProgress();QTimer::singleShot(0, this, [this] { updateWingModel(); });}
    });
  }
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
  fuselageHolePanel_=new FuselageCutPanel{planViewport_->fuselageHoleEditor(),dataPanel_,true,&planViewport_->fuselageSketchEditor()};
  dataLayout->addWidget(fuselageHolePanel_);
  connect(&planViewport_->fuselageHoleEditor(),&SketchEditor::changed,this,[this]{if(!restoringProject_)QTimer::singleShot(0,this,[this]{updateFuselageModel();});});
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
    planViewport_->formerEditor().preserveThicknessAtScale(projectLengthScale());
    sparPanel_->setUnits(reference.units);lighteningPanel_->setUnits(reference.units);
    fuselageThickenPanel_->setUnits(reference.units);
    // Unscaled images retain pixel coordinates until outlines can be calibrated
    // against the aircraft dimensions. Page width is not aircraft wingspan.
    planViewport_->setReferenceBackground(reference.image.pages, reference.toScale);
    updateWorkspaceAvailability();
    invalidateWing();
  });
  buildAssemblyPanel(dataLayout);
  exportPanel_=new ExportPanel{dataContents_};dataLayout->addWidget(exportPanel_,1);exportPanel_->hide();
  inspectPanel_=new InspectPanel{dataContents_};dataLayout->addWidget(inspectPanel_,1);inspectPanel_->hide();
  inspectPanel_->namesChanged=[this]{if(!restoringProject_)updateProjectTitle();};
  inspectPanel_->visibilityChanged=[this]{displayInspect();};
  exportPanel_->exportRequested=[this]{exportComponents();};
  weightBalancePanel_=new WeightBalancePanel{*planViewport_,dataContents_};
  dataLayout->addWidget(weightBalancePanel_,1);weightBalancePanel_->hide();
  weightBalancePanel_->changed=[this]{if(!restoringProject_)updateProjectTitle();};
  for(int i=0;i<4;++i) {
    fiberglassPanels_[i]=new FiberglassPanel{planViewport_->fiberglassEditor(i),i,dataContents_};dataLayout->addWidget(fiberglassPanels_[i]);
    fiberglassPanels_[i]->hide();
    fiberglassPanels_[i]->changed=[this]{if(!restoringProject_)updateProjectTitle();};
  }
  // Panel footers consume available list/tab height, above bottom actions.
  for(int i=0;i<dataLayout->count();++i) {
    auto* panel=dataLayout->itemAt(i)->widget();if(!panel||panel==weightBalancePanel_||panel==assemblyPanel_||panel==exportPanel_)continue;
    auto* box=qobject_cast<QVBoxLayout*>(panel->layout());if(!box)continue;
    int slot=box->count();
    if(panel==airfoilPanel_)slot=box->indexOf(panel->findChild<QPushButton*>("smoothAirfoil"));
    auto* statistics=new QLabel{panel};statistics->setObjectName("airplaneStatistics");statistics->setTextFormat(Qt::RichText);
    statistics->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);statistics->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statistics->setToolTip("Outline planform areas include controls, before cutouts; wing and horizontal stabilizer include both halves. CG is measured from the placed wing root leading edge, positive toward the tail.");
    box->insertWidget(slot,statistics);statistics->hide();statisticsLabels_.push_back(statistics);
  }
  balanceStatus_=new QLabel{this};balanceStatus_->setObjectName("weightBalanceStatus");
  balanceStatus_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
  balanceStatus_->setFixedHeight(balanceStatus_->fontMetrics().height()+6);
  statusBar()->addWidget(balanceStatus_,1);balanceStatus_->hide();
  weightBalancePanel_->resultsChanged=[this](QString text){
    balanceStatus_->setToolTip(text);
    QStringList summary;
    for(const auto& line:text.split('\n'))if(line.startsWith("Total weight:")||line.startsWith("Center of Gravity:"))summary<<line;
    balanceStatus_->setText(summary.join("  |  "));
  };
  planViewport_->balanceOverlay=[this](QPainter& painter){weightBalancePanel_->paint(painter);};
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
    for(int i=0;i<4;++i)fiberglassPanels_[i]->setActive(index==0&&dataPanel_->property("workspaceIndex").toInt()==i+1&&dataPanel_->property("activeTool").toString()=="Fiberglass");
    const bool fuselageOutline=dataPanel_->property("workspaceIndex").toInt()==2 && dataPanel_->property("activeTool").toString()=="Outline";
    fuselageOutlinePanel_->setActive(fuselageOutline&&index==0,!restoringProject_);
    updateFuselageStationMode();
    updateStabilizerEditors();
    if(index==1)stabilizerCancelled_.fill(false);
    if(index==1&&!restoringProject_&&fuselageRetryPending_) {
      // Keep failed attempts suppressed during ordinary UI refreshes, but an
      // explicit return to 3D must retry unchanged project data.
      builtFuselageFingerprint_.clear();fuselageRetryPending_=false;
    }
    if(index==1 && !restoringProject_){planViewport_->controlSurfaceEditor().finishEditing();wingDirty_=true;}
    statusBar()->showMessage(index == 0
        ? "Wheel: zoom  |  Scrollbars: scroll"
        : "Left drag: orbit  |  Right drag: pan  |  Wheel: zoom");
    updateWingModel();
  });
  cancelProcessing_=new QPushButton{"Cancel",dataPanel_};
  cancelProcessing_->setObjectName("cancelProcessing");panelLayout->addWidget(cancelProcessing_);
  cancelProcessing_->hide();
  cancelProcessing_->setToolTip("Cancel model regeneration at the next safe point");
  connect(cancelProcessing_,&QPushButton::clicked,this,[this] {
    if(!modelJob_&&!fuselageJob_&&!stabilizerProcessing()&&!assemblyProcessing())return;
    if(assemblyPrepareJob_)assemblyPrepareJob_->cancel();if(assemblyCutJob_)assemblyCutJob_->cancel();
    for(auto& job:stabilizerJobs_)if(job)job->cancel();
    if(modelJob_)modelJob_->cancel();if(fuselageJob_)fuselageJob_->cancel();cancelProcessing_->setText("Cancelling...");cancelProcessing_->setEnabled(false);
    statusBar()->showMessage("Cancelling processing at the next safe point...");
  });
  auto* jobTimer=new QTimer{this};jobTimer->setInterval(40);
  connect(jobTimer,&QTimer::timeout,this,[this]{pollModelJob();});jobTimer->start();
  selectWorkspace(0);
  savedFingerprint_=projectFingerprint();
  auto* modifiedTimer=new QTimer{this}; modifiedTimer->setInterval(400);
  connect(modifiedTimer,&QTimer::timeout,this,[this]{if(!restoringProject_){captureEdit();updateProjectTitle();}});
  modifiedTimer->start();
  resetEditHistory();
  qApp->installEventFilter(this);
}

void MainWindow::buildToolBars() {
  workspaceToolBar_ = addToolBar("Design");
  workspaceToolBar_->setObjectName("workspaceToolBar");
  workspaceToolBar_->setMovable(false);
  auto* group = new QActionGroup{this};
  const std::array<const char*, 9> names{
      "Reference", "Wing", "Fuselage", "Horiz Stab", "Vert Stab", "Assembly", "Export", "Weight and Balance", "Inspect"};
  for (int index = 0; index < static_cast<int>(names.size()); ++index) {
    auto* action = new QAction{names[index],this};
    workspaceActions_[index]=action;action->setData(index);
    action->setCheckable(true);
    group->addAction(action);
    if (index == 0) action->setChecked(true);
    action->setEnabled(index == 0);
    connect(action, &QAction::triggered, this, [this, index] { selectWorkspace(index); });
  }
  for(int index:{0,1,2,3,4,5,8,7,6})workspaceToolBar_->addAction(workspaceActions_[index]);
  addToolBarBreak();
  componentToolBar_ = addToolBar("Component");
  componentToolBar_->setObjectName("componentToolBar");
  componentToolBar_->setMovable(false);
}

void MainWindow::selectWorkspace(int index) {
  if(index==6&&!exportAssemblyParts())return;
  if(dataPanel_->property("workspaceIndex").toInt()==8&&index!=8){viewport_->clearShape();displayedComponent_=-1;}
  const int outgoing = dataPanel_->property("workspaceIndex").toInt()-3;
  if(outgoing>=0 && outgoing<2 && displayedComponent_==outgoing+3 && graphicsTabs_->currentWidget()==viewport_)
    stabilizerCameras_[outgoing]=viewport_->cameraState();
  {QSignalBlocker block{graphicsTabs_};graphicsTabs_->setTabEnabled(0,index!=5&&index!=6&&index!=8);graphicsTabs_->setTabEnabled(1,index!=7); }
  assemblyPanel_->setVisible(index==5);
  exportPanel_->setVisible(index==6);inspectPanel_->setVisible(index==8);
  weightBalancePanel_->setActive(index==7);balanceStatus_->setVisible(index==7);
  if(index==6)exportPanel_->setParts(namedExportParts(*exportAssemblyParts()));
  if(index==5){assemblyEntry_=true;assemblyAttemptFingerprint_.clear();}
  componentToolBar_->clear();
  const std::array<QStringList, 9> tools{{
      {},
      {"Outline", "Airfoil Stations", "Airfoils", "Dihedral", "Ailerons/Flaps", "Spars", "Lightening", "Fiberglass"},
      {"Outline", "Profile Stations", "Edit Profiles", "Thicken", "Cut", "Servo Tray", "Formers", "Holes", "Fiberglass"},
      {"Outline", "Airfoil", "Hinge Line", "Cut", "Fiberglass"},
      {"Outline", "Airfoil", "Hinge Line", "Cut", "Fiberglass"},
      {},
      {},
      {},
      {}
  }};
  referencePanel_->setVisible(index == 0);
  if (index == 0) graphicsTabs_->setCurrentWidget(planViewport_);
  dataPanel_->setProperty("workspaceIndex", index);
  dataPanel_->setProperty("activeTool", QString{});
  auto* stabilizerGroup = (index==3 || index==4) ? new QActionGroup{componentToolBar_} : nullptr;
  auto* fuselageGroup = index==2 ? new QActionGroup{componentToolBar_} : nullptr;
  for (const auto& name : tools.at(index)) {
    auto* action = componentToolBar_->addAction(name);
    if(stabilizerGroup){action->setCheckable(true);stabilizerGroup->addAction(action);}
    if(fuselageGroup){action->setCheckable(true);fuselageGroup->addAction(action);action->setEnabled(name=="Outline" || (name=="Profile Stations"&&fuselageOutlinePanel_->outlinesDefined()));}
    connect(action, &QAction::triggered, this, [this, name] {
      dataPanel_->setProperty("activeTool", name);
      if(name=="Fiberglass")graphicsTabs_->setCurrentWidget(planViewport_);
      if ((dataPanel_->property("workspaceIndex").toInt()==3 || dataPanel_->property("workspaceIndex").toInt()==4) && (name=="Outline" || name=="Hinge Line" || name=="Cut")) graphicsTabs_->setCurrentWidget(planViewport_);
      if(dataPanel_->property("workspaceIndex").toInt()==2&&(name=="Outline"||name=="Profile Stations"||name=="Edit Profiles"||name=="Cut"||name=="Servo Tray"||name=="Formers"||name=="Holes"))graphicsTabs_->setCurrentWidget(planViewport_);
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
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Edit Profiles" ? "Select a station; draw a closed section using Line, Spline or Circle; open 3D to generate the fuselage"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Thicken" ? "Set station wall thickness in Reference units, or enter mm/in; 3D generation now hollows the fuselage"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Holes" ? "Choose a wall, Add Hole, then draw a closed loop inside its outline"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Cut" ? "Choose Top, Bottom, Left or Right View; draw a cut path, then open 3D to create separate cut-out components"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Formers" ? "Enter former thickness, Add Former, then drag its position or top/bottom edges; overlapping placements are blocked"
          : dataPanel_->property("workspaceIndex").toInt()==2 && name=="Servo Tray" ? "Enter tray width and height, then drag the rectangle into position on Side View; Thicken provides inner walls; supports extend 5 mm inward and down"
          : (dataPanel_->property("workspaceIndex").toInt()==3 || dataPanel_->property("workspaceIndex").toInt()==4) && name=="Outline" ? "Trace one open outline; align its endpoint line within 10 degrees of horizontal or vertical"
          : (dataPanel_->property("workspaceIndex").toInt()==3 || dataPanel_->property("workspaceIndex").toInt()==4) && name=="Airfoil" ? "Load one DAT airfoil for this stabilizer; open 3D View to generate its model"
          : (dataPanel_->property("workspaceIndex").toInt()==3 || dataPanel_->property("workspaceIndex").toInt()==4) && name=="Hinge Line" ? "Draw connected hinge segments; the vertical fin uses the segment closest to span direction, and the horizontal stabilizer uses the longest segment for Tape or Standard relief"
          : (dataPanel_->property("workspaceIndex").toInt()==3 || dataPanel_->property("workspaceIndex").toInt()==4) && name=="Cut" ? "Add closed Cut Shapes to remove material through the stabilizer; select a shape to edit or delete"
          : name=="Fiberglass" ? "Draw fiberglass coverage for Weight and Balance only; select a patch to edit its material and surface."
          : name + ": editor not implemented yet");
      updateEditorVisibility();
    });
  }
  if (index == 1) {
    const int selected = applyWingWorkflow(*componentToolBar_, wingDefinitions_);
    dataPanel_->setProperty("activeTool", tools.at(index).at(selected));
  }
  if(index==2 || index==3 || index==4) {
    componentToolBar_->actions().front()->setChecked(true);
    dataPanel_->setProperty("activeTool","Outline");
    if(!restoringProject_)graphicsTabs_->setCurrentWidget(planViewport_);
  }
  if(index==5||index==6||index==8)graphicsTabs_->setCurrentWidget(viewport_);
  if(index==6)displayAssembly();
  if(index==7)graphicsTabs_->setCurrentWidget(planViewport_);
  if(index==2)updateFuselageProgress();
  if(index==3 || index==4)updateStabilizerProgress();
  componentToolBar_->setVisible(!tools.at(index).empty());
  if(cancelProcessing_) {
    const bool processing=property("modelProcessing").toBool();
    cancelProcessing_->setVisible(processing);
    cancelProcessing_->setEnabled(processing);
  }
  statusBar()->showMessage(index == 0 ? "Set the project reference and dimensions" : index == 1 ? "Wing workspace ready" : index==2 ? "Fuselage workspace ready" : "Stabilizer Outline: trace one open line/spline chain including the control surface");
  updateEditorVisibility();
  if(index==6)statusBar()->showMessage("Select Assembly parts and formats, then Export Components.");
  if(index==7){updateWeightBalance(!restoringProject_);statusBar()->clearMessage();}
  if(index==8){statusBar()->showMessage("Inspect: check components to show them; edit names for Export.");updateInspect(true);}
}

void MainWindow::updateWeightBalance(bool frameSide) {
  weightBalancePanel_->setCgHeightLine({});
  try {
    if(projectLengthScale()<=0)throw std::runtime_error("Complete the wing outline and stations to establish the project scale.");
    const auto& reference=projectReference();
    const auto& side=planViewport_->fuselageSketchEditor().layers()[1];
    const auto transform=geometry::fuselageSideTransform(side,scaledFuselageLength());
    const auto boundary=closedSketchBoundary(side);
    if(!boundary)throw std::runtime_error("Define a closed fuselage Side View first.");
    double x0=boundary->front().x(),x1=x0,y0=boundary->front().y(),y1=y0;
    for(auto point:*boundary){x0=std::min(x0,point.x());x1=std::max(x1,point.x());y0=std::min(y0,point.y());y1=std::max(y1,point.y());}
    weightBalancePanel_->configure(reference.units,transform,
        {((x0+x1)/2-transform.left)*transform.scale,(transform.verticalOrigin-(y0+y1)/2)*transform.scale});
    if(frameSide) {
      QRectF bounds{QPointF{x0,y0},QPointF{x1,y1}};
      for(int i=0;i<static_cast<int>(weightBalancePanel_->state().parts.size());++i)
        bounds=bounds.united(weightBalancePanel_->partRectangle(i));
      const double zoom=std::clamp(std::min(planViewport_->viewport()->width()/(bounds.width()*1.2),
          planViewport_->viewport()->height()/(bounds.height()*1.2)),.00001,1000.);
      planViewport_->restoreView({zoom,bounds.center()});
    }
    const auto fingerprint=assemblyFingerprint();
    if(assemblyOriginals_.fuselage.IsNull() || assemblySourceFingerprint_!=fingerprint ||
        (assemblyState_.cuts&&!assemblyCutParts_)) {
      balanceMassCache_.reset();balanceMassSources_.clear();
      weightBalancePanel_->setFoam({}, {}, "Generate the current Assembly to calculate the complete model.");return;
    }
    // Reuse the generation fingerprint, but also track placement and the actual
    // immutable solids: rebuilding/cutting identical inputs can replace shapes.
    QJsonArray placement;
    for(const auto offset:assemblyState_.offsets){placement.append(offset.x());placement.append(offset.y());}
    for(const auto angle:assemblyState_.rotationDegrees)placement.append(angle);
    placement.append(assemblyState_.cuts);placement.append(QString::number(projectEpoch_));
    const auto project=projectDocument();auto covering=encodeProject(project,false)["fiberglass"].toArray();
    for(int i=0;i<covering.size();++i) {
      auto component=covering[i].toObject();auto sketch=component["sketch"].toObject();
      sketch.remove("selected");sketch.remove("editing");sketch.remove("active");if(sketch["pending"].toArray().isEmpty())sketch.remove("tool");component["sketch"]=sketch;
      auto patches=component["patches"].toArray();for(int j=0;j<patches.size();++j){const auto patch=patches[j].toObject();patches[j]=QJsonObject{{"side",patch["side"]},{"wrap",patch["wrap"]}};}component["patches"]=patches;covering[i]=component;
    }
    const auto key=fingerprint+QJsonDocument{placement}.toJson(QJsonDocument::Compact)+QJsonDocument{covering}.toJson(QJsonDocument::Compact);
    const auto& source=assemblyState_.cuts?*assemblyCutParts_:assemblyOriginals_;
    std::vector<TopoDS_Shape> shapes{source.fuselage,source.wing,source.horizontal,source.vertical,source.elevator,source.rudder};
    for(const auto& insert:source.inserts)shapes.push_back(insert.shape);
    const bool sameShapes=shapes.size()==balanceMassSources_.size() &&
        std::equal(shapes.begin(),shapes.end(),balanceMassSources_.begin(),
            [](const auto& a,const auto& b){return a.IsEqual(b);});
    if(!balanceMassCache_ || balanceMassFingerprint_!=key || !sameShapes) {
      balanceMassCache_.reset();
      ProcessingScope processing{this,"Calculating Weight and Balance foam, plywood and Carbon Fiber statistics..."};
      auto mass=geometry::foamMassProperties(*exportAssemblyParts());
      mass.fiberglass=geometry::fiberglassMassProperties(project,assemblyOriginals_,*exportAssemblyParts());
      balanceMassCache_=std::move(mass);
      balanceMassFingerprint_=key;balanceMassSources_=std::move(shapes);
      statusBar()->clearMessage();
    }
    // Material edits reuse measured areas and centroids, just as density edits
    // reuse solid volumes. Separate overlapping patches intentionally add mass.
    const QString componentNames[]{"Wing","Fuselage","Horiz Stab","Vert Stab"};std::size_t coveredIndex=0;
    for(int component=0;component<4;++component)for(std::size_t i=0;i<project.fiberglass[component].patches.size();++i) {
      if(project.fiberglass[component].sketch.layers[i].curves.empty())continue;
      const auto& patch=project.fiberglass[component].patches[i];auto& measured=balanceMassCache_->fiberglass.at(coveredIndex++);
      measured.name=componentNames[component]+" / "+patch.name;measured.clothGrams=measured.areaMm2*patch.clothGm2*1e-6;measured.resinVolumeMm3=measured.areaMm2*resinThickness(patch);
    }
    const double sourceLeadingEdge=geometry::wingRootLeadingEdgeX(planViewport_->sketchEditor().layers(),
        planViewport_->sketchEditor().stationEditor().lines(),reference.toScale?std::nullopt:reference.wingspanMm);
    const double leadingEdge=gp_Pnt{sourceLeadingEdge,0,0}.Transformed(
        geometry::assemblyComponentPlacement(assemblyOriginals_,assemblyState_,0)).X();
    statistics_.balance=StatisticsBalance{*balanceMassCache_,leadingEdge,statisticsMassKey()};
    weightBalancePanel_->setFoam(*balanceMassCache_,leadingEdge,{});
    const auto& stations=planViewport_->sketchEditor().stationEditor().lines();
    const auto calibration=wingCalibration(planViewport_->sketchEditor().layers(),stations,
        reference.toScale?std::nullopt:reference.wingspanMm);
    const auto span=[&](const auto& station){const auto center=(station.first.position+station.second.position)*.5;
      return QPointF::dotProduct(center,calibration.spanDirection);};
    const auto root=std::min_element(stations.begin(),stations.end(),[&](const auto& a,const auto& b){return span(a)<span(b);});
    const auto& library=airfoilPanel_->library().entries();
    if(root!=stations.end()&&root->airfoil&&*root->airfoil<library.size()) {
      const auto profile=normalizedAirfoil(library[*root->airfoil]).resampled(201);
      double low=profile.front().y,high=low;
      for(const auto point:profile){low=std::min(low,point.y);high=std::max(high,point.y);}
      const double height=(low+.2*(high-low))*calibration.rootChordMm;
      const auto wingPlacement=geometry::assemblyComponentPlacement(assemblyOriginals_,assemblyState_,0);
      const auto a=gp_Pnt{sourceLeadingEdge,0,height}.Transformed(wingPlacement);
      const auto b=gp_Pnt{sourceLeadingEdge+calibration.rootChordMm,0,height}.Transformed(wingPlacement);
      weightBalancePanel_->setCgHeightLine(QLineF{{a.X(),a.Z()},{b.X(),b.Z()}});
    }
  } catch(const Standard_Failure& error){statusBar()->clearMessage();weightBalancePanel_->setFoam({}, {}, QString::fromUtf8(error.what()));}
    catch(const std::exception& error){statusBar()->clearMessage();weightBalancePanel_->setFoam({}, {}, QString::fromUtf8(error.what()));}
}

QByteArray MainWindow::statisticsMassKey() const {
  QJsonArray placement;for(const auto offset:assemblyState_.offsets){placement.append(offset.x());placement.append(offset.y());}
  for(const auto angle:assemblyState_.rotationDegrees)placement.append(angle);placement.append(assemblyState_.cuts);
  auto patches=encodeProject(projectDocument(),false)["fiberglass"].toArray();
  for(int i=0;i<patches.size();++i){auto component=patches[i].toObject();auto sketch=component["sketch"].toObject();sketch.remove("selected");sketch.remove("editing");sketch.remove("active");if(sketch["pending"].toArray().isEmpty())sketch.remove("tool");component["sketch"]=sketch;patches[i]=component;}
  return QCryptographicHash::hash("airplane-statistics-v2"+assemblyFingerprint()+QJsonDocument{placement}.toJson(QJsonDocument::Compact)+QJsonDocument{patches}.toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();
}
void MainWindow::updateStatistics() {
  if(restoringProject_||statisticsLabels_.empty())return;
  const int workspace=dataPanel_->property("workspaceIndex").toInt();
  if(workspace==5||workspace==6) {
    // Placement edits refresh the title frequently. Invalidate stale saved mass,
    // but defer solid integration until a workspace displaying statistics needs it.
    // Export also has no statistics footer and must never trigger mass integration.
    if(!statistics_.balance||statistics_.balance->sourceKey!=statisticsMassKey()) {
      statistics_.balance.reset();statistics_.weightGrams.reset();
      statistics_.cgFromLeadingEdgeMm.reset();statistics_.wingLoadingGramsPerDm2.reset();
    }
    return;
  }
  auto balance=statistics_.balance;
  auto next=outlineStatistics(projectDocument());
  if(balance && balance->sourceKey!=statisticsMassKey())balance.reset();
  // Measure existing solids only. Never start regeneration for statistics.
  if(!assemblyOriginals_.fuselage.IsNull() && assemblySourceFingerprint_==assemblyFingerprint() &&
      (!assemblyState_.cuts||assemblyCutParts_) && !modelJob_&&!fuselageJob_&&!stabilizerProcessing()&&!assemblyProcessing()) {
    const auto& source=assemblyState_.cuts?*assemblyCutParts_:assemblyOriginals_;
    std::vector<TopoDS_Shape> shapes{source.fuselage,source.wing,source.horizontal,source.vertical,source.elevator,source.rudder};
    for(const auto& insert:source.inserts)shapes.push_back(insert.shape);
    const bool sameSources=shapes.size()==balanceMassSources_.size() && std::equal(shapes.begin(),shapes.end(),balanceMassSources_.begin(),
        [](const auto& a,const auto& b){return a.IsEqual(b);});
    if(!balance||!balanceMassCache_||!sameSources) {
      updateWeightBalance();balance=balanceMassCache_?statistics_.balance:std::nullopt;
    }
  }
  next.balance=balance;
  if(balance) {
    const auto mass=calculateBalance(weightBalancePanel_->state(),balance->materials);
    next.weightGrams=mass.grams;
    if(mass.grams>0)next.cgFromLeadingEdgeMm=mass.centerMm.x()-balance->leadingEdgeMm;
    if(next.wingAreaMm2)next.wingLoadingGramsPerDm2=mass.grams*10000 / *next.wingAreaMm2;
  }
  statistics_=std::move(next);const auto text=statisticsText(statistics_,projectReference().units);
  weightBalancePanel_->setWingArea(statistics_.wingAreaMm2);
  for(auto* label:statisticsLabels_){if(label->text()!=text)label->setText(text);label->setVisible(!text.isEmpty());}
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
  const auto& profiles=planViewport_->fuselageProfileEditor().layers();
  const bool hasProfiles=std::any_of(profiles.begin(),profiles.end(),[](const auto& layer){return !layer.curves.empty();});
  const bool complete=hasStations&&fuselageProfilePanel_->allProfilesClosed();
  for(auto* action:componentToolBar_->actions()) {
    const bool enabled=action->text()=="Outline" || (action->text()=="Fiberglass"?ready:action->text()=="Profile Stations"?ready:action->text()=="Edit Profiles"?(hasStations||hasProfiles):complete);
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
    const double scale=projectLengthScale();
    servoTrayPanel_->configure(reference.units,scale,{(left+right)/2,(top+bottom)/2});
    formerPanel_->configure(reference.units,scale,QRectF{QPointF{left,top},QPointF{right,bottom}});
  }
  const bool formers=dataPanel_->property("workspaceIndex").toInt()==2&&dataPanel_->property("activeTool").toString()=="Formers";
  formerPanel_->setVisible(formers);formerPanel_->setEnabled(graphicsTabs_->currentIndex()==0);formerPanel_->setActive(formers&&graphicsTabs_->currentIndex()==0);
  servoTrayPanel_->setVisible(tray);servoTrayPanel_->setEnabled(graphicsTabs_->currentIndex()==0);
  servoTrayPanel_->setActive(tray&&graphicsTabs_->currentIndex()==0);
  const bool cut=dataPanel_->property("workspaceIndex").toInt()==2 && dataPanel_->property("activeTool").toString()=="Cut";
  const bool holes=dataPanel_->property("workspaceIndex").toInt()==2&&dataPanel_->property("activeTool").toString()=="Holes";
  fuselageHolePanel_->setVisible(holes);fuselageHolePanel_->setEnabled(graphicsTabs_->currentIndex()==0);
  fuselageHolePanel_->setActive(holes&&graphicsTabs_->currentIndex()==0,!restoringProject_);
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
void MainWindow::updateStabilizerProgress() {
  if(!restoringProject_&&!property("modelProcessing").toBool())updateWorkspaceAvailability();
  if (restoringProject_ || stabilizerProcessing() || assemblyProcessing()) return;
  const int index=dataPanel_->property("workspaceIndex").toInt()-3;
  if(index<0 || index>1)return;
  const bool valid=stabilizerOutlineDefined(planViewport_->stabilizerSketchEditor(index).layers().front());
  bool fallback=false;
  for(auto* action:componentToolBar_->actions()) {
    action->setEnabled(action->text()=="Outline" || valid);
    if(!action->isEnabled() && action->isChecked())fallback=true;
  }
  if(fallback) {
    componentToolBar_->actions().front()->setChecked(true);dataPanel_->setProperty("activeTool","Outline");
    updateEditorVisibility();
  }
}
void MainWindow::updateStabilizerEditors() {
  const int workspace = dataPanel_->property("workspaceIndex").toInt();
  const bool outline = dataPanel_->property("activeTool").toString() == "Outline";
  const bool cut=dataPanel_->property("activeTool").toString()=="Cut";
  for(int i=0;i<2;++i) {
    const bool visible=workspace==i+3&&cut;
    stabilizerCutPanels_[i]->setVisible(visible);
    stabilizerCutPanels_[i]->setEnabled(graphicsTabs_->currentIndex()==0);
    stabilizerCutPanels_[i]->setActive(visible&&graphicsTabs_->currentIndex()==0,!restoringProject_);
  }
  const bool hinge=dataPanel_->property("activeTool").toString()=="Hinge Line";
  for(int i=0;i<2;++i) {
    const bool visible=workspace==i+3 && hinge;
    stabilizerHingePanels_[i]->setVisible(visible);
    stabilizerHingePanels_[i]->setEnabled(graphicsTabs_->currentIndex()==0);
    stabilizerHingePanels_[i]->setActive(visible && graphicsTabs_->currentIndex()==0);
  }
  // Finish the outgoing controller before enabling the incoming one.
  for (int i = 0; i < 2; ++i)
    if (workspace != i + 3 || !outline || graphicsTabs_->currentIndex() != 0)
      stabilizerOutlinePanels_[i]->setActive(false, !restoringProject_);
  for (int i = 0; i < 2; ++i) {
    const bool visible = workspace == i + 3 && outline;
    stabilizerAirfoilPanels_[i]->setVisible(workspace == i + 3 && dataPanel_->property("activeTool").toString() == "Airfoil");
    stabilizerOutlinePanels_[i]->setVisible(visible);
    stabilizerOutlinePanels_[i]->setEnabled(graphicsTabs_->currentIndex() == 0);
    if (visible && graphicsTabs_->currentIndex() == 0)
      stabilizerOutlinePanels_[i]->setActive(true, !restoringProject_);
  }
}
void MainWindow::updateEditorVisibility() {
  for (int i = 0; i < 2; ++i)
    if (dataPanel_->property("workspaceIndex").toInt() != i + 3 ||
        dataPanel_->property("activeTool").toString() != "Outline" || graphicsTabs_->currentIndex() != 0)
      stabilizerOutlinePanels_[i]->setActive(false, !restoringProject_);
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
  updateStabilizerEditors();
  for(int i=0;i<4;++i) {
    fiberglassPanels_[i]->configure(projectReference().units);
    fiberglassPanels_[i]->setActive(dataPanel_->property("workspaceIndex").toInt()==i+1&&dataPanel_->property("activeTool").toString()=="Fiberglass"&&graphicsTabs_->currentIndex()==0);
  }
  updateWingModel();
}

void MainWindow::invalidateWing() {
  if(restoringProject_) return;
  wingDirty_ = true;
  // Coalesce nested assignment/progress signals and read a completed edit snapshot.
  QTimer::singleShot(0, this, [this] { updateWingModel(); });
}

MainWindow::~MainWindow() {
  qApp->removeEventFilter(this);
  // No worker refers to this window. Destruction still joins before GUI-owned
  // snapshots/OCCT runtime resources are released.
  if(assemblyPrepareJob_){assemblyPrepareJob_->cancel();assemblyPrepareJob_.reset();QApplication::restoreOverrideCursor();}
  if(assemblyCutJob_){assemblyCutJob_->cancel();assemblyCutJob_.reset();QApplication::restoreOverrideCursor();}
  for(auto& job:stabilizerJobs_)if(job){job->cancel();job.reset();QApplication::restoreOverrideCursor();}
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
  cancelProcessing_->setText("Cancel");cancelProcessing_->setEnabled(active);
  cancelProcessing_->setVisible(active);
  if(active)QApplication::setOverrideCursor(Qt::WaitCursor);else QApplication::restoreOverrideCursor();
}

void MainWindow::updateWingModel() {
  // Editor warnings run a nested event loop. Do not start queued generation
  // until the user dismisses the warning and editor finalization completes.
  if(QApplication::activeModalWidget())return;
  if(restoringProject_ || modelJob_ || fuselageJob_ || stabilizerProcessing() || assemblyProcessing())return;
  if(dataPanel_->property("workspaceIndex").toInt()==5){updateAssembly();return;}
  const int stabilizer=dataPanel_->property("workspaceIndex").toInt()-3;
  if(stabilizer>=0 && stabilizer<2){updateStabilizerModel(stabilizer);return;}
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
    modelJob_=std::make_unique<processing::BackgroundJob<geometry::WingBuildResult>>(
      [input=std::move(input)](std::stop_token stop,const auto& progress) {
        return geometry::buildWingModel(input,progress,{{stop}});
      });
    jobEpoch_=projectEpoch_;jobFingerprint_=fingerprint;setModelProcessing(true);
    viewport_->setProperty("wingModelReady",false);
    statusBar()->showMessage("Preparing wing geometry...");
  } catch(const std::exception& error) {statusBar()->showMessage("Could not start wing generation: "+QString::fromUtf8(error.what()));}
}

void MainWindow::pollModelJob() {
  if(assemblyProcessing()){pollAssemblyJob();return;}
  for(int i=0;i<2;++i)if(stabilizerJobs_[i]){pollStabilizerJob(i);return;}
  if(fuselageJob_){pollFuselageJob();return;}
  if(!modelJob_)return;
  for(const auto& message:modelJob_->messages())
    if(!modelJob_->cancelled())statusBar()->showMessage(QString::fromStdString(message));
  if(!modelJob_->ready())return;
  const bool cancelled=modelJob_->cancelled();
  const bool obsolete=jobEpoch_!=projectEpoch_ || jobFingerprint_!=wingFingerprint();
  geometry::WingBuildResult model;QString error;
  try {model=modelJob_->take();}
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
        wingShape_=model.shape;wingSparMaterials_=std::move(model.spars);displayedComponent_=1;
        viewport_->displayShape(model.shape,!camera.has_value());if(camera)viewport_->restoreCamera(camera);
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

double MainWindow::projectLengthScale() const {
  if(projectReference().toScale)return 1.;
  if(!projectReference().wingspanMm)return 0.;
  try {
    return wingCalibration(planViewport_->sketchEditor().layers(),
        planViewport_->sketchEditor().stationEditor().lines(),projectReference().wingspanMm).scale;
  } catch(const std::exception&) {return 0.;} // Wing definition is still incomplete.
}
std::optional<double> MainWindow::scaledFuselageLength() const {
  if(projectReference().toScale)return std::nullopt;
  const auto& layers=planViewport_->fuselageSketchEditor().layers();
  if(layers.size()<2)return 0.;
  const auto boundary=closedSketchBoundary(layers[1]);
  if(!boundary)return 0.;
  double left=boundary->front().x(),right=left;
  for(auto point:*boundary){left=std::min(left,point.x());right=std::max(right,point.x());}
  return (right-left)*projectLengthScale();
}
QByteArray MainWindow::stabilizerFingerprint(int index) const {
  const auto p=encodeProject(projectDocument(),false);
  return QJsonDocument{QJsonObject{
      {"outline",p[index==0?"horizontalStabilizerOutline":"verticalStabilizerOutline"].toObject()["layers"]},
      {"hinge",p[index==0?"horizontalStabilizerHinge":"verticalStabilizerHinge"].toObject()["layers"]},
      {"hingeCut",p["stabilizerHingeCuts"].toArray()[index]},
      {"cuts",p[index==0?"horizontalStabilizerCuts":"verticalStabilizerCuts"].toObject()["layers"]},
      {"airfoil",p["stabilizerAirfoils"].toArray()[index]}, {"scale",projectLengthScale()}}}.toJson(QJsonDocument::Compact);
}
void MainWindow::updateStabilizerModel(int index) {
  if(restoringProject_ || modelJob_ || fuselageJob_ || stabilizerProcessing() || assemblyProcessing() || graphicsTabs_->currentWidget()!=viewport_)return;
  const auto fingerprint=stabilizerFingerprint(index);
  // A cache entry represents completed geometry only. Navigation must neither
  // launch a worker nor replace the successful fingerprint with an attempted one.
  if(!stabilizerShapes_[index].IsNull() && fingerprint==builtStabilizerFingerprints_[index]) {
    if(displayedComponent_!=index+3 || !viewport_->property("stabilizerModelReady").toBool()) {
      viewport_->displayShape(stabilizerShapes_[index],!stabilizerCameras_[index]);
      if(stabilizerCameras_[index])viewport_->restoreCamera(stabilizerCameras_[index]);
    }
    displayedComponent_=index+3;
    viewport_->setProperty("stabilizerModelReady",true);
    viewport_->setProperty("stabilizerComponent",index);
    int bodies=0;for(TopExp_Explorer e{stabilizerShapes_[index],TopAbs_SOLID};e.More();e.Next())++bodies;
    viewport_->setProperty("stabilizerBodyCount",bodies);
    statusBar()->showMessage(index==0?"Horizontal stabilizer: using cached model":"Vertical stabilizer: using cached model");
    return;
  }
  if(stabilizerCancelled_[index] && fingerprint==stabilizerJobFingerprints_[index])return;
  stabilizerCancelled_[index]=false;
  viewport_->setProperty("stabilizerModelReady",false);
  const auto& outline=planViewport_->stabilizerSketchEditor(index).layers().front();
  if(!stabilizerOutlineDefined(outline)) {
    viewport_->clearShape();
    statusBar()->showMessage("Stabilizer: complete a valid open outline before entering 3D.");return;
  }
  geometry::StabilizerSolidInput input{outline,stabilizerAirfoilPanels_[index]->airfoil(),projectLengthScale(),index==0,planViewport_->stabilizerHingeEditor(index).layers()[0],stabilizerHingePanels_[index]->cut(),planViewport_->stabilizerCutEditor(index).layers()};
  try {
    geometry::validateStabilizerCuts(input.cutShapes);
    stabilizerJobs_[index]=std::make_unique<processing::BackgroundJob<geometry::StabilizerBuildResult>>(
        [input=std::move(input)](std::stop_token stop,const auto& progress) {
          return geometry::buildStabilizerModel(input,progress,{stop});
        });
    stabilizerJobFingerprints_[index]=fingerprint;stabilizerJobEpochs_[index]=projectEpoch_;
    setProperty("stabilizerModelJobCount",property("stabilizerModelJobCount").toInt()+1);
    setModelProcessing(true);
    statusBar()->showMessage(index==0?"Preparing horizontal stabilizer...":"Preparing vertical stabilizer...");
  } catch(const std::exception& error) {
    stabilizerCancelled_[index]=true;stabilizerJobFingerprints_[index]=fingerprint;
    viewport_->clearShape();
    statusBar()->showMessage("Could not start stabilizer generation: "+QString::fromUtf8(error.what()));
  }
}
void MainWindow::pollStabilizerJob(int index) {
  auto& job=stabilizerJobs_[index];
  for(const auto& message:job->messages())if(!job->cancelled())statusBar()->showMessage(QString::fromStdString(message));
  if(!job->ready())return;
  const bool cancelled=job->cancelled();
  const bool obsolete=stabilizerJobEpochs_[index]!=projectEpoch_ || stabilizerJobFingerprints_[index]!=stabilizerFingerprint(index);
  TopoDS_Shape shape;geometry::StabilizerBuildResult result;QString error;
  try{result=job->take();shape=result.shape;}
  catch(const Standard_Failure& failure){error=QString::fromUtf8(failure.what());}
  catch(const std::exception& failure){error=QString::fromUtf8(failure.what());}
  catch(...){error="Unknown geometry processing failure.";}
  job.reset();setModelProcessing(false);
  if(!obsolete) {
    if(cancelled) {
      stabilizerCancelled_[index]=true;
      statusBar()->showMessage("Stabilizer generation cancelled. The previous display is retained. Re-enter 3D View to retry.");
    } else if(!error.isEmpty()) {
      stabilizerCancelled_[index]=true;
      viewport_->clearShape();
      statusBar()->showMessage("Stabilizer generation failed: "+error);
    } else try {
      auto camera=stabilizerShapes_[index].IsNull()?stabilizerCameras_[index]:viewport_->cameraState();
      viewport_->displayShape(shape,!camera);if(camera)viewport_->restoreCamera(camera);
      stabilizerModels_[index]=result;stabilizerShapes_[index]=shape;builtStabilizerFingerprints_[index]=stabilizerJobFingerprints_[index];displayedComponent_=index+3;
      stabilizerCameras_[index]=viewport_->cameraState();
      viewport_->setProperty("stabilizerModelReady",true);
      viewport_->setProperty("stabilizerModelRevision",viewport_->property("stabilizerModelRevision").toInt()+1);
      viewport_->setProperty("stabilizerComponent",index);
      int bodies=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++bodies;
      viewport_->setProperty("stabilizerBodyCount",bodies);
      statusBar()->showMessage(index==0?"Horizontal stabilizer updated: mirrored stabilizer and control bodies":"Vertical stabilizer updated: fin and control bodies");
    } catch(const Standard_Failure& failure){statusBar()->showMessage("Stabilizer display failed: "+QString::fromUtf8(failure.what()));}
      catch(const std::exception& failure){statusBar()->showMessage("Stabilizer display failed: "+QString::fromUtf8(failure.what()));}
  }
  if(closingAfterProcessing_){closingAfterProcessing_=false;close();}
  else if(obsolete){updateWorkspaceAvailability();updateProjectTitle();updateWingModel();}
}

QByteArray MainWindow::fuselageFingerprint() const {
  const auto p=encodeProject(projectDocument(),false);
  return QJsonDocument{QJsonObject{{"noseOpen",p["fuselageNoseOpen"]},{"tailOpen",p["fuselageTailOpen"]},{"outlines",p["fuselageOutline"].toObject()["layers"]},
      {"stations",p["fuselageStations"].toObject()["lines"]},
      {"profiles",p["fuselageProfiles"].toObject()["layers"]},
      {"formerAngles",p["formers"].toObject()["rotationDegrees"]},{"formers",p["formers"].toObject()["rectangles"]},{"tray",p["servoTray"].toObject()["rectangle"]},{"thicken",p["fuselageThickening"]},{"cuts",p["fuselageCuts"].toObject()["layers"]},{"holes",p["fuselageHoles"].toObject()["layers"]},
      {"length",projectReference().toScale?QJsonValue{}:QJsonValue{scaledFuselageLength().value_or(0.)}}}}.toJson(QJsonDocument::Compact);
}
void MainWindow::updateFuselageModel() {
  if(restoringProject_||modelJob_||fuselageJob_||stabilizerProcessing()||assemblyProcessing()||graphicsTabs_->currentWidget()!=viewport_||dataPanel_->property("workspaceIndex").toInt()!=2)return;
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
  builtFuselageFingerprint_=fingerprint;fuselageRetryPending_=false;
  viewport_->setProperty("fuselageModelReady",false);
  viewport_->setProperty("servoTrayReady",false);viewport_->setProperty("formerCount",0);servoTrayTopFaces_.Nullify();
  if(!fuselageOutlinePanel_->outlinesDefined()||!fuselageProfilePanel_->allProfilesClosed()) {
    viewport_->clearShape();fuselageShape_.Nullify();
    statusBar()->showMessage("Fuselage: close both outlines and assign a closed profile to every station before opening 3D.");return;
  }
  geometry::FuselageSolidInput input{planViewport_->fuselageSketchEditor().layers(),
      planViewport_->fuselageSketchEditor().stationEditor().lines(),planViewport_->fuselageProfileEditor().layers(),
      scaledFuselageLength(),fuselageThickenPanel_->enabled(),planViewport_->fuselageCutEditor().layers(),planViewport_->servoTrayEditor().state().rectangle,planViewport_->formerEditor().state().rectangles,planViewport_->formerEditor().state().rotationDegrees,planViewport_->fuselageHoleEditor().layers()};
  input.noseOpen=fuselageOutlinePanel_->noseOpen();input.tailOpen=fuselageOutlinePanel_->tailOpen();
  try {
    // A separate owned worker and immutable snapshot; no Wing state is read.
    fuselageJob_=std::make_unique<processing::BackgroundJob<geometry::FuselageBuildResult>>(
      [input=std::move(input)](std::stop_token stop,const auto& progress){return geometry::buildFuselageModel(input,progress,{stop});});
    fuselageJobEpoch_=projectEpoch_;fuselageJobFingerprint_=fingerprint;setModelProcessing(true);
    statusBar()->showMessage("Preparing fuselage geometry...");
  } catch(const std::exception& error){fuselageRetryPending_=true;statusBar()->showMessage("Could not start fuselage generation: "+QString::fromUtf8(error.what()));}
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
      fuselageRetryPending_=true;fuselageShape_.Nullify();viewport_->clearShape();statusBar()->showMessage("Fuselage generation failed: "+error);
    } else try {
      const auto& shape=result.shape;servoTrayTopFaces_=result.servoTrayTopFaces;
      viewport_->setProperty("servoTrayReady",!result.servoTray.IsNull());
      viewport_->setProperty("formerCount",static_cast<int>(result.formers.size()));
      const auto camera=restoredFuselageCamera_?restoredFuselageCamera_:fuselageShape_.IsNull()?std::nullopt:viewport_->cameraState();
      fuselageModel_=result;fuselageShape_=shape;displayedComponent_=2;viewport_->displayShape(shape,!camera.has_value());if(camera)viewport_->restoreCamera(camera);
      restoredFuselageCamera_.reset();
      viewport_->setProperty("fuselageModelReady",true);
      viewport_->setProperty("fuselageModelRevision",viewport_->property("fuselageModelRevision").toInt()+1);
      int bodies=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++bodies;
      viewport_->setProperty("fuselageBodyCount",bodies);
      const int cutouts=bodies-2-static_cast<int>(result.formers.size())-(result.servoTray.IsNull()?0:1);
      statusBar()->showMessage(QString{"Fuselage updated: left/right main halves with 4 alignment pins; %1 intact cut-out parts; %2 formers%3%4; %5 bodies"}
          .arg(cutouts).arg(result.formers.size())
          .arg(result.formers.empty()?QString{}:QString{" with 4 x 3 mm retaining rails"})
          .arg(result.servoTray.IsNull()?QString{}:QString{" and servo tray"}).arg(bodies));
    } catch(const Standard_Failure& failure){fuselageRetryPending_=true;statusBar()->showMessage("Fuselage display failed: "+QString::fromUtf8(failure.what()));}
      catch(const std::exception& failure){fuselageRetryPending_=true;statusBar()->showMessage("Fuselage display failed: "+QString::fromUtf8(failure.what()));}
  }
  if(closingAfterProcessing_){closingAfterProcessing_=false;close();}
  else if(obsolete){updateWorkspaceAvailability();updateProjectTitle();updateWingModel();}
}

void MainWindow::updateWorkspaceAvailability() {
  if (!workspaceToolBar_) return;
  applyReferenceWorkflow(*workspaceToolBar_, projectReference(), wingDefinitions_.outlineDefined && wingDefinitions_.stationsDefined && wingDefinitions_.airfoilsDefined);
  const bool fuselageReady=workspaceActions_.at(2)->isEnabled()&&fuselageOutlinePanel_->outlinesDefined()&&fuselageProfilePanel_->allProfilesClosed();
  workspaceActions_.at(3)->setEnabled(fuselageReady);
  workspaceActions_.at(4)->setEnabled(fuselageReady);
  workspaceActions_.at(5)->setEnabled(fuselageReady &&
      stabilizerOutlineDefined(planViewport_->stabilizerSketchEditor(0).layers().front()) &&
      stabilizerOutlineDefined(planViewport_->stabilizerSketchEditor(1).layers().front()));
  updateExportAvailability();
  workspaceActions_.at(8)->setEnabled(projectOpen_);
  workspaceActions_.at(7)->setEnabled(projectOpen_&&referenceReady(projectReference())&&projectLengthScale()>0&&
      closedSketchBoundary(planViewport_->fuselageSketchEditor().layers()[1]).has_value());
  const int current = dataPanel_->property("workspaceIndex").toInt();
  if (!workspaceActions_.at(current)->isEnabled()) {
    const int fallback = referenceReady(projectReference()) ? 1 : 0;
    workspaceActions_.at(fallback)->setChecked(true);
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
  undoAction_=edit->addAction("&Undo");undoAction_->setObjectName("editUndo");
  undoAction_->setShortcut(QKeySequence{Qt::CTRL|Qt::Key_Z});
  connect(undoAction_,&QAction::triggered,this,[this]{applyEdit(false);});
  redoAction_=edit->addAction("&Redo");redoAction_->setObjectName("editRedo");
  redoAction_->setShortcut(QKeySequence{Qt::CTRL|Qt::Key_Y});
  connect(redoAction_,&QAction::triggered,this,[this]{applyEdit(true);});
  edit->addSeparator();
  auto* copyAction = edit->addAction("&Copy");
  copyAction->setShortcut(QKeySequence::Copy);
  connect(copyAction, &QAction::triggered, this, [this] { copyFocusedText(); });
  auto* pasteAction = edit->addAction("&Paste");
  pasteAction->setShortcut(QKeySequence::Paste);
  connect(pasteAction, &QAction::triggered, this, [this] { pasteFocusedText(); });

  auto* view = menuBar()->addMenu("&View");
  view->setObjectName("viewMenu");
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
  viewport_->setViewActions(view->actions());
  auto* help = menuBar()->addMenu("&Help");
  auto* about = help->addAction("&About");
  connect(about, &QAction::triggered, this, [this] { showAbout(); });
}

void MainWindow::setCameraView(CameraView cameraView) {
  if(dataPanel_->property("workspaceIndex").toInt()==7)return;
  graphicsTabs_->setCurrentWidget(viewport_);
  viewport_->setCameraView(cameraView);
}

void MainWindow::resetProject() {
  if(assemblyPrepareJob_)assemblyPrepareJob_->cancel();if(assemblyCutJob_)assemblyCutJob_->cancel();
  inspectPreparing_=false;inspectFitAfterBuild_=false;inspectPanel_->restore({});
  statistics_={};
  weightBalancePanel_->restore({});weightBalancePanel_->setFoam({}, {}, "Generate Assembly to calculate the complete model.");
  for(int i=0;i<4;++i)fiberglassPanels_[i]->restore(ProjectDocument{}.fiberglass[i]);
  assemblyState_={};assemblyOriginals_={};assemblyCutParts_.reset();fuselageModel_={};stabilizerModels_={};
  assemblySourceFingerprint_.clear();assemblyAttemptFingerprint_.clear();assemblySelected_=-1;
  for(auto* button:assemblySelect_)button->setChecked(false);
  balanceMassCache_.reset();balanceMassSources_.clear();balanceMassFingerprint_.clear();
  ++projectEpoch_;for(auto& job:stabilizerJobs_)if(job)job->cancel();if(modelJob_)modelJob_->cancel();if(fuselageJob_)fuselageJob_->cancel();
  ProcessingScope processing{this, "Creating empty project..."};
  restoringProject_=true;projectOpen_=true;centralWidget()->setEnabled(true);
  workspaceToolBar_->setEnabled(!modelJob_&&!fuselageJob_&&!stabilizerProcessing()&&!assemblyProcessing());componentToolBar_->setEnabled(!modelJob_&&!fuselageJob_&&!stabilizerProcessing()&&!assemblyProcessing());
  projectPath_.clear();
  builtWingFingerprint_.clear();builtFuselageFingerprint_.clear();fuselageRetryPending_=false;
  for(int i=0;i<2;++i){builtStabilizerFingerprints_[i].clear();stabilizerShapes_[i].Nullify();stabilizerAirfoilPanels_[i]->restore({});stabilizerCameras_[i].reset();stabilizerCancelled_[i]=false;}
  viewport_->setProperty("stabilizerModelReady",false);
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
  fuselageHolePanel_->setActive(false,false);fuselageHolePanel_->restoreControls();
  servoTrayPanel_->setActive(false);
  formerPanel_->setActive(false);
  fuselageOutlinePanel_->reset();
  for (auto* panel : stabilizerOutlinePanels_) panel->reset();
  for(auto* panel:stabilizerHingePanels_)panel->restore(HingeCut::Tape);
  controlSurfacePanel_->restoreControls();
  graphicsTabs_->setCurrentWidget(planViewport_);
  workspaceActions_.front()->setChecked(true);
  selectWorkspace(0);
  updateWorkspaceAvailability();
  statusBar()->showMessage("New project ready; specify the Reference dimensions");
  restoringProject_=false;
  savedFingerprint_=projectFingerprint();updateProjectTitle();
  if(!applyingHistory_)resetEditHistory();
}

ProjectDocument MainWindow::projectDocument() const {
  ProjectDocument p;
  p.statistics=statistics_;
  p.componentNames=inspectPanel_->names();
  p.weightBalance=weightBalancePanel_->state();
  for(int i=0;i<4;++i)p.fiberglass[i]=fiberglassPanels_[i]->state();
  p.assembly=assemblyState_;
  p.reference=projectReference();
  p.wingspanText=findChild<QLineEdit*>("referenceWingspan")->text();
  p.fuselageText={}; // Legacy field retained in the file schema only.
  p.wing=planViewport_->sketchEditor().state();p.airfoilSketch=planViewport_->airfoilSketchEditor().state();
  for (int i = 0; i < 2; ++i) {p.stabilizerCuts[i]=planViewport_->stabilizerCutEditor(i).state();p.stabilizerHinges[i]=planViewport_->stabilizerHingeEditor(i).state();p.stabilizerHingeCuts[i]=stabilizerHingePanels_[i]->cut();p.stabilizerOutlines[i] = planViewport_->stabilizerSketchEditor(i).state();p.stabilizerAirfoils[i]=stabilizerAirfoilPanels_[i]->selection();}
  p.fuselageThickening=fuselageThickenPanel_->enabled();
  p.fuselageProfiles=planViewport_->fuselageProfileEditor().state();
  p.fuselageCuts=planViewport_->fuselageCutEditor().state();
  p.fuselageHoles=planViewport_->fuselageHoleEditor().state();
  p.servoTray=planViewport_->servoTrayEditor().state();
  p.formers=planViewport_->formerEditor().state();
  p.fuselageStations=planViewport_->fuselageSketchEditor().stationEditor().state();
  p.fuselageNoseOpen=fuselageOutlinePanel_->noseOpen();p.fuselageTailOpen=fuselageOutlinePanel_->tailOpen();
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
  snapshot.remove("airplaneStatistics"); // Derived cache is not an edit or undo step.
  snapshot.remove("ui"); // Still saved/restored, but navigation is not a document edit.
  auto fiberglass=snapshot["fiberglass"].toArray();
  for(int i=0;i<fiberglass.size();++i){auto component=fiberglass[i].toObject();auto sketch=component["sketch"].toObject();sketch.remove("selected");sketch.remove("editing");if(sketch["pending"].toArray().isEmpty()){sketch.remove("tool");sketch.remove("active");}component["sketch"]=sketch;fiberglass[i]=component;}
  snapshot["fiberglass"]=fiberglass;
  for (const char* key : {"wingOutline", "airfoilSketches", "fuselageOutline", "fuselageProfiles", "fuselageCuts", "fuselageHoles", "horizontalStabilizerOutline", "verticalStabilizerOutline", "horizontalStabilizerHinge", "verticalStabilizerHinge", "horizontalStabilizerCuts", "verticalStabilizerCuts"}) {
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
      auto spar=value.toObject();spar.remove("sizeText");spar.remove("heightText");spar.remove("insideDiameterText");entries.append(spar);
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
  if(!restoringProject_) {
    invalidateAssembly();
    if(dataPanel_->property("workspaceIndex").toInt()==7&&assemblyOriginals_.fuselage.IsNull())
      weightBalancePanel_->setFoam({}, {}, "Generate the current Assembly to calculate the complete model.");
  }
  if(!restoringProject_)updateStatistics();
  updateEditActions();
  setWindowModified(projectModified());
  setWindowTitle(projectOpen_?(projectPath_.isEmpty()?"Untitled":QFileInfo{projectPath_}.fileName())+"[*] - FoamAirplaneStudio":"FoamAirplaneStudio");
  if(!modelJob_&&!fuselageJob_&&!stabilizerProcessing()&&!assemblyProcessing()){saveAction_->setEnabled(projectOpen_);saveAsAction_->setEnabled(projectOpen_);closeAction_->setEnabled(projectOpen_);}
}
bool MainWindow::saveProjectFile(const QString& path,QString& error) {
  invalidateAssembly();
  if(!projectOpen_) {error="No project is open.";return false;}
  updateStatistics();
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
  if(modelJob_||fuselageJob_||stabilizerProcessing()||assemblyProcessing()) {
    if(assemblyPrepareJob_)assemblyPrepareJob_->cancel();if(assemblyCutJob_)assemblyCutJob_->cancel();
    for(auto& job:stabilizerJobs_)if(job)job->cancel();
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
void MainWindow::restoreProject(const ProjectDocument& saved) {
  // Opening is an editing operation: never regenerate from a saved 3D view.
  // Assembly is 3D-only, so restore its data in the Fuselage 2D workspace.
  auto p=saved;if(!applyingHistory_)p.viewport=0;
  if(!applyingHistory_&&(p.workspace==5||p.workspace==6||p.workspace==8)) {
    p.workspace=2;p.tool="Outline";
    // Keep a pending sketch attached to its original layer. Otherwise open Side
    // View and keep the saved panel and active sketch layer consistent.
    if(p.fuselage.pending.empty()){p.fuselage.active=1;p.fuselage.selected=-1;}
    p.fuselageView=p.fuselage.active;
  }

  if(!applyingHistory_)resetProject();
  restoringProject_=true;
  if(applyingHistory_&&(assemblyState_.offsets!=p.assembly.offsets||assemblyState_.rotationDegrees!=p.assembly.rotationDegrees))assemblyCutParts_.reset();
  statistics_=p.statistics;assemblyState_=p.assembly;weightBalancePanel_->restore(p.weightBalance);inspectPanel_->restore(p.componentNames,applyingHistory_);
  statusBar()->showMessage("Restoring reference, sketches and project settings...");
  statusBar()->repaint();
  referencePanel_->restoreReference(p.reference);
  planViewport_->setReferenceBackground(p.reference.image.pages,p.reference.toScale);
  {
    auto* span=findChild<QLineEdit*>("referenceWingspan");
    QSignalBlocker a{span};span->setText(p.wingspanText);
  }
  planViewport_->sketchEditor().restoreState(p.wing);
  planViewport_->airfoilSketchEditor().restoreState(p.airfoilSketch);
  planViewport_->sketchEditor().stationEditor().restoreState(p.stations);
  wingOutlinePanel_->restoreControls();
  planViewport_->fuselageSketchEditor().restoreState(p.fuselage);
  fuselageOutlinePanel_->restoreControls(p.fuselageView);

  planViewport_->fuselageProfileEditor().restoreState(p.fuselageProfiles);
  planViewport_->fuselageSketchEditor().stationEditor().restoreState(p.fuselageStations);
  for (int i = 0; i < 2; ++i) {planViewport_->stabilizerSketchEditor(i).restoreState(p.stabilizerOutlines[i]);stabilizerAirfoilPanels_[i]->restore(p.stabilizerAirfoils[i]);}
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
  const int workspace=workspaceActions_[p.workspace]->isEnabled()?p.workspace:0;
  workspaceActions_[workspace]->setChecked(true);selectWorkspace(workspace);
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
  graphicsTabs_->setCurrentIndex(workspace==5?1:p.viewport);planViewport_->restoreView(p.plan);
  planViewport_->controlSurfaceEditor().restore(p.controls);
  sparPanel_->restore(p.spars,p.reference.units,p.selectedSparPanel);
  lighteningPanel_->restore(p.lightening,p.reference.units);
  controlSurfacePanel_->restoreControls();
  auto fuselage=p.fuselage;
  fuselage.editing=workspace==2&&p.tool=="Outline"&&p.viewport==0&&p.fuselageView>=0;
  planViewport_->fuselageSketchEditor().restoreState(fuselage);
  fuselageOutlinePanel_->restoreControls(p.fuselageView);
  {
    bool nose=false,tail=false;
    // Resolve legacy inference once when loading; subsequent outline edits do not change the user's choices.
    if((!p.fuselageNoseOpen||!p.fuselageTailOpen)&&!p.fuselageStations.lines.empty())try {
      const auto ends=geometry::registerFuselageEnds(p.fuselage.layers.at(1),scaledFuselageLength());
      double first=1e100,last=-1e100;
      for(const auto& station:p.fuselageStations.lines){first=std::min(first,station.first.position.x());last=std::max(last,station.first.position.x());}
      nose=ends.noseStation(first);tail=ends.tailStation(last);
    }catch(const std::exception&){}
    fuselageOutlinePanel_->setEnds(p.fuselageNoseOpen.value_or(nose),p.fuselageTailOpen.value_or(tail));
  }
  fuselageThickenPanel_->setUnits(p.reference.units);
  fuselageThickenPanel_->restore(p.fuselageThickening);
  updateFuselageProgress();
  updateFuselageStationMode();
  planViewport_->fuselageSketchEditor().stationEditor().restoreState(p.fuselageStations);
  fuselageProfilePanel_->selectStation();
  auto profileState=p.fuselageProfiles;profileState.editing=workspace==2&&p.tool=="Edit Profiles"&&p.viewport==0&&profileState.editing;
  planViewport_->fuselageProfileEditor().restoreState(profileState);
  fuselageProfilePanel_->restoreControls();
  auto holes=p.fuselageHoles;holes.editing=workspace==2&&p.tool=="Holes"&&p.viewport==0;
  planViewport_->fuselageHoleEditor().restoreState(holes);fuselageHolePanel_->restoreControls();
  auto cuts=p.fuselageCuts;cuts.editing=workspace==2&&p.tool=="Cut"&&p.viewport==0;
  planViewport_->fuselageCutEditor().restoreState(cuts);fuselageCutPanel_->restoreControls();
  planViewport_->servoTrayEditor().restore(p.servoTray);
  planViewport_->formerEditor().restore(p.formers);
  updateFuselageProgress();
  for (int i = 0; i < 2; ++i) {
    auto state = p.stabilizerOutlines[i];
    state.editing = workspace == i + 3 && p.tool == "Outline" && p.viewport == 0;
    planViewport_->stabilizerSketchEditor(i).restoreState(state);
    stabilizerOutlinePanels_[i]->restoreControls();
    auto hinge=p.stabilizerHinges[i];hinge.editing=workspace==i+3 && p.tool=="Hinge Line" && p.viewport==0;
    planViewport_->stabilizerHingeEditor(i).restoreState(hinge);
    stabilizerHingePanels_[i]->restore(p.stabilizerHingeCuts[i]);
    auto stabCuts=p.stabilizerCuts[i];stabCuts.editing=workspace==i+3&&p.tool=="Cut"&&p.viewport==0;
    planViewport_->stabilizerCutEditor(i).restoreState(stabCuts);stabilizerCutPanels_[i]->restoreControls();
  }
  for(int i=0;i<4;++i){auto state=p.fiberglass[i];state.sketch.editing=workspace==i+1&&p.tool=="Fiberglass"&&p.viewport==0;fiberglassPanels_[i]->restore(state);}
  if(workspace==3 || workspace==4)stabilizerCameras_[workspace-3]=p.camera;
  else if(workspace==2)restoredFuselageCamera_=p.camera;else restoredWingCamera_=p.camera;
  // Establish the saved reference scale after restoring the former rectangles;
  // opening a project or undoing a scale edit must not rescale them again.
  updateFuselageStationMode();
  restoringProject_=false;
  if(!applyingHistory_)assemblySourceFingerprint_=assemblyFingerprint();
  updateStabilizerProgress();wingDirty_=true;
  if(!applyingHistory_)updateWingModel();
  viewport_->restoreCamera(p.camera);
  if(!applyingHistory_){savedFingerprint_=projectFingerprint();resetEditHistory();}
  updateProjectTitle();
}

void MainWindow::updateEditActions() {
  if(!undoAction_||!redoAction_)return;
  const bool enabled=projectOpen_&&!restoringProject_&&!applyingHistory_&&!modelJob_&&!fuselageJob_&&!stabilizerProcessing()&&!assemblyProcessing();
  undoAction_->setEnabled(enabled&&editPosition_>0);
  redoAction_->setEnabled(enabled&&editPosition_<editHistory_.size());
}
void MainWindow::resetEditHistory() {
  editHistory_.clear();editPosition_=0;editMouseDown_=false;
  editBaseline_=projectDocument();editFingerprint_=projectFingerprint();updateEditActions();
}
void MainWindow::captureEdit() {
  if(!editBaseline_||applyingHistory_||restoringProject_||!projectOpen_||editMouseDown_||
      modelJob_||fuselageJob_||stabilizerProcessing()||assemblyProcessing()||
      planViewport_->sketchEditor().stationEditor().moving()||
      planViewport_->fuselageSketchEditor().stationEditor().moving())return;
  invalidateAssembly();
  auto fingerprint=projectFingerprint();auto current=projectDocument();
  if(fingerprint!=editFingerprint_) {
    editHistory_.erase(editHistory_.begin()+editPosition_,editHistory_.end());
    editHistory_.push_back({*editBaseline_,current});
    if(editHistory_.size()>100)editHistory_.erase(editHistory_.begin());
    editPosition_=editHistory_.size();
  }
  // Refresh navigation/selection even when it does not create a revision, so
  // undo returns to the editor in which the next change actually happened.
  editBaseline_=std::move(current);editFingerprint_=std::move(fingerprint);updateEditActions();
}
void MainWindow::applyEdit(bool redo) {
  captureEdit();
  if(!projectOpen_||modelJob_||fuselageJob_||stabilizerProcessing()||assemblyProcessing())return;
  if(redo?editPosition_==editHistory_.size():editPosition_==0)return;
  const auto target=redo?editHistory_[editPosition_++].after:editHistory_[--editPosition_].before;
  applyingHistory_=true;editMouseDown_=false;
  restoreProject(target);
  applyingHistory_=false;
  editBaseline_=projectDocument();editFingerprint_=projectFingerprint();
  updateProjectTitle();
  const int workspace=dataPanel_->property("workspaceIndex").toInt();
  if(workspace==8)updateInspect(false,false);
  else if(workspace==5)displayAssembly();
  else if(workspace==7)updateWeightBalance();
  planViewport_->viewport()->update();
  statusBar()->showMessage(redo?"Redid edit":"Undid edit",3000);
}
bool MainWindow::eventFilter(QObject* object,QEvent* event) {
  auto* widget=qobject_cast<QWidget*>(object);
  if(!widget||(widget!=this&&!isAncestorOf(widget))||applyingHistory_||restoringProject_)
    return QMainWindow::eventFilter(object,event);
  const auto type=event->type();
  if(type==QEvent::ShortcutOverride) {
    auto* key=static_cast<QKeyEvent*>(event);
    if(key->modifiers()==Qt::ControlModifier&&(key->key()==Qt::Key_Z||key->key()==Qt::Key_Y)) {
      captureEdit();key->accept();return true;
    }
  }
  if(type==QEvent::KeyPress) {
    auto* key=static_cast<QKeyEvent*>(event);
    if(key->modifiers()==Qt::ControlModifier&&(key->key()==Qt::Key_Z||key->key()==Qt::Key_Y)) {
      applyEdit(key->key()==Qt::Key_Y);return true;
    }
  }
  if(type==QEvent::MouseButtonPress||type==QEvent::KeyPress)captureEdit();
  if(type==QEvent::MouseButtonPress)editMouseDown_=true;
  if(type==QEvent::MouseButtonRelease)editMouseDown_=false;
  if(type==QEvent::MouseButtonRelease||type==QEvent::KeyPress||type==QEvent::FocusOut)
    QTimer::singleShot(0,this,[this]{captureEdit();});
  return QMainWindow::eventFilter(object,event);
}

const ProjectReference& MainWindow::projectReference() const {
  return referencePanel_->projectReference();
}

void MainWindow::showAbout() {
  QMessageBox::about(this, "About FoamAirplaneStudio",
      QString{"<h2>FoamAirplaneStudio</h2>"
              "<p>Version %1</p>"
              "<p>Design foam RC airplanes from reference drawings. Create wings, "
              "fuselages and stabilizers, assemble components, and calculate "
              "weight, center of gravity and wing loading. Export parts for "
              "CNC routing, 3D printing and laser cutting.</p>"
              "<p>Developed using OpenAI Codex.</p>"
              "<p>Copyright &copy; 2026 Barry Foust. GNU GPL version 3 only; "
              "absolutely no warranty.</p>"
              "<p>Uses Qt 6, Open CASCADE Technology, and FreeType.</p>"}
          .arg(QApplication::applicationVersion().toHtmlEscaped()));
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

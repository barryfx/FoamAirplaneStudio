#include "gui/FuselageOutlinePanel.h"
#include "gui/FuselageProfilePanel.h"
#include "gui/StabilizerOutlinePanel.h"
#include "geometry/StabilizerCut.h"
#include "gui/InspectPanel.h"
#include <QFileInfo>
#include "gui/MainWindow.h"
#include "gui/AirfoilPanel.h"
#include "gui/DihedralPanel.h"
#include "gui/LighteningPanel.h"
#include "gui/SparPanel.h"
#include "gui/FuselageThickenPanel.h"
#include "gui/StabilizerAirfoilPanel.h"
#include "gui/StabilizerHingePanel.h"
#include "gui/PlanViewport.h"
#include "gui/WingCalibration.h"
#include "gui/ExportPanel.h"
#include "gui/FileSelectionDialog.h"
#include "gui/ProcessingScope.h"
#include "geometry/WingSolidBuilder.h"
#include "processing/IndexedTasks.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <QButtonGroup>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QAction>
#include <QToolBar>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <TopExp_Explorer.hxx>
#include <numbers>
#include <numeric>
#include <cmath>

namespace designrc::gui {
std::string MainWindow::exportProjectName() const {
  return exportProjectStem(projectPath_.isEmpty()?QString{"Untitled"}:QFileInfo{projectPath_}.completeBaseName()).toStdString();
}
std::vector<geometry::ExportPart> MainWindow::namedExportParts(const geometry::AssemblyParts& parts) const {
  auto result=geometry::assemblyExportParts(parts);
  for(auto& part:result) {
    const auto it=inspectPanel_->names().constFind(QString::fromStdString(part.id));
    if(it!=inspectPanel_->names().cend())part.name=it.value().toStdString();
  }
  return result;
}
geometry::AssemblyParts MainWindow::cachedModelParts() const {
  geometry::AssemblyParts parts;
  if(builtWingFingerprint_==wingFingerprint()){parts.wing=wingShape_;parts.sparMaterials=wingSparMaterials_;}
  if(!parts.wing.IsNull()) {
    const auto& reference=projectReference();
    const auto root=wingCalibration(planViewport_->sketchEditor().layers(),planViewport_->sketchEditor().stationEditor().lines(),
        reference.toScale?std::nullopt:reference.wingspanMm);
    parts.rootCenters[0]={root.leadingEdgeX+root.rootChordMm*.5,0,0};
  }
  for(int i=0;i<2;++i)if(builtStabilizerFingerprints_[i]==stabilizerFingerprint(i)) {
    (i==0?parts.horizontal:parts.vertical)=stabilizerModels_[i].fixed;
    (i==0?parts.elevator:parts.rudder)=stabilizerModels_[i].control;
    const auto& outline=planViewport_->stabilizerSketchEditor(i).layers().front();
    std::vector<int> degree(outline.points.size());
    for(const auto& curve:outline.curves)for(std::size_t n=1;n<curve.points.size();++n){++degree[curve.points[n-1]];++degree[curve.points[n]];}
    std::vector<QPointF> ends;for(std::size_t n=0;n<degree.size();++n)if(degree[n]==1)ends.push_back(outline.points[n]);
    if(ends.size()==2)parts.rootCenters[i+1]={QLineF{ends[0],ends[1]}.length()*projectLengthScale()*.5,0,0};
  }
  if(builtFuselageFingerprint_!=fuselageFingerprint())return parts;
  parts.fuselage=fuselageModel_.body;
  int bodyNumber=0;
  for(TopExp_Explorer body{parts.fuselage,TopAbs_SOLID};body.More();body.Next()) {
    const auto id="Fuselage/"+std::to_string(bodyNumber);
    parts.fuselageParts.push_back({"Fuselage "+std::to_string(++bodyNumber),body.Current(),{},id});
  }
  if(!fuselageModel_.servoTray.IsNull())parts.inserts.push_back({"Servo Tray",fuselageModel_.servoTray,{},"Servo Tray"});
  const auto document=projectDocument();
  std::vector<std::size_t> order(fuselageModel_.formers.size());std::iota(order.begin(),order.end(),0);
  std::stable_sort(order.begin(),order.end(),[&](auto a,auto b){return document.formers.rectangles.at(a).center().x()<document.formers.rectangles.at(b).center().x();});
  if(!order.empty()) {
    const auto transform=geometry::fuselageSideTransform(document.fuselage.layers[1],scaledFuselageLength());
    int number=0;
    for(auto i:order) {
      const auto center=document.formers.rectangles.at(i).center();
      const double angle=document.formers.rotationDegrees.at(i)*std::numbers::pi/180.;
      const gp_Pnt origin{(center.x()-transform.left)*transform.scale,0,(transform.verticalOrigin-center.y())*transform.scale};
      const gp_Pln plane{gp_Ax3{origin,gp_Dir{std::cos(angle),0,-std::sin(angle)},gp_Dir{0,1,0}}};
      parts.inserts.push_back({"Former "+std::to_string(++number),fuselageModel_.formers[i],plane,"Former/"+std::to_string(i)});
    }
  }
  return parts;
}
void MainWindow::updateInspect(bool fit,bool regenerate) {
  if(regenerate&&!exportAssemblyParts()) {
    if(fit)assemblyAttemptFingerprint_.clear();
    inspectFitAfterBuild_=fit;
    updateAssembly(true);
  }
  const auto assembly=exportAssemblyParts();
  inspectPanel_->setParts(geometry::assemblyExportParts(assembly?*assembly:cachedModelParts()));
  displayInspect();if(fit)viewport_->fitAll();
}
void MainWindow::displayInspect() {
  if(dataPanel_->property("workspaceIndex").toInt()!=8)return;
  viewport_->displayInspection(inspectPanel_->visibleShapes());displayedComponent_=8;
}
void MainWindow::updateExportAvailability() {
  if(!workspaceToolBar_)return;
  const bool ready=projectOpen_&&!property("modelProcessing").toBool()&&exportAssemblyParts().has_value();
  workspaceActions_.at(6)->setEnabled(ready);
  if(exportPanel_)exportPanel_->setEnabled(ready);
}
void MainWindow::exportComponents() {
  if(!exportAssemblyParts()) {
    updateExportAvailability();
    QMessageBox::warning(this,"Export Components","Generate the current Assembly before exporting.");return;
  }
  const auto selected=exportPanel_->selectedParts();if(selected.empty())return;
  FileSelectionDialog dialog{this,"componentExportDirectory","Export Components",QFileDialog::Directory};
  dialog.setOption(QFileDialog::ShowDirsOnly,true);
  if(dialog.exec()!=QDialog::Accepted||dialog.selectedFiles().isEmpty())return;
  const QDir destination{dialog.selectedFiles().front()};
  std::vector<std::string> names;
  try {names=geometry::exportFileNames(selected,exportPanel_->formerFormat(),exportPanel_->componentFormat(),exportProjectName());}
  catch(const std::exception& error){QMessageBox::warning(this,"Export Components",QString::fromUtf8(error.what()));return;}
  QStringList existing;
  for(const auto& name:names)if(destination.exists(QString::fromStdString(name)))existing<<QString::fromStdString(name);
  if(!existing.empty()&&QMessageBox::question(this,"Replace exported files?",
      "Replace these files in the selected folder?\n"+existing.join('\n'),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
  QString error;int written=0;
  {
    ProcessingScope scope{this,"Exporting selected Assembly components..."};
    try {
      // Finish all CAD conversions before replacing any destination file.
      // Qt handles Unicode destination paths and atomic per-file replacement.
      QTemporaryDir staging;
      if(!staging.isValid())throw std::runtime_error("Could not create export staging directory.");
      geometry::writeComponentExports(selected,exportPanel_->formerFormat(),exportPanel_->componentFormat(),
          std::filesystem::path{staging.path().toStdWString()},exportProjectName());
      for(const auto& name:names) {
        const auto filename=QString::fromStdString(name);
        QFile input{QDir{staging.path()}.filePath(filename)};
        QSaveFile output{destination.filePath(filename)};
        if(!input.open(QIODevice::ReadOnly)||!output.open(QIODevice::WriteOnly))
          throw std::runtime_error(("Cannot write "+filename+": "+output.errorString()).toStdString());
        while(!input.atEnd()) {
          const auto bytes=input.read(1024*1024);
          if(bytes.isEmpty()&&input.error()!=QFile::NoError)throw std::runtime_error("Could not read staged export.");
          if(output.write(bytes)!=bytes.size())throw std::runtime_error(output.errorString().toStdString());
        }
        if(!output.commit())throw std::runtime_error(output.errorString().toStdString());
        ++written;
      }
    } catch(const Standard_Failure& e){error=QString::fromUtf8(e.what());}
      catch(const std::exception& e){error=QString::fromUtf8(e.what());}
  }
  if(!error.isEmpty()) {
    QMessageBox::warning(this,"Export failed",QString{"%1\n%2 of %3 files written."}.arg(error).arg(written).arg(names.size()));
    statusBar()->showMessage("Export failed: "+error);
  } else statusBar()->showMessage(QString{"Exported %1 files to %2"}.arg(written).arg(destination.absolutePath()));
}
void MainWindow::buildAssemblyPanel(QVBoxLayout* layout) {
  assemblyPanel_=new QWidget{dataContents_};assemblyPanel_->setObjectName("assemblyPanel");
  auto* box=new QVBoxLayout{assemblyPanel_};box->setContentsMargins(0,0,0,0);
  auto* instructions=new QLabel{
      "Position the aircraft components against the fuselage in the left-side 3D view. "
      "Select Wing, Horiz Stab or Vert Stab, then use the arrow keys: Left/Right move toward the nose/tail; "
      "Up/Down raise/lower the part. Each step is 1 mm (Shift: 10 mm; Ctrl: 0.1 mm). "
      "Rotate Clockwise/Counter-Clockwise changes the angle by 0.5 degrees about the midpoint of the root chord. "
      "Positive angles are clockwise in the standard side view, independent of camera orbit. "
      "The fuselage stays fixed and all parts remain on its centerline. "
      "The reference image is aligned behind the model; sketch lines are hidden. "
      "Mouse: left-drag to orbit, right-drag to pan, wheel to zoom around the cursor. Re-enter Assembly to return to the left-side view.\n\n"
      "Cut Intersections checks only Rudder against Elevator, cuts the wing and stabilizer seats in the fuselage, "
      "and cuts the fin slot in Horiz Stab. A collision dialog names any interfering control surfaces. "
      "Successful cuts lock positioning and become the export geometry. Undo Cuts restores the original parts "
      "at their current positions so you can adjust and try again.",assemblyPanel_};
  instructions->setObjectName("assemblyInstructions");instructions->setWordWrap(true);box->addWidget(instructions);
  auto* group=new QButtonGroup{assemblyPanel_};group->setExclusive(true);
  const std::array<const char*,3> names{"Wing","Horiz Stab","Vert Stab"};
  for(int i=0;i<3;++i) {
    auto* button=new QPushButton{names[i],assemblyPanel_};assemblySelect_[i]=button;
    button->setObjectName(QString{"assemblySelect%1"}.arg(i));button->setCheckable(true);
    button->setFocusPolicy(Qt::ClickFocus);group->addButton(button);box->addWidget(button);
    connect(button,&QPushButton::clicked,this,[this,i]{assemblySelected_=i;displayAssembly();});
  }
  for(int i=0;i<2;++i) {
    auto* button=new QPushButton{i==0?"Rotate Clockwise":"Rotate Counter-Clockwise",assemblyPanel_};assemblyRotate_[i]=button;
    button->setObjectName(i==0?"assemblyRotateClockwise":"assemblyRotateCounterClockwise");
    button->setEnabled(false);box->addWidget(button);
    connect(button,&QPushButton::clicked,this,[this,i]{rotateAssembly(i==0?.5:-.5);});
  }
  assemblyRotationLabel_=new QLabel{"Rotation: select a component",assemblyPanel_};
  assemblyRotationLabel_->setObjectName("assemblyRotationAngle");box->addWidget(assemblyRotationLabel_);
  box->addStretch();assemblyCutButton_=new QPushButton{"Cut Intersections",assemblyPanel_};
  assemblyCutButton_->setObjectName("assemblyCutIntersections");box->addWidget(assemblyCutButton_);
  connect(assemblyCutButton_,&QPushButton::clicked,this,[this]{toggleAssemblyCuts();});
  layout->addWidget(assemblyPanel_,1);assemblyPanel_->hide();
  for(int key:{Qt::Key_Left,Qt::Key_Right,Qt::Key_Up,Qt::Key_Down})
    for(auto modifier:{Qt::NoModifier,Qt::ShiftModifier,Qt::ControlModifier}) {
      auto* shortcut=new QShortcut{QKeySequence{key|modifier},assemblyPanel_};
      shortcut->setContext(Qt::WindowShortcut);
      connect(shortcut,&QShortcut::activated,this,[this,key,modifier]{moveAssembly(key,modifier);});
    }
}
QByteArray MainWindow::assemblyFingerprint() const {
  return wingFingerprint()+fuselageFingerprint()+stabilizerFingerprint(0)+stabilizerFingerprint(1);
}
void MainWindow::invalidateAssembly() {
  if(assemblySourceFingerprint_.isEmpty() || assemblySourceFingerprint_==assemblyFingerprint())return;
  assemblyOriginals_={};assemblyCutParts_.reset();assemblyState_.cuts=false;
  assemblySourceFingerprint_.clear();assemblyAttemptFingerprint_.clear();
  setProperty("assemblyReady",false);
  updateExportAvailability();
}
std::optional<geometry::AssemblyParts> MainWindow::exportAssemblyParts() const {
  if(assemblyOriginals_.fuselage.IsNull() || assemblySourceFingerprint_!=assemblyFingerprint())return {};
  if(assemblyState_.cuts)return assemblyCutParts_;
  return geometry::placeAssembly(assemblyOriginals_,assemblyState_);
}
void MainWindow::updateAssembly(bool inspect) {
  if(restoringProject_||assemblyProcessing()||modelJob_||fuselageJob_||stabilizerProcessing())return;
  invalidateAssembly();
  if(!assemblyOriginals_.fuselage.IsNull()) {
    if(assemblyState_.cuts&&!assemblyCutParts_){toggleAssemblyCuts();return;}
    displayAssembly(assemblyEntry_);assemblyEntry_=false;return;
  }
  const std::array<bool,4> requested=inspect?std::array<bool,4>{
      wingDefinitions_.airfoilsDefined,
      fuselageOutlinePanel_->outlinesDefined()&&fuselageProfilePanel_->allProfilesClosed(),
      projectLengthScale()>0&&stabilizerOutlineDefined(planViewport_->stabilizerSketchEditor(0).layers().front()),
      projectLengthScale()>0&&stabilizerOutlineDefined(planViewport_->stabilizerSketchEditor(1).layers().front())
    }:std::array<bool,4>{true,true,true,true};
  if(inspect&&std::none_of(requested.begin(),requested.end(),[](bool ready){return ready;}))return;
  for(auto* button:assemblySelect_)button->setEnabled(false);assemblyCutButton_->setEnabled(false);
  // Initialize defaults on the GUI thread before capturing immutable inputs.
  if(requested[1]) {
    if(!fuselageThickenPanel_->enabled())fuselageThickenPanel_->enter(fuselageWingLeadingEdge());
    else fuselageThickenPanel_->synchronize(fuselageWingLeadingEdge());
  }
  const auto fingerprint=assemblyFingerprint();
  if(assemblyAttemptFingerprint_==fingerprint)return;
  assemblyAttemptFingerprint_=fingerprint;assemblyJobFingerprint_=fingerprint;assemblyJobEpoch_=projectEpoch_;
  assemblyComponentFingerprints_={wingFingerprint(),fuselageFingerprint(),stabilizerFingerprint(0),stabilizerFingerprint(1)};
  const auto p=projectDocument();
  geometry::WingSolidInput wing{p.wing.layers,p.stations.lines,airfoilPanel_->library().entries(),
      p.reference.toScale?std::nullopt:p.reference.wingspanMm,p.dihedralDegrees,p.controls.panels,p.spars,p.lightening};
  geometry::FuselageSolidInput fuselage{p.fuselage.layers,p.fuselageStations.lines,p.fuselageProfiles.layers,
      scaledFuselageLength(),p.fuselageThickening,p.fuselageCuts.layers,p.servoTray.rectangle,p.formers.rectangles,p.formers.rotationDegrees,p.fuselageHoles.layers};
  std::vector<geometry::StabilizerSolidInput> stabilizers;
  for(int i=0;i<2;++i)stabilizers.push_back({p.stabilizerOutlines[i].layers.front(),stabilizerAirfoilPanels_[i]->airfoil(),
      projectLengthScale(),i==0,p.stabilizerHinges[i].layers.front(),p.stabilizerHingeCuts[i],p.stabilizerCuts[i].layers});
  try {
    for(int i=0;i<2;++i)if(requested[i+2])geometry::validateStabilizerCuts(stabilizers[i].cutShapes);
  } catch(const std::exception& error) {
    statusBar()->showMessage(QString{inspect?"Inspect generation failed: ":"Assembly generation failed: "}+QString::fromUtf8(error.what()));
    return;
  }
  AssemblyPrepared cached;
  if(builtWingFingerprint_==assemblyComponentFingerprints_[0]){cached.wing=wingShape_;cached.spars=wingSparMaterials_;}
  if(builtFuselageFingerprint_==assemblyComponentFingerprints_[1]&&!fuselageShape_.IsNull())cached.fuselage=fuselageModel_;
  for(int i=0;i<2;++i)if(builtStabilizerFingerprints_[i]==assemblyComponentFingerprints_[i+2])cached.stabilizers[i]=stabilizerModels_[i];
  const bool missing=(requested[0]&&cached.wing.IsNull())||(requested[1]&&cached.fuselage.shape.IsNull())||
      (requested[2]&&cached.stabilizers[0].shape.IsNull())||(requested[3]&&cached.stabilizers[1].shape.IsNull());
  if(inspect&&!missing)return;
  inspectPreparing_=inspect;
  assemblyPrepareJob_=std::make_unique<processing::BackgroundJob<AssemblyPrepared>>(
      [cached,requested,inspect,wing=std::move(wing),fuselage=std::move(fuselage),stabilizers=std::move(stabilizers)]
      (std::stop_token stop,const auto& progress) mutable {
        geometry::ProcessingControl control{stop};control.checkpoint();
        std::vector<int> missing;
        if(requested[0]&&cached.wing.IsNull())missing.push_back(0);
        if(requested[1]&&cached.fuselage.shape.IsNull())missing.push_back(1);
        for(int i=0;i<2;++i)if(requested[i+2]&&cached.stabilizers[i].shape.IsNull())missing.push_back(i+2);
        // Each task reads its own immutable input and writes one distinct result
        // slot. Cached OCCT shapes are never mutated. BackgroundJob serializes
        // progress messages; the GUI receives results only after all tasks join.
        processing::runIndexedTasks(missing.size(),[&](std::size_t task,std::stop_token token) {
          geometry::ProcessingControl componentControl{token};componentControl.checkpoint();
          const int component=missing[task];
          if(component==0) {
            // Avoid nested panel workers competing with the other components.
            // A lone missing Wing retains its normal panel concurrency.
            cached.wing=geometry::buildWingSolid(wing,progress,{componentControl,missing.size()>1?1u:0u,&cached.spars});
          } else if(component==1)cached.fuselage=geometry::buildFuselageModel(fuselage,progress,componentControl);
          else cached.stabilizers[component-2]=geometry::buildStabilizerModel(stabilizers[component-2],progress,componentControl);
        },stop);
        if(!inspect&&(cached.stabilizers[0].fixed.IsNull()||cached.stabilizers[1].fixed.IsNull()))
          throw std::runtime_error("Assembly requires fixed horizontal and vertical stabilizer material.");
        control.checkpoint();return cached;
      });
  setModelProcessing(true);statusBar()->showMessage(inspect?"Inspect: regenerating changed component models...":"Preparing Assembly from current component models...");
}
void MainWindow::displayAssembly(bool entry) {
  if(assemblyOriginals_.fuselage.IsNull())return;
  const auto parts=assemblyState_.cuts&&assemblyCutParts_?*assemblyCutParts_:geometry::placeAssembly(assemblyOriginals_,assemblyState_);
  geometry::AssemblyParts h;h.horizontal=parts.horizontal;h.elevator=parts.elevator;
  geometry::AssemblyParts v;v.vertical=parts.vertical;v.rudder=parts.rudder;
  std::vector<TopoDS_Shape> displayed{parts.fuselage,parts.wing,geometry::assemblyShape(h),geometry::assemblyShape(v)};
  for(const auto& insert:parts.inserts)displayed.push_back(insert.shape);
  viewport_->displayAssembly(displayed,
      assemblyState_.cuts||assemblySelected_<0?-1:assemblySelected_+1);
  const auto& reference=projectReference();
  const auto& outlines=planViewport_->fuselageSketchEditor().layers();
  if(outlines.size()>1)viewport_->setAssemblyReference(reference,
      geometry::fuselageSideTransform(outlines[1],scaledFuselageLength()));
  if(entry) {
    Bnd_Box box;BRepBndLib::AddOptimal(parts.fuselage,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    const double cx=(x0+x1)/2,cz=(z0+z1)/2;
    Bnd_Box all;BRepBndLib::AddOptimal(geometry::assemblyShape(parts),all,false,false);all.Get(x0,y0,z0,x1,y1,z1);
    const double aspect=static_cast<double>(std::max(1,viewport_->width()))/std::max(1,viewport_->height());
    const double scale=2.2*std::max({std::abs(z0-cz),std::abs(z1-cz),std::abs(x0-cx)/aspect,std::abs(x1-cx)/aspect,1.});
    // Looking from -Y with +Z up makes nose left and tail right in fuselage axes.
    viewport_->restoreCamera(CameraState{{cx,-scale*3,cz},{cx,0,cz},{0,0,1},scale,45,0});
  }
  displayedComponent_=5;setProperty("assemblyReady",true);setProperty("assemblyCuts",assemblyState_.cuts);
  updateExportAvailability();
  for(auto* button:assemblySelect_)button->setEnabled(!assemblyState_.cuts);
  for(auto* button:assemblyRotate_)button->setEnabled(!assemblyState_.cuts&&assemblySelected_>=0);
  assemblyRotationLabel_->setText(assemblySelected_<0?QString{"Rotation: select a component"}:
      QString{"Rotation: %1%2°"}.arg(assemblyState_.rotationDegrees[assemblySelected_]>0?"+":"")
        .arg(assemblyState_.rotationDegrees[assemblySelected_],0,'f',1));
  assemblyCutButton_->setEnabled(true);assemblyCutButton_->setText(assemblyState_.cuts?"Undo Cuts":"Cut Intersections");
  statusBar()->showMessage(assemblyState_.cuts?"Assembly cuts ready for export. Undo Cuts to reposition.":"Assembly: select a component and move it with arrow keys (1 mm; Shift 10 mm; Ctrl 0.1 mm).");
}
void MainWindow::moveAssembly(int key,Qt::KeyboardModifiers modifiers) {
  if(dataPanel_->property("workspaceIndex").toInt()!=5||assemblySelected_<0||assemblyState_.cuts||assemblyProcessing()||assemblyOriginals_.fuselage.IsNull())return;
  const double step=modifiers.testFlag(Qt::ShiftModifier)?10.:modifiers.testFlag(Qt::ControlModifier)?.1:1.;
  auto offset=assemblyState_.offsets[assemblySelected_];
  if(key==Qt::Key_Left)offset.rx()-=step;if(key==Qt::Key_Right)offset.rx()+=step;
  if(key==Qt::Key_Up)offset.ry()+=step;if(key==Qt::Key_Down)offset.ry()-=step;
  if(std::abs(offset.x())>1e7||std::abs(offset.y())>1e7)return;
  assemblyState_.offsets[assemblySelected_]=offset;displayAssembly();updateProjectTitle();
}
void MainWindow::rotateAssembly(double degrees) {
  if(dataPanel_->property("workspaceIndex").toInt()!=5||assemblySelected_<0||assemblyState_.cuts||assemblyProcessing()||assemblyOriginals_.fuselage.IsNull())return;
  auto& angle=assemblyState_.rotationDegrees[assemblySelected_];angle=std::remainder(angle+degrees,360.);
  if(angle==0)angle=0; // Avoid a negative-zero readout after a complete turn.
  displayAssembly();updateProjectTitle();
}
void MainWindow::toggleAssemblyCuts() {
  if(assemblyProcessing()||assemblyOriginals_.fuselage.IsNull())return;
  if(assemblyCutParts_) {
    assemblyCutParts_.reset();assemblyState_.cuts=false;displayAssembly();updateProjectTitle();return;
  }
  const auto parts=geometry::placeAssembly(assemblyOriginals_,assemblyState_);
  assemblyJobFingerprint_=assemblyFingerprint();assemblyJobEpoch_=projectEpoch_;
  assemblyCutJob_=std::make_unique<processing::BackgroundJob<geometry::AssemblyCutResult>>(
      [parts](std::stop_token stop,const auto& progress){return geometry::cutAssemblyIntersections(parts,progress,{stop});});
  setModelProcessing(true);
}
void MainWindow::pollAssemblyJob() {
  const bool preparing=bool(assemblyPrepareJob_);
  const auto messages=preparing?assemblyPrepareJob_->messages():assemblyCutJob_->messages();
  for(const auto& message:messages)statusBar()->showMessage(QString::fromStdString(message));
  if(preparing?!assemblyPrepareJob_->ready():!assemblyCutJob_->ready())return;
  const bool cancelled=preparing?assemblyPrepareJob_->cancelled():assemblyCutJob_->cancelled();
  const bool obsolete=assemblyJobEpoch_!=projectEpoch_||assemblyJobFingerprint_!=assemblyFingerprint();
  QString error;AssemblyPrepared prepared;geometry::AssemblyCutResult cut;
  try {if(preparing)prepared=assemblyPrepareJob_->take();else cut=assemblyCutJob_->take();}
  catch(const Standard_Failure& e){error=QString::fromUtf8(e.what());}
  catch(const std::exception& e){error=QString::fromUtf8(e.what());}
  catch(...){error="Unknown assembly processing failure.";}
  const bool inspecting=inspectPreparing_||dataPanel_->property("workspaceIndex").toInt()==8;inspectPreparing_=false;
  assemblyPrepareJob_.reset();assemblyCutJob_.reset();setModelProcessing(false);
  if(!obsolete) {
    if(cancelled||!error.isEmpty()||!cut.collisions.empty()) {
      assemblyState_.cuts=false;
      if(!assemblyOriginals_.fuselage.IsNull())displayAssembly(assemblyEntry_);
      if(cancelled)statusBar()->showMessage(inspecting?"Inspect regeneration cancelled. Re-enter Inspect to retry.":"Assembly cancelled; originals retained. Re-enter Assembly or retry Cut Intersections.");
      else {
        for(const auto& collision:cut.collisions){if(!error.isEmpty())error+='\n';error+=QString::fromStdString(collision);}
        statusBar()->showMessage((inspecting?"Inspect: ":"Assembly: ")+error);
        QMessageBox::warning(this,inspecting?"Inspect generation failed":cut.collisions.empty()?"Assembly failed":"Assembly collisions",error);
      }
    } else if(preparing) {
      viewport_->setProperty("wingModelReady",!prepared.wing.IsNull());viewport_->setProperty("fuselageModelReady",!prepared.fuselage.shape.IsNull());
      viewport_->setProperty("servoTrayReady",!prepared.fuselage.servoTray.IsNull());
      viewport_->setProperty("formerCount",static_cast<int>(prepared.fuselage.formers.size()));
      wingShape_=prepared.wing;wingSparMaterials_=std::move(prepared.spars);fuselageModel_=prepared.fuselage;fuselageShape_=prepared.fuselage.shape;
      servoTrayTopFaces_=prepared.fuselage.servoTrayTopFaces;stabilizerModels_=prepared.stabilizers;
      builtWingFingerprint_=assemblyComponentFingerprints_[0];builtFuselageFingerprint_=assemblyComponentFingerprints_[1];
      for(int i=0;i<2;++i){stabilizerShapes_[i]=prepared.stabilizers[i].shape;builtStabilizerFingerprints_[i]=assemblyComponentFingerprints_[i+2];}
      const bool complete=!prepared.wing.IsNull()&&!prepared.fuselage.body.IsNull()&&
          !prepared.stabilizers[0].fixed.IsNull()&&!prepared.stabilizers[1].fixed.IsNull();
      if(!inspecting||complete) {
        assemblyOriginals_=cachedModelParts();assemblySourceFingerprint_=assemblyJobFingerprint_;setProperty("assemblyReady",true);
        if(!assemblyState_.positioned)assemblyState_=geometry::initialAssemblyPlacement(assemblyOriginals_);
        if(!inspecting)displayAssembly(true);assemblyEntry_=false;
        if(assemblyState_.cuts&&!closingAfterProcessing_)toggleAssemblyCuts();
      }
    } else {assemblyCutParts_=cut.parts;assemblyState_.cuts=true;displayAssembly();}
  }
  updateProjectTitle();
  updateExportAvailability();
  if(inspecting&&!closingAfterProcessing_&&dataPanel_->property("workspaceIndex").toInt()==8) {
    updateInspect(inspectFitAfterBuild_,false);
    if(!cancelled&&error.isEmpty()&&!obsolete&&!assemblyProcessing())statusBar()->showMessage("Inspect: current components ready.");
  }
  if(obsolete&&!closingAfterProcessing_){if(inspecting)updateInspect(false);else updateWingModel();}
  if(closingAfterProcessing_){closingAfterProcessing_=false;close();}
}
}

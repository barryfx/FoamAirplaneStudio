#include "gui/MainWindow.h"
#include "gui/AirfoilPanel.h"
#include "gui/DihedralPanel.h"
#include "gui/LighteningPanel.h"
#include "gui/SparPanel.h"
#include "gui/FuselageThickenPanel.h"
#include "gui/StabilizerAirfoilPanel.h"
#include "gui/StabilizerHingePanel.h"
#include "gui/PlanViewport.h"
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
#include <Standard_Failure.hxx>
#include <algorithm>

namespace designrc::gui {
void MainWindow::buildAssemblyPanel(QVBoxLayout* layout) {
  assemblyPanel_=new QWidget{dataContents_};assemblyPanel_->setObjectName("assemblyPanel");
  auto* box=new QVBoxLayout{assemblyPanel_};box->setContentsMargins(0,0,0,0);
  auto* instructions=new QLabel{
      "Position the aircraft components against the fuselage in the left-side 3D view. "
      "Select Wing, Horiz Stab or Vert Stab, then use the arrow keys: Left/Right move toward the nose/tail; "
      "Up/Down raise/lower the part. Each step is 1 mm (Shift: 10 mm; Ctrl: 0.1 mm). "
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
}
std::optional<geometry::AssemblyParts> MainWindow::exportAssemblyParts() const {
  if(assemblyOriginals_.fuselage.IsNull() || assemblySourceFingerprint_!=assemblyFingerprint())return {};
  if(assemblyState_.cuts)return assemblyCutParts_;
  return geometry::placeAssembly(assemblyOriginals_,assemblyState_);
}
void MainWindow::updateAssembly() {
  if(restoringProject_||assemblyProcessing()||modelJob_||fuselageJob_||stabilizerProcessing())return;
  invalidateAssembly();
  if(!assemblyOriginals_.fuselage.IsNull()) {
    if(assemblyState_.cuts&&!assemblyCutParts_){toggleAssemblyCuts();return;}
    displayAssembly(assemblyEntry_);assemblyEntry_=false;return;
  }
  for(auto* button:assemblySelect_)button->setEnabled(false);assemblyCutButton_->setEnabled(false);
  // Initialize defaults on the GUI thread before capturing immutable inputs.
  if(!fuselageThickenPanel_->enabled())fuselageThickenPanel_->enter(fuselageWingLeadingEdge());
  else fuselageThickenPanel_->synchronize(fuselageWingLeadingEdge());
  const auto fingerprint=assemblyFingerprint();
  if(assemblyAttemptFingerprint_==fingerprint)return;
  assemblyAttemptFingerprint_=fingerprint;assemblyJobFingerprint_=fingerprint;assemblyJobEpoch_=projectEpoch_;
  assemblyComponentFingerprints_={wingFingerprint(),fuselageFingerprint(),stabilizerFingerprint(0),stabilizerFingerprint(1)};
  const auto p=projectDocument();
  geometry::WingSolidInput wing{p.wing.layers,p.stations.lines,airfoilPanel_->library().entries(),
      p.reference.toScale?std::nullopt:p.reference.wingspanMm,p.dihedralDegrees,p.controls.panels,p.spars,p.lightening};
  geometry::FuselageSolidInput fuselage{p.fuselage.layers,p.fuselageStations.lines,p.fuselageProfiles.layers,
      p.reference.toScale?std::nullopt:p.reference.fuselageLengthMm,p.fuselageThickening,p.fuselageCuts.layers,p.servoTray.rectangle,p.formers.rectangles,p.formers.rotationDegrees};
  std::vector<geometry::StabilizerSolidInput> stabilizers;
  for(int i=0;i<2;++i)stabilizers.push_back({p.stabilizerOutlines[i].layers.front(),stabilizerAirfoilPanels_[i]->airfoil(),
      stabilizerScale(),i==0,p.stabilizerHinges[i].layers.front(),p.stabilizerHingeCuts[i],p.stabilizerCuts[i].layers});
  AssemblyPrepared cached;
  if(builtWingFingerprint_==assemblyComponentFingerprints_[0])cached.wing=wingShape_;
  if(builtFuselageFingerprint_==assemblyComponentFingerprints_[1]&&!fuselageShape_.IsNull())cached.fuselage=fuselageModel_;
  for(int i=0;i<2;++i)if(builtStabilizerFingerprints_[i]==assemblyComponentFingerprints_[i+2])cached.stabilizers[i]=stabilizerModels_[i];
  assemblyPrepareJob_=std::make_unique<processing::BackgroundJob<AssemblyPrepared>>(
      [cached,wing=std::move(wing),fuselage=std::move(fuselage),stabilizers=std::move(stabilizers)]
      (std::stop_token stop,const auto& progress) mutable {
        geometry::ProcessingControl control{stop};control.checkpoint();
        std::vector<int> missing;
        if(cached.wing.IsNull())missing.push_back(0);
        if(cached.fuselage.shape.IsNull())missing.push_back(1);
        for(int i=0;i<2;++i)if(cached.stabilizers[i].shape.IsNull())missing.push_back(i+2);
        // Each task reads its own immutable input and writes one distinct result
        // slot. Cached OCCT shapes are never mutated. BackgroundJob serializes
        // progress messages; the GUI receives results only after all tasks join.
        processing::runIndexedTasks(missing.size(),[&](std::size_t task,std::stop_token token) {
          geometry::ProcessingControl componentControl{token};componentControl.checkpoint();
          const int component=missing[task];
          if(component==0) {
            // Avoid nested panel workers competing with the other components.
            // A lone missing Wing retains its normal panel concurrency.
            cached.wing=geometry::buildWingSolid(wing,progress,{componentControl,missing.size()>1?1u:0u});
          } else if(component==1)cached.fuselage=geometry::buildFuselageModel(fuselage,progress,componentControl);
          else cached.stabilizers[component-2]=geometry::buildStabilizerModel(stabilizers[component-2],progress,componentControl);
        },stop);
        if(cached.stabilizers[0].fixed.IsNull()||cached.stabilizers[1].fixed.IsNull())
          throw std::runtime_error("Assembly requires fixed horizontal and vertical stabilizer material.");
        control.checkpoint();return cached;
      });
  setModelProcessing(true);statusBar()->showMessage("Preparing Assembly from current component models...");
}
void MainWindow::displayAssembly(bool entry) {
  if(assemblyOriginals_.fuselage.IsNull())return;
  const auto parts=assemblyCutParts_?*assemblyCutParts_:geometry::placeAssembly(assemblyOriginals_,assemblyState_);
  geometry::AssemblyParts h;h.horizontal=parts.horizontal;h.elevator=parts.elevator;
  geometry::AssemblyParts v;v.vertical=parts.vertical;v.rudder=parts.rudder;
  viewport_->displayAssembly({parts.fuselage,parts.wing,geometry::assemblyShape(h),geometry::assemblyShape(v)},
      assemblyState_.cuts||assemblySelected_<0?-1:assemblySelected_+1);
  const auto& reference=projectReference();
  const auto& outlines=planViewport_->fuselageSketchEditor().layers();
  if(outlines.size()>1)viewport_->setAssemblyReference(reference,
      geometry::fuselageSideTransform(outlines[1],reference.toScale?std::nullopt:reference.fuselageLengthMm));
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
  for(auto* button:assemblySelect_)button->setEnabled(!assemblyState_.cuts);
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
  assemblyPrepareJob_.reset();assemblyCutJob_.reset();setModelProcessing(false);
  if(!obsolete) {
    if(cancelled||!error.isEmpty()||!cut.collisions.empty()) {
      assemblyState_.cuts=false;
      if(!assemblyOriginals_.fuselage.IsNull())displayAssembly(assemblyEntry_);
      if(cancelled)statusBar()->showMessage("Assembly cancelled; originals retained. Re-enter Assembly or retry Cut Intersections.");
      else {
        for(const auto& collision:cut.collisions){if(!error.isEmpty())error+='\n';error+=QString::fromStdString(collision);}
        statusBar()->showMessage("Assembly: "+error);
        QMessageBox::warning(this,cut.collisions.empty()?"Assembly failed":"Assembly collisions",error);
      }
    } else if(preparing) {
      viewport_->setProperty("wingModelReady",true);viewport_->setProperty("fuselageModelReady",true);
      viewport_->setProperty("servoTrayReady",!prepared.fuselage.servoTray.IsNull());
      viewport_->setProperty("formerCount",static_cast<int>(prepared.fuselage.formers.size()));
      wingShape_=prepared.wing;fuselageModel_=prepared.fuselage;fuselageShape_=prepared.fuselage.shape;
      servoTrayTopFaces_=prepared.fuselage.servoTrayTopFaces;stabilizerModels_=prepared.stabilizers;
      builtWingFingerprint_=assemblyComponentFingerprints_[0];builtFuselageFingerprint_=assemblyComponentFingerprints_[1];
      for(int i=0;i<2;++i){stabilizerShapes_[i]=prepared.stabilizers[i].shape;builtStabilizerFingerprints_[i]=assemblyComponentFingerprints_[i+2];}
      assemblyOriginals_={fuselageShape_,wingShape_,prepared.stabilizers[0].fixed,prepared.stabilizers[1].fixed,
          prepared.stabilizers[0].control,prepared.stabilizers[1].control};
      assemblySourceFingerprint_=assemblyJobFingerprint_;
      if(!assemblyState_.positioned)assemblyState_=geometry::initialAssemblyPlacement(assemblyOriginals_);
      displayAssembly(true);assemblyEntry_=false;
      if(assemblyState_.cuts&&!closingAfterProcessing_)toggleAssemblyCuts();
    } else {assemblyCutParts_=cut.parts;assemblyState_.cuts=true;displayAssembly();}
  }
  updateProjectTitle();
  if(obsolete&&!closingAfterProcessing_)updateWingModel();
  if(closingAfterProcessing_){closingAfterProcessing_=false;close();}
}
}

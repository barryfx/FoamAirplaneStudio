#include <QMessageBox>
#include "gui/MainWindow.h"
#include "WaitForModel.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include "gui/PlanViewport.h"
#include "gui/AirfoilPanel.h"
#include "gui/OcctViewport.h"
#include <QTabWidget>
#include <QPushButton>
#include <QScreen>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QCheckBox>
#include <QFile>
#include <QRadioButton>
#include <QApplication>
#include <QAction>
#include <QLineEdit>
#include <QLabel>
#include <QToolBar>
#include <QMouseEvent>
#include <QKeyEvent>
#include <cassert>
#include <cmath>
#include <stdexcept>
#undef assert
#define assert(condition) do { if (!(condition)) qFatal("Assertion failed at line %d: %s", __LINE__, #condition); } while(false)
using namespace designrc::gui;
int main(int argc, char** argv) {
  QApplication app{argc, argv};
  MainWindow window; window.show(); app.processEvents();waitForModel(window);
  int processingStages=0;
  QObject::connect(window.statusBar(), &QStatusBar::messageChanged, &window, [&](const QString& message) {
    if(message.endsWith("...")) {
      assert(QApplication::overrideCursor());
      assert(QApplication::overrideCursor()->shape()==Qt::WaitCursor);
      ++processingStages;
    }
  });
  window.findChild<QLineEdit*>("referenceWingspan")->setText("1000");
  window.findChild<QLineEdit*>("referenceFuselageLength")->setText("700");
  auto* workspace = window.findChild<QToolBar*>("workspaceToolBar");
  workspace->actions()[1]->trigger();
  auto* tools = window.findChild<QToolBar*>("componentToolBar");
  auto* view = static_cast<PlanViewport*>(window.findChild<QGraphicsView*>());
  assert(view);
  auto click = [&](QPointF point) {
    const auto local = view->mapFromScene(point);
    for (auto type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
      QMouseEvent event{type, QPointF{local}, QPointF{view->viewport()->mapToGlobal(local)},
          Qt::LeftButton, type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier};
      QApplication::sendEvent(view->viewport(), &event);
    }
  };
  auto& sketch = view->sketchEditor(); sketch.setTool(SketchTool::Line);
  click({100, 100}); click({700, 100}); click({700, 100}); click({700, 400});
  // A slightly oblique open root must still generate a solid wing.
  click({700, 400}); click({108, 400});
  assert(tools->actions()[1]->isEnabled());
  tools->actions()[1]->trigger(); app.processEvents();waitForModel(window);
  assert(window.findChild<QWidget*>("airfoilStationsPanel")->isVisible());
  assert(!window.findChild<QWidget*>("wingOutlinePanel")->isVisible());
  assert(sketch.stationEditor().enabled());
  const auto outline = sketch.layers()[0].points;
  click({200, 100}); click({200, 400});
  assert(!tools->actions()[2]->isEnabled());
  click({600, 100}); click({600, 400});
  assert(tools->actions()[2]->isEnabled());
  assert(tools->actions()[1]->isChecked()); // Stay in Stations during placement.
  assert(sketch.layers()[0].points == outline);
  const QString capture = qEnvironmentVariable("FOAM_STATION_WORKFLOW_CAPTURE");
  if (!capture.isEmpty()) assert(window.grab().save(capture));
  tools->actions()[2]->trigger();
  assert(sketch.stationEditor().selectedLine() == 0);
  QKeyEvent escape{QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier};
  QApplication::sendEvent(view, &escape);
  click({800,500}); assert(sketch.stationEditor().selectedLine() == 0);
  auto* airfoils = window.findChild<AirfoilPanel*>("airfoilPanel"); assert(airfoils && airfoils->isVisible());
  QTemporaryDir temporary; assert(temporary.isValid());
  const auto dat = temporary.filePath("foil.dat");
  { QFile file{dat}; assert(file.open(QIODevice::WriteOnly)); file.write("Assignment test\n1 0\n0.5 0.1\n0 0\n0.5 -0.1\n1 0\n"); }
  QString error; assert(airfoils->loadAirfoil(dat, error));
  click({200, 250}); assert(!tools->actions()[3]->isEnabled());
  assert(!workspace->actions()[2]->isEnabled());
  click({600, 250}); assert(tools->actions()[3]->isEnabled());
  assert(workspace->actions()[2]->isEnabled());
  assert(tools->actions()[2]->isChecked());
  const QString airfoilCapture = qEnvironmentVariable("FOAM_AIRFOIL_WORKFLOW_CAPTURE");
  app.processEvents();waitForModel(window);
  assert(airfoils->findChild<QRadioButton*>("airfoilChoice0")->isVisible());
  if (!airfoilCapture.isEmpty()) assert(window.grab().save(airfoilCapture));
  assert(!sketch.stationEditor().enabled());
  click({200, 250});
  QKeyEvent remove{QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier};
  QApplication::sendEvent(view, &remove);
  assert(sketch.stationEditor().lines().size() == 2);
  tools->actions()[3]->trigger(); app.processEvents();waitForModel(window);
  auto* dihedral = window.findChild<QWidget*>("dihedralPanel"); assert(dihedral->isVisible());
  auto* angle = dihedral->findChild<QDoubleSpinBox*>("rootDihedral");
  assert(angle && angle->value()==0);
  auto* tabs = window.findChild<QTabWidget*>("viewportTabs");
  tabs->setCurrentIndex(1); app.processEvents();waitForModel(window);
  auto* solidView = static_cast<OcctViewport*>(tabs->widget(1));
  if (!solidView->property("wingModelReady").toBool()) qFatal("%s",qPrintable(window.statusBar()->currentMessage()));
  const int initialRevision = solidView->property("wingModelRevision").toInt();
  const auto savedProject = temporary.filePath("navigation.foam");
  assert(window.saveProjectFile(savedProject,error));
  const auto firstCamera = solidView->cameraState(); assert(firstCamera);
  auto camera = *firstCamera;
  camera.eye = {300,-400,200}; camera.center = {100,0,0}; camera.up = {0,0,1};
  camera.scale = 1100; camera.fov = 45;
  solidView->restoreCamera(camera);
  const auto checkCamera = [&] {
    const auto actual=solidView->cameraState(); assert(actual);
    for(int i=0;i<3;++i) {
      assert(std::abs(actual->eye[i]-camera.eye[i])<1e-6);
      assert(std::abs(actual->center[i]-camera.center[i])<1e-6);
      assert(std::abs(actual->up[i]-camera.up[i])<1e-6);
    }
    assert(std::abs(actual->scale-camera.scale)<1e-6);
    assert(std::abs(actual->fov-camera.fov)<1e-6);
    assert(actual->projection==camera.projection);
  };
  const int beforeNavigation=processingStages;
  for(auto* action : tools->actions()) {action->trigger(); app.processEvents();waitForModel(window);}
  workspace->actions()[2]->trigger(); app.processEvents();waitForModel(window);
  assert(window.findChild<QWidget*>("dataPanel")->property("workspaceIndex").toInt()==2);
  QTimer::singleShot(0, [] {
    auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
    assert(box && box->text().contains("Top View") && box->text().contains("Side View"));
    box->accept();
  });
  workspace->actions()[1]->trigger(); app.processEvents();waitForModel(window);
  tabs->setCurrentIndex(0); app.processEvents();waitForModel(window);
  view->restoreView({2.0,{300,250}}); app.processEvents();waitForModel(window);
  tools->actions()[2]->trigger(); click({600,250});
  tabs->setCurrentIndex(1); app.processEvents();waitForModel(window);
  tools->actions()[3]->trigger(); app.processEvents();waitForModel(window);
  assert(processingStages==beforeNavigation);
  assert(solidView->property("wingModelRevision").toInt()==initialRevision);
  assert(!window.projectModified()); checkCamera();
  // Opening the saved project after navigation must not ask to save/discard.
  // A modal prompt fails immediately instead of hanging unattended validation.
  QTimer::singleShot(0, [&] { assert(!QApplication::activeModalWidget()); });
  assert(window.openProjectFile(savedProject,error)); app.processEvents();waitForModel(window);
  assert(workspace->actions()[2]->isEnabled());
  solidView->restoreCamera(camera);
  const int reopenedRevision=solidView->property("wingModelRevision").toInt();
  angle->setValue(3); app.processEvents();waitForModel(window);
  assert(window.projectModified()); checkCamera();
  assert(angle->value()==3);
  assert(solidView->property("wingModelReady").toBool());
  assert(solidView->property("wingModelRevision").toInt()==reopenedRevision+1);
  const QString wingCapture = qEnvironmentVariable("FOAM_WING_CAPTURE");
  if (!wingCapture.isEmpty()) assert(window.screen()->grabWindow(window.winId()).save(wingCapture));
  tabs->setCurrentIndex(0); angle->setValue(5); app.processEvents();waitForModel(window);
  assert(solidView->property("wingModelRevision").toInt()==reopenedRevision+1);
  tabs->setCurrentIndex(1); app.processEvents();waitForModel(window);
  assert(solidView->property("wingModelReady").toBool());
  assert(solidView->property("wingModelRevision").toInt()==reopenedRevision+2);
  checkCamera();
  tabs->setCurrentIndex(0);tools->actions()[4]->trigger();app.processEvents();waitForModel(window);
  assert(window.statusBar()->currentMessage().contains("Delete removes it"));
  assert(window.statusBar()->currentMessage().contains("Escape cancels drawing or selection"));
  auto* ailerons=window.findChild<QCheckBox*>("addAilerons");
  auto* flaps=window.findChild<QCheckBox*>("addFlaps");
  assert(ailerons->isVisible() && !ailerons->isChecked() && !flaps->isChecked());
  ailerons->click();click({450,320});click({730,450});
  flaps->click();click({180,320});click({400,450});
  window.findChild<QRadioButton*>("FlapsStandardHinge")->click();app.processEvents();waitForModel(window);
  assert(window.projectDocument().controls.panels[0][0].rectangle);
  assert(window.projectDocument().controls.panels[0][1].rectangle);
  const auto controlPlanCapture=qEnvironmentVariable("FOAM_CONTROL_PLAN_CAPTURE");
  if(!controlPlanCapture.isEmpty())assert(window.grab().save(controlPlanCapture));
  tabs->setCurrentIndex(1);app.processEvents();waitForModel(window);
  if(!solidView->property("wingModelReady").toBool())qFatal("%s",qPrintable(window.statusBar()->currentMessage()));
  checkCamera();
  const auto undersideCapture=qEnvironmentVariable("FOAM_UNDERSIDE_CAPTURE");
  if(!undersideCapture.isEmpty()) {
    solidView->setCameraView(CameraView::Bottom);app.processEvents();waitForModel(window);
    assert(window.screen()->grabWindow(window.winId()).save(undersideCapture));
    solidView->restoreCamera(camera);
  }
  const auto controlWingCapture=qEnvironmentVariable("FOAM_CONTROL_WING_CAPTURE");
  if(!controlWingCapture.isEmpty())assert(window.screen()->grabWindow(window.winId()).save(controlWingCapture));
  tabs->setCurrentIndex(0);
  click({500,350});QApplication::sendEvent(view,&remove);
  assert(ailerons->isChecked() && !window.projectDocument().controls.panels[0][0].rectangle);
  tabs->setCurrentIndex(1);app.processEvents();waitForModel(window);
  assert(!ailerons->isChecked() && flaps->isChecked());
  assert(!window.projectDocument().controls.panels[0][0].enabled);
  assert(solidView->property("wingModelReady").toBool());
  tabs->setCurrentIndex(0);click({250,350});QApplication::sendEvent(view,&remove);
  assert(flaps->isChecked() && !window.projectDocument().controls.panels[0][1].rectangle);
  tools->actions()[3]->trigger();app.processEvents();waitForModel(window);
  assert(!flaps->isChecked() && !window.projectDocument().controls.panels[0][1].enabled);
  tools->actions()[5]->trigger();app.processEvents();waitForModel(window);
  assert(window.findChild<QWidget*>("sparPanel")->isVisible());
  window.findChild<QCheckBox*>("sparTop")->click();
  window.findChild<QCheckBox*>("sparBottom")->click();
  window.findChild<QCheckBox*>("sparMid")->click();
  window.findChild<QComboBox*>("sparBottomShape")->setCurrentIndex(1);
  window.findChild<QDoubleSpinBox*>("sparBottomChord")->setValue(50);
  const auto sparInput=qEnvironmentVariable("FOAM_SPAR_INPUT_CAPTURE");
  if(!sparInput.isEmpty())assert(window.saveProjectFile(sparInput,error));
  tabs->setCurrentIndex(1);app.processEvents();waitForModel(window);
  if(!solidView->property("wingModelReady").toBool())qFatal("%s",qPrintable(window.statusBar()->currentMessage()));
  checkCamera();
  const int sparRevision=solidView->property("wingModelRevision").toInt();
  const auto sparCapture=qEnvironmentVariable("FOAM_SPAR_WING_CAPTURE");
  if(!sparCapture.isEmpty())assert(window.screen()->grabWindow(window.winId()).save(sparCapture));
  const auto sparFile=temporary.filePath("spars.foam");assert(window.saveProjectFile(sparFile,error));
  tools->actions()[3]->trigger();tools->actions()[5]->trigger();app.processEvents();waitForModel(window);
  assert(!window.projectModified());assert(solidView->property("wingModelRevision").toInt()==sparRevision);
  tabs->setCurrentIndex(0);
  tools->actions()[1]->trigger(); click({200, 250}); QApplication::sendEvent(view, &remove);
  assert(sketch.stationEditor().lines().size() == 1 && !tools->actions()[2]->isEnabled());
  assert(!workspace->actions()[2]->isEnabled());
  assert(sketch.layers()[0].points == outline);
  tabs->setCurrentIndex(1); app.processEvents();waitForModel(window);
  assert(!solidView->property("wingModelReady").toBool());
  assert(processingStages>=6);
  assert(!QApplication::overrideCursor());
  return 0;
}

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
#include "TestCheck.h"
#include <cmath>
#include <stdexcept>
using namespace designrc::gui;
int main(int argc, char** argv) {
  QApplication app{argc, argv};
  MainWindow window; window.show(); app.processEvents();waitForModel(window);
  int processingStages=0;
  QObject::connect(window.statusBar(), &QStatusBar::messageChanged, &window, [&](const QString& message) {
    if(message.endsWith("...")) {
      TEST_CHECK(QApplication::overrideCursor());
      TEST_CHECK(QApplication::overrideCursor()->shape()==Qt::WaitCursor);
      ++processingStages;
    }
  });
  window.findChild<QLineEdit*>("referenceWingspan")->setText("1000");
  auto* workspace = window.findChild<QToolBar*>("workspaceToolBar");
  workspace->actions()[1]->trigger();
  auto* tools = window.findChild<QToolBar*>("componentToolBar");
  auto* view = static_cast<PlanViewport*>(window.findChild<QGraphicsView*>());
  TEST_CHECK(view);
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
  TEST_CHECK(tools->actions()[1]->isEnabled());
  tools->actions()[1]->trigger(); app.processEvents();waitForModel(window);
  TEST_CHECK(window.findChild<QWidget*>("airfoilStationsPanel")->isVisible());
  TEST_CHECK(!window.findChild<QWidget*>("wingOutlinePanel")->isVisible());
  TEST_CHECK(sketch.stationEditor().enabled());
  const auto outline = sketch.layers()[0].points;
  click({200, 100}); click({200, 400});
  TEST_CHECK(!tools->actions()[2]->isEnabled());
  click({600, 100}); click({600, 400});
  TEST_CHECK(tools->actions()[2]->isEnabled());
  TEST_CHECK(tools->actions()[1]->isChecked()); // Stay in Stations during placement.
  TEST_CHECK(sketch.layers()[0].points == outline);
  const QString capture = qEnvironmentVariable("FOAM_STATION_WORKFLOW_CAPTURE");
  if (!capture.isEmpty()) TEST_CHECK(window.grab().save(capture));
  tools->actions()[2]->trigger();
  TEST_CHECK(sketch.stationEditor().selectedLine() == 0);
  QKeyEvent escape{QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier};
  QApplication::sendEvent(view, &escape);
  click({800,500}); TEST_CHECK(sketch.stationEditor().selectedLine() == 0);
  auto* airfoils = window.findChild<AirfoilPanel*>("airfoilPanel"); TEST_CHECK(airfoils && airfoils->isVisible());
  QTemporaryDir temporary; TEST_CHECK(temporary.isValid());
  const auto dat = temporary.filePath("foil.dat");
  { QFile file{dat}; TEST_CHECK(file.open(QIODevice::WriteOnly)); file.write("Assignment test\n1 0\n0.5 0.1\n0 0\n0.5 -0.1\n1 0\n"); }
  QString error; TEST_CHECK(airfoils->loadAirfoil(dat, error));
  click({200, 250}); TEST_CHECK(!tools->actions()[3]->isEnabled());
  TEST_CHECK(!workspace->actions()[2]->isEnabled());
  click({600, 250}); TEST_CHECK(tools->actions()[3]->isEnabled());
  TEST_CHECK(workspace->actions()[2]->isEnabled());
  TEST_CHECK(tools->actions()[2]->isChecked());
  const QString airfoilCapture = qEnvironmentVariable("FOAM_AIRFOIL_WORKFLOW_CAPTURE");
  app.processEvents();waitForModel(window);
  TEST_CHECK(airfoils->findChild<QRadioButton*>("airfoilChoice0")->isVisible());
  if (!airfoilCapture.isEmpty()) TEST_CHECK(window.grab().save(airfoilCapture));
  TEST_CHECK(!sketch.stationEditor().enabled());
  click({200, 250});
  QKeyEvent remove{QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier};
  QApplication::sendEvent(view, &remove);
  TEST_CHECK(sketch.stationEditor().lines().size() == 2);
  tools->actions()[3]->trigger(); app.processEvents();waitForModel(window);
  auto* dihedral = window.findChild<QWidget*>("dihedralPanel"); TEST_CHECK(dihedral->isVisible());
  auto* angle = dihedral->findChild<QDoubleSpinBox*>("rootDihedral");
  TEST_CHECK(angle && angle->value()==0);
  auto* tabs = window.findChild<QTabWidget*>("viewportTabs");
  tabs->setCurrentIndex(1); app.processEvents();waitForModel(window);
  auto* solidView = static_cast<OcctViewport*>(tabs->widget(1));
  if (!solidView->property("wingModelReady").toBool()) qFatal("%s",qPrintable(window.statusBar()->currentMessage()));
  const int initialRevision = solidView->property("wingModelRevision").toInt();
  const auto savedProject = temporary.filePath("navigation.foam");
  TEST_CHECK(window.saveProjectFile(savedProject,error));
  const auto firstCamera = solidView->cameraState(); TEST_CHECK(firstCamera);
  auto camera = *firstCamera;
  camera.eye = {300,-400,200}; camera.center = {100,0,0}; camera.up = {0,0,1};
  camera.scale = 1100; camera.fov = 45;
  solidView->restoreCamera(camera);
  const auto checkCamera = [&] {
    const auto actual=solidView->cameraState(); TEST_CHECK(actual);
    for(int i=0;i<3;++i) {
      TEST_CHECK(std::abs(actual->eye[i]-camera.eye[i])<1e-6);
      TEST_CHECK(std::abs(actual->center[i]-camera.center[i])<1e-6);
      TEST_CHECK(std::abs(actual->up[i]-camera.up[i])<1e-6);
    }
    TEST_CHECK(std::abs(actual->scale-camera.scale)<1e-6);
    TEST_CHECK(std::abs(actual->fov-camera.fov)<1e-6);
    TEST_CHECK(actual->projection==camera.projection);
  };
  const int beforeNavigation=processingStages;
  for(auto* action : tools->actions()) {action->trigger(); app.processEvents();waitForModel(window);}
  workspace->actions()[2]->trigger(); app.processEvents();waitForModel(window);
  TEST_CHECK(window.findChild<QWidget*>("dataPanel")->property("workspaceIndex").toInt()==2);
  QTimer::singleShot(0, [] {
    auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
    TEST_CHECK(box && box->text().contains("Top View") && box->text().contains("Side View"));
    box->accept();
  });
  workspace->actions()[1]->trigger(); app.processEvents();waitForModel(window);
  tabs->setCurrentIndex(0); app.processEvents();waitForModel(window);
  view->restoreView({2.0,{300,250}}); app.processEvents();waitForModel(window);
  tools->actions()[2]->trigger(); click({600,250});
  tabs->setCurrentIndex(1); app.processEvents();waitForModel(window);
  tools->actions()[3]->trigger(); app.processEvents();waitForModel(window);
  TEST_CHECK(processingStages==beforeNavigation);
  TEST_CHECK(solidView->property("wingModelRevision").toInt()==initialRevision);
  TEST_CHECK(!window.projectModified()); checkCamera();
  // Opening the saved project after navigation must not ask to save/discard.
  // A modal prompt fails immediately instead of hanging unattended validation.
  QTimer::singleShot(0, [&] { TEST_CHECK(!QApplication::activeModalWidget()); });
  TEST_CHECK(window.openProjectFile(savedProject,error)); app.processEvents();waitForModel(window);
  TEST_CHECK(workspace->actions()[2]->isEnabled());
  TEST_CHECK(tabs->currentIndex()==0); // Open always defers regeneration in 2D.
  solidView->restoreCamera(camera);
  const int reopenedRevision=solidView->property("wingModelRevision").toInt();
  angle->setValue(3); app.processEvents();waitForModel(window);
  TEST_CHECK(window.projectModified()); checkCamera();
  TEST_CHECK(angle->value()==3);
  TEST_CHECK(!solidView->property("wingModelReady").toBool());
  TEST_CHECK(solidView->property("wingModelRevision").toInt()==reopenedRevision);
  tabs->setCurrentIndex(1);app.processEvents();waitForModel(window);
  TEST_CHECK(solidView->property("wingModelReady").toBool());
  TEST_CHECK(solidView->property("wingModelRevision").toInt()==reopenedRevision+1);
  // The first generated display after Open fits the model. Establish the
  // custom camera after that first fit, then test retention on later rebuilds.
  solidView->restoreCamera(camera);checkCamera();
  const QString wingCapture = qEnvironmentVariable("FOAM_WING_CAPTURE");
  if (!wingCapture.isEmpty()) TEST_CHECK(window.screen()->grabWindow(window.winId()).save(wingCapture));
  tabs->setCurrentIndex(0); angle->setValue(5); app.processEvents();waitForModel(window);
  TEST_CHECK(solidView->property("wingModelRevision").toInt()==reopenedRevision+1);
  tabs->setCurrentIndex(1); app.processEvents();waitForModel(window);
  TEST_CHECK(solidView->property("wingModelReady").toBool());
  TEST_CHECK(solidView->property("wingModelRevision").toInt()==reopenedRevision+2);
  checkCamera();
  tabs->setCurrentIndex(0);tools->actions()[4]->trigger();app.processEvents();waitForModel(window);
  TEST_CHECK(window.statusBar()->currentMessage().contains("Delete removes it"));
  TEST_CHECK(window.statusBar()->currentMessage().contains("Escape cancels drawing or selection"));
  auto* ailerons=window.findChild<QCheckBox*>("addAilerons");
  auto* flaps=window.findChild<QCheckBox*>("addFlaps");
  TEST_CHECK(ailerons->isVisible() && !ailerons->isChecked() && !flaps->isChecked());
  ailerons->click();click({450,320});click({730,450});
  flaps->click();click({180,320});click({400,450});
  window.findChild<QRadioButton*>("FlapsStandardHinge")->click();app.processEvents();waitForModel(window);
  TEST_CHECK(window.projectDocument().controls.panels[0][0].rectangle);
  TEST_CHECK(window.projectDocument().controls.panels[0][1].rectangle);
  const auto controlPlanCapture=qEnvironmentVariable("FOAM_CONTROL_PLAN_CAPTURE");
  if(!controlPlanCapture.isEmpty())TEST_CHECK(window.grab().save(controlPlanCapture));
  tabs->setCurrentIndex(1);app.processEvents();waitForModel(window);
  if(!solidView->property("wingModelReady").toBool())qFatal("%s",qPrintable(window.statusBar()->currentMessage()));
  checkCamera();
  const auto undersideCapture=qEnvironmentVariable("FOAM_UNDERSIDE_CAPTURE");
  if(!undersideCapture.isEmpty()) {
    solidView->setCameraView(CameraView::Bottom);app.processEvents();waitForModel(window);
    TEST_CHECK(window.screen()->grabWindow(window.winId()).save(undersideCapture));
    solidView->restoreCamera(camera);
  }
  const auto controlWingCapture=qEnvironmentVariable("FOAM_CONTROL_WING_CAPTURE");
  if(!controlWingCapture.isEmpty())TEST_CHECK(window.screen()->grabWindow(window.winId()).save(controlWingCapture));
  tabs->setCurrentIndex(0);
  click({500,350});QApplication::sendEvent(view,&remove);
  TEST_CHECK(ailerons->isChecked() && !window.projectDocument().controls.panels[0][0].rectangle);
  tabs->setCurrentIndex(1);app.processEvents();waitForModel(window);
  TEST_CHECK(!ailerons->isChecked() && flaps->isChecked());
  TEST_CHECK(!window.projectDocument().controls.panels[0][0].enabled);
  TEST_CHECK(solidView->property("wingModelReady").toBool());
  tabs->setCurrentIndex(0);click({250,350});QApplication::sendEvent(view,&remove);
  TEST_CHECK(flaps->isChecked() && !window.projectDocument().controls.panels[0][1].rectangle);
  tools->actions()[3]->trigger();app.processEvents();waitForModel(window);
  TEST_CHECK(!flaps->isChecked() && !window.projectDocument().controls.panels[0][1].enabled);
  tools->actions()[5]->trigger();app.processEvents();waitForModel(window);
  TEST_CHECK(window.findChild<QWidget*>("sparPanel")->isVisible());
  window.findChild<QCheckBox*>("sparTop")->click();
  window.findChild<QCheckBox*>("sparBottom")->click();
  window.findChild<QCheckBox*>("sparMid")->click();
  window.findChild<QComboBox*>("sparBottomShape")->setCurrentIndex(1);
  window.findChild<QDoubleSpinBox*>("sparBottomChord")->setValue(50);
  const auto sparInput=qEnvironmentVariable("FOAM_SPAR_INPUT_CAPTURE");
  if(!sparInput.isEmpty())TEST_CHECK(window.saveProjectFile(sparInput,error));
  tabs->setCurrentIndex(1);app.processEvents();waitForModel(window);
  if(!solidView->property("wingModelReady").toBool())qFatal("%s",qPrintable(window.statusBar()->currentMessage()));
  checkCamera();
  const int sparRevision=solidView->property("wingModelRevision").toInt();
  const auto sparCapture=qEnvironmentVariable("FOAM_SPAR_WING_CAPTURE");
  if(!sparCapture.isEmpty())TEST_CHECK(window.screen()->grabWindow(window.winId()).save(sparCapture));
  const auto sparFile=temporary.filePath("spars.foam");TEST_CHECK(window.saveProjectFile(sparFile,error));
  tools->actions()[3]->trigger();tools->actions()[5]->trigger();app.processEvents();waitForModel(window);
  TEST_CHECK(!window.projectModified());TEST_CHECK(solidView->property("wingModelRevision").toInt()==sparRevision);
  tabs->setCurrentIndex(0);
  tools->actions()[1]->trigger(); click({200, 250}); QApplication::sendEvent(view, &remove);
  TEST_CHECK(sketch.stationEditor().lines().size() == 1 && !tools->actions()[2]->isEnabled());
  TEST_CHECK(!workspace->actions()[2]->isEnabled());
  TEST_CHECK(sketch.layers()[0].points == outline);
  tabs->setCurrentIndex(1); app.processEvents();waitForModel(window);
  TEST_CHECK(!solidView->property("wingModelReady").toBool());
  TEST_CHECK(processingStages>=6);
  TEST_CHECK(!QApplication::overrideCursor());
  return 0;
}

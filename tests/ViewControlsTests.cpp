#include "gui/MainWindow.h"
#include "gui/StartupSplash.h"
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTimer>
#include "gui/OcctViewport.h"
#include <QAction>
#include <QPushButton>
#include <QElapsedTimer>
#include <QStatusBar>
#include "processing/IndexedTasks.h"
#include <atomic>
#include <condition_variable>
#include <QApplication>
#include <QMenu>
#include <QScreen>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>
#include <stdexcept>

#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__));}while(false)
namespace designrc::gui {
class ViewControlsTest {
public:
  static void startup(const QString& directory) {
    QApplication::setStyle("Fusion");QApplication::setApplicationVersion("GUI test");
    StartupSplash splash;QElapsedTimer elapsed;elapsed.start();MainWindow w;
    CHECK(splash.isVisible()&&!splash.pixmap().isNull());CHECK(!w.isVisible());
    CHECK(splash.pixmap().width()<=512&&splash.pixmap().width()==splash.pixmap().height());
    QMouseEvent press{QEvent::MouseButtonPress,QPointF{10,10},QPointF{10,10},Qt::LeftButton,Qt::LeftButton,Qt::NoModifier};
    QApplication::sendEvent(&splash,&press);CHECK(splash.isVisible());
    if(!directory.isEmpty())CHECK(splash.grab().save(directory+"/startup-splash.png"));
    int ticks=0;QTimer heartbeat;QObject::connect(&heartbeat,&QTimer::timeout,[&]{++ticks;});heartbeat.start(20);
    splash.finishWhenReady(w);
    while(!w.isVisible()&&elapsed.elapsed()<6000){QApplication::processEvents();std::this_thread::sleep_for(std::chrono::milliseconds{1});}
    CHECK(w.isVisible()&&w.isMaximized()&&!splash.isVisible());CHECK(elapsed.elapsed()>=2950&&elapsed.elapsed()<6000);CHECK(ticks>20);
    CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyProcessing()&&!w.stabilizerProcessing());
    QMenu* help=nullptr;for(auto* action:w.menuBar()->actions())if(action->text()=="&Help")help=action->menu();
    CHECK(help&&help->actions().size()==1);CHECK(help->actions().front()->text()=="&About");
    bool inspected=false;
    QTimer::singleShot(100,&w,[&] {
      auto* dialog=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());CHECK(dialog);
      const auto text=dialog->text();CHECK(text.contains("Design foam RC airplanes"));CHECK(text.contains("Developed using OpenAI Codex"));
      CHECK(!text.contains("under development")&&!text.contains("derived from DesignRC")&&!text.contains("licenses")&&!text.contains(QApplication::applicationDirPath()));
      CHECK(text.contains("GNU GPL")&&text.contains("Open CASCADE"));
      if(!directory.isEmpty())CHECK(dialog->grab().save(directory+"/startup-about.png"));
      inspected=true;dialog->accept();
    });
    help->actions().front()->trigger();CHECK(inspected);
    std::cout<<"Splash duration: "<<elapsed.elapsed()<<" ms including About GUI check; responsive timer ticks: "<<ticks<<"\n";
  }
  static void toolbarOrder() {
    ProjectDocument project;project.reference.toScale=true;
    QImage reference{200,100,QImage::Format_RGB32};reference.fill(Qt::white);
    project.reference.image.physicalSizeMm=QSizeF{200,100};
    project.reference.image.pages.push_back({reference,QSizeF{200,100}});
    const SketchLayer outline{{{0,0},{100,0},{100,20},{0,20}},
      {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};
    project.fuselage.layers={outline,outline};
    MainWindow w;w.show();w.restoreProject(project);QApplication::processEvents();
    const auto actions=w.workspaceToolBar_->actions();
    CHECK(actions.size()==9);
    const QStringList names{"Inspect","Weight and Balance","Export"};
    const std::array<int,3> ids{8,7,6};
    for(int i=0;i<3;++i) {
      CHECK(actions[6+i]->text()==names[i]);CHECK(actions[6+i]->data().toInt()==ids[i]);
      CHECK(actions[6+i]==w.workspaceActions_[ids[i]]);
      if(i>0)CHECK(w.workspaceToolBar_->actionGeometry(actions[5+i]).left()<w.workspaceToolBar_->actionGeometry(actions[6+i]).left());
    }
    CHECK(actions[6]->isEnabled()&&!actions[8]->isEnabled());
    auto click=[&](int position,int id) {
      auto* button=qobject_cast<QToolButton*>(w.workspaceToolBar_->widgetForAction(actions[position]));CHECK(button);
      button->click();QApplication::processEvents();
      CHECK(w.projectDocument().workspace==id&&actions[position]->isChecked());
      CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_);
    };
    click(6,8);CHECK(!w.graphicsTabs_->isTabEnabled(0));
    actions[7]->setEnabled(true);click(7,7);CHECK(!w.graphicsTabs_->isTabEnabled(1));
    // An empty cached compound enables Export without generating any model.
    TopoDS_Compound empty;BRep_Builder{}.MakeCompound(empty);w.assemblyOriginals_.fuselage=empty;
    w.assemblySourceFingerprint_=w.assemblyFingerprint();w.updateExportAvailability();
    CHECK(actions[8]->isEnabled());click(8,6);CHECK(!w.graphicsTabs_->isTabEnabled(0));
    w.updateWorkspaceAvailability();CHECK(actions[6]->isEnabled());
  }
  static void run(const QString& capture) {
    toolbarOrder();
    MainWindow w;w.resize(1280,900);w.show();QApplication::processEvents();
    auto* controls=w.findChild<QToolBar*>("viewportViewControls");
    auto* menu=w.findChild<QMenu*>("viewMenu");CHECK(controls&&menu);
    CHECK(controls->actions()==menu->actions());CHECK(controls->actions().size()==8);
    CHECK(!controls->isVisible());
    w.graphicsTabs_->setCurrentWidget(w.viewport_);QApplication::processEvents();
    CHECK(controls->isVisible());CHECK(w.viewport_->cameraState());
    const auto initial=w.viewport_->cameraState();
    auto same=[](const CameraState& a,const CameraState& b){
      for(int i=0;i<3;++i)CHECK(std::abs(a.eye[i]-b.eye[i])<1e-7&&std::abs(a.center[i]-b.center[i])<1e-7&&std::abs(a.up[i]-b.up[i])<1e-7);
      CHECK(std::abs(a.scale-b.scale)<1e-7);CHECK(a.projection==b.projection);
    };
    const QStringList names{"Fit View","Reset","Top","Bottom","Front","Back","Left","Right"};
    for(int i=0;i<names.size();++i) {
      auto* action=menu->actions()[i];CHECK(action->text()==names[i]);
      auto* button=qobject_cast<QToolButton*>(controls->widgetForAction(action));CHECK(button&&button->isVisible());
      CHECK(button->defaultAction()==action);
      w.viewport_->restoreCamera(initial);action->trigger();const auto expected=w.viewport_->cameraState();CHECK(expected);
      w.viewport_->restoreCamera(initial);
      int triggers=0;const auto connection=QObject::connect(action,&QAction::triggered,&w,[&]{++triggers;});
      button->click();QApplication::processEvents();CHECK(triggers==1);same(*expected,*w.viewport_->cameraState());
      action->setEnabled(false);CHECK(!button->isEnabled());button->click();CHECK(triggers==1);
      action->setEnabled(true);QObject::disconnect(connection);
    }
    for(const auto size:{QSize{1000,720},QSize{1500,950},QSize{1280,900}}) {
      w.resize(size);QApplication::processEvents();
      CHECK(std::abs(controls->geometry().center().x()-w.viewport_->rect().center().x())<=1);
      CHECK(w.viewport_->height()-controls->geometry().bottom()-1==12);
      CHECK(w.viewport_->rect().contains(controls->geometry()));
    }
    w.setModelProcessing(true);CHECK(!controls->isEnabled());
    for(auto* action:controls->actions())CHECK(!action->isEnabled());
    w.setModelProcessing(false);CHECK(controls->isEnabled());CHECK(!QApplication::overrideCursor());
    for(auto* action:controls->actions())CHECK(action->isEnabled());
    w.graphicsTabs_->setCurrentIndex(0);QApplication::processEvents();CHECK(!controls->isVisible());
    menu->actions()[1]->trigger();QApplication::processEvents();CHECK(controls->isVisible());
    CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_);CHECK(!w.projectModified());
    // A failed job stays suppressed during ordinary refreshes, but explicit
    // 2D -> 3D navigation invalidates its attempt marker without editing data.
    const auto fingerprint=w.fuselageFingerprint();
    w.builtFuselageFingerprint_=fingerprint;w.fuselageJobFingerprint_=fingerprint;
    w.fuselageJobEpoch_=w.projectEpoch_;
    w.fuselageJob_=std::make_unique<processing::BackgroundJob<geometry::FuselageBuildResult>>(
      [](std::stop_token,const auto&) -> geometry::FuselageBuildResult {throw std::runtime_error("Injected offset failure");});
    while(!w.fuselageJob_->ready())std::this_thread::sleep_for(std::chrono::milliseconds{1});
    w.pollFuselageJob();CHECK(w.fuselageRetryPending_);CHECK(w.builtFuselageFingerprint_==fingerprint);
    w.graphicsTabs_->setCurrentIndex(0);CHECK(w.fuselageRetryPending_);
    w.graphicsTabs_->setCurrentIndex(1);CHECK(!w.fuselageRetryPending_);CHECK(w.builtFuselageFingerprint_.isEmpty());
    CHECK(!w.fuselageJob_);CHECK(!w.projectModified());
    // Exercise the actual Cancel button with all four pin-style workers alive.
    std::atomic_int entered{0},exited{0};
    w.fuselageJobFingerprint_=w.fuselageFingerprint();w.fuselageJobEpoch_=w.projectEpoch_;
    w.fuselageJob_=std::make_unique<processing::BackgroundJob<geometry::FuselageBuildResult>>(
      [&](std::stop_token stop,const auto&) -> geometry::FuselageBuildResult {
        processing::runIndexedTasks(4,[&](std::size_t,std::stop_token child) {
          ++entered;
          std::mutex mutex;std::condition_variable_any wake;std::unique_lock lock{mutex};
          wake.wait(lock,child,[]{return false;});++exited;
          geometry::ProcessingControl{child}.checkpoint();
        },stop,4);
        return {};
      });
    w.setModelProcessing(true);
    QElapsedTimer timeout;timeout.start();
    while(entered!=4&&timeout.elapsed()<5000){QApplication::processEvents();std::this_thread::sleep_for(std::chrono::milliseconds{1});}
    CHECK(entered==4);CHECK(w.cancelProcessing_->isEnabled());CHECK(w.cancelProcessing_->isVisible());
    w.cancelProcessing_->click();CHECK(w.fuselageJob_->cancelled());CHECK(!w.cancelProcessing_->isEnabled());
    CHECK(w.cancelProcessing_->text()=="Cancelling...");
    timeout.restart();
    while(w.fuselageJob_&&timeout.elapsed()<5000){QApplication::processEvents();std::this_thread::sleep_for(std::chrono::milliseconds{1});}
    CHECK(!w.fuselageJob_);CHECK(exited==4);CHECK(!w.property("modelProcessing").toBool());
    CHECK(!QApplication::overrideCursor());CHECK(w.statusBar()->currentMessage().contains("cancelled"));
    CHECK(!w.viewport_->property("fuselageModelReady").toBool());CHECK(!w.projectModified());
    if(!capture.isEmpty())CHECK(w.screen()->grabWindow(w.winId()).save(capture));
  }
};
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QTemporaryDir settings;
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
  app.setOrganizationName("ViewControlsTests");app.setApplicationName("ViewControlsTests");
  try {if(argc>1&&QString::fromLocal8Bit(argv[1])=="--startup-only") {
    designrc::gui::ViewControlsTest::startup(argc>2?QString::fromLocal8Bit(argv[2]):QString{});
    std::cout<<"Startup and About GUI checks passed (no model generation)\n";return 0;
  }designrc::gui::ViewControlsTest::run(argc>1?QString::fromLocal8Bit(argv[1]):QString{});std::cout<<"View controls GUI checks passed\n";return 0;}
  catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

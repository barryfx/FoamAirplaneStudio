#include "gui/MainWindow.h"
#include "gui/OcctViewport.h"
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QScreen>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>
#include <cmath>
#include <iostream>
#include <stdexcept>

#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__));}while(false)
namespace designrc::gui {
class ViewControlsTest {
public:
  static void run(const QString& capture) {
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
    if(!capture.isEmpty())CHECK(w.screen()->grabWindow(w.winId()).save(capture));
  }
};
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QTemporaryDir settings;
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
  app.setOrganizationName("ViewControlsTests");app.setApplicationName("ViewControlsTests");
  try {designrc::gui::ViewControlsTest::run(argc>1?QString::fromLocal8Bit(argv[1]):QString{});std::cout<<"View controls GUI checks passed\n";return 0;}
  catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

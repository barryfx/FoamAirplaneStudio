#include "gui/PlanViewport.h"
#include "gui/ControlSurfacePanel.h"
#include <QApplication>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <QScreen>
#include <QTabBar>
#include <iostream>
using namespace designrc::gui;
#define CHECK(c) do {if(!(c))qFatal("%s at line %d",#c,__LINE__);} while(false)
int main(int argc,char** argv) {
  QApplication app{argc,argv};QWidget window;auto* layout=new QVBoxLayout{&window};
  auto* view=new PlanViewport{&window};auto& editor=view->controlSurfaceEditor();
  auto* panel=new ControlSurfacePanel{editor,&window};layout->addWidget(panel);layout->addWidget(view);
  window.resize(900,900);window.show();app.processEvents();editor.setEditing(true);
  auto* a=panel->findChild<QCheckBox*>("addAilerons");auto* f=panel->findChild<QCheckBox*>("addFlaps");
  auto* drawA=panel->findChild<QPushButton*>("drawAilerons");
  auto* drawF=panel->findChild<QPushButton*>("drawFlaps");
  CHECK(!drawA->isChecked() && !drawF->isChecked());
  CHECK(!a->isChecked() && !f->isChecked());
  auto* tape=panel->findChild<QRadioButton*>("AileronsTapeHinge");
  auto* standard=panel->findChild<QRadioButton*>("AileronsStandardHinge");CHECK(!tape->isVisible());
  a->click();app.processEvents();CHECK(tape->isVisible() && tape->isChecked());CHECK(editor.state().drawing==0);CHECK(drawA->isChecked() && !drawF->isChecked());
  auto mouse=[&](QEvent::Type type,QPointF scene,Qt::MouseButton button,Qt::MouseButtons buttons) {
    const auto pos=view->mapFromScene(scene);QMouseEvent e{type,QPointF{pos},QPointF{view->viewport()->mapToGlobal(pos)},button,buttons,Qt::NoModifier};
    QApplication::sendEvent(view->viewport(),&e);
  };
  auto click=[&](QPointF p){mouse(QEvent::MouseButtonPress,p,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,p,Qt::LeftButton,Qt::NoButton);};
  click({100,100});CHECK(editor.state().first);click({500,300});CHECK(editor.state().panels[0][0].rectangle);
  const auto rectangle=*editor.state().panels[0][0].rectangle;CHECK(rectangle.width()>390 && rectangle.height()>190);
  CHECK(editor.state().drawing==-1);CHECK(!drawA->isChecked());
  standard->click();CHECK(standard->isChecked() && !tape->isChecked());CHECK(editor.state().panels[0][0].hinge==HingeCut::Standard);
  f->click();app.processEvents();CHECK(editor.state().drawing==1);CHECK(drawF->isChecked() && !drawA->isChecked());
  mouse(QEvent::MouseButtonPress,{550,100},Qt::LeftButton,Qt::LeftButton);
  mouse(QEvent::MouseMove,{800,300},Qt::NoButton,Qt::LeftButton);
  mouse(QEvent::MouseButtonRelease,{800,300},Qt::LeftButton,Qt::NoButton);
  CHECK(editor.state().panels[0][1].rectangle);CHECK(!drawF->isChecked());
  // Reject overlapping redraws immediately, keeping both committed rectangles.
  const auto originalFlap=editor.state().panels[0][1].rectangle;
  drawF->click();click({150,150});click({600,350});
  CHECK(editor.state().panels[0][1].rectangle==originalFlap);
  CHECK(editor.state().panels[0][0].rectangle==rectangle && drawF->isChecked());
  CHECK(panel->findChild<QLabel*>("controlOverlapWarning")->text().contains("must not overlap"));
  editor.cancel();
  auto overlapping=editor.state();overlapping.panels[0][1].rectangle=rectangle;
  editor.restore(overlapping);
  CHECK(panel->findChild<QLabel*>("controlOverlapWarning")->text().contains("overlap"));
  overlapping.panels[0][1].rectangle=originalFlap;editor.restore(overlapping);
  CHECK(panel->findChild<QLabel*>("controlOverlapWarning")->text().isEmpty());
  panel->findChild<QPushButton*>("drawAilerons")->click();click({50,50});
  QKeyEvent escape{QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier};QApplication::sendEvent(view,&escape);
  CHECK(editor.state().drawing==-1 && !editor.state().first);CHECK(editor.state().panels[0][0].rectangle==rectangle);
  CHECK(!drawA->isChecked());
  drawA->click();CHECK(drawA->isChecked());drawA->click();CHECK(!drawA->isChecked() && editor.state().drawing==-1);
  auto restored=editor.state();restored.drawing=0;editor.restore(restored);CHECK(drawA->isChecked());
  editor.setEditing(false);CHECK(!drawA->isChecked());editor.setEditing(true);
  const auto saved=editor.state();
  int dataChanges=0;editor.changed=[&]{++dataChanges;};
  QKeyEvent remove{QEvent::KeyPress,Qt::Key_Delete,Qt::NoModifier};
  click(rectangle.center());CHECK(editor.selectedRectangle()==0);CHECK(dataChanges==0);
  const auto capture=qEnvironmentVariable("FOAM_CONTROL_SELECTION_CAPTURE");
  app.processEvents();
  if(!capture.isEmpty())CHECK(window.screen()->grabWindow(window.winId()).save(capture));
  QApplication::sendEvent(view,&escape);CHECK(editor.selectedRectangle()==-1);CHECK(dataChanges==0);
  QApplication::sendEvent(view,&remove);CHECK(editor.state().panels[0][0].rectangle==rectangle);CHECK(dataChanges==0);
  const auto flap=*editor.state().panels[0][1].rectangle;
  click(flap.center());CHECK(editor.selectedRectangle()==1);
  click(rectangle.center());CHECK(editor.selectedRectangle()==0);
  QApplication::sendEvent(view,&remove);CHECK(!editor.state().panels[0][0].rectangle);
  CHECK(editor.state().panels[0][1].rectangle==flap);CHECK(editor.state().panels[0][0].enabled);
  CHECK(editor.state().panels[0][0].hinge==HingeCut::Standard);CHECK(dataChanges==1);
  click(flap.center());QApplication::sendEvent(view,&remove);CHECK(!editor.state().panels[0][1].rectangle);CHECK(dataChanges==2);
  editor.restore(saved);CHECK(editor.selectedRectangle()==-1);
  // Edge tolerance remains seven screen pixels even with zoom.
  view->scale(2,2);
  const auto nearEdge=view->mapToScene(view->mapFromScene(rectangle.topLeft())+QPoint{-5,20});
  click(nearEdge);CHECK(editor.selectedRectangle()==0);
  click({-100,-100});CHECK(editor.selectedRectangle()==-1);
  click(rectangle.center());editor.begin(1);CHECK(editor.selectedRectangle()==-1);
  QApplication::sendEvent(view,&remove);CHECK(editor.state().panels[0][0].rectangle==rectangle);
  editor.cancel();click(flap.center());editor.setEditing(false);CHECK(editor.selectedRectangle()==-1);
  QApplication::sendEvent(view,&remove);CHECK(editor.state().panels[0][1].rectangle==flap);
  editor.setEditing(true);editor.changed={};
  bool instructions=false;
  for(auto* label:panel->findChildren<QLabel*>())if(label->text().contains("Escape clears the highlight and Delete removes"))instructions=true;
  CHECK(instructions);
  a->click();CHECK(!tape->isVisible());CHECK(!editor.state().panels[0][0].enabled);
  CHECK(editor.state().panels[0][0].rectangle==rectangle); // Disabling preserves settings for reuse.
  editor.setEditing(false);click({20,20});click({40,40});CHECK(editor.state().panels[0][0].rectangle==rectangle);
  editor.mapPoints([](QPointF p){return p*2;});CHECK(editor.state().panels[0][0].rectangle->width()==rectangle.width()*2);
  CHECK(view->sketchEditor().layers()[0].curves.empty());
  // Leaving the mode clears enabled controls without a committed rectangle.
  ControlSurfaceState unfinished;unfinished.panels[0][0].enabled=true;
  unfinished.panels[0][1]={true,HingeCut::Standard,rectangle};
  unfinished.drawing=0;unfinished.first=QPointF{10,10};
  editor.setEditing(true);editor.restore(unfinished);editor.setEditing(false);
  CHECK(!a->isChecked() && f->isChecked());
  CHECK(!editor.state().first && editor.state().drawing==-1);
  CHECK(editor.state().panels[0][1].rectangle==rectangle);
  CHECK(editor.state().panels[0][1].hinge==HingeCut::Standard);
  editor.restore(unfinished);editor.finishEditing(); // Same cleanup used by 3D entry.
  CHECK(!a->isChecked() && f->isChecked() && !drawA->isChecked());
  view->clearPlan();panel->restoreControls();CHECK(!a->isChecked() && !f->isChecked());
  CHECK(!editor.state().panels[0][0].rectangle && !editor.state().panels[0][1].rectangle);
  editor.setPanelCount(2);editor.setEditing(true);
  auto* panelTabs=panel->findChild<QTabBar*>("controlPanelTabs");CHECK(panelTabs->count()==2);
  panelTabs->setCurrentIndex(1);f->click();click({100,100});click({400,300});
  panel->findChild<QRadioButton*>("FlapsStandardHinge")->click();
  CHECK(editor.state().panels[1][1].rectangle && !editor.state().panels[0][1].enabled);
  panelTabs->setCurrentIndex(0);CHECK(!f->isChecked());
  a->click();click({200,100});click({500,300});CHECK(!editor.state().panels[0][0].rectangle);
  editor.cancel();click({200,200});QApplication::sendEvent(view,&remove);CHECK(editor.state().panels[1][1].rectangle);
  panelTabs->setCurrentIndex(1);CHECK(f->isChecked());CHECK(panel->findChild<QRadioButton*>("FlapsStandardHinge")->isChecked());
  editor.setPanelCount(1);editor.setPanelCount(2);CHECK(!editor.state().panels[1][1].enabled);
  std::cout<<"Checkboxes, hinge exclusivity, rectangle clicks/drag, selection, deletion, Escape, mode locking, remap and reset passed.\n";
}

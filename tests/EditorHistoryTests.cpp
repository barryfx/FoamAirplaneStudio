#include "gui/MainWindow.h"
#include "gui/PlanViewport.h"
#include "gui/WeightBalancePanel.h"
#include "gui/InspectPanel.h"
#include "gui/StiffenerPanel.h"
#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QTemporaryDir>
#include <QScreen>
#include <QToolBar>
#include <iostream>
#include <stdexcept>
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__));}while(false)
namespace designrc::gui {
static void key(QWidget* target,int code,Qt::KeyboardModifiers modifiers=Qt::NoModifier) {
  QKeyEvent event{QEvent::KeyPress,code,modifiers};QApplication::sendEvent(target,&event);QApplication::processEvents();
}
static void mouse(PlanViewport& view,QEvent::Type type,QPointF point,Qt::MouseButtons buttons) {
  const auto local=view.mapFromScene(point);
  QMouseEvent event{type,QPointF{local},QPointF{view.viewport()->mapToGlobal(local)},type==QEvent::MouseMove?Qt::NoButton:Qt::LeftButton,buttons,Qt::NoModifier};
  QApplication::sendEvent(view.viewport(),&event);
}
static void click(PlanViewport& view,QPointF point) {
  mouse(view,QEvent::MouseButtonPress,point,Qt::LeftButton);mouse(view,QEvent::MouseButtonRelease,point,Qt::NoButton);QApplication::processEvents();
}
static SketchLayer curves() {
  return {{{0,0},{80,0},{0,80},{40,120},{80,80},{160,80},{190,80}},
    {{SketchTool::Line,{0,1}},{SketchTool::Spline,{2,3,4}},{SketchTool::Circle,{5,6}}}};
}
class EditorHistoryTest {
public:
  static void run(const QString& capture) {
    MainWindow w;w.resize(1200,850);w.show();QApplication::processEvents();
    auto& view=*w.planViewport_;
    auto* undo=w.findChild<QAction*>("editUndo");auto* redo=w.findChild<QAction*>("editRedo");
    CHECK(undo&&redo&&!undo->isEnabled()&&!redo->isEnabled());
    CHECK(undo->shortcut()==QKeySequence(Qt::CTRL|Qt::Key_Z));CHECK(redo->shortcut()==QKeySequence(Qt::CTRL|Qt::Key_Y));
    auto project=w.projectDocument();project.reference.toScale=true;
    project.reference.image.physicalSizeMm=QSizeF{500,400};
    project.reference.image.pages.push_back({QImage{500,400,QImage::Format_RGB32},QSizeF{500,400}});
    project.reference.image.pages[0].pixels.fill(Qt::white);
    project.workspace=1;project.tool="Outline";project.wing.editing=true;project.wing.layers={curves()};
    w.restoreProject(project);QApplication::processEvents();w.resetEditHistory();
    // GUI selection, highlight, individual deletion and keyboard undo/redo.
    auto& editor=view.sketchEditor();editor.setEditing(true);editor.setTool(SketchTool::None);
    view.setSceneRect(-100,-100,700,600);
    view.fitInView(QRectF{-50,-50,600,500},Qt::KeepAspectRatio);
    click(view,{40,0});CHECK(editor.selectedCurve()==0);const auto selected=view.grab().toImage();
    CHECK(w.editHistory_.empty());
    if(!capture.isEmpty()) {
      QImage image{500,260,QImage::Format_RGB32};image.fill(Qt::white);
      {QPainter painter{&image};painter.translate(60,60);editor.paint(painter);}
      CHECK(image.save(capture));
    }
    click(view,{40,120});CHECK(editor.selectedCurve()==1);CHECK(selected!=view.grab().toImage());
    click(view,{190,80});CHECK(editor.selectedCurve()==2);
    click(view,{40,0});CHECK(editor.selectedCurve()==0);
    key(&view,Qt::Key_Delete);CHECK(editor.layers()[0].curves.size()==2);CHECK(undo->isEnabled());
    key(&view,Qt::Key_Z,Qt::ControlModifier);CHECK(editor.layers()[0].curves.size()==3);CHECK(redo->isEnabled());
    key(&view,Qt::Key_Y,Qt::ControlModifier);CHECK(editor.layers()[0].curves.size()==2);
    undo->trigger();CHECK(editor.layers()[0].curves.size()==3);
    // A multi-event drag produces one revision, including when the timer fires.
    w.resetEditHistory();const auto before=editor.layers()[0].points;
    mouse(view,QEvent::MouseButtonPress,{0,0},Qt::LeftButton);
    mouse(view,QEvent::MouseMove,{10,10},Qt::LeftButton);w.captureEdit();CHECK(w.editHistory_.empty());
    mouse(view,QEvent::MouseMove,{20,20},Qt::LeftButton);
    mouse(view,QEvent::MouseButtonRelease,{20,20},Qt::NoButton);QApplication::processEvents();
    CHECK(w.editHistory_.size()==1);CHECK(editor.layers()[0].points!=before);
    undo->trigger();CHECK(editor.layers()[0].points==before);redo->trigger();CHECK(editor.layers()[0].points!=before);
    // Every sketch collection participates in the same history, preserving types.
    std::vector<SketchEditor*> sketches{&view.sketchEditor(),&view.airfoilSketchEditor(),&view.fuselageSketchEditor(),
      &view.fuselageProfileEditor(),&view.fuselageCutEditor(),&view.fuselageHoleEditor()};
    for(int i=0;i<2;++i){sketches.push_back(&view.stabilizerSketchEditor(i));sketches.push_back(&view.stabilizerHingeEditor(i));sketches.push_back(&view.stabilizerCutEditor(i));}
    for(auto* sketch:sketches) {
      w.resetEditHistory();const auto old=sketch->state();auto state=old;state.layers[0]=curves();state.layers[0].points[0]+={3,4};
      sketch->restoreState(state);w.captureEdit();CHECK(w.editHistory_.size()==1);
      undo->trigger();CHECK(sketch->layers()[0].points==old.layers[0].points);
      redo->trigger();CHECK(sketch->layers()[0].points==state.layers[0].points);
    }
    // Snapshot equality checks non-sketch editors and dependent station metadata.
    auto roundTrip=[&](auto change) {
      w.resetEditHistory();const auto before=w.projectFingerprint();change();w.captureEdit();const auto after=w.projectFingerprint();
      CHECK(before!=after);CHECK(w.editHistory_.size()==1);undo->trigger();CHECK(w.projectFingerprint()==before);
      redo->trigger();CHECK(w.projectFingerprint()==after);
    };
    roundTrip([&]{auto state=view.controlSurfaceEditor().state();state.panels[0][0].enabled=true;state.panels[0][0].rectangle=QRectF{10,10,30,10};view.controlSurfaceEditor().restore(state);});
    roundTrip([&]{auto state=view.formerEditor().state();state.rectangles.push_back({10,10,3,20});state.rotationDegrees.push_back(15);view.formerEditor().restore(state);});
    roundTrip([&]{auto state=view.servoTrayEditor().state();state.rectangle=QRectF{20,20,30,10};view.servoTrayEditor().restore(state);});
    roundTrip([&]{auto state=w.weightBalancePanel_->state();state.parts.push_back({"Battery",20,30,50,125,{70,10},false});w.weightBalancePanel_->restore(state);});
    roundTrip([&]{auto state=w.weightBalancePanel_->state();state.parts[0].centerMm+={20,10};w.weightBalancePanel_->restore(state);});
    roundTrip([&]{auto state=w.weightBalancePanel_->state();state.parts.clear();w.weightBalancePanel_->restore(state);});
    roundTrip([&]{w.assemblyState_.offsets[0]={12,34};});
    roundTrip([&]{w.inspectPanel_->restore({{"Wing","Main Wing"}});});
    const auto wingBeforeStiffeners=w.wingFingerprint(),fuselageBeforeStiffeners=w.fuselageFingerprint();
    roundTrip([&]{auto state=w.stiffenerPanel_->state();state.count=2;w.stiffenerPanel_->restore(state);});
    CHECK(w.wingFingerprint()==wingBeforeStiffeners&&w.fuselageFingerprint()!=fuselageBeforeStiffeners);
    // Stations move as one click/hover/drop edit and restore with their source curve.
    project.wing.layers={{{{100,100},{400,100},{400,300},{100,300}},
      {{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}}}}};
    project.tool="Airfoil Stations";w.restoreProject(project);
    auto& stations=editor.stationEditor();editor.setEditing(false);stations.setEnabled(true);
    click(view,{180,100});click(view,{180,300});CHECK(stations.lines().size()==1);
    w.resetEditHistory();const auto stationBefore=stations.lines()[0].first.position;
    click(view,stationBefore);mouse(view,QEvent::MouseMove,{230,100},Qt::NoButton);w.captureEdit();CHECK(w.editHistory_.empty());
    click(view,{230,100});CHECK(w.editHistory_.size()==1);const auto stationAfter=stations.lines()[0].first.position;
    CHECK(stationAfter!=stationBefore);undo->trigger();CHECK(stations.lines()[0].first.position==stationBefore);
    redo->trigger();CHECK(stations.lines()[0].first.position==stationAfter);
    // Switch through the real toolbar so history restores the matching editing mode.
    for(auto* action:w.componentToolBar_->actions())if(action->text()=="Outline")action->trigger();
    editor.setEditing(true);editor.setTool(SketchTool::None);stations.setEnabled(false);w.resetEditHistory();
    click(view,{300,100});key(&view,Qt::Key_Delete);CHECK(stations.lines().empty());
    undo->trigger();CHECK(stations.lines().size()==1);CHECK(editor.layers()[0].curves.size()==3);
    redo->trigger();CHECK(stations.lines().empty());CHECK(editor.layers()[0].curves.size()==2);
    // Save point stays independent from history; new edits discard the redo branch.
    QTemporaryDir output;QString error;if(!w.saveProjectFile(output.filePath("History.foam"),error))throw std::runtime_error(error.toStdString());CHECK(!w.projectModified());
    undo->trigger();CHECK(w.projectModified());redo->trigger();CHECK(!w.projectModified());
    undo->trigger();auto state=w.weightBalancePanel_->state();state.densityKgM3=40;w.weightBalancePanel_->restore(state);w.captureEdit();CHECK(!redo->isEnabled());
    CHECK(w.projectPath_==output.filePath("History.foam"));
    w.resetProject();CHECK(!undo->isEnabled()&&!redo->isEnabled());
    CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_&&!w.stabilizerProcessing());
  }
};
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QApplication::setStyle("Fusion");QTemporaryDir settings;
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
  app.setOrganizationName("EditorHistoryTests");app.setApplicationName("EditorHistoryTests");
  try {designrc::gui::EditorHistoryTest::run(argc>1?QString::fromLocal8Bit(argv[1]):QString{});std::cout<<"Editor history GUI checks passed\n";return 0;}
  catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

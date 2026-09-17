#include "gui/FuselageProfilePanel.h"
#include "gui/SketchBoundary.h"
#include "gui/FuselageStationOrder.h"
#include <QGraphicsView>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
namespace designrc::gui {
FuselageProfilePanel::FuselageProfilePanel(QGraphicsView& view,SketchEditor& source,SketchEditor& profiles,QWidget* parent)
    : QWidget{parent},view_{view},source_{source},profiles_{profiles} {
  setObjectName("fuselageProfilePanel");
  auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* text=new QLabel{"Click a Side View station to highlight it and edit its cross-section. Draw one closed loop anywhere over the 2D reference using Line or Spline. Draw left/right as width and up/down as height. Click the first point to close a spline; Escape finishes it. Turn Line/Spline off to move points, select curves for Delete, or select another station. Each sketch stays attached when its station moves. Delete Profile removes the entire assigned sketch.\n\nOpen 3D View when every station has a closed profile. Width and height fit independently to the Top and Side outlines, aligned at their noses (left ends).",this};
  text->setWordWrap(true);layout->addWidget(text);
  selection_=new QLabel{this};selection_->setObjectName("fuselageSelectedStation");selection_->setWordWrap(true);layout->addWidget(selection_);
  tools_=new QWidget{this};auto* row=new QHBoxLayout{tools_};row->setContentsMargins(0,0,0,0);
  for(auto tool:{SketchTool::Line,SketchTool::Spline}) {
    auto* button=new QPushButton{tool==SketchTool::Line?"Line":"Spline",tools_};button->setCheckable(true);
    button->setObjectName(tool==SketchTool::Line?"fuselageProfileLine":"fuselageProfileSpline");
    button->setProperty("tool",static_cast<int>(tool));row->addWidget(button);
    connect(button,&QPushButton::clicked,this,[this,tool](bool checked){chooseTool(tool,checked);});
  }
  layout->addWidget(tools_);
  remove_=new QPushButton{"Delete Profile",this};remove_->setObjectName("deleteFuselageProfile");layout->addWidget(remove_);layout->addStretch();
  connect(remove_,&QPushButton::clicked,this,[this] {
    auto& stations=source_.stationEditor();const int selected=stations.selectedLine();
    if(selected<0 || selected>=static_cast<int>(stations.lines().size()))return;
    const auto slot=stations.lines()[selected].profile;if(!slot)return;
    profiles_.setEditing(false);auto state=profiles_.state();state.layers.at(*slot)={};state.tool=SketchTool::None;state.selected=-1;
    profiles_.restoreState(state);stations.assignSelectedProfile({});selectStation();emit profiles_.changed();
  });
  connect(&source_,&SketchEditor::stationSelectionChanged,this,[this]{selectStation();});
  view_.viewport()->installEventFilter(this);
}
void FuselageProfilePanel::setActive(bool active) {
  if(active_==active)return;
  profiles_.setEditing(false);profiles_.setTool(SketchTool::None);active_=active;
  source_.stationEditor().setSelectionEnabled(active);selectStation();
}
void FuselageProfilePanel::selectStation() {
  if(!active_)return;
  auto& stations=source_.stationEditor();const int selected=stations.selectedLine();
  profiles_.setEditing(false);profiles_.setTool(SketchTool::None);
  const bool valid=selected>=0 && selected<static_cast<int>(stations.lines().size());
  tools_->setVisible(valid);remove_->setVisible(valid);
  selection_->setText(valid?QString{"Station %1 selected"}.arg(fuselageStationNumber(stations.lines(),selected)):"Click a Side View station to select it.");
  remove_->setEnabled(valid&&stations.lines()[selected].profile.has_value());
  if(valid)if(auto slot=stations.lines()[selected].profile) {
    profiles_.setActiveLayer(static_cast<int>(*slot));profiles_.setEditing(true);
  }
  for(auto* button:tools_->findChildren<QPushButton*>())button->setChecked(false);
  view_.viewport()->update();
}
void FuselageProfilePanel::chooseTool(SketchTool tool,bool checked) {
  auto& stations=source_.stationEditor();const int selected=stations.selectedLine();
  if(!active_ || selected<0 || selected>=static_cast<int>(stations.lines().size()))return;
  if(!stations.lines()[selected].profile && checked) {
    int slot=static_cast<int>(profiles_.layers().size());
    // Reuse only unassigned empty slots. Deleting/reordering stations cannot retarget profiles.
    for(int i=0;i<slot;++i)if(profiles_.layers()[i].curves.empty() &&
        std::none_of(stations.lines().begin(),stations.lines().end(),[i](const auto& line){return line.profile==static_cast<std::size_t>(i);})) {slot=i;break;}
    if(slot==profiles_.layers().size())profiles_.setLayerCount(slot+1);
    profiles_.setActiveLayer(slot);stations.assignSelectedProfile(slot);
  }
  profiles_.setEditing(true);profiles_.setTool(checked?tool:SketchTool::None);remove_->setEnabled(true);
  for(auto* button:tools_->findChildren<QPushButton*>())button->setChecked(button->property("tool").toInt()==static_cast<int>(profiles_.tool()));
}
void FuselageProfilePanel::restoreControls() {
  for(auto* button:tools_->findChildren<QPushButton*>())button->setChecked(button->property("tool").toInt()==static_cast<int>(profiles_.tool()));
}
bool FuselageProfilePanel::allProfilesClosed() const {
  const auto& lines=source_.stationEditor().lines();if(lines.empty())return false;
  return std::all_of(lines.begin(),lines.end(),[this](const auto& line){return line.profile && *line.profile<profiles_.layers().size() && closedSketchBoundary(profiles_.layers()[*line.profile]).has_value();});
}
bool FuselageProfilePanel::eventFilter(QObject* watched,QEvent* event) {
  if(!active_ || watched!=view_.viewport() || event->type()!=QEvent::MouseButtonPress || profiles_.tool()!=SketchTool::None)return false;
  auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
  const auto point=view_.mapToScene(mouse->position().toPoint());
  for(const auto& line:source_.stationEditor().lines()) {
    const auto a=line.first.position,d=line.second.position-a;const double length=QPointF::dotProduct(d,d);
    if(length>0 && QLineF{point,a+d*std::clamp(QPointF::dotProduct(point-a,d)/length,0.,1.)}.length()<=8/view_.transform().m11())
      return source_.stationEditor().event(event,true);
  }
  return false;
}
}

#include "gui/FuselageProfilePanel.h"
#include "gui/SketchBoundary.h"
#include "gui/FuselageStationOrder.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QPainterPathStroker>
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
  auto* text=new QLabel{"Click a Side View station to highlight it and edit its cross-section. Draw one closed loop anywhere over the 2D reference using Line, Spline or Circle. Click the circle center, then a point to set its radius. Draw left/right as width and up/down as height. Click the first point to close a spline; Escape finishes it. Turn drawing tools off to move points, select curves for Delete, or select another station. Each sketch stays attached when its station moves. To recover a profile left by a deleted station, select an unassigned station and double-click the unassigned profile with drawing tools off. Single-click an unattached profile to highlight it, then press Delete or Delete Profile to remove it. Delete Profile removes the entire assigned sketch.\n\nOpen 3D View when every station has a closed profile. Width and height fit independently to the Top and Side outlines, aligned at their noses (left ends).",this};
  text->setWordWrap(true);layout->addWidget(text);
  selection_=new QLabel{this};selection_->setObjectName("fuselageSelectedStation");selection_->setWordWrap(true);layout->addWidget(selection_);
  tools_=new QWidget{this};auto* stack=new QVBoxLayout{tools_};stack->setContentsMargins(0,0,0,0);
  auto* row=new QHBoxLayout;stack->addLayout(row);
  for(auto tool:{SketchTool::Line,SketchTool::Spline,SketchTool::Circle}) {
    const QString name=tool==SketchTool::Line?"Line":tool==SketchTool::Spline?"Spline":"Circle";
    auto* button=new QPushButton{name,tools_};button->setCheckable(true);
    button->setObjectName("fuselageProfile"+name);
    button->setProperty("tool",static_cast<int>(tool));
    if(tool==SketchTool::Circle)stack->addWidget(button);else row->addWidget(button);
    connect(button,&QPushButton::clicked,this,[this,tool](bool checked){chooseTool(tool,checked);});
  }
  layout->addWidget(tools_);
  auto* copyRow=new QHBoxLayout;layout->addLayout(copyRow);
  copy_=new QPushButton{"Copy Profile",this};copy_->setObjectName("copyFuselageProfile");copy_->setCheckable(true);copyRow->addWidget(copy_);
  paste_=new QPushButton{"Paste Profile",this};paste_->setObjectName("pasteFuselageProfile");copyRow->addWidget(paste_);
  paste_->setToolTip("Assign an independent copy to the selected station, replacing its existing profile.");
  move_=new QPushButton{"Move Profile",this};move_->setObjectName("moveFuselageProfile");move_->setCheckable(true);layout->addWidget(move_);
  connect(copy_,&QPushButton::clicked,this,[this](bool checked){setAction(checked?Action::Copy:Action::None);});
  connect(move_,&QPushButton::clicked,this,[this](bool checked){setAction(checked?Action::Move:Action::None);});
  connect(paste_,&QPushButton::clicked,this,[this]{pasteProfile();});
  remove_=new QPushButton{"Delete Profile",this};remove_->setObjectName("deleteFuselageProfile");layout->addWidget(remove_);layout->addStretch();
  connect(remove_,&QPushButton::clicked,this,[this] {
    const int orphan=selectedOrphan_;
    setAction(Action::None);
    auto& stations=source_.stationEditor();const int selected=stations.selectedLine();
    std::optional<std::size_t> slot;
    if(orphan>=0&&orphan<static_cast<int>(profiles_.layers().size())&&
        std::none_of(stations.lines().begin(),stations.lines().end(),[orphan](const auto& line){return line.profile==static_cast<std::size_t>(orphan);}))slot=orphan;
    else if(selected>=0&&selected<static_cast<int>(stations.lines().size()))slot=stations.lines()[selected].profile;
    if(!slot)return;
    profiles_.setEditing(false);auto state=profiles_.state();state.layers.at(*slot)={};state.tool=SketchTool::None;state.selected=-1;
    profiles_.restoreState(state);if(orphan<0)stations.assignSelectedProfile({});selectStation();emit profiles_.changed();
  });
  connect(&source_,&SketchEditor::stationSelectionChanged,this,[this]{selectStation();});
  connect(&profiles_,&SketchEditor::changed,this,[this]{syncActions();});
  view_.viewport()->installEventFilter(this);
  view_.installEventFilter(this);syncActions();
}
void FuselageProfilePanel::setActive(bool active) {
  if(active_==active)return;
  setAction(Action::None);
  profiles_.setEditing(false);profiles_.setTool(SketchTool::None);active_=active;
  source_.stationEditor().setSelectionEnabled(active);selectStation();syncActions();
}
void FuselageProfilePanel::selectStation() {
  if(!active_)return;
  setAction(Action::None);
  auto& stations=source_.stationEditor();const int selected=stations.selectedLine();
  profiles_.setEditing(false);profiles_.setTool(SketchTool::None);
  const bool valid=selected>=0 && selected<static_cast<int>(stations.lines().size());
  tools_->setVisible(valid);remove_->setVisible(valid);
  selection_->setText(valid?QString{"Station %1 selected"}.arg(fuselageStationNumber(stations.lines(),selected)):"Click a Side View station to select it.");
  remove_->setEnabled(valid&&stations.lines()[selected].profile.has_value());
  if(valid)if(auto slot=stations.lines()[selected].profile) {
    profiles_.setActiveLayer(static_cast<int>(*slot));profiles_.setEditing(true);
  }
  if(valid&&!stations.lines()[selected].profile)selection_->setText(selection_->text()+". Draw a new profile, or double-click an unassigned profile to reuse it.");
  for(auto* button:tools_->findChildren<QPushButton*>())button->setChecked(false);
  syncActions();
  view_.viewport()->update();
}
void FuselageProfilePanel::chooseTool(SketchTool tool,bool checked) {
  setAction(Action::None);
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
  includeProfileBounds();
  action_=Action::None;dragged_=-1;selectedOrphan_=-1;dragPoints_.clear();syncActions();
  for(auto* button:tools_->findChildren<QPushButton*>())button->setChecked(button->property("tool").toInt()==static_cast<int>(profiles_.tool()));
}
void FuselageProfilePanel::syncActions() {
  const bool hasProfiles=std::any_of(profiles_.layers().begin(),profiles_.layers().end(),[](const auto& layer){return !layer.curves.empty();});
  const auto selected=source_.stationEditor().selectedLine();
  const bool station=selected>=0&&selected<static_cast<int>(source_.stationEditor().lines().size());
  int highlighted=-1;
  if(active_&&station)if(auto slot=source_.stationEditor().lines()[selected].profile)highlighted=static_cast<int>(*slot);
  if(active_&&dragged_>=0)highlighted=dragged_;
  if(active_&&selectedOrphan_>=0)highlighted=selectedOrphan_;
  remove_->setVisible(active_&&(station||selectedOrphan_>=0));
  remove_->setEnabled(active_&&(selectedOrphan_>=0||(station&&source_.stationEditor().lines()[selected].profile.has_value())));
  profiles_.setHighlightedLayer(highlighted);
  copy_->setEnabled(active_&&hasProfiles);move_->setEnabled(active_&&hasProfiles);
  paste_->setEnabled(active_&&station&&clipboard_.has_value());
  copy_->setChecked(action_==Action::Copy);move_->setChecked(action_==Action::Move);
}
void FuselageProfilePanel::setAction(Action action) {
  action_=action;dragged_=-1;selectedOrphan_=-1;dragPoints_.clear();
  if(active_) {
    profiles_.setTool(SketchTool::None);
    const auto& stations=source_.stationEditor();const int selected=stations.selectedLine();
    profiles_.setEditing(action==Action::None&&selected>=0&&selected<static_cast<int>(stations.lines().size())&&stations.lines()[selected].profile.has_value());
  }
  for(auto* button:tools_->findChildren<QPushButton*>())button->setChecked(false);
  view_.viewport()->setCursor(action==Action::Copy?Qt::CrossCursor:action==Action::Move?Qt::OpenHandCursor:Qt::ArrowCursor);
  if(action==Action::Copy)selection_->setText("Click a profile to copy. Then select the destination station and Paste Profile.");
  if(action==Action::Move)selection_->setText("Hold the left mouse button on a profile and drag. Release to drop it.");
  if(action==Action::None) {
    const auto& stations=source_.stationEditor();const auto selected=stations.selectedLine();
    selection_->setText(selected>=0&&selected<static_cast<int>(stations.lines().size())
        ?QString{"Station %1 selected"}.arg(fuselageStationNumber(stations.lines(),selected)):"Click a Side View station to select it.");
  }
  syncActions();
}
int FuselageProfilePanel::profileAt(QPointF point) const {
  QPainterPathStroker stroke;stroke.setWidth(16);
  const auto transform=view_.viewportTransform();
  for(int i=static_cast<int>(profiles_.layers().size())-1;i>=0;--i) {
    const auto& layer=profiles_.layers()[i];
    if(const auto boundary=closedSketchBoundary(layer)) {
      QPainterPath area;area.moveTo(boundary->front());for(auto p:*boundary)area.lineTo(p);
      if(area.contains(point))return i;
    }
    for(const auto& curve:layer.curves) {
      std::vector<QPointF> points;for(auto id:curve.points)points.push_back(layer.points[id]);
      if(stroke.createStroke(transform.map(SketchEditor::fittedPath(points,curve.type))).contains(transform.map(point)))return i;
    }
  }
  return -1;
}
void FuselageProfilePanel::pasteProfile() {
  if(!active_||!clipboard_)return;
  auto& stations=source_.stationEditor();const int selected=stations.selectedLine();
  if(selected<0||selected>=static_cast<int>(stations.lines().size()))return;
  setAction(Action::None);profiles_.finish();
  auto state=profiles_.state();auto copied=*clipboard_;
  auto bounds=[](const SketchLayer& layer) {
    QPainterPath all;
    for(const auto& curve:layer.curves){std::vector<QPointF> points;for(auto id:curve.points)points.push_back(layer.points[id]);all.addPath(SketchEditor::fittedPath(points,curve.type));}
    return all.boundingRect();
  };
  auto occupied=view_.scene()->sceneRect();
  for(const auto& layer:state.layers)occupied=occupied.united(bounds(layer));
  for(const auto& layer:source_.layers())occupied=occupied.united(bounds(layer));
  const auto size=bounds(copied);const double gap=std::max(20.,size.width()*.15);
  const QPointF delta{occupied.right()+gap-size.left(),0};
  for(auto& point:copied.points)point+=delta;
  int slot=static_cast<int>(state.layers.size());
  // Replacing one station must never edit another station's shared source slot.
  if(auto existing=stations.lines()[selected].profile) {
    const auto uses=std::count_if(stations.lines().begin(),stations.lines().end(),[&](const auto& line){return line.profile==existing;});
    if(uses==1)slot=static_cast<int>(*existing);
  }
  if(slot==static_cast<int>(state.layers.size()))state.layers.push_back(copied);else state.layers[slot]=copied;
  state.active=slot;state.selected=-1;state.tool=SketchTool::None;state.pending.clear();state.editing=true;
  profiles_.restoreState(state);stations.assignSelectedProfile(slot);selectStation();
  const auto target=bounds(copied).adjusted(-gap,-gap,gap,gap);
  const auto expanded=occupied.united(target);view_.scene()->setSceneRect(expanded);view_.setSceneRect(expanded);
  view_.centerOn(target.center());emit profiles_.changed();syncActions();
}
void FuselageProfilePanel::dragProfile(QPointF point) {
  if(dragged_<0)return;
  auto state=profiles_.state();if(dragged_>=static_cast<int>(state.layers.size()))return;
  auto moved=dragPoints_;for(auto& p:moved)p+=point-dragStart_;
  if(moved==state.layers[dragged_].points)return;
  state.layers[dragged_].points=std::move(moved);profiles_.restoreState(state);
  view_.viewport()->setCursor(Qt::ClosedHandCursor);emit profiles_.changed();
}
void FuselageProfilePanel::includeProfileBounds() {
  auto bounds=view_.scene()->sceneRect();
  for(const auto& layer:profiles_.layers())for(const auto& curve:layer.curves) {
    std::vector<QPointF> points;for(auto id:curve.points)points.push_back(layer.points[id]);
    bounds=bounds.united(SketchEditor::fittedPath(points,curve.type).boundingRect().adjusted(-20,-20,20,20));
  }
  view_.scene()->setSceneRect(bounds);view_.setSceneRect(bounds);
}
bool FuselageProfilePanel::allProfilesClosed() const {
  const auto& lines=source_.stationEditor().lines();if(lines.empty())return false;
  return std::all_of(lines.begin(),lines.end(),[this](const auto& line){return line.profile && *line.profile<profiles_.layers().size() && closedSketchBoundary(profiles_.layers()[*line.profile]).has_value();});
}
bool FuselageProfilePanel::eventFilter(QObject* watched,QEvent* event) {
  if(active_&&selectedOrphan_>=0&&event->type()==QEvent::KeyPress) {
    const int key=static_cast<QKeyEvent*>(event)->key();
    if(key==Qt::Key_Delete){remove_->click();return true;}
    if(key==Qt::Key_Escape){selectStation();return true;}
  }
  if(active_&&action_!=Action::None) {
    if(event->type()==QEvent::KeyPress&&static_cast<QKeyEvent*>(event)->key()==Qt::Key_Escape){setAction(Action::None);selectStation();return true;}
    if(watched==view_.viewport()) {
      if(event->type()==QEvent::MouseButtonPress) {
        auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
        const auto point=view_.mapToScene(mouse->position().toPoint());const int hit=profileAt(point);
        if(hit>=0) {
          if(action_==Action::Copy){clipboard_=profiles_.layers()[hit];setAction(Action::None);profiles_.setHighlightedLayer(hit);selection_->setText("Profile copied. Select a destination station, then Paste Profile.");}
          else {dragged_=hit;dragStart_=point;dragPoints_=profiles_.layers()[hit].points;profiles_.setHighlightedLayer(hit);view_.viewport()->setCursor(Qt::ClosedHandCursor);}
          return true;
        }
        if(action_==Action::Copy)return true;
      }
      if(event->type()==QEvent::MouseMove&&dragged_>=0) {
        auto* mouse=static_cast<QMouseEvent*>(event);
        if(mouse->buttons()&Qt::LeftButton)dragProfile(view_.mapToScene(mouse->position().toPoint()));
        return true;
      }
      if(event->type()==QEvent::MouseButtonRelease&&dragged_>=0) {
        auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
        dragProfile(view_.mapToScene(mouse->position().toPoint()));dragged_=-1;dragPoints_.clear();includeProfileBounds();view_.viewport()->setCursor(Qt::OpenHandCursor);return true;
      }
    }
  }
  if(!active_ || watched!=view_.viewport() || (event->type()!=QEvent::MouseButtonPress&&event->type()!=QEvent::MouseButtonDblClick) || profiles_.tool()!=SketchTool::None)return false;
  auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
  const auto point=view_.mapToScene(mouse->position().toPoint());
  for(const auto& line:source_.stationEditor().lines()) {
    const auto a=line.first.position,d=line.second.position-a;const double length=QPointF::dotProduct(d,d);
    if(length>0 && QLineF{point,a+d*std::clamp(QPointF::dotProduct(point-a,d)/length,0.,1.)}.length()<=8/view_.transform().m11())
      return source_.stationEditor().event(event,true);
  }
  // Single clicks select orphaned sketches without changing the document.
  // Double-click explicitly recovers one for an unassigned station.
  auto& stations=source_.stationEditor();const int selected=stations.selectedLine();
  const int hit=profileAt(point);
  if(hit>=0&&std::none_of(stations.lines().begin(),stations.lines().end(),
      [hit](const auto& station){return station.profile==static_cast<std::size_t>(hit);})) {
    if(event->type()==QEvent::MouseButtonDblClick&&selected>=0&&selected<static_cast<int>(stations.lines().size())&&!stations.lines()[selected].profile) {
      stations.assignSelectedProfile(static_cast<std::size_t>(hit));
      selectStation();emit profiles_.changed();return true;
    }
    selectedOrphan_=hit;profiles_.setEditing(false);profiles_.setActiveLayer(hit);
    selection_->setText("Unattached profile selected. Press Delete or Delete Profile to remove it. Double-click to assign it to an unassigned selected station.");
    syncActions();view_.setFocus();return true;
  }
  if(selectedOrphan_>=0)selectStation();
  return false;
}
}

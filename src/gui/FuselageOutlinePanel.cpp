#include "gui/FuselageOutlinePanel.h"
#include "gui/SketchBoundary.h"
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSignalBlocker>
namespace designrc::gui {
namespace {
QString outlineProblem(const SketchLayer& layer) {
  if(layer.curves.empty())return "no outline has been drawn";
  std::vector<int> degree(layer.points.size());
  std::vector<std::vector<std::size_t>> neighbors(layer.points.size());
  for(const auto& curve:layer.curves) {
    if(curve.points.size()<2)return "contains an incomplete curve";
    for(std::size_t j=1;j<curve.points.size();++j) {
      const auto a=curve.points[j-1],b=curve.points[j];
      if(a>=degree.size()||b>=degree.size())return "contains an invalid point reference";
      ++degree[a];++degree[b];neighbors[a].push_back(b);neighbors[b].push_back(a);
    }
  }
  int ends=0,branches=0,unused=0,chains=0;std::vector<bool> visited(degree.size());
  for(std::size_t i=0;i<degree.size();++i) {
    ends+=degree[i]==1;branches+=degree[i]>2;unused+=degree[i]==0;
    if(!degree[i]||visited[i])continue;
    ++chains;std::vector<std::size_t> pending{i};visited[i]=true;
    while(!pending.empty()) {
      const auto point=pending.back();pending.pop_back();
      for(auto next:neighbors[point])if(!visited[next]){visited[next]=true;pending.push_back(next);}
    }
  }
  QStringList problems;
  if(ends)problems.append(QString{"%1 unconnected endpoints (shown in red)"}.arg(ends));
  if(branches)problems.append(QString{"%1 branching junctions; each outline point must have two connections"}.arg(branches));
  if(chains>1)problems.append(ends==0&&branches==0
      ?QString{"%1 separate closed loops; this view requires exactly one"}.arg(chains)
      :QString{"%1 disconnected chains"}.arg(chains));
  if(unused)problems.append(QString{"%1 unused points"}.arg(unused));
  return problems.isEmpty()?QString{"the curve cannot form a boundary with nonzero area"}:problems.join("; ");
}
}
FuselageOutlinePanel::FuselageOutlinePanel(SketchEditor& editor, QWidget* parent)
    : QWidget{parent}, editor_{editor} {
  setObjectName("fuselageOutlinePanel");
  editor_.setClosedLoopMode(true);
  editor_.setLayerCount(2);
  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0,0,0,0);
  auto* description = new QLabel{
      "Trace one closed fuselage outline in Top View and one in Side View over the reference drawing. "
      "Click Top View or Side View to switch directly between the outlines; click the selected view again to release it. "
      "Choose Line for two-point segments or Spline for fitted curves. Escape finishes a spline; "
      "click its first point to close it. Tools stay on until clicked again. With both tools off, "
      "drag points to move them, or select a curve and press Delete. Nearby points snap together. "
      "Red endpoints still need a connection. Both outlines are checked when leaving Outline. "
      "Two valid loops enable Profile Stations. Choose Open or Closed independently for the nose and tail. "
      "Open removes the end wall when thickened and follows a straight slanted Side View end edge; "
      "Closed retains a foam end wall. The traced outlines must remain closed loops.", this};
  description->setWordWrap(true); layout->addWidget(description);
  for (int i=0;i<2;++i) {
    views_[i]=new QPushButton{i==0?"Top View":"Side View",this};
    views_[i]->setObjectName(i==0?"fuselageTopView":"fuselageSideView");
    views_[i]->setCheckable(true);layout->addWidget(views_[i]);
    tools_[i]=new QWidget{this};auto* row=new QHBoxLayout{tools_[i]};row->setContentsMargins(0,0,0,0);
    for (auto tool:{SketchTool::Line,SketchTool::Spline}) {
      auto* button=new QPushButton{tool==SketchTool::Line?"Line":"Spline",tools_[i]};
      button->setCheckable(true);button->setProperty("sketchTool",static_cast<int>(tool));row->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,tool](bool checked){
        editor_.setTool(checked?tool:SketchTool::None);syncControls();
      });
    }
    layout->addWidget(tools_[i]);
    connect(views_[i],&QPushButton::clicked,this,[this,i](bool checked){
      editor_.setEditing(false);editor_.setTool(SketchTool::None);
      view_=checked?i:-1;
      if(view_>=0) {editor_.setActiveLayer(view_);if(drawingRequested)drawingRequested();}
      editor_.setEditing(active_&&view_>=0);syncControls();
    });
  }
  for(int i=0;i<2;++i) {
    auto* group=new QButtonGroup{this};
    const QString end=i==0?"Nose":"Tail";
    openEnds_[i]=new QRadioButton{"Fuselage "+end+" Open",this};
    closedEnds_[i]=new QRadioButton{"Fuselage "+end+" Closed",this};
    openEnds_[i]->setObjectName("fuselage"+end+"Open");
    closedEnds_[i]->setObjectName("fuselage"+end+"Closed");
    group->addButton(openEnds_[i]);group->addButton(closedEnds_[i]);
    (i==0?openEnds_[i]:closedEnds_[i])->setChecked(true);
    layout->addWidget(openEnds_[i]);layout->addWidget(closedEnds_[i]);
    connect(openEnds_[i],&QRadioButton::toggled,this,[this]{if(endsChanged)endsChanged();});
  }
  layout->addStretch();syncControls();
}
void FuselageOutlinePanel::syncControls() {
  for(int i=0;i<2;++i) {
    QSignalBlocker block{views_[i]};views_[i]->setChecked(view_==i);
    views_[i]->setEnabled(true);tools_[i]->setVisible(view_==i);
    for(auto* button:tools_[i]->findChildren<QPushButton*>()) {
      QSignalBlocker guard{button};button->setChecked(button->property("sketchTool").toInt()==static_cast<int>(editor_.tool()));
    }
  }
}
QStringList FuselageOutlinePanel::invalidViews() const {
  QStringList invalid;
  for(int i=0;i<2;++i)
    if(i>=static_cast<int>(editor_.layers().size()) || !closedSketchBoundary(editor_.layers()[i]))
      invalid.append(QString{i==0?"Top View: ":"Side View: "}+
          (i>=static_cast<int>(editor_.layers().size())?QString{"no outline has been drawn"}:outlineProblem(editor_.layers()[i])));
  return invalid;
}
bool FuselageOutlinePanel::outlinesDefined() const {return invalidViews().empty();}
void FuselageOutlinePanel::setActive(bool active,bool warn) {
  editor_.setShowOpenEndpoints(active);
  if(active_==active && editor_.state().editing==(active&&view_>=0))return;
  const bool leaving=active_&&!active;
  active_=active; // Set before finish emits callbacks.
  editor_.setEditing(active&&view_>=0);
  syncControls();
  if(leaving&&warn) {
    const auto invalid=invalidViews();
    if(!invalid.empty())QMessageBox::warning(this,"Fuselage outlines",
        invalid.join("\n\n") +
        "\n\nEach view needs one closed loop. Return to Outline to correct these issues. Your sketches have been retained.");
  }
}
void FuselageOutlinePanel::restoreControls(int view) {view_=view;syncControls();}
bool FuselageOutlinePanel::noseOpen() const {return openEnds_[0]->isChecked();}
bool FuselageOutlinePanel::tailOpen() const {return openEnds_[1]->isChecked();}
void FuselageOutlinePanel::setEnds(bool nose,bool tail) {
  for(int i=0;i<2;++i) {
    QSignalBlocker block{openEnds_[i]};
    ( (i==0?nose:tail)?openEnds_[i]:closedEnds_[i])->setChecked(true);
  }
}
void FuselageOutlinePanel::reset() {
  setEnds(true,false);
  editor_.setShowOpenEndpoints(false);
  active_=false;view_=-1;editor_.reset();editor_.setLayerCount(2);syncControls();
}
}

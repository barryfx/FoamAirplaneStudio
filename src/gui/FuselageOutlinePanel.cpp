#include "gui/FuselageOutlinePanel.h"
#include "gui/SketchBoundary.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSignalBlocker>
namespace designrc::gui {
FuselageOutlinePanel::FuselageOutlinePanel(SketchEditor& editor, QWidget* parent)
    : QWidget{parent}, editor_{editor} {
  setObjectName("fuselageOutlinePanel");
  editor_.setClosedLoopMode(true);
  editor_.setLayerCount(2);
  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0,0,0,0);
  auto* description = new QLabel{
      "Trace one closed fuselage outline in Top View and one in Side View over the reference drawing. "
      "Click a view to activate it; click it again to release it and enable the other view. "
      "Choose Line for two-point segments or Spline for fitted curves. Escape finishes a spline; "
      "click its first point to close it. Tools stay on until clicked again. With both tools off, "
      "drag points to move them, or select a curve and press Delete. Nearby points snap together. "
      "Both outlines are checked when leaving Outline. Two valid loops enable Profile Stations.", this};
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
  layout->addStretch();syncControls();
}
void FuselageOutlinePanel::syncControls() {
  for(int i=0;i<2;++i) {
    QSignalBlocker block{views_[i]};views_[i]->setChecked(view_==i);
    views_[i]->setEnabled(view_<0||view_==i);tools_[i]->setVisible(view_==i);
    for(auto* button:tools_[i]->findChildren<QPushButton*>()) {
      QSignalBlocker guard{button};button->setChecked(button->property("sketchTool").toInt()==static_cast<int>(editor_.tool()));
    }
  }
}
QStringList FuselageOutlinePanel::invalidViews() const {
  QStringList invalid;
  for(int i=0;i<2;++i)
    if(i>=static_cast<int>(editor_.layers().size()) || !closedSketchBoundary(editor_.layers()[i]))
      invalid.append(i==0?"Top View":"Side View");
  return invalid;
}
bool FuselageOutlinePanel::outlinesDefined() const {return invalidViews().empty();}
void FuselageOutlinePanel::setActive(bool active,bool warn) {
  if(active_==active && editor_.state().editing==(active&&view_>=0))return;
  const bool leaving=active_&&!active;
  active_=active; // Set before finish emits callbacks.
  editor_.setEditing(active&&view_>=0);
  syncControls();
  if(leaving&&warn) {
    const auto invalid=invalidViews();
    if(!invalid.empty())QMessageBox::warning(this,"Fuselage outlines",
        "These outlines do not form a single closed loop with nonzero area: " + invalid.join(", ") +
        ". Return to Outline to complete them. Your sketches have been retained.");
  }
}
void FuselageOutlinePanel::restoreControls(int view) {view_=view;syncControls();}
void FuselageOutlinePanel::reset() {
  active_=false;view_=-1;editor_.reset();editor_.setLayerCount(2);syncControls();
}
}

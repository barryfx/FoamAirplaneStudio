#include "gui/StabilizerCutPanel.h"
#include "gui/SketchBoundary.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>
#include <QSignalBlocker>
namespace designrc::gui {
StabilizerCutPanel::StabilizerCutPanel(SketchEditor& editor,bool horizontal,QWidget* parent)
 : QWidget{parent},editor_{editor} {
  setObjectName(horizontal?"horizontalStabilizerCutPanel":"verticalStabilizerCutPanel");
  editor_.setClosedLoopMode(true);editor_.setSnapAcrossLayers(false);editor_.setLayerSelectionMode(true);
  auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* text=new QLabel{"Add a Cut Shape, then draw one closed loop with Line and/or Spline over the stabilizer outline. "
    "Join each segment to the previous endpoint and close the loop at its first point. Escape finishes a spline. "
    "Material inside the loop is removed through the full thickness of all intersecting stabilizer and control-surface bodies; a body may separate into several pieces. "
    "Horizontal cuts are mirrored to the other half. With drawing off, click a shape or select it from the list; drag its points to edit. "
    "Delete Cut Shape removes the entire loop. With drawing tools off, click a curve to highlight it and press Delete to remove only that curve. Unclosed loops are retained for editing and warn when leaving Cut.",this};
  text->setWordWrap(true);layout->addWidget(text);
  auto* add=new QPushButton{"Add Cut Shape",this};layout->addWidget(add);
  shapes_=new QComboBox{this};shapes_->setObjectName("stabilizerCutShapes");layout->addWidget(shapes_);
  connect(shapes_,&QComboBox::currentIndexChanged,this,[this](int index){if(index>=0)editor_.setActiveLayer(index);});
  connect(add,&QPushButton::clicked,this,[this]{
    editor_.finish();
    const auto& layers=editor_.layers();
    if(layers.size()!=1 || !layers[0].curves.empty() || !layers[0].points.empty())editor_.setLayerCount(static_cast<int>(layers.size())+1);
    editor_.setActiveLayer(static_cast<int>(editor_.layers().size())-1);editor_.setTool(SketchTool::Line);restoreControls();
  });
  auto* row=new QHBoxLayout;layout->addLayout(row);
  for(auto tool:{SketchTool::Line,SketchTool::Spline}) {
    auto* button=new QPushButton{tool==SketchTool::Line?"Line":"Spline",this};button->setCheckable(true);
    button->setProperty("sketchTool",static_cast<int>(tool));row->addWidget(button);
    connect(button,&QPushButton::clicked,this,[this,tool](bool checked){editor_.setTool(checked?tool:SketchTool::None);restoreControls();});
  }
  auto* remove=new QPushButton{"Delete Cut Shape",this};layout->addWidget(remove);
  connect(remove,&QPushButton::clicked,this,[this]{editor_.deleteActiveLayer();});
  connect(&editor_,&SketchEditor::changed,this,[this]{restoreControls();});layout->addStretch();restoreControls();
}
void StabilizerCutPanel::restoreControls() {
  QSignalBlocker block{shapes_};shapes_->clear();
  for(std::size_t i=0;i<editor_.layers().size();++i)shapes_->addItem(QString{"Cut Shape %1"}.arg(i+1));
  shapes_->setCurrentIndex(editor_.activeLayer());
  for(auto* b:findChildren<QPushButton*>())if(b->property("sketchTool").isValid()) {
    QSignalBlocker guard{b};b->setChecked(b->property("sketchTool").toInt()==static_cast<int>(editor_.tool()));
  }
}
void StabilizerCutPanel::setActive(bool active,bool warn) {
  const bool leaving=active_&&!active;const bool pending=!editor_.state().pending.empty();active_=active;editor_.setEditing(active);restoreControls();
  bool incomplete=pending;
  for(const auto& layer:editor_.layers())if(!layer.curves.empty()&&!closedSketchBoundary(layer))incomplete=true;
  if(leaving&&warn&&incomplete) {
    QMessageBox::warning(this,"Unclosed Cut Shape","Every Cut Shape must be one closed loop before model generation. Return to Cut to close or delete the incomplete shape. Your drawing has been retained.");
  }
}
}

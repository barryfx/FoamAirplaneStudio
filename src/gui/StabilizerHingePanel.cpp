#include "gui/StabilizerHingePanel.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>
#include <QSignalBlocker>
namespace designrc::gui {
StabilizerHingePanel::StabilizerHingePanel(SketchEditor& editor,bool horizontal,QWidget* parent)
 : QWidget{parent},editor_{editor} {
  setObjectName(horizontal?"horizontalStabilizerHingePanel":"verticalStabilizerHingePanel");
  editor_.setContinuousLineMode(true);
  auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* text=new QLabel{QString{"Draw connected straight line segments across the %1 outline to separate the %2. "
    "Start and end on or beyond the outline boundary (the open root edge also counts). "
    "Click Draw Hinge Line, then click each corner. Escape finishes; with drawing off, drag points or select a segment and press Delete. "
    "The longest segment defines the hinge and receives the bevel on the trailing-edge side only. "
    "Tape Hinge Cut leaves upper-surface contact with a 45-degree gap below. Standard Hinge Cut leaves mid-thickness contact with 45-degree gaps above and below."}
    .arg(horizontal?"horizontal stabilizer":"vertical stabilizer",horizontal?"elevator":"rudder"),this};
  text->setWordWrap(true);layout->addWidget(text);
  tape_=new QRadioButton{"Tape Hinge Cut",this};standard_=new QRadioButton{"Standard Hinge Cut",this};
  auto* group=new QButtonGroup{this};group->setExclusive(true);group->addButton(tape_);group->addButton(standard_);
  layout->addWidget(tape_);layout->addWidget(standard_);tape_->setChecked(true);
  for(auto* radio:{tape_,standard_})connect(radio,&QRadioButton::toggled,this,[this,radio](bool checked){
    if(!checked)return;cut_=radio==tape_?HingeCut::Tape:HingeCut::Standard;if(changed)changed();
  });
  auto* draw=new QPushButton{"Draw Hinge Line",this};draw->setCheckable(true);layout->addWidget(draw);
  connect(draw,&QPushButton::clicked,this,[this,draw](bool checked){editor_.setTool(checked?SketchTool::Line:SketchTool::None);draw->setChecked(editor_.tool()==SketchTool::Line);});
  connect(&editor_,&SketchEditor::changed,this,[this,draw]{QSignalBlocker block{draw};draw->setChecked(editor_.tool()==SketchTool::Line);});
  connect(&editor_,&SketchEditor::sessionFinished,this,[this,draw]{editor_.setTool(SketchTool::None);draw->setChecked(false);});
  editor_.setEscapeEndsSession(true);layout->addStretch();
}
void StabilizerHingePanel::setActive(bool active) {editor_.setEditing(active);}
void StabilizerHingePanel::restore(HingeCut cut) {
  cut_=cut;QSignalBlocker a{tape_},b{standard_};tape_->setChecked(cut==HingeCut::Tape);standard_->setChecked(cut==HingeCut::Standard);
  for(auto* draw:findChildren<QPushButton*>())draw->setChecked(editor_.tool()==SketchTool::Line);
}
}

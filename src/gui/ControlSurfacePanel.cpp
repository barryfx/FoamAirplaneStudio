#include "gui/ControlSurfacePanel.h"
#include "gui/ControlSurfaceEditor.h"
#include <QCheckBox>
#include <QTabBar>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSignalBlocker>
namespace designrc::gui {
ControlSurfacePanel::ControlSurfacePanel(ControlSurfaceEditor& editor,QWidget* parent):QWidget{parent},editor_{editor} {
  setObjectName("controlSurfacePanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  tabs_=new QTabBar{this};tabs_->setObjectName("controlPanelTabs");layout->addWidget(tabs_);
  connect(tabs_,&QTabBar::currentChanged,this,[this](int panel){if(panel>=0)editor_.setPanel(panel);});
  for(int i=0;i<2;++i) {
    const QString name=i==0?"Ailerons":"Flaps";
    checks_[i]=new QCheckBox{"Add "+name,this};checks_[i]->setObjectName("add"+name);layout->addWidget(checks_[i]);
    details_[i]=new QWidget{this};auto* fields=new QVBoxLayout{details_[i]};fields->setContentsMargins(0,0,0,12);
    auto* instruction=new QLabel{"Draw a rectangle to define the "+name+" over the wing outline to define the "+(i==0?QString{"aileron"}:QString{"flap"})+" cut.  It should hang over the outside of the wing. Aileron and flap rectangles must not overlap.  Click two opposite corners or drag to draw. Press Escape or click the lit Draw Rectangle button to cancel drawing. When not drawing, click a rectangle to highlight it; Escape clears the highlight and Delete removes the highlighted rectangle.",details_[i]};
    instruction->setWordWrap(true);fields->addWidget(instruction);
    auto* group=new QButtonGroup{details_[i]};group->setExclusive(true);
    for(int h=0;h<2;++h) {
      auto* radio=new QRadioButton{h==0?"Tape Hinge Cut":"Standard Hinge Cut",details_[i]};
      radio->setObjectName(name+(h==0?"TapeHinge":"StandardHinge"));hinges_[i][h]=radio;
      group->addButton(radio,h);fields->addWidget(radio);
    }
    connect(group,&QButtonGroup::idClicked,this,[this,i](int h){editor_.setHinge(i,static_cast<HingeCut>(h));});
    auto* draw=new QPushButton{"Draw Rectangle",details_[i]};draw->setObjectName("draw"+name);fields->addWidget(draw);
    draw_[i]=draw;draw->setCheckable(true);
    draw->setStyleSheet("QPushButton:checked { border: 2px solid #159bd7; background: #d7efff; color: #102a43; }");
    connect(draw,&QPushButton::clicked,this,[this,i](bool checked){if(checked)editor_.begin(i);else editor_.cancel();});
    layout->addWidget(details_[i]);
    connect(checks_[i],&QCheckBox::toggled,this,[this,i](bool enabled){details_[i]->setVisible(enabled);editor_.setEnabled(i,enabled);});
  }
  connect(&editor_,&ControlSurfaceEditor::drawingStateChanged,this,&ControlSurfacePanel::restoreControls);
  auto* overlap=new QLabel{this};overlap->setObjectName("controlOverlapWarning");
  overlap->setWordWrap(true);overlap->setStyleSheet("color: #d34836; font-weight: bold;");layout->addWidget(overlap);
  connect(&editor_,&ControlSurfaceEditor::overlapRejected,this,[overlap]{
    overlap->setText("Aileron and flap rectangles must not overlap. Draw a new rectangle; the previous rectangle has been kept.");
  });
  connect(&editor_,&ControlSurfaceEditor::drawingStateChanged,this,[this,overlap]{
    if(editor_.state().drawing>=0)return;
    overlap->setText(controlSurfacesOverlap(editor_.state().panels)
      ? "Aileron and flap rectangles overlap. Redraw or delete one before generating the wing." : "");
  });
  if(controlSurfacesOverlap(editor_.state().panels))overlap->setText("Aileron and flap rectangles overlap. Redraw or delete one before generating the wing.");
  layout->addStretch();restoreControls();
}
void ControlSurfacePanel::updateDrawingButtons() {
  for(int i=0;i<2;++i)draw_[i]->setChecked(editor_.state().drawing==i);
}
void ControlSurfacePanel::restoreControls() {
  QSignalBlocker tabsBlock{tabs_};const int count=static_cast<int>(editor_.state().panels.size());
  while(tabs_->count()>count)tabs_->removeTab(tabs_->count()-1);
  while(tabs_->count()<count)tabs_->addTab(QString::number(tabs_->count()+1));
  tabs_->setCurrentIndex(editor_.panel());
  updateDrawingButtons();
  for(int i=0;i<2;++i) {
    QSignalBlocker block{checks_[i]};const auto& surface=editor_.state().panels[editor_.panel()][i];
    checks_[i]->setChecked(surface.enabled);details_[i]->setVisible(surface.enabled);
    hinges_[i][static_cast<int>(surface.hinge)]->setChecked(true);
  }
}
}

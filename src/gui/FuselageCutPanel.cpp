#include "gui/FuselageCutPanel.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
namespace designrc::gui {
FuselageCutPanel::FuselageCutPanel(SketchEditor& editor,QWidget* parent):QWidget{parent},editor_{editor} {
  setObjectName("fuselageCutPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* text=new QLabel{"Choose Top View or Side View, then draw a connected cut path over that outline with Line and Spline. Click two endpoints for each Line; start the next segment at the previous endpoint to join it. Escape finishes a Spline; click its first point to close it if wanted. A closed loop is optional. Turn Line/Spline off to move points or select a segment and press Delete.\n\nOpen paths must reach or cross the fuselage boundary at both ends to separate a hatch or section. Top View cuts pass through the full height; Side View cuts pass through the full width. Open 3D View to regenerate separate bodies. Cuts have no kerf or clearance, keep every resulting body, remain visible across 2D modes, and are saved with the project.",this};
  text->setObjectName("fuselageCutInstructions");text->setWordWrap(true);layout->addWidget(text);
  auto* views=new QHBoxLayout;layout->addLayout(views);
  for(int i=0;i<2;++i) {
    views_[i]=new QPushButton{i==0?"Top View":"Side View",this};views_[i]->setCheckable(true);
    views_[i]->setObjectName(i==0?"fuselageCutTop":"fuselageCutSide");views->addWidget(views_[i]);
    connect(views_[i],&QPushButton::clicked,this,[this,i]{editor_.setActiveLayer(i);sync();});
  }
  auto* tools=new QHBoxLayout;layout->addLayout(tools);
  for(int i=0;i<2;++i) {
    tools_[i]=new QPushButton{i==0?"Line":"Spline",this};tools_[i]->setCheckable(true);
    tools_[i]->setObjectName(i==0?"fuselageCutLine":"fuselageCutSpline");tools->addWidget(tools_[i]);
    connect(tools_[i],&QPushButton::clicked,this,[this,i](bool checked){editor_.setTool(checked?(i==0?SketchTool::Line:SketchTool::Spline):SketchTool::None);sync();});
  }
  layout->addStretch();sync();
}
void FuselageCutPanel::sync() {
  for(int i=0;i<2;++i) {
    QSignalBlocker a{views_[i]},b{tools_[i]};views_[i]->setChecked(editor_.activeLayer()==i);
    tools_[i]->setChecked(editor_.tool()==(i==0?SketchTool::Line:SketchTool::Spline));
  }
}
void FuselageCutPanel::setActive(bool active) {
  if(active_!=active){active_=active;editor_.setEditing(active);}sync();
}
void FuselageCutPanel::restoreControls(){sync();}
}

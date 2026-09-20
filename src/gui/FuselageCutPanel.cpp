#include "gui/FuselageCutPanel.h"
#include "gui/SketchPaths.h"
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
namespace designrc::gui {
FuselageCutPanel::FuselageCutPanel(SketchEditor& editor,QWidget* parent,bool holes,SketchEditor* outlines)
 :QWidget{parent},editor_{editor},outlines_{outlines},holes_{holes} {
  const QString prefix=holes?"fuselageHoles":"fuselageCut";setObjectName(prefix+"Panel");
  auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* text=new QLabel{holes?
    "Choose Top, Bottom, Left or Right wall, then Add Hole and draw a closed loop with Line and/or Spline. Top/Bottom use the Top View outline; Left/Right use the Side View outline. Every hole must fit entirely inside that outline. Join segments at their endpoints; click the first point to close the loop. Escape finishes a spline. Select a hole from the list or click its boundary to edit; Delete Hole removes the selected hole. Incomplete loops warn when leaving Holes.\n\nGeneration removes material through the chosen wall only, stopping at the inner cavity. A hole that cannot reach the cavity without opening another wall must be moved or resized. Holes are saved with the project.":
    "Choose Top View or Side View, then Add Cut and draw a connected path with Line and/or Spline. Join segments at their endpoints; Escape finishes a spline. A closed loop is optional. Select a cut from the list or click its path to edit; Delete Cut removes the selected path. Add Cut finishes the current path and starts another.\n\nOpen paths must cross the outline at both ends. Top View cuts pass through the full height; Side View cuts pass through the full width. Generation splits the fuselage, retaining all resulting bodies without kerf. Cuts remain visible and are saved with the project.",this};
  text->setObjectName(prefix+"Instructions");text->setWordWrap(true);layout->addWidget(text);
  auto* views=new QHBoxLayout;layout->addLayout(views);
  const QStringList labels=holes?QStringList{"Top","Bottom","Left","Right"}:QStringList{"Top View","Side View"};
  for(int i=0;i<labels.size();++i) {
    auto* b=new QPushButton{labels[i],this};b->setCheckable(true);views_.push_back(b);views->addWidget(b);
    b->setObjectName(prefix+(holes?labels[i]:(i==0?"Top":"Side")));
    connect(b,&QPushButton::clicked,this,[this,i]{editor_.setActiveLayer(i);selectedPath_=-1;sync();});
  }
  auto* add=new QPushButton{holes?"Add Hole":"Add Cut",this};add->setObjectName(prefix+"Add");layout->addWidget(add);
  paths_=new QComboBox{this};paths_->setObjectName(prefix+"Paths");paths_->setPlaceholderText(holes?"Select Hole":"Select Cut");layout->addWidget(paths_);
  connect(paths_,&QComboBox::currentIndexChanged,this,[this](int index){
    if(syncing_)return;selectedPath_=index;editor_.finish();auto state=editor_.state();const auto paths=sketchPaths(state.layers[state.active]);
    state.selected=index>=0&&index<static_cast<int>(paths.size())&&!paths[index].curves.empty()?static_cast<int>(paths[index].curves.front()):-1;
    state.tool=SketchTool::None;editor_.restoreState(state);sync();
  });
  connect(add,&QPushButton::clicked,this,[this]{editor_.finish();selectedPath_=-1;editor_.setTool(SketchTool::Line);sync();});
  auto* tools=new QHBoxLayout;layout->addLayout(tools);
  for(int i=0;i<2;++i) {
    auto* b=new QPushButton{i==0?"Line":"Spline",this};b->setCheckable(true);tools_.push_back(b);tools->addWidget(b);b->setObjectName(prefix+(i==0?"Line":"Spline"));
    connect(b,&QPushButton::clicked,this,[this,i](bool checked){editor_.setTool(checked?(i==0?SketchTool::Line:SketchTool::Spline):SketchTool::None);sync();});
  }
  remove_=new QPushButton{holes?"Delete Hole":"Delete Cut",this};remove_->setObjectName(prefix+"Delete");layout->addWidget(remove_);
  connect(remove_,&QPushButton::clicked,this,[this]{
    auto state=editor_.state();const auto paths=sketchPaths(state.layers[state.active]);
    if(selectedPath_>=0&&selectedPath_<static_cast<int>(paths.size()))eraseSketchPath(state.layers[state.active],paths[selectedPath_]);
    state.pending.clear();state.selected=-1;state.tool=SketchTool::None;selectedPath_=-1;editor_.restoreState(state);emit editor_.changed();
  });
  connect(&editor_,&SketchEditor::changed,this,[this]{
    if(syncing_)return;
    if(active_&&holes_&&outlines_) {
      const auto state=editor_.state();const auto& outline=outlines_->layers().at(state.active<2?0:1);
      if(closedSketchBoundary(outline)&&!sketchPathInside(state.layers[state.active],outline)) {
        editor_.restoreState(accepted_);sync();QMessageBox::warning(this,"Hole outside fuselage","Keep the entire hole inside the selected fuselage outline.");return;
      }
      accepted_=state;
    }
    sync();
  });layout->addStretch();accepted_=editor_.state();sync();
}
void FuselageCutPanel::sync() {
  if(syncing_)return;syncing_=true;
  for(std::size_t i=0;i<views_.size();++i){QSignalBlocker b{views_[i]};views_[i]->setChecked(editor_.activeLayer()==static_cast<int>(i));}
  for(int i=0;i<2;++i){QSignalBlocker b{tools_[i]};tools_[i]->setChecked(editor_.tool()==(i==0?SketchTool::Line:SketchTool::Spline));}
  const auto paths=sketchPaths(editor_.layers()[editor_.activeLayer()]);
  for(std::size_t i=0;i<paths.size();++i)if(std::find(paths[i].curves.begin(),paths[i].curves.end(),editor_.selectedCurve())!=paths[i].curves.end())selectedPath_=static_cast<int>(i);
  if(selectedPath_>=static_cast<int>(paths.size()))selectedPath_=-1;
  QSignalBlocker b{paths_};paths_->clear();for(std::size_t i=0;i<paths.size();++i)paths_->addItem(QString{holes_?"Hole %1":"Cut %1"}.arg(i+1));paths_->setCurrentIndex(selectedPath_);
  remove_->setEnabled(selectedPath_>=0||!editor_.state().pending.empty());syncing_=false;
}
void FuselageCutPanel::setActive(bool active,bool warn) {
  const bool leaving=active_&&!active;const bool pending=!editor_.state().pending.empty();if(active_!=active){active_=active;editor_.setEditing(active);accepted_=editor_.state();}sync();
  bool incomplete=pending;
  if(holes_)for(const auto& layer:editor_.layers())for(const auto& path:sketchPaths(layer))if(!closedSketchBoundary(path.layer))incomplete=true;
  if(leaving&&holes_&&warn&&incomplete)QMessageBox::warning(this,"Unclosed Hole","Close or delete every incomplete hole before generating the fuselage. Your drawing has been retained.");
}
void FuselageCutPanel::restoreControls(){accepted_=editor_.state();selectedPath_=-1;sync();}
}

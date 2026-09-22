#include "gui/ExportPanel.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>
namespace designrc::gui {
ExportPanel::ExportPanel(QWidget* parent):QWidget{parent} {
  setObjectName("exportPanel");
  auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* text=new QLabel{"Choose a format for formers and for the other Assembly components. "
      "Check the parts to export, or use All to select or clear every part. "
      "Export Components asks for a folder and remembers it for next time. "
      "STEP writes selected formers and components together in a STEP file named after the project; STL and DXF write one file per checked part. "
      "Exports use the current Assembly, including its cuts. Former DXF uses a full-size mid-plane section, flattened in millimeters.",this};
  text->setWordWrap(true);text->setObjectName("exportInstructions");layout->addWidget(text);
  const auto radios=[&](const char* first,const char* second,const char* id1,const char* id2) {
    auto* group=new QButtonGroup{this};
    auto* a=new QRadioButton{first,this};auto* b=new QRadioButton{second,this};
    a->setObjectName(id1);b->setObjectName(id2);group->addButton(a);group->addButton(b);
    a->setChecked(true);layout->addWidget(a);layout->addWidget(b);return a;
  };
  dxf_=radios("Formers DXF","Formers STL","formersDxf","formersStl");
  formerStep_=new QRadioButton{"Formers STEP",this};formerStep_->setObjectName("formersStep");
  dxf_->group()->addButton(formerStep_);layout->addWidget(formerStep_);
  formerStep_->setChecked(true);
  layout->addSpacing(8);
  step_=radios("Components STEP","Components STL","componentsStep","componentsStl");
  all_=new QCheckBox{"All",this};all_->setObjectName("exportAll");layout->addWidget(all_);
  auto* scroll=new QScrollArea{this};scroll->setWidgetResizable(true);
  auto* content=new QWidget{scroll};list_=new QVBoxLayout{content};list_->setAlignment(Qt::AlignTop);
  scroll->setWidget(content);layout->addWidget(scroll,1);
  export_=new QPushButton{"Export Components",this};export_->setObjectName("exportComponents");
  layout->addWidget(export_);export_->setEnabled(false);
  connect(all_,&QCheckBox::clicked,this,[this](bool checked){
    for(auto* box:checks_){QSignalBlocker block{box};box->setChecked(checked);}updateSelection();
  });
  connect(export_,&QPushButton::clicked,this,[this]{if(exportRequested)exportRequested();});
}
void ExportPanel::setParts(std::vector<geometry::ExportPart> parts) {
  for(auto* box:checks_)delete box;
  checks_.clear();parts_=std::move(parts);
  for(const auto& part:parts_) {
    auto* box=new QCheckBox{QString::fromStdString(part.name),this};
    box->setObjectName("exportPart"+QString::number(checks_.size()));list_->addWidget(box);checks_.push_back(box);
    connect(box,&QCheckBox::toggled,this,[this]{updateSelection();});
  }
  updateSelection();
}
void ExportPanel::updateSelection() {
  std::size_t selected=0;for(auto* box:checks_)if(box->isChecked())++selected;
  QSignalBlocker block{all_};all_->setChecked(!checks_.empty()&&selected==checks_.size());
  all_->setEnabled(!checks_.empty());export_->setEnabled(selected>0);
}
std::vector<geometry::ExportPart> ExportPanel::selectedParts() const {
  std::vector<geometry::ExportPart> selected;
  for(std::size_t i=0;i<checks_.size();++i)if(checks_[i]->isChecked())selected.push_back(parts_[i]);
  return selected;
}
geometry::FormerExportFormat ExportPanel::formerFormat() const {
  if(formerStep_->isChecked())return geometry::FormerExportFormat::Step;
  return dxf_->isChecked()?geometry::FormerExportFormat::Dxf:geometry::FormerExportFormat::Stl;
}
geometry::ComponentExportFormat ExportPanel::componentFormat() const {
  return step_->isChecked()?geometry::ComponentExportFormat::Step:geometry::ComponentExportFormat::Stl;
}
}

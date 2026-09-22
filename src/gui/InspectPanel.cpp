#include "gui/InspectPanel.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>
namespace designrc::gui {
InspectPanel::InspectPanel(QWidget* parent):QWidget{parent} {
  setObjectName("inspectPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* text=new QLabel{"Check components to show them in 3D. Edit a name and press Enter or leave the field to save it. "
      "Names are used in Export and individual filenames. Visibility affects only Inspect. "
      "Changed components regenerate when you enter Inspect. Complete their definitions to include new models.",this};
  text->setWordWrap(true);layout->addWidget(text);
  auto* scroll=new QScrollArea{this};scroll->setWidgetResizable(true);
  auto* content=new QWidget{scroll};list_=new QVBoxLayout{content};list_->setAlignment(Qt::AlignTop);
  scroll->setWidget(content);layout->addWidget(scroll,1);
  message_=new QLabel{this};message_->setObjectName("inspectMessage");message_->setWordWrap(true);layout->addWidget(message_);
}
void InspectPanel::restore(ComponentNames names,bool preserveVisibility) {names_=std::move(names);if(!preserveVisibility)hidden_.clear();setParts({});}
void InspectPanel::setParts(std::vector<geometry::ExportPart> parts) {
  while(auto* item=list_->takeAt(0)){delete item->widget();delete item;}
  parts_=std::move(parts);message_->setText(parts_.empty()?"No current generated components are available.":QString{});
  for(std::size_t i=0;i<parts_.size();++i) {
    const auto id=QString::fromStdString(parts_[i].id);
    auto* row=new QWidget{this};auto* line=new QHBoxLayout{row};line->setContentsMargins(0,0,0,0);
    auto* check=new QCheckBox{row};check->setObjectName("inspectVisible"+QString::number(i));check->setChecked(!hidden_.contains(id));
    auto* edit=new QLineEdit{names_.value(id,QString::fromStdString(parts_[i].name)),row};
    edit->setObjectName("inspectName"+QString::number(i));edit->setMaxLength(120);edit->setProperty("componentId",id);
    check->setAccessibleName("Show "+edit->text());edit->setAccessibleName("Component name");
    line->addWidget(check);line->addWidget(edit,1);list_->addWidget(row);
    connect(check,&QCheckBox::toggled,this,[this,id](bool checked){if(checked)hidden_.remove(id);else hidden_.insert(id);if(visibilityChanged)visibilityChanged();});
    connect(edit,&QLineEdit::editingFinished,this,[this,i,id,edit,check]{
      const auto original=QString::fromStdString(parts_[i].name);const auto previous=names_.value(id,original);
      const auto name=edit->text().trimmed();bool valid=validComponentName(name);
      for(std::size_t j=0;j<parts_.size();++j)if(j!=i)
        if(names_.value(QString::fromStdString(parts_[j].id),QString::fromStdString(parts_[j].name)).compare(name,Qt::CaseInsensitive)==0)valid=false;
      if(!valid){edit->setText(previous);message_->setText("Use a unique name valid as a filename (no slashes, reserved names, or trailing dots).");return;}
      edit->setText(name);check->setAccessibleName("Show "+name);message_->clear();
      if(name==previous)return;
      if(name==original)names_.remove(id);else names_[id]=name;
      if(namesChanged)namesChanged();
    });
  }
}
std::vector<TopoDS_Shape> InspectPanel::visibleShapes() const {
  std::vector<TopoDS_Shape> shapes;
  for(const auto& part:parts_)if(!hidden_.contains(QString::fromStdString(part.id)))shapes.push_back(part.shape);
  return shapes;
}
}

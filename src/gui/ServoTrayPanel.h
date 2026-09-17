#pragma once
#include "gui/ServoTrayEditor.h"
#include "gui/LengthEntry.h"
#include <QLineEdit>
#include <QFormLayout>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
namespace designrc::gui {
class ServoTrayPanel final : public QWidget {
public:
  ServoTrayPanel(ServoTrayEditor& editor,QWidget* parent):QWidget{parent},editor_{editor} {
    setObjectName("servoTrayPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
    auto* text=new QLabel{"Enter Width (fore/aft length) and Height (tray thickness). Fields use Reference units; mm or in may be entered explicitly. Editing a dimension creates or resizes the rectangle about its center. Drag the rectangle on Side View to place it; its dimensions stay fixed while moving. Use the fields to resize it. Delete removes the tray. Movement and resizing cannot overlap a former.\n\nThe default tray thickness is valid without editing. Regeneration automatically uses the station wall defaults when Thicken has not been opened; existing wall values are preserved. Zero thickness is invalid. Open 3D View to fit a separate tray to the inside surfaces, without passing through the fuselage. Two support ledges join the fuselage sides below it, extending 5 mm inward and 5 mm down from the tray underside. Cuts also split these supports, while the tray remains a separate part. Its top-face outline is retained for future DXF/SVG laser export. The rectangle remains visible in other 2D modes and is saved with the project.",this};
    text->setObjectName("servoTrayInstructions");text->setWordWrap(true);layout->addWidget(text);
    auto* fields=new QFormLayout;layout->addLayout(fields);
    width_=new QLineEdit{this};height_=new QLineEdit{this};
    width_->setObjectName("servoTrayWidth");height_->setObjectName("servoTrayHeight");
    fields->addRow("Width",width_);fields->addRow("Height",height_);
    for(auto* field:{width_,height_})connect(field,&QLineEdit::editingFinished,this,[this,field]{
      if(!field->isModified())return;
      const auto w=lengthInMm(width_->text(),units_),h=lengthInMm(height_->text(),units_);
      if(w&&h){widthMm_=*w;heightMm_=*h;editor_.setDimensions({*w/scale_,*h/scale_},center_);}
      else emit editor_.message("Tray width and thickness must be greater than zero.");
      field->setModified(false);sync();
    });
    create_=new QPushButton{"Place Rectangle",this};create_->setObjectName("servoTrayPlace");layout->addWidget(create_);
    connect(create_,&QPushButton::clicked,this,[this]{editor_.setDimensions({widthMm_/scale_,heightMm_/scale_},center_);});
    remove_=new QPushButton{"Delete Tray",this};remove_->setObjectName("servoTrayDelete");layout->addWidget(remove_);layout->addStretch();
    auto* message=new QLabel{this};message->setWordWrap(true);layout->insertWidget(layout->count()-1,message);
    connect(&editor_,&ServoTrayEditor::message,message,&QLabel::setText);
    connect(remove_,&QPushButton::clicked,&editor_,&ServoTrayEditor::remove);
    connect(&editor_,&ServoTrayEditor::controlsChanged,this,[this]{sync();});sync();
  }
  void setActive(bool active){editor_.setEditing(active);sync();}
  void configure(ProjectUnits units,double scale,QPointF center){units_=units;scale_=scale;center_=center;sync();}
private:
  void sync(){
    if(const auto r=editor_.state().rectangle){widthMm_=r->width()*scale_;heightMm_=r->height()*scale_;}
    if(!width_->hasFocus()||!width_->isModified())width_->setText(formattedLength(widthMm_,units_));
    if(!height_->hasFocus()||!height_->isModified())height_->setText(formattedLength(heightMm_,units_));
    remove_->setEnabled(editor_.state().rectangle.has_value());
    create_->setEnabled(!editor_.state().rectangle);
  }
  ServoTrayEditor& editor_;QPushButton* create_{};QPushButton* remove_{};
  QLineEdit* width_{};QLineEdit* height_{};
  ProjectUnits units_=ProjectUnits::Millimeters;double scale_=1,widthMm_=100,heightMm_=3;QPointF center_;
};
}

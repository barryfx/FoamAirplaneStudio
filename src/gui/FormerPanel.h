#pragma once
#include "gui/FormerEditor.h"
#include "gui/LengthEntry.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSignalBlocker>
namespace designrc::gui {
class FormerPanel final : public QWidget {
public:
  FormerPanel(FormerEditor& editor,QWidget* parent):QWidget{parent},editor_{editor}{
    setObjectName("formerPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
    auto* text=new QLabel{"Enter Width (former thickness) in Reference units, or add mm or in. Add Former places a vertical rectangle above and below the Side View outline at an available position. Drag inside it to slide it horizontally or vertically. Move it up/down for a partial-height former, or drag its top/bottom handle to change height. A rectangle may cross just the top or bottom outline edge: the former follows the inner fuselage contour and stops at the opposite rectangle edge. Crossing both edges makes a full-height former. Click a former to select it; the Width field then changes that former. Rotation Angle applies to the selected former about its center in Side View; negative degrees rotate counter-clockwise. New formers start at 0 degrees. Click empty space to set the thickness for new formers. Delete removes the selected former.\n\nFormers cannot overlap each other or the servo tray; blocked edits leave the last valid placement. Touching edges are allowed. The default 3 mm thickness works without editing. Regeneration automatically initializes missing wall defaults and preserves existing values, even if Thicken has not been opened. Zero thickness is invalid. Each former is fitted to the inner cavity as a separate solid, trimmed clear of the walls and tray supports. Each former has retaining rails immediately ahead and behind it on both inner sides: 4 mm fore/aft wide and 3 mm inward deep, extending the available side height and clearing other inserts. The main fuselage splits into left/right halves at the centre plane; hatch and other cut-out pieces stay whole. Two top and two bottom alignment pins locate the halves near the ends: 3 mm projection, 3.5 mm socket depth, diameter 4 mm or the local wall thickness if smaller. Rectangles remain visible in other 2D tabs and are saved with the project.",this};text->setObjectName("formerInstructions");text->setWordWrap(true);layout->addWidget(text);
    auto* fields=new QFormLayout;layout->addLayout(fields);width_=new QLineEdit{this};width_->setObjectName("formerWidth");fields->addRow("Width (thickness)",width_);
    connect(width_,&QLineEdit::editingFinished,this,[this]{if(!width_->isModified())return;const auto mm=lengthInMm(width_->text(),units_);if(mm)editor_.setThickness(*mm);else emit editor_.message("Former thickness must be greater than zero.");width_->setModified(false);sync();});
    auto* add=new QPushButton{"Add Former",this};add->setObjectName("formerAdd");layout->addWidget(add);connect(add,&QPushButton::clicked,&editor_,&FormerEditor::add);
    auto* rotationFields=new QFormLayout;layout->addLayout(rotationFields);
    rotation_=new QDoubleSpinBox{this};rotation_->setObjectName("formerRotationAngle");rotation_->setRange(-360,360);rotation_->setDecimals(3);rotation_->setSuffix(" degrees");rotation_->setKeyboardTracking(false);
    rotationFields->addRow("Rotation Angle",rotation_);
    connect(rotation_,&QDoubleSpinBox::valueChanged,this,[this](double value){editor_.setRotation(value);sync();});
    remove_=new QPushButton{"Delete Former",this};remove_->setObjectName("formerDelete");layout->addWidget(remove_);connect(remove_,&QPushButton::clicked,&editor_,&FormerEditor::remove);
    message_=new QLabel{this};message_->setWordWrap(true);message_->setObjectName("formerMessage");layout->addWidget(message_);layout->addStretch();
    connect(&editor_,&FormerEditor::message,message_,&QLabel::setText);connect(&editor_,&FormerEditor::controlsChanged,this,[this]{sync();});sync();
  }
  void configure(ProjectUnits units,double scale,QRectF side){units_=units;scale_=scale;editor_.configure(scale,side);sync();}
  void setActive(bool active){editor_.setEditing(active);}
private:
  void sync(){const int selected=editor_.selected();const double mm=selected>=0?editor_.state().rectangles[selected].width()*scale_:editor_.state().thicknessMm;
    if(!width_->hasFocus()||!width_->isModified())width_->setText(formattedLength(mm,units_));remove_->setEnabled(selected>=0);QSignalBlocker block{rotation_};rotation_->setEnabled(selected>=0);rotation_->setValue(selected>=0?formerAngle(editor_.state().rotationDegrees,selected):0);}
  FormerEditor& editor_;QLineEdit* width_{};QDoubleSpinBox* rotation_{};QPushButton* remove_{};QLabel* message_{};ProjectUnits units_=ProjectUnits::Millimeters;double scale_=1;
};
}

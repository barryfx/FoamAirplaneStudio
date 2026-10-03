#include "gui/StiffenerPanel.h"
#include "gui/LengthEntry.h"
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <algorithm>
namespace designrc::gui {
StiffenerPanel::StiffenerPanel(QWidget* parent):QWidget{parent} {
  setObjectName("stiffenerPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* description=new QLabel{"Add carbon fiber stiffeners along the fuselage sides. The count is 0–3 per side; grooves are mirrored. Zero disables grooves. "
    "Start and Stop are percentages of fuselage length measured from the nose. "
    "One stiffener follows mid-height; multiple stiffeners are evenly spaced vertically. "
    "Strip Width is vertical and Height is inward groove depth. Round grooves use the rod diameter. "
    "Dimensions default to mm, independent of project units. Enter in explicitly to display a dimension in inches. Open 3D View to regenerate. Carbon weight is included in Weight and Balance.",this};
  description->setWordWrap(true);layout->addWidget(description);form_=new QFormLayout;layout->addLayout(form_);
  count_=new QSpinBox{this};count_->setObjectName("stiffenerCount");count_->setRange(0,3);form_->addRow("Number of Stiffeners",count_);
  shape_=new QComboBox{this};shape_->setObjectName("stiffenerShape");shape_->addItem("Strip",static_cast<int>(SparShape::Strip));shape_->addItem("Round",static_cast<int>(SparShape::Round));form_->addRow("Shape",shape_);
  const auto percent=[&](const char* label,const char* name){auto* field=new QDoubleSpinBox{this};field->setObjectName(name);field->setRange(0,100);field->setDecimals(2);field->setSuffix(" %");field->setKeyboardTracking(false);form_->addRow(label,field);return field;};
  start_=percent("Start Position (% from nose)","stiffenerStart");stop_=percent("Stop Position (% from nose)","stiffenerStop");
  const auto length=[&](const char* label,const char* name,double StiffenerState::*member,ProjectUnits* displayUnits) {
    auto* field=new QLineEdit{this};field->setObjectName(name);form_->addRow(label,field);
    connect(field,&QLineEdit::editingFinished,this,[this,field,member,displayUnits]{if(updating_||!field->isModified())return;auto next=state_;const auto value=lengthInMm(field->text(),ProjectUnits::Millimeters);if(value)next.*member=*value;
      try{validateStiffeners(next);if(value){state_=next;const bool inches=field->text().contains(QRegularExpression{R"((in(?:ch(?:es)?)?|")\s*$)",QRegularExpression::CaseInsensitiveOption});*displayUnits=inches?ProjectUnits::Inches:ProjectUnits::Millimeters;if(changed)changed();}}catch(const std::exception&){}refresh();});return field;
  };
  width_=length("Width","stiffenerWidth",&StiffenerState::widthMm,&widthUnits_);height_=length("Height / depth","stiffenerHeight",&StiffenerState::heightMm,&heightUnits_);
  diameter_=length("Diameter","stiffenerDiameter",&StiffenerState::diameterMm,&diameterUnits_);
  const auto edit=[this]{if(updating_)return;auto next=state_;next.count=count_->value();next.shape=static_cast<SparShape>(shape_->currentData().toInt());next.startPercent=start_->value();next.stopPercent=stop_->value();
    try{validateStiffeners(next);state_=next;if(changed)changed();}catch(const std::exception&){}refresh();};
  connect(count_,&QSpinBox::valueChanged,this,edit);connect(shape_,&QComboBox::currentIndexChanged,this,edit);
  connect(start_,&QDoubleSpinBox::valueChanged,this,edit);connect(stop_,&QDoubleSpinBox::valueChanged,this,edit);layout->addStretch();refresh();
}
void StiffenerPanel::restore(const StiffenerState& state){state_=state;state_.count=std::clamp(state_.count,0,3);widthUnits_=heightUnits_=diameterUnits_=ProjectUnits::Millimeters;refresh();}
void StiffenerPanel::setUnits(ProjectUnits){/* Stock dimensions use per-field explicit units. */}
void StiffenerPanel::refresh(){updating_=true;count_->setValue(state_.count);shape_->setCurrentIndex(shape_->findData(static_cast<int>(state_.shape)));start_->setValue(state_.startPercent);stop_->setValue(state_.stopPercent);
  width_->setText(formattedLength(state_.widthMm,widthUnits_));height_->setText(formattedLength(state_.heightMm,heightUnits_));diameter_->setText(formattedLength(state_.diameterMm,diameterUnits_));
  const bool strip=state_.shape==SparShape::Strip;form_->setRowVisible(width_,strip);form_->setRowVisible(height_,strip);form_->setRowVisible(diameter_,!strip);updating_=false;}
}

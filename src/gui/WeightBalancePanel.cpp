#include "gui/LengthEntry.h"
#include "gui/WeightBalancePanel.h"
#include "gui/PlanViewport.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <cmath>
#include <algorithm>

namespace designrc::gui {
WeightBalancePanel::WeightBalancePanel(PlanViewport& view,QWidget* parent):QWidget{parent},view_{view} {
  setObjectName("weightBalancePanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* description=new QLabel{ "Add RC components and drag their labeled rectangles over the fuselage Side View. "
      "Width goes into the screen, height is vertical, and length runs nose to tail. Each part's mass acts at its rectangle center. "
      "Parts are visible only here.\n\nGenerate and position Assembly for the foam weight and balance. "
      "Foam includes the wing, fuselage, stabilizers and controls, with current Assembly cuts. "
      "Formers and servo tray use Aero Plywood density and are included automatically. "
      "Enabled wing spars use Carbon Fiber density and are included automatically. Fiberglass sketches add cloth and resin estimates. Add other unmodeled items as parts. "
      "The datum is the placed wing root leading edge; positive is toward the tail.",this};
  description->setWordWrap(true);layout->addWidget(description);
  auto* add=new QPushButton{"Add Part",this};add->setObjectName("balanceAddPart");layout->addWidget(add);
  parts_=new QComboBox{this};parts_->setObjectName("balanceParts");layout->addWidget(parts_);
  auto* buttons=new QHBoxLayout;edit_=new QPushButton{"Edit",this};delete_=new QPushButton{"Delete",this};
  edit_->setObjectName("balanceEditPart");delete_->setObjectName("balanceDeletePart");buttons->addWidget(edit_);buttons->addWidget(delete_);layout->addLayout(buttons);
  auto* materials=new QPushButton{"Material Densities…",this};materials->setObjectName("balanceMaterials");layout->addWidget(materials);
  auto* dialog=new QDialog{this};dialog->setObjectName("balanceMaterialsDialog");dialog->setWindowTitle("Material Densities");
  auto* dialogLayout=new QVBoxLayout{dialog};auto* form=new QFormLayout;dialogLayout->addLayout(form);
  density_=new QDoubleSpinBox{dialog};density_->setObjectName("balanceDensity");
  density_->setDecimals(3);density_->setRange(.001,10000);density_->setValue(state_.densityKgM3);density_->setSuffix(" kg/m³");
  density_->setToolTip("XPS starting value: 25.63 kg/m³. Enter the density of your actual foam.");
  form->addRow("Foam density",density_);
  plywoodDensity_=new QDoubleSpinBox{this};plywoodDensity_->setObjectName("balancePlywoodDensity");
  plywoodDensity_->setDecimals(3);plywoodDensity_->setRange(.001,10000);plywoodDensity_->setValue(state_.plywoodDensityKgM3);plywoodDensity_->setSuffix(" kg/m³");
  plywoodDensity_->setToolTip("Birch Aero Plywood starting value: 680 kg/m³. Adjust for your stock.");
  form->addRow("Aero Plywood density",plywoodDensity_);
  carbonFiberDensity_=new QDoubleSpinBox{this};carbonFiberDensity_->setObjectName("balanceCarbonFiberDensity");
  carbonFiberDensity_->setDecimals(3);carbonFiberDensity_->setRange(.001,10000);carbonFiberDensity_->setValue(state_.carbonFiberDensityKgM3);carbonFiberDensity_->setSuffix(" kg/m³");
  carbonFiberDensity_->setToolTip("Carbon/epoxy composite starting value: 1540 kg/m³. Adjust for your spar stock; tube bores are excluded from material volume.");
  form->addRow("Carbon Fiber density",carbonFiberDensity_);
  resinDensity_=new QDoubleSpinBox{dialog};resinDensity_->setObjectName("balanceResinDensity");resinDensity_->setDecimals(3);resinDensity_->setRange(.001,10000);resinDensity_->setSuffix(" kg/m³");
  resinDensity_->setToolTip("1500 kg/m³ (1.5 g/cm³) default. Adjust to your foam-compatible resin's cured density.");form->addRow("Resin density",resinDensity_);
  auto* dialogButtons=new QDialogButtonBox{QDialogButtonBox::Ok|QDialogButtonBox::Cancel,dialog};dialogLayout->addWidget(dialogButtons);
  connect(dialogButtons,&QDialogButtonBox::accepted,dialog,&QDialog::accept);connect(dialogButtons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
  connect(materials,&QPushButton::clicked,this,[this,dialog]{
    density_->setValue(state_.densityKgM3);plywoodDensity_->setValue(state_.plywoodDensityKgM3);carbonFiberDensity_->setValue(state_.carbonFiberDensityKgM3);resinDensity_->setValue(state_.resinDensityKgM3);
    if(dialog->exec()!=QDialog::Accepted)return;
    state_.densityKgM3=density_->value();state_.plywoodDensityKgM3=plywoodDensity_->value();state_.carbonFiberDensityKgM3=carbonFiberDensity_->value();state_.resinDensityKgM3=resinDensity_->value();updateResults();if(changed)changed();
  });
  layout->addWidget(new QLabel{"Component volume and weight",this});
  breakdown_=new QTableWidget{0,4,this};breakdown_->setObjectName("balanceBreakdown");
  breakdown_->setHorizontalHeaderLabels({"Component / material","cm³","g","oz"});
  breakdown_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  breakdown_->setSelectionBehavior(QAbstractItemView::SelectRows);breakdown_->verticalHeader()->hide();
  breakdown_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  breakdown_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
  breakdown_->setMinimumHeight(180);layout->addWidget(breakdown_,1);
  auto* volumeHint=new QLabel{"Volumes are solid material, excluding cavities. 1,000 cm³ = 1 liter. Added parts use entered weights; their box dimensions are not material volumes.",this};
  volumeHint->setWordWrap(true);layout->addWidget(volumeHint);
  results_=new QLabel{this};results_->setObjectName("balanceResults");results_->setWordWrap(true);results_->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(results_);
  connect(add,&QPushButton::clicked,this,[this]{editPart(true);});
  connect(edit_,&QPushButton::clicked,this,[this]{editPart(false);});
  connect(delete_,&QPushButton::clicked,this,[this]{deletePart();});
  connect(parts_,&QComboBox::currentIndexChanged,this,[this](int i){dragging_=false;edit_->setEnabled(i>=0);delete_->setEnabled(i>=0);view_.viewport()->update();});
  view_.viewport()->installEventFilter(this);view_.installEventFilter(this);refreshList(-1);
}
void WeightBalancePanel::restore(const WeightBalanceState& state) {
  state_=state;dragging_=false;QSignalBlocker block{density_},plywoodBlock{plywoodDensity_},carbonBlock{carbonFiberDensity_};carbonFiberDensity_->setValue(state_.carbonFiberDensityKgM3);density_->setValue(state_.densityKgM3);plywoodDensity_->setValue(state_.plywoodDensityKgM3);refreshList(-1);updateResults();
}
void WeightBalancePanel::configure(ProjectUnits units,geometry::FuselageSideTransform transform,QPointF initialCenter) {
  units_=units;transform_=transform;initialCenter_=initialCenter;updateResults();view_.viewport()->update();
}
void WeightBalancePanel::setActive(bool active) {active_=active;dragging_=false;setVisible(active);view_.viewport()->update();}
void WeightBalancePanel::setFoam(std::optional<FoamMassProperties> foam,std::optional<double> leadingEdge,QString message) {
  foam_=foam;leadingEdge_=leadingEdge;unavailable_=std::move(message);updateResults();
}
void WeightBalancePanel::setWingArea(std::optional<double> mm2) { if(wingAreaMm2_!=mm2){wingAreaMm2_=mm2;updateResults();} }
void WeightBalancePanel::setCgHeightLine(std::optional<QLineF> line) {cgHeightLine_=line;view_.viewport()->update();}
std::optional<QPointF> WeightBalancePanel::cgScenePosition() const {
  if(!foam_||!leadingEdge_||!cgHeightLine_||transform_.scale<=0||std::abs(cgHeightLine_->dx())<1e-8)return {};
  const auto mass=calculateBalance(state_,*foam_);if(mass.grams<=0)return {};
  // The marker's horizontal position is the same physical CG used in the
  // numerical LE-relative result. Its height is a visual wing datum, not CG Z.
  const double x=mass.centerMm.x();
  const double z=cgHeightLine_->y1()+(x-cgHeightLine_->x1())*cgHeightLine_->dy()/cgHeightLine_->dx();
  return QPointF{transform_.left+x/transform_.scale,transform_.verticalOrigin-z/transform_.scale};
}
int WeightBalancePanel::selected() const {return parts_->currentIndex();}
QPointF WeightBalancePanel::sceneToModel(QPointF point) const {
  return {(point.x()-transform_.left)*transform_.scale,(transform_.verticalOrigin-point.y())*transform_.scale};
}
QRectF WeightBalancePanel::partRectangle(int index) const {
  const auto& part=state_.parts.at(index);
  const QPointF center{transform_.left+part.centerMm.x()/transform_.scale,transform_.verticalOrigin-part.centerMm.y()/transform_.scale};
  const QSizeF size{part.lengthMm/transform_.scale,part.heightMm/transform_.scale};
  return {center-QPointF{size.width()/2,size.height()/2},size};
}
void WeightBalancePanel::refreshList(int index) {
  QSignalBlocker block{parts_};parts_->clear();for(const auto& part:state_.parts)parts_->addItem(part.name);
  parts_->setCurrentIndex(index);edit_->setEnabled(index>=0);delete_->setEnabled(index>=0);view_.viewport()->update();
}
void WeightBalancePanel::editPart(bool add) {
  const int index=selected();if(!add&&index<0)return;
  if(add&&state_.parts.size()>=1000)return;
  BalancePart part=add?BalancePart{}:state_.parts[index];if(add)part.centerMm=initialCenter_;
  QDialog dialog{this};dialog.setObjectName("balancePartDialog");dialog.setWindowTitle(add?"Add Part":"Edit Part");
  auto* layout=new QVBoxLayout{&dialog};auto* form=new QFormLayout;
  auto* name=new QLineEdit{part.name,&dialog};name->setObjectName("balancePartName");name->setMaxLength(200);form->addRow("Name",name);
  auto dimension=[&](const char* label,const char* object,double value,LengthUnit unit){
    auto* field=new QLineEdit{formattedLength(value,displayLengthUnits(unit,units_)),&dialog};field->setObjectName(object);
    form->addRow(label,field);return field;
  };
  auto* width=dimension("Width (into screen)","balancePartWidth",part.widthMm,part.widthUnit);
  auto* height=dimension("Height (vertical)","balancePartHeight",part.heightMm,part.heightUnit);
  auto* length=dimension("Length (nose to tail)","balancePartLength",part.lengthMm,part.lengthUnit);
  auto* weight=new QDoubleSpinBox{&dialog};weight->setObjectName("balancePartWeight");weight->setDecimals(6);
  auto* massUnit=new QComboBox{&dialog};massUnit->setObjectName("balancePartMassUnit");massUnit->addItems({"grams","ounces"});massUnit->setCurrentIndex(part.ounces?1:0);
  double massScale=part.ounces?gramsPerOunce:1.;weight->setRange(.001/massScale,1e6/massScale);weight->setValue(part.grams/massScale);
  auto* massRow=new QHBoxLayout;massRow->addWidget(weight);massRow->addWidget(massUnit);form->addRow("Weight",massRow);
  connect(massUnit,&QComboBox::currentIndexChanged,&dialog,[&](int i){const double grams=weight->value()*massScale;massScale=i?gramsPerOunce:1.;weight->setRange(.001/massScale,1e6/massScale);weight->setValue(grams/massScale);});
  layout->addLayout(form);
  auto* error=new QLabel{&dialog};error->setWordWrap(true);error->setStyleSheet("color: #b00020");layout->addWidget(error);
  auto* buttons=new QDialogButtonBox{QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog};layout->addWidget(buttons);
  connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
  connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
    const auto text=name->text().trimmed();
    if(text.isEmpty()){error->setText("Enter a part name.");return;}
    for(int i=0;i<static_cast<int>(state_.parts.size());++i)if((add||i!=index)&&state_.parts[i].name.compare(text,Qt::CaseInsensitive)==0) {
      error->setText("Use a different name for each part.");return;
    }
    const auto w=lengthInMm(width->text(),units_),h=lengthInMm(height->text(),units_),l=lengthInMm(length->text(),units_);
    if(!w||!h||!l||*w<.001||*h<.001||*l<.001||*w>10000||*h>10000||*l>10000){error->setText("Enter dimensions from 0.001 to 10000 mm; mm or in suffixes are accepted.");return;}
    part.name=text;part.widthMm=*w;part.heightMm=*h;part.lengthMm=*l;
    part.widthUnit=enteredLengthUnit(width->text(),units_);part.heightUnit=enteredLengthUnit(height->text(),units_);part.lengthUnit=enteredLengthUnit(length->text(),units_);
    part.grams=std::clamp(weight->value()*massScale,.001,1e6);part.ounces=massUnit->currentIndex()==1;dialog.accept();
  });
  if(dialog.exec()!=QDialog::Accepted)return;
  if(add)state_.parts.push_back(part);else state_.parts[index]=part;
  refreshList(add?static_cast<int>(state_.parts.size())-1:index);updateResults();if(changed)changed();
}
void WeightBalancePanel::deletePart() {
  const int index=selected();if(index<0)return;dragging_=false;
  state_.parts.erase(state_.parts.begin()+index);refreshList(-1);updateResults();if(changed)changed();
}
void WeightBalancePanel::updateResults() {
  breakdown_->setRowCount(0);
  const auto row=[&](QString name,std::optional<double> volume,double grams) {
    const int i=breakdown_->rowCount();breakdown_->insertRow(i);
    const QString values[]{name,volume?QString::number(*volume/1000.,'f',2):QString{"—"},QString::number(grams,'f',2),QString::number(grams/gramsPerOunce,'f',3)};
    for(int column=0;column<4;++column) {
      auto* item=new QTableWidgetItem{values[column]};item->setToolTip(values[column]);
      if(column)item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
      breakdown_->setItem(i,column,item);
    }
  };
  if(foam_) {
    for(const auto& component:foam_->components)
      row(component.name+(component.carbonFiber?" / Carbon Fiber":component.plywood?" / Plywood":" / Foam")+(component.volumeMm3==0?" (not present)":""),
          component.volumeMm3,component.volumeMm3*(component.carbonFiber?state_.carbonFiberDensityKgM3:component.plywood?state_.plywoodDensityKgM3:state_.densityKgM3)*1e-6);
    row("Foam subtotal",foam_->volumeMm3,foam_->volumeMm3*state_.densityKgM3*1e-6);
    row("Plywood subtotal",foam_->plywoodVolumeMm3,foam_->plywoodVolumeMm3*state_.plywoodDensityKgM3*1e-6);
    row("Carbon Fiber subtotal",foam_->carbonFiberVolumeMm3,foam_->carbonFiberVolumeMm3*state_.carbonFiberDensityKgM3*1e-6);
    double cloth=0,resin=0;
    for(const auto& patch:foam_->fiberglass) {
      const double resinGrams=patch.resinVolumeMm3*state_.resinDensityKgM3*1e-6;
      row(patch.name+QString{" / Fiberglass (%1 cm²)"}.arg(patch.areaMm2/100,0,'f',2),{},patch.clothGrams);
      row(patch.name+QString{" / Resin (%1 cm²)"}.arg(patch.areaMm2/100,0,'f',2),patch.resinVolumeMm3,resinGrams);cloth+=patch.clothGrams;resin+=resinGrams;
    }
    if(!foam_->fiberglass.empty()){row("Fiberglass cloth subtotal",{},cloth);row("Resin subtotal",{},resin);}
  }
  for(const auto& part:state_.parts)row(part.name+" / Added part",{},part.grams);
  breakdown_->resizeRowsToContents();
  const auto result=calculateBalance(state_,foam_.value_or(FoamMassProperties{}));
  QString text;
  if(!foam_)text=QString{"Parts weight: %1 g (%2 oz)\nTotal weight: unavailable\nCenter of Gravity: unavailable\n%3"}.arg(result.grams,0,'f',2).arg(result.grams/gramsPerOunce,0,'f',2).arg(unavailable_);
  else {
    text=QString{"Total weight: %1 g (%2 oz)\nFoam: %3 g"}.arg(result.grams,0,'f',2).arg(result.grams/gramsPerOunce,0,'f',2).arg(foam_->volumeMm3*state_.densityKgM3*1e-6,0,'f',2);
    text+=QString{" | Aero Plywood: %1 g"}.arg(foam_->plywoodVolumeMm3*state_.plywoodDensityKgM3*1e-6,0,'f',2);
    text+=QString{" | Carbon Fiber: %1 g"}.arg(foam_->carbonFiberVolumeMm3*state_.carbonFiberDensityKgM3*1e-6,0,'f',2);
    if(!foam_->fiberglass.empty()) {
      double cloth=0,resin=0;for(const auto& patch:foam_->fiberglass){cloth+=patch.clothGrams;resin+=patch.resinVolumeMm3*state_.resinDensityKgM3*1e-6;}
      text+=QString{"\nFiberglass: %1 g | Resin: %2 g"}.arg(cloth,0,'f',2).arg(resin,0,'f',2);
    }
    if(wingAreaMm2_&&*wingAreaMm2_>0) {
      const bool inches=units_==ProjectUnits::Inches;
      const double loading=result.grams / *wingAreaMm2_ * (inches?304.8*304.8/gramsPerOunce:10000.);
      text+=QString{"\nWing Loading: %1 %2"}.arg(loading,0,'f',2).arg(inches?"oz/ft²":"g/dm²");
    }
    if(leadingEdge_&&result.grams>0) {
      const double distance=(result.centerMm.x()-*leadingEdge_)/(units_==ProjectUnits::Inches?25.4:1.);
      text+=QString{"\nCenter of Gravity: %1%2 %3 from wing root LE"}.arg(distance>=0?"+":"").arg(distance,0,'f',3).arg(units_==ProjectUnits::Inches?"in":"mm");
    } else text+="\nCenter of Gravity: unavailable — define the wing root datum.";
    text+="\n(+ toward tail; − toward nose)";
  }
  results_->setText(text);if(resultsChanged)resultsChanged(text);view_.viewport()->update();
}
void WeightBalancePanel::paint(QPainter& painter) const {
  if(!active_)return;painter.save();
  for(int i=0;i<static_cast<int>(state_.parts.size());++i) {
    const auto rectangle=partRectangle(i);QPen pen{i==selected()?QColor{230,110,0}:QColor{0,95,155}};pen.setWidth(i==selected()?3:2);pen.setCosmetic(true);
    painter.setPen(pen);painter.setBrush(QColor{50,160,220,65});painter.drawRect(rectangle);
    // Device-size labels stay readable at any reference scale and zoom.
    const auto center=painter.worldTransform().map(rectangle.center());painter.save();painter.resetTransform();
    const QFontMetricsF metrics{painter.font()};const auto text=state_.parts[i].name;
    QRectF label=metrics.boundingRect(text).adjusted(-4,-2,4,2);label.moveCenter(center);
    painter.fillRect(label,QColor{255,255,255,220});painter.setPen(Qt::black);painter.drawText(label,Qt::AlignCenter,text);painter.restore();
  }
  if(const auto position=cgScenePosition();position&&!cgSymbol_.isNull()) {
    const auto center=painter.worldTransform().map(*position);
    painter.save();painter.resetTransform();painter.setRenderHint(QPainter::SmoothPixmapTransform);
    // Constant logical-pixel size keeps the marker small and legible at any zoom.
    painter.drawPixmap(QRectF{center-QPointF{14,14},QSizeF{28,28}},cgSymbol_,cgSymbol_.rect());
    painter.restore();
  }
  painter.restore();
}
bool WeightBalancePanel::eventFilter(QObject* watched,QEvent* event) {
  if(!active_||!isEnabled()||!view_.isEnabled())return false;
  if(event->type()==QEvent::KeyPress) {
    auto* key=static_cast<QKeyEvent*>(event);
    if(key->key()==Qt::Key_Delete){deletePart();return true;}
    if(key->key()==Qt::Key_Escape){dragging_=false;parts_->setCurrentIndex(-1);return true;}
  }
  if(watched!=view_.viewport())return false;
  if(event->type()==QEvent::MouseButtonPress) {
    auto* mouse=static_cast<QMouseEvent*>(event);if(mouse->button()!=Qt::LeftButton)return false;
    const auto position=view_.mapToScene(mouse->position().toPoint());int hit=-1;
    // Prefer the selected rectangle to make overlapping parts movable via the list.
    if(selected()>=0&&partRectangle(selected()).contains(position))hit=selected();
    else for(int i=static_cast<int>(state_.parts.size())-1;i>=0;--i)if(partRectangle(i).contains(position)){hit=i;break;}
    parts_->setCurrentIndex(hit);dragging_=hit>=0;
    if(dragging_)dragOffset_=state_.parts[hit].centerMm-sceneToModel(position);
    view_.setFocus();return true;
  }
  if(event->type()==QEvent::MouseMove&&dragging_&&selected()>=0) {
    auto* mouse=static_cast<QMouseEvent*>(event);
    const auto center=sceneToModel(view_.mapToScene(mouse->position().toPoint()))+dragOffset_;
    if(std::abs(center.x())<=1e7&&std::abs(center.y())<=1e7){state_.parts[selected()].centerMm=center;updateResults();if(changed)changed();}
    return true;
  }
  if(event->type()==QEvent::MouseButtonRelease&&static_cast<QMouseEvent*>(event)->button()==Qt::LeftButton){dragging_=false;return true;}
  return false;
}
}

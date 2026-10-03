#include "gui/FiberglassPanel.h"
#include "gui/SketchPaths.h"
#include <QApplication>
#include <QMessageBox>
#include <QPainterPathStroker>
#include <QTimer>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace designrc::gui {
FiberglassPanel::FiberglassPanel(SketchEditor& editor,int component,QWidget* parent)
    : QWidget{parent},editor_{editor},component_{component} {
  setObjectName(QString{"fiberglassPanel%1"}.arg(component));
  warningTimer_=new QTimer{this};warningTimer_->setSingleShot(true);warningTimer_->setInterval(150);
  connect(warningTimer_,&QTimer::timeout,this,[this]{warnDrawingView();});
  editor_.setClosedLoopMode(true);editor_.setSnapAcrossLayers(false);editor_.setLayerSelectionMode(true);
  auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* text=new QLabel{"Fiberglass is only for Weight and Balance; it does not change manufactured parts. "
    "Add a patch and draw connected Lines, Splines or a Circle over the component outline. "
    "The overlapping area receives cloth and resin. Open paths are allowed when their endpoints can be joined outside the outline. "
    "Wrap Sides covers both faces and edges within the boundary; One Sided covers the selected exterior surface. "
    "Wing and horizontal stabilizer patches follow the mirrored half. Overlapping patches add layers. "
    "Escape finishes a spline. Turn drawing off to select or drag points; Delete removes a selected curve. "
    "Generate Assembly to calculate the actual surface area and center of gravity.",this};
  text->setWordWrap(true);layout->addWidget(text);
  auto* add=new QPushButton{"Add Fiberglass Shape",this};add->setObjectName("fiberglassAdd");layout->addWidget(add);
  list_=new QComboBox{this};list_->setObjectName("fiberglassShapes");layout->addWidget(list_);
  auto* tools=new QHBoxLayout;layout->addLayout(tools);
  for(auto tool:{SketchTool::Line,SketchTool::Spline,SketchTool::Circle}) {
    auto* button=new QPushButton{tool==SketchTool::Line?"Line":tool==SketchTool::Spline?"Spline":"Circle",this};
    button->setCheckable(true);button->setProperty("fiberglassTool",static_cast<int>(tool));tools->addWidget(button);
    connect(button,&QPushButton::clicked,this,[this,tool](bool checked){editor_.setTool(checked?tool:SketchTool::None);refresh();});
  }
  auto* form=new QFormLayout;layout->addLayout(form);
  name_=new QLineEdit{this};name_->setObjectName("fiberglassName");name_->setMaxLength(160);form->addRow("Name",name_);
  auto* coverage=new QHBoxLayout;wrap_=new QRadioButton{"Wrap Sides",this};oneSide_=new QRadioButton{"One Sided",this};
  wrap_->setObjectName("fiberglassWrap");oneSide_->setObjectName("fiberglassOneSided");coverage->addWidget(wrap_);coverage->addWidget(oneSide_);form->addRow(coverage);
  side_=new QComboBox{this};side_->setObjectName("fiberglassSide");
  if(component!=3){side_->addItem("Top",0);side_->addItem("Bottom",1);}
  if(component==1||component==3){side_->addItem("Left",2);side_->addItem("Right",3);}
  form->addRow("Drawing surface",side_);
  side_->setToolTip("Fuselage Top/Bottom use Top View; Left/Right use Side View. Also sets the projection for Wrap Sides.");
  cloth_=new QDoubleSpinBox{this};cloth_->setObjectName("fiberglassClothWeight");cloth_->setDecimals(4);cloth_->setRange(.001,100000);
  clothUnits_=new QComboBox{this};clothUnits_->setObjectName("fiberglassClothUnits");clothUnits_->addItems({"g/m²","oz/yd²"});
  auto* clothRow=new QHBoxLayout;clothRow->addWidget(cloth_);clothRow->addWidget(clothUnits_);form->addRow("Cloth Weight",clothRow);
  automatic_=new QCheckBox{"Estimate resin thickness from cloth weight",this};automatic_->setObjectName("fiberglassAutomaticResin");form->addRow(automatic_);
  resin_=new QDoubleSpinBox{this};resin_->setObjectName("fiberglassResinThickness");resin_->setDecimals(5);resin_->setRange(.000001,100);form->addRow("Resin thickness",resin_);
  resin_->setToolTip("Equivalent resin-only thickness: cloth wet-out plus a thin coat; excludes glass volume. Uncheck estimation to enter your own value.");
  auto* remove=new QPushButton{"Delete Fiberglass Shape",this};remove->setObjectName("fiberglassDelete");layout->addWidget(remove);layout->addStretch();
  if(component==3)patches_[0].side=CoverSide::Left;
  connect(add,&QPushButton::clicked,this,[this]{
    editor_.finish();const bool reuse=patches_.size()==1&&editor_.layers()[0].curves.empty();
    if(reuse)patches_[0].imperialCloth=units_==ProjectUnits::Inches;
    if(!reuse){auto patch=FiberglassPatch{};patch.name=QString{"Patch %1"}.arg(patches_.size()+1);patch.imperialCloth=units_==ProjectUnits::Inches;if(component_==3)patch.side=CoverSide::Left;patches_.push_back(patch);editor_.setLayerCount(static_cast<int>(patches_.size()));}
    editor_.setActiveLayer(static_cast<int>(patches_.size())-1);editor_.setTool(SketchTool::Line);refresh();if(changed)changed();
  });
  connect(remove,&QPushButton::clicked,this,[this]{patches_.erase(patches_.begin()+editor_.activeLayer());if(patches_.empty()){patches_.resize(1);patches_[0].imperialCloth=units_==ProjectUnits::Inches;if(component_==3)patches_[0].side=CoverSide::Left;}editor_.deleteActiveLayer();refresh();if(changed)changed();});
  connect(list_,&QComboBox::currentIndexChanged,this,[this](int i){if(!refreshing_&&i>=0){editor_.setActiveLayer(i);refresh();}});
  connect(name_,&QLineEdit::editingFinished,this,[this]{edit();});
  connect(wrap_,&QRadioButton::toggled,this,[this]{edit();});
  connect(side_,&QComboBox::currentIndexChanged,this,[this]{edit();});
  connect(cloth_,&QDoubleSpinBox::valueChanged,this,[this]{edit();});
  connect(resin_,&QDoubleSpinBox::valueChanged,this,[this]{edit();});
  connect(automatic_,&QCheckBox::toggled,this,[this]{edit();});
  connect(clothUnits_,&QComboBox::currentIndexChanged,this,[this](int i){if(refreshing_)return;auto& patch=patches_.at(editor_.activeLayer());patch.imperialCloth=i==1;patch.projectClothUnits=(patch.imperialCloth==(units_==ProjectUnits::Inches));refresh();if(changed)changed();});
  connect(&editor_,&SketchEditor::changed,this,[this]{refresh();warningTimer_->start();if(changed)changed();});refresh();
}
FiberglassState FiberglassPanel::state() const {return {editor_.state(),patches_};}
void FiberglassPanel::restore(const FiberglassState& state) {warningTimer_->stop();lastWarning_.clear();patches_=state.patches;editor_.restoreState(state.sketch);refresh();}
void FiberglassPanel::configure(ProjectUnits units) {
  units_=units;
  refresh();
}
void FiberglassPanel::setActive(bool active){setVisible(active);editor_.setEditing(active);refresh();}
void FiberglassPanel::edit() {
  if(refreshing_)return;
  auto& patch=patches_.at(editor_.activeLayer());
  patch.name=name_->text().trimmed();if(patch.name.isEmpty())patch.name=QString{"Patch %1"}.arg(editor_.activeLayer()+1);
  patch.wrap=wrap_->isChecked();patch.side=static_cast<CoverSide>(side_->currentData().toInt());
  if(sender()==cloth_)patch.clothGm2=cloth_->value()*(imperialCloth(patch)?gramsPerSquareMeterPerOzYard:1);
  patch.automaticResin=automatic_->isChecked();
  if(!patch.automaticResin&&(sender()==resin_||sender()==automatic_))patch.resinThicknessMm=resin_->value()*(units_==ProjectUnits::Inches?25.4:1);
  refresh();warningTimer_->start();if(changed)changed();
}
void FiberglassPanel::refresh() {
  if(refreshing_)return;refreshing_=true;
  list_->clear();for(const auto& patch:patches_)list_->addItem(patch.name);list_->setCurrentIndex(editor_.activeLayer());
  const auto& patch=patches_.at(editor_.activeLayer());name_->setText(patch.name);wrap_->setChecked(patch.wrap);oneSide_->setChecked(!patch.wrap);
  side_->setCurrentIndex(side_->findData(static_cast<int>(patch.side)));
  clothUnits_->setCurrentIndex(imperialCloth(patch)?1:0);cloth_->setValue(patch.clothGm2/(imperialCloth(patch)?gramsPerSquareMeterPerOzYard:1));
  automatic_->setChecked(patch.automaticResin);resin_->setEnabled(!patch.automaticResin);resin_->setSuffix(units_==ProjectUnits::Inches?" in":" mm");
  resin_->setValue(resinThickness(patch)/(units_==ProjectUnits::Inches?25.4:1));
  for(auto* button:findChildren<QPushButton*>())if(button->property("fiberglassTool").isValid())button->setChecked(button->property("fiberglassTool").toInt()==static_cast<int>(editor_.tool()));
  refreshing_=false;
}
bool FiberglassPanel::imperialCloth(const FiberglassPatch& patch) const {
  return patch.projectClothUnits?units_==ProjectUnits::Inches:patch.imperialCloth;
}
void FiberglassPanel::warnDrawingView() {
  if(component_!=1||!isVisible()||!editor_.state().editing||!drawingViews)return;
  // Do not interrupt a point drag or show a modal warning for every mouse move.
  if(QApplication::mouseButtons()!=Qt::NoButton){warningTimer_->start();return;}
  const auto views=drawingViews();const auto& patch=patches_.at(editor_.activeLayer());
  const bool top=patch.side==CoverSide::Top||patch.side==CoverSide::Bottom;
  const auto& layer=editor_.layers().at(editor_.activeLayer());QPainterPath lines;
  for(const auto& curve:layer.curves){std::vector<QPointF> points;for(auto id:curve.points)points.push_back(layer.points[id]);lines.addPath(SketchEditor::fittedPath(points,curve.type));}
  QPainterPathStroker stroke;stroke.setWidth(1e-6);auto footprint=stroke.createStroke(lines);
  if(const auto boundary=closedSketchBoundary(layer))footprint=footprint.united(sketchPolygon(*boundary));
  const auto overlaps=[&](int view){const auto boundary=closedSketchBoundary(views[view]);return boundary&&sketchPolygon(*boundary).intersects(footprint);};
  if(!overlaps(top?1:0)||overlaps(top?0:1)){lastWarning_.clear();return;}
  const QString message=QString{"Drawing Surface is %1 (Fuselage %2 View), but the lines appear over the Fuselage %3 View."}
      .arg(side_->currentText(),top?"Top":"Side",top?"Side":"Top");
  const auto key=QString::number(editor_.activeLayer())+message;if(lastWarning_==key)return;lastWarning_=key;
  QMessageBox::warning(this,"Fiberglass drawing view",message);
}
}

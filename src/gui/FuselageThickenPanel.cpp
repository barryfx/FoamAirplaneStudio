#include "gui/FuselageThickenPanel.h"
#include "gui/LengthEntry.h"
#include "gui/FuselageStationOrder.h"
#include <QLineEdit>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
namespace designrc::gui {
FuselageThickenPanel::FuselageThickenPanel(SketchEditor& source,QWidget* parent):QWidget{parent},source_{source} {
  setObjectName("fuselageThickenPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* instructions=new QLabel{"Set the inward wall thickness at each profile station. Fields use the units selected under Reference. Enter a decimal alone for project units, or add mm or in to override them. The outside follows the Top and Side outlines; thickness transitions smoothly between stations. A complete fuselage generates with these defaults without visiting Thicken. This tab lets you change them; Cut, Servo Tray and Formers are optional. Open 3D View to regenerate. Each end is open when the Top/Side outlines end at its outermost profile, and closed when the outlines extend beyond it. Narrow extended tips remain solid.\n\nAll new station values default to 8 mm at or forward of the wing leading edge and 5 mm aft of it. Your edited values stay attached when stations move.",this};
  instructions->setObjectName("fuselageThickenInstructions");instructions->setWordWrap(true);layout->addWidget(instructions);
  defaults_=new QLabel{this};defaults_->setWordWrap(true);layout->addWidget(defaults_);
  fields_=new QFormLayout;layout->addLayout(fields_);layout->addStretch();
  connect(&source_,&SketchEditor::stationsChanged,this,[this]{if(!updating_)QTimer::singleShot(0,this,[this]{rebuild();});});
}
void FuselageThickenPanel::enter(double wingLeadingEdge) {
  const bool first=!enabled_;enabled_=true;synchronize(wingLeadingEdge);rebuild();if(first&&changed)changed();
}
void FuselageThickenPanel::synchronize(double wingLeadingEdge) {
  if(!enabled_||updating_)return;
  updating_=true;auto& stations=source_.stationEditor();
  for(int i=0;i<static_cast<int>(stations.lines().size());++i)if(!stations.lines()[i].thicknessMm)
    stations.setThicknessMm(i,stations.lines()[i].first.position.x()<=wingLeadingEdge+1e-7?8.:5.);
  defaults_->setText(QString{"Defaults compare Side View stations with wing LE at drawing X = %1. Existing thickness values are preserved."}.arg(wingLeadingEdge,0,'f',2));
  updating_=false;
}
void FuselageThickenPanel::restore(bool enabled) {enabled_=enabled;rebuild();}
void FuselageThickenPanel::setUnits(ProjectUnits units) {
  if(units_==units)return;
  units_=units;rebuild();
}
void FuselageThickenPanel::rebuild() {
  while(fields_->rowCount())fields_->removeRow(0);
  const auto& stations=source_.stationEditor().lines();
  const auto order=fuselageStationOrder(stations);
  for(int number=0;number<static_cast<int>(order.size());++number) {
    const int index=order[number];
    auto* edit=new QLineEdit{this};edit->setObjectName(QString{"fuselageThickness%1"}.arg(index));
    edit->setPlaceholderText("e.g. 8 mm or 0.315 in");
    edit->setToolTip("Positive decimal; optional mm or in suffix. Bare numbers use Reference units.");
    edit->setText(formattedLength(stations[index].thicknessMm.value_or(5),displayLengthUnits(stations[index].thicknessUnit,units_)));
    fields_->addRow(QString{"Station %1"}.arg(number+1),edit);
    connect(edit,&QLineEdit::editingFinished,this,[this,index,edit]{
      // A focus change must not round a stored value through its display text.
      if(!edit->isModified())return;
      auto& stations=source_.stationEditor();
      if(index>=static_cast<int>(stations.lines().size()))return;
      const auto mm=lengthInMm(edit->text(),units_);
      const auto previous=stations.lines()[index].thicknessMm.value_or(5);
      const bool valid=mm&&*mm>=.001&&*mm<=10000;
      const auto unit=valid?enteredLengthUnit(edit->text(),units_):stations.lines()[index].thicknessUnit;
      edit->setText(formattedLength(valid?*mm:previous,displayLengthUnits(unit,units_)));
      if(valid) {
        stations.setThicknessMm(index,*mm,unit);
        if(changed)changed();
      }
    });
  }
}
}

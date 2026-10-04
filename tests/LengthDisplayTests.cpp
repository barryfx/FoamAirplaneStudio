#include "TestCheck.h"
#include "gui/LengthEntry.h"
#include "gui/ServoTrayPanel.h"
#include "gui/FormerPanel.h"
#include "gui/FuselageThickenPanel.h"
#include "gui/StiffenerPanel.h"
#include "gui/ProjectDocument.h"
#include <QApplication>
#include <QGraphicsView>
#include <QPushButton>
#include <iostream>
using namespace designrc::gui;
static void enter(QLineEdit* field,const QString& text) {
  TEST_CHECK(field);field->setText(text);field->setModified(true);
  QMetaObject::invokeMethod(field,"editingFinished");
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};
  try {
    QGraphicsView view;ServoTrayEditor tray{view};ServoTrayPanel trayPanel{tray,nullptr};
    trayPanel.configure(ProjectUnits::Inches,1,{100,100});
    enter(trayPanel.findChild<QLineEdit*>("servoTrayWidth"),"50 mm");
    enter(trayPanel.findChild<QLineEdit*>("servoTrayHeight"),".125 in");
    trayPanel.configure(ProjectUnits::Millimeters,1,{100,100});
    TEST_CHECK(trayPanel.findChild<QLineEdit*>("servoTrayWidth")->text()=="50 mm");
    TEST_CHECK(trayPanel.findChild<QLineEdit*>("servoTrayHeight")->text()=="0.125 in");
    const auto savedTray=tray.state();tray.restore({});tray.restore(savedTray);
    TEST_CHECK(trayPanel.findChild<QLineEdit*>("servoTrayHeight")->text()=="0.125 in");
    FormerEditor former{view};FormerPanel formerPanel{former,nullptr};
    formerPanel.configure(ProjectUnits::Inches,1,{0,0,200,30});
    enter(formerPanel.findChild<QLineEdit*>("formerWidth"),"3 mm");former.add();
    TEST_CHECK(former.state().thicknessUnits.at(0)==LengthUnit::Millimeters);
    enter(formerPanel.findChild<QLineEdit*>("formerWidth"),".125 in");
    formerPanel.configure(ProjectUnits::Millimeters,1,{0,0,200,30});
    TEST_CHECK(formerPanel.findChild<QLineEdit*>("formerWidth")->text()=="0.125 in");
    const auto savedFormer=former.state();former.restore(savedFormer);former.add();
    TEST_CHECK(former.state().thicknessUnits.size()==2);former.remove();
    TEST_CHECK(former.state().thicknessUnits.size()==1);
    SketchEditor source{&view};StationState stations;ConstrainedLine line;line.thicknessMm=5;stations.lines.push_back(line);
    source.stationEditor().restoreState(stations);FuselageThickenPanel wall{source,nullptr};wall.setUnits(ProjectUnits::Inches);wall.restore(true);
    enter(wall.findChild<QLineEdit*>("fuselageThickness0"),"6 mm");wall.setUnits(ProjectUnits::Millimeters);wall.setUnits(ProjectUnits::Inches);
    TEST_CHECK(wall.findChild<QLineEdit*>("fuselageThickness0")->text()=="6 mm");
    enter(wall.findChild<QLineEdit*>("fuselageThickness0"),".25 in");wall.setUnits(ProjectUnits::Millimeters);
    TEST_CHECK(wall.findChild<QLineEdit*>("fuselageThickness0")->text()=="0.25 in");
    StiffenerPanel stiff;enter(stiff.findChild<QLineEdit*>("stiffenerWidth"),".125 in");
    auto stiffState=stiff.state();stiff.restore({});stiff.restore(stiffState);
    TEST_CHECK(stiff.findChild<QLineEdit*>("stiffenerWidth")->text()=="0.125 in");
    QString error;auto project=readProject(QString::fromLocal8Bit(argv[1]),error);TEST_CHECK(project);
    project->stiffeners=stiffState;project->servoTray.widthUnit=LengthUnit::Millimeters;project->servoTray.heightUnit=LengthUnit::Inches;
    project->formers.thicknessUnit=LengthUnit::Inches;
    project->formers.thicknessUnits.assign(project->formers.rectangles.size(),LengthUnit::Millimeters);
    project->fuselageStations.lines.at(0).thicknessUnit=LengthUnit::Inches;
    project->fiberglass[1].patches[0].resinThicknessUnit=LengthUnit::Inches;
    BalancePart part;part.name="Unit test part";part.widthUnit=LengthUnit::Inches;part.heightUnit=LengthUnit::Millimeters;project->weightBalance.parts={part};
    const auto json=encodeProject(*project);const auto restored=decodeProject(json);TEST_CHECK(encodeProject(restored)==json);
    TEST_CHECK(restored.fuselageStations.lines.at(0).thicknessUnit==LengthUnit::Inches);
    TEST_CHECK(restored.stiffeners.widthUnit==LengthUnit::Inches);
    TEST_CHECK(restored.servoTray.heightUnit==LengthUnit::Inches);
    TEST_CHECK(restored.weightBalance.parts[0].widthUnit==LengthUnit::Inches);
    TEST_CHECK(restored.fiberglass[1].patches[0].resinThicknessUnit==LengthUnit::Inches);
    auto bad=json;auto data=bad["stiffeners"].toObject();data["widthUnit"]=9;bad["stiffeners"]=data;
    bool rejected=false;try{decodeProject(bad);}catch(const std::exception&){rejected=true;}TEST_CHECK(rejected);
    std::cout<<"Distance display units, editor restore and project round-trip passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

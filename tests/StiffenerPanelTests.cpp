#include "TestCheck.h"
#include "gui/StiffenerPanel.h"
#include <QApplication>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <iostream>
using namespace designrc::gui;
int main(int argc,char** argv) {
  QApplication app{argc,argv};
  StiffenerPanel panel;panel.show();app.processEvents();
  auto* count=panel.findChild<QSpinBox*>("stiffenerCount");
  auto* width=panel.findChild<QLineEdit*>("stiffenerWidth");
  auto* height=panel.findChild<QLineEdit*>("stiffenerHeight");
  auto* diameter=panel.findChild<QLineEdit*>("stiffenerDiameter");
  auto enter=[](QLineEdit* field,const QString& text){field->setText(text);field->setModified(true);QMetaObject::invokeMethod(field,"editingFinished");};
  TEST_CHECK(panel.state().startPercent==20&&panel.state().stopPercent==90);
  TEST_CHECK(panel.findChild<QDoubleSpinBox*>("stiffenerStart")->value()==20);
  TEST_CHECK(panel.findChild<QDoubleSpinBox*>("stiffenerStop")->value()==90);
  TEST_CHECK(count->minimum()==0&&count->maximum()==3);
  count->setValue(4);TEST_CHECK(count->value()==3&&panel.state().count==3);
  count->setValue(-1);TEST_CHECK(panel.state().count==0);
  auto invalid=panel.state();invalid.count=4;bool rejected=false;
  try{validateStiffeners(invalid);}catch(const std::runtime_error& error){rejected=std::string{error.what()}.find("0-3")!=std::string::npos;}
  TEST_CHECK(rejected);
  panel.setUnits(ProjectUnits::Inches);
  TEST_CHECK(width->text()=="3 mm"&&height->text()=="1 mm"&&diameter->text()=="3 mm");
  enter(width,"4");TEST_CHECK(panel.state().widthMm==4&&width->text()=="4 mm");
  enter(height,"0.025 in");TEST_CHECK(std::abs(panel.state().heightMm-.635)<1e-9&&height->text()=="0.025 in");
  count->setValue(2);panel.setUnits(ProjectUnits::Millimeters);
  TEST_CHECK(height->text()=="0.025 in"&&width->text()=="4 mm");
  enter(width,".125 inches");TEST_CHECK(width->text()=="0.125 in");
  enter(width,"bad");TEST_CHECK(width->text()=="0.125 in");
  enter(width,"2");TEST_CHECK(width->text()=="2 mm"&&panel.state().widthMm==2);
  panel.findChild<QComboBox*>("stiffenerShape")->setCurrentIndex(1);
  TEST_CHECK(diameter->isVisible()&&!width->isVisible());
  enter(diameter,".1\"");TEST_CHECK(diameter->text()=="0.1 in");
  panel.setUnits(ProjectUnits::Inches);TEST_CHECK(diameter->text()=="0.1 in");
  panel.restore({});TEST_CHECK(width->text()=="3 mm"&&height->text()=="1 mm"&&diameter->text()=="3 mm");
  TEST_CHECK(panel.state().startPercent==20&&panel.state().stopPercent==90);
  std::cout<<"Stiffener GUI defaults, count limits, mm entry and explicit inch display passed\n";
}

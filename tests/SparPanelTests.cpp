#include "gui/SparPanel.h"
#include "gui/DihedralPanel.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QScreen>
#include <QTabBar>
#include <cmath>
using namespace designrc::gui;
#define CHECK(c) do{if(!(c))qFatal("%s at %d",#c,__LINE__);}while(false)
int main(int argc,char** argv) {
  QApplication app{argc,argv};SparPanel panel;panel.resize(380,850);panel.show();app.processEvents();
  DihedralPanel dihedral;dihedral.setPanelCount(2);int dihedralChanges=0;dihedral.changed=[&]{++dihedralChanges;};
  auto* angle=dihedral.findChild<QDoubleSpinBox*>("rootDihedral");auto* angleTabs=dihedral.findChild<QTabBar*>("dihedralPanelTabs");
  CHECK(angle->value()==0);angle->setValue(3);angleTabs->setCurrentIndex(1);CHECK(angle->value()==0 && dihedralChanges==1);
  angle->setValue(5);angleTabs->setCurrentIndex(0);CHECK(angle->value()==3 && dihedralChanges==2);
  CHECK(dihedral.values()==std::vector<double>({3,5}));dihedral.setPanelCount(1);dihedral.setPanelCount(2);angleTabs->setCurrentIndex(1);CHECK(angle->value()==0);
  dihedral.restore({1.25,-2.5},1);CHECK(angle->value()==-2.5 && dihedralChanges==2);
  int changes=0;panel.changed=[&]{++changes;};
  for(const auto* name:{"Top","Bottom","Mid"}) {
    auto* check=panel.findChild<QCheckBox*>(QString{"spar"}+name);CHECK(!check->isChecked());check->click();
  }
  CHECK(changes==3);for(const auto& s:panel.state()[0])CHECK(s.enabled);
  auto* shape=panel.findChild<QComboBox*>("sparTopShape");shape->setCurrentIndex(1);
  auto* width=panel.findChild<QLineEdit*>("sparTopSize");auto* height=panel.findChild<QLineEdit*>("sparTopHeight");
  CHECK(height->isVisible());CHECK(!panel.findChild<QLineEdit*>("sparMidHeight")->isVisible());
  auto enter=[](QLineEdit* field,const QString& text){field->setText(text);QMetaObject::invokeMethod(field,"editingFinished");};
  enter(width,"25.4");enter(height,"2.54 mm");const int before=changes;
  panel.setUnits(ProjectUnits::Inches);CHECK(changes==before);CHECK(width->text()=="25.4 mm");CHECK(height->text()=="2.54 mm");
  enter(width,".5 in");CHECK(std::abs(panel.state()[0][0].sizeMm-12.7)<1e-8);
  panel.setUnits(ProjectUnits::Millimeters);CHECK(width->text()==".5 in");
  enter(height,".125 in");CHECK(std::abs(panel.state()[0][0].heightMm-3.175)<1e-8);
  enter(width,"bad");CHECK(width->text()==".5 in");
  const auto saved=panel.state();panel.restore({},ProjectUnits::Millimeters);CHECK(!panel.state()[0][0].enabled);
  panel.restore(saved,ProjectUnits::Inches);CHECK(shape->currentIndex()==1 && height->isVisible());CHECK(width->text()==".5 in" && height->text()==".125 in");
  panel.setPanelCount(2);auto* tabs=panel.findChild<QTabBar*>("sparPanelTabs");CHECK(tabs->count()==2);
  const int beforeTab=changes;tabs->setCurrentIndex(1);CHECK(changes==beforeTab);
  CHECK(!panel.findChild<QCheckBox*>("sparTop")->isChecked());
  panel.findChild<QCheckBox*>("sparMid")->click();
  panel.findChild<QDoubleSpinBox*>("sparMidChord")->setValue(60);
  CHECK(panel.state()[1][2].enabled && panel.state()[1][2].chordPercent==60);
  tabs->setCurrentIndex(0);CHECK(shape->currentIndex()==1 && panel.state()[0][0].sizeMm==12.7);
  panel.restore(panel.state(),ProjectUnits::Inches,1);CHECK(panel.selectedPanel()==1);
  panel.setPanelCount(1);CHECK(panel.selectedPanel()==0 && panel.state().size()==1);
  panel.setPanelCount(2);CHECK(!panel.state()[1][2].enabled);
  app.processEvents();const auto capture=qEnvironmentVariable("FOAM_SPAR_PANEL_CAPTURE");
  if(!capture.isEmpty())CHECK(panel.screen()->grabWindow(panel.winId()).save(capture));
}

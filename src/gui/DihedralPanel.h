#pragma once
#include <QWidget>
#include <QDoubleSpinBox>
#include <QTabBar>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <algorithm>
#include <functional>
#include <vector>
namespace designrc::gui {
class DihedralPanel final : public QWidget {
public:
  explicit DihedralPanel(QWidget* parent=nullptr):QWidget{parent} {
    setObjectName("dihedralPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
    auto* hint=new QLabel{"Set each panel's root angle relative to the preceding panel. Positive angles raise the outer wing; the first panel is relative to horizontal.",this};
    hint->setWordWrap(true);layout->addWidget(hint);
    tabs_=new QTabBar{this};tabs_->setObjectName("dihedralPanelTabs");tabs_->addTab("1");layout->addWidget(tabs_);
    angle_=new QDoubleSpinBox{this};angle_->setObjectName("rootDihedral");angle_->setRange(-80,80);angle_->setDecimals(3);
    angle_->setSingleStep(.5);angle_->setSuffix(" degrees");angle_->setKeyboardTracking(false);
    auto* fields=new QFormLayout;fields->addRow("Root Dihedral",angle_);layout->addLayout(fields);layout->addStretch();
    connect(tabs_,&QTabBar::currentChanged,this,[this]{refresh();});
    connect(angle_,&QDoubleSpinBox::valueChanged,this,[this](double value){values_[selectedPanel()]=value;if(changed)changed();});
  }
  const std::vector<double>& values() const{return values_;}
  int selectedPanel() const{return std::max(0,tabs_->currentIndex());}
  void setPanelCount(int count) {
    count=std::clamp(count,1,100);values_.resize(count,0);QSignalBlocker block{tabs_};
    while(tabs_->count()>count)tabs_->removeTab(tabs_->count()-1);
    while(tabs_->count()<count)tabs_->addTab(QString::number(tabs_->count()+1));
    if(tabs_->currentIndex()<0)tabs_->setCurrentIndex(0);refresh();
  }
  void restore(const std::vector<double>& values,int selected=0) {
    setPanelCount(static_cast<int>(values.size()));values_=values.empty()?std::vector<double>{0}:values;
    QSignalBlocker block{tabs_};tabs_->setCurrentIndex(std::clamp(selected,0,static_cast<int>(values_.size())-1));refresh();
  }
  std::function<void()> changed;
private:
  void refresh(){QSignalBlocker block{angle_};angle_->setValue(values_[selectedPanel()]);}
  std::vector<double> values_{0};QTabBar* tabs_{};QDoubleSpinBox* angle_{};
};
}

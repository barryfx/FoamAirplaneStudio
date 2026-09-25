#include "gui/SparPanel.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include "gui/LengthEntry.h"
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QTabBar>
#include <algorithm>
namespace designrc::gui {
SparPanel::SparPanel(QWidget* parent):QWidget{parent} {
  setObjectName("sparPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
  auto* hint=new QLabel{"All spars are assumed to be Carbon Fiber. Top and bottom spars are solid rods or strips; mid-height spars are hollow tubes. Choose spar locations. Chord position is measured from the local leading edge; length starts at the selected panel root and is a percentage of that panel span. Mid spars sit at 50% local thickness and split only the main wing panel, with alignment tabs and holes.",this};
  hint->setWordWrap(true);layout->addWidget(hint);
  tabs_=new QTabBar{this};tabs_->setObjectName("sparPanelTabs");tabs_->addTab("1");layout->addWidget(tabs_);
  for(int i=0;i<3;++i) {
    const QString name=i==0?"Top":i==1?"Bottom":"Mid";
    checks_[i]=new QCheckBox{name+" wing height",this};checks_[i]->setObjectName("spar"+name);layout->addWidget(checks_[i]);
    details_[i]=new QWidget{this};auto* form=new QFormLayout{details_[i]};form->setContentsMargins(0,0,0,12);
    shapes_[i]=new QComboBox{details_[i]};shapes_[i]->addItems({"Round","Strip"});shapes_[i]->setObjectName("spar"+name+"Shape");
    if(i<2)form->addRow("Shape",shapes_[i]);else shapes_[i]->hide();
    for(int j=0;j<4;++j) {
      const QString field=j==0?"Chord":j==1?"Length":j==2?"Size":"Height";
      auto* label=new QLabel{j==0?"Chord location (% from LE)":j==1?"Length (% of panel)":j==2?"Diameter":"Height",details_[i]};
      if(j==2)sizeLabels_[i]=label;if(j==3)heightLabels_[i]=label;
      if(j<2) {
        auto* value=new QDoubleSpinBox{details_[i]};values_[i][j]=value;
        value->setObjectName("spar"+name+field);value->setKeyboardTracking(false);value->setDecimals(2);
        value->setRange(j==0?0:0.01,100);value->setSingleStep(1);form->addRow(label,value);
        connect(value,&QDoubleSpinBox::valueChanged,this,[this,i,j](double v) {
          auto& spar=state_[selectedPanel()][i];
          if(j==0)spar.chordPercent=v;else spar.lengthPercent=v;
          if(changed)changed();
        });
      } else {
        auto* edit=new QLineEdit{details_[i]};lengths_[i][j-2]=edit;
        edit->setObjectName("spar"+name+field);edit->setPlaceholderText("e.g. 3 mm or 0.125 in");
        edit->setToolTip("Positive length, 0.0001 to 10000 mm. Optional mm or in suffix; bare numbers use Project Units.");
        form->addRow(label,edit);
        connect(edit,&QLineEdit::editingFinished,this,[this,i,j,edit] {
          const auto mm=lengthInMm(edit->text(),units_);
          if(mm && *mm>=0.0001 && *mm<=10000) {
            auto& spar=state_[selectedPanel()][i];
            auto& stored=j==2?spar.sizeText:spar.heightText;
            auto& value=j==2?spar.sizeMm:spar.heightMm;
            if(i==2 && j==2 && !spar.insideDiameterText.empty() && *mm<=spar.insideDiameterMm) {
              edit->setToolTip("Outside Diameter must exceed Inside Diameter.");updateControls();return;
            }
            if(i==2 && j==2 && spar.insideDiameterText.empty())spar.insideDiameterMm=std::max(0.0,*mm-1.0);
            const auto text=explicitLength(edit->text(),units_).toStdString();
            const bool different=value!=*mm || stored!=text;
            value=*mm;stored=text;
            updateControls();if(different && changed)changed();
          } else updateControls(); // Invalid edits restore the last committed dimension.
        });
      }
    }
    if(i==2) {
      insideDiameter_=new QLineEdit{details_[i]};insideDiameter_->setObjectName("sparMidInsideDiameter");
      insideDiameter_->setToolTip("Inside Diameter must be smaller than Outside Diameter. Use mm or in; zero means a solid rod. Defaults to Outside Diameter minus 1 mm until edited.");
      form->addRow("Inside Diameter",insideDiameter_);
      connect(insideDiameter_,&QLineEdit::editingFinished,this,[this] {
        auto& spar=state_[selectedPanel()][2];
        if(spar.insideDiameterText.empty() && insideDiameter_->text()==formattedLength(spar.insideDiameterMm,ProjectUnits::Millimeters))return;
        const auto mm=lengthInMm(insideDiameter_->text(),units_,true);
        if(mm && *mm<spar.sizeMm) {
          const auto text=explicitLength(insideDiameter_->text(),units_,true).toStdString();
          const bool different=spar.insideDiameterMm!=*mm || spar.insideDiameterText!=text;
          spar.insideDiameterMm=*mm;spar.insideDiameterText=text;updateControls();if(different&&changed)changed();
        } else updateControls();
      });
    }
    connect(checks_[i],&QCheckBox::toggled,this,[this,i](bool enabled){state_[selectedPanel()][i].enabled=enabled;updateControls();if(changed)changed();});
    connect(shapes_[i],&QComboBox::currentIndexChanged,this,[this,i](int shape){state_[selectedPanel()][i].shape=static_cast<SparShape>(shape);updateControls();if(changed)changed();});
    layout->addWidget(details_[i]);
  }
  connect(tabs_,&QTabBar::currentChanged,this,[this]{updateControls();});
  layout->addStretch();updateControls();
}
int SparPanel::selectedPanel() const {return std::max(0,tabs_->currentIndex());}
void SparPanel::setPanelCount(int count) {
  count=std::clamp(count,1,100);if(static_cast<int>(state_.size())==count)return;
  state_.resize(count);QSignalBlocker block{tabs_};
  while(tabs_->count()>count)tabs_->removeTab(tabs_->count()-1);
  while(tabs_->count()<count)tabs_->addTab(QString::number(tabs_->count()+1));
  tabs_->setCurrentIndex(std::clamp(tabs_->currentIndex(),0,count-1));updateControls();
}
void SparPanel::restore(const PanelSpars& state,ProjectUnits units,int selected) {
  setPanelCount(static_cast<int>(state.size()));state_=state.empty()?PanelSpars(1):state;units_=units;
  QSignalBlocker block{tabs_};tabs_->setCurrentIndex(std::clamp(selected,0,static_cast<int>(state_.size())-1));updateControls();
}
void SparPanel::setUnits(ProjectUnits units) {units_=units;updateControls();}
void SparPanel::updateControls() {
  for(int i=0;i<3;++i) {
    const auto& s=state_[selectedPanel()][i];QSignalBlocker check{checks_[i]},shape{shapes_[i]};
    checks_[i]->setChecked(s.enabled);details_[i]->setVisible(s.enabled);shapes_[i]->setCurrentIndex(static_cast<int>(s.shape));
    const bool strip=i<2 && s.shape==SparShape::Strip;
    sizeLabels_[i]->setText(i==2?"Outside Diameter":strip?"Width":"Diameter");heightLabels_[i]->setVisible(strip);lengths_[i][1]->setVisible(strip);
    if(i==2)insideDiameter_->setText(s.insideDiameterText.empty()?formattedLength(s.insideDiameterMm,ProjectUnits::Millimeters):QString::fromStdString(s.insideDiameterText));
    const std::array<double,2> data{s.chordPercent,s.lengthPercent};
    for(int j=0;j<2;++j) {
      QSignalBlocker block{values_[i][j]};values_[i][j]->setValue(data[j]);
      const auto& text=j==0?s.sizeText:s.heightText;
      lengths_[i][j]->setText(text.empty()?formattedLength(j==0?s.sizeMm:s.heightMm,i==2?ProjectUnits::Millimeters:units_):QString::fromStdString(text));
    }
  }
}
}

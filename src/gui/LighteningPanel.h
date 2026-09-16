#pragma once
#include "gui/LighteningState.h"
#include "gui/LengthEntry.h"
#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <functional>
namespace designrc::gui {
class LighteningPanel final : public QWidget {
public:
  explicit LighteningPanel(QWidget* parent=nullptr):QWidget{parent} {
    setObjectName("lighteningPanel");auto* layout=new QVBoxLayout{this};layout->setContentsMargins(0,0,0,0);
    auto* hint=new QLabel{tr("Hollow the main wing and split it into upper and lower halves for access. Ailerons, flaps and the outer tip remain solid. Wall Thickness is the minimum material retained at skins, leading/trailing edges, control cuts and spars. Crossmembers are evenly spaced within the hollowing range; Crossmember Thickness sets their spanwise width. Solid material also remains at panel joints. Start Distance is measured from the first panel root; Stop Distance is measured inward from the last airfoil station. Distances follow the unfolded half-wing span. Thin areas remain solid. Pocket floors and ceilings are stepped conservatively to preserve minimum walls. Alignment pins and sockets retain wall-thickness support. The split follows the mid spar when present, otherwise the half-thickness height at 30% chord."),this};
    hint->setWordWrap(true);layout->addWidget(hint);
    enabled_=new QCheckBox{"Enable Lightening",this};enabled_->setObjectName("lighteningEnabled");layout->addWidget(enabled_);
    fields_=new QWidget{this};auto* form=new QFormLayout{fields_};form->setContentsMargins(0,0,0,0);
    const char* titles[]={"Wall Thickness","Crossmember Thickness","Start Distance from Root","Stop Distance before Last Station"};
    const char* names[]={"lighteningWall","lighteningRib","lighteningStart","lighteningStop"};
    for(int i=0;i<4;++i) {
      edits_[i]=new QLineEdit{fields_};edits_[i]->setObjectName(names[i]);edits_[i]->setPlaceholderText("e.g. 2 mm or 0.125 in");
      form->addRow(titles[i],edits_[i]);
      connect(edits_[i],&QLineEdit::editingFinished,this,[this,i]{
        auto mm=lengthInMm(edits_[i]->text(),units_,i>=2);
        if(mm && *mm>=(i>=2?0:.0001) && *mm<=10000) {
          const auto text=explicitLength(edits_[i]->text(),units_,i>=2).toStdString();
          const bool different=*values()[i]!=*mm || state_.text[i]!=text;
          *values()[i]=*mm;state_.text[i]=text;refresh();if(different && changed)changed();
        } else refresh();
      });
    }
    count_=new QSpinBox{fields_};count_->setObjectName("lighteningCrossmembers");count_->setRange(0,100);count_->setKeyboardTracking(false);
    form->insertRow(1,"Number of Crossmembers",count_);layout->addWidget(fields_);layout->addStretch();
    connect(enabled_,&QCheckBox::toggled,this,[this](bool v){state_.enabled=v;refresh();if(changed)changed();});
    connect(count_,&QSpinBox::valueChanged,this,[this](int v){state_.crossmembers=v;if(changed)changed();});refresh();
  }
  const LighteningState& state() const{return state_;}
  void restore(const LighteningState& state,ProjectUnits units){state_=state;units_=units;refresh();}
  void setUnits(ProjectUnits units){units_=units;refresh();}
  std::function<void()> changed;
private:
  std::array<double*,4> values(){return {&state_.wallMm,&state_.ribMm,&state_.startMm,&state_.stopMm};}
  void refresh(){QSignalBlocker a{enabled_},b{count_};enabled_->setChecked(state_.enabled);fields_->setEnabled(state_.enabled);count_->setValue(state_.crossmembers);
    for(int i=0;i<4;++i)edits_[i]->setText(state_.text[i].empty()?formattedLength(*values()[i],units_):QString::fromStdString(state_.text[i]));}
  LighteningState state_;ProjectUnits units_=ProjectUnits::Millimeters;QCheckBox* enabled_{};QWidget* fields_{};QSpinBox* count_{};std::array<QLineEdit*,4> edits_{};
};
}

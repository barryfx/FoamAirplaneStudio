#include "gui/AirfoilPanel.h"
#include "gui/AirfoilSmoothingDialog.h"
#include "gui/ProcessingScope.h"
#include "gui/FileSelectionDialog.h"
#include "gui/PlanViewport.h"
#include <QButtonGroup>
#include <QTabBar>
#include <algorithm>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QShortcut>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QRegularExpression>
#include <QVBoxLayout>
namespace designrc::gui {
AirfoilPanel::AirfoilPanel(PlanViewport& view, QWidget* parent) : QWidget{parent}, view_{view} {
  setObjectName("airfoilPanel");
  auto* layout = new QVBoxLayout{this}; layout->setContentsMargins(0, 0, 0, 0);
  tabs_=new QTabBar{this};tabs_->setObjectName("airfoilPanelTabs");tabs_->addTab("1");layout->addWidget(tabs_);
  connect(tabs_,&QTabBar::currentChanged,this,[this](int panel){
    if(panel<0 || panel==panel_)return;
    panelChoices_[panel_]=chosen_;panel_=panel;chosen_=panelChoices_[panel_];
    if(chosen_<0 && !library_.entries().empty())chosen_=0;
    if(chosen_>=0)group_->button(chosen_)->setChecked(true);
    view_.sketchEditor().stationEditor().setActivePanel(panel_);
  });
  auto* description = new QLabel{"Load airfoil .dat files or Sketch airfoils on the reference image and click on a station to assign one to that station. Select an airfoil in the list to export its normalized .dat coordinates.", this};
  description->setWordWrap(true); description_ = description; layout->addWidget(description);
  load_ = new QPushButton{"Load Airfoil .dat File", this}; load_->setObjectName("loadAirfoilDat"); layout->addWidget(load_);
  sketchButton_ = new QPushButton{"Sketch Airfoil", this}; sketchButton_->setObjectName("sketchAirfoil");
  sketchButton_->setCheckable(true); layout->addWidget(sketchButton_);
  tools_ = new QWidget{this}; auto* toolsLayout = new QHBoxLayout{tools_}; toolsLayout->setContentsMargins(0, 0, 0, 0);
  for (auto tool : {SketchTool::Line, SketchTool::Spline}) {
    auto* button = new QPushButton{tool == SketchTool::Line ? "Line" : "Spline", tools_};
    button->setObjectName(tool == SketchTool::Line ? "airfoilLine" : "airfoilSpline"); button->setCheckable(true);
    button->setProperty("sketchTool", static_cast<int>(tool)); toolsLayout->addWidget(button);
    connect(button, &QPushButton::clicked, this, [this, tool](bool checked) {
      view_.airfoilSketchEditor().setTool(checked ? tool : SketchTool::None);
      for (auto* button : tools_->findChildren<QPushButton*>())
        button->setChecked(button->property("sketchTool").toInt() == static_cast<int>(view_.airfoilSketchEditor().tool()));
    });
  }
  layout->addWidget(tools_);
  list_ = new QWidget{this}; auto* listOuter = new QVBoxLayout{list_}; listOuter->setContentsMargins(0, 0, 0, 0);
  auto* title = new QLabel{"Airfoil for selected station", list_}; listOuter->addWidget(title);
  auto* scroll = new QScrollArea{list_}; scroll->setWidgetResizable(true);
  auto* entries = new QWidget{scroll}; listLayout_ = new QVBoxLayout{entries}; listLayout_->addStretch();
  scroll->setWidget(entries); listOuter->addWidget(scroll); layout->addWidget(list_, 1);
  group_ = new QButtonGroup{this}; group_->setExclusive(true);
  smooth_=new QPushButton{"Smooth Airfoil",this};smooth_->setObjectName("smoothAirfoil");layout->addWidget(smooth_);
  connect(smooth_,&QPushButton::clicked,this,[this] {
    if(sketching_||chosen_<0||chosen_>=static_cast<int>(library_.entries().size()))return;
    try {
      const auto& entry=library_.entries()[chosen_];
      const auto base=entry.name+QString::fromUtf8(" — Smoothed");auto name=base;int suffix=2;
      while(std::any_of(library_.entries().begin(),library_.entries().end(),[&](const auto& e){return e.name==name;}))
        name=base+QString{" %1"}.arg(suffix++);
      AirfoilSmoothingDialog dialog{normalizedAirfoil(entry),name,this};
      if(dialog.exec()!=QDialog::Accepted)return;
      library_.addProfile(dialog.copyName(),dialog.result());addLibraryButton();
    }catch(const std::exception& e){QMessageBox::warning(this,"Smooth Airfoil",QString::fromUtf8(e.what()));}
  });
  export_=new QPushButton{"Export Selected Airfoil .dat",this};
  export_->setObjectName("exportAirfoilDat");layout->addWidget(export_);
  connect(export_,&QPushButton::clicked,this,[this] {
    if(chosen_<0||chosen_>=static_cast<int>(library_.entries().size())||sketching_)return;
    FileSelectionDialog dialog{this,"airfoilDatExport","Export Airfoil .dat",QFileDialog::AnyFile,QFileDialog::AcceptSave};
    dialog.setNameFilter("Airfoil files (*.dat)");dialog.setDefaultSuffix("dat");
    auto name=library_.entries()[chosen_].name;
    name.replace(QRegularExpression{R"([<>:"/\\|?*\x00-\x1f])"},"_");
    dialog.selectFile(name+".dat");
    if(dialog.exec()!=QDialog::Accepted)return;
    QString error;
    if(!library_.exportDat(chosen_,dialog.selectedFiles().front(),error))
      QMessageBox::warning(this,"Export Airfoil",error);
  });
  connect(group_, &QButtonGroup::idClicked, this, [this](int id) {
    chosen_ = id;
    updateControls();
    view_.sketchEditor().stationEditor().assignSelectedAirfoil(static_cast<std::size_t>(id));
  });
  connect(load_, &QPushButton::clicked, this, [this] {
    FileSelectionDialog dialog{this, "airfoilDat", "Load Airfoil .dat File"}; dialog.setNameFilter("Airfoil files (*.dat)");
    if (dialog.exec() != QDialog::Accepted) return;
    QString error; if (!loadAirfoil(dialog.selectedFiles().front(), error)) QMessageBox::warning(this, "Load Airfoil", error);
  });
  connect(sketchButton_, &QPushButton::clicked, this, &AirfoilPanel::toggleSketch);
  escape_ = new QShortcut{QKeySequence{Qt::Key_Escape}, this};
  connect(escape_, &QShortcut::activated, this, [this] { finishSketch(); });
  connect(&view_.airfoilSketchEditor(), &SketchEditor::sessionFinished, this, [this] { finishSketch(); });
  connect(&view_.sketchEditor(), &SketchEditor::stationSelectionChanged, this, &AirfoilPanel::selectStation);
  updateControls();
}
AirfoilState AirfoilPanel::state() const {
  auto choices=panelChoices_;choices[panel_]=chosen_;
  return {library_.entries(),chosen_,draft_,sketching_,draftName_,panel_,choices};
}
void AirfoilPanel::setPanelCount(int count) {
  count=std::clamp(count,1,100);if(count==static_cast<int>(panelChoices_.size()))return;
  panelChoices_[panel_]=chosen_;panelChoices_.resize(count,library_.entries().empty()?-1:0);
  panel_=std::min(panel_,count-1);chosen_=panelChoices_[panel_];
  QSignalBlocker block{tabs_};
  while(tabs_->count()>count)tabs_->removeTab(tabs_->count()-1);
  while(tabs_->count()<count)tabs_->addTab(QString::number(tabs_->count()+1));
  tabs_->setCurrentIndex(panel_);
  if(active_)view_.sketchEditor().stationEditor().setActivePanel(panel_);
}
void AirfoilPanel::updateControls() {
  export_->setEnabled(!sketching_&&chosen_>=0&&chosen_<static_cast<int>(library_.entries().size()));
  smooth_->setEnabled(export_->isEnabled());
  tabs_->setEnabled(!sketching_);
  description_->setEnabled(!sketching_); load_->setEnabled(!sketching_); list_->setEnabled(!sketching_);
  tools_->setVisible(sketching_); sketchButton_->setChecked(sketching_);
  escape_->setEnabled(sketching_);
}
void AirfoilPanel::setActive(bool active) {
  if (!active && sketching_) finishSketch();
  active_ = active; setVisible(active);
  if(active)view_.sketchEditor().stationEditor().setActivePanel(panel_);
  view_.sketchEditor().stationEditor().setSelectionEnabled(active && !sketching_);
}
bool AirfoilPanel::loadAirfoil(const QString& path, QString& error) {
  ProcessingScope processing{this, "Loading airfoil..."};
  if (!library_.loadDat(path, error)) {processing.update("Airfoil load failed: " + error);return false;}
  addLibraryButton(); processing.update("Airfoil loaded"); return true;
}
void AirfoilPanel::addLibraryButton() {
  const int id = static_cast<int>(library_.entries().size()) - 1;
  auto* button = new QRadioButton{library_.entries().back().name, list_};
  button->setObjectName(QString{"airfoilChoice%1"}.arg(id)); group_->addButton(button, id);
  listLayout_->insertWidget(listLayout_->count() - 1, button);
  if (chosen_ < 0) {
    chosen_ = 0; group_->button(0)->setChecked(true);
    view_.sketchEditor().stationEditor().assignSelectedAirfoil(0);
  }
  updateControls();emit libraryChanged();
}
void AirfoilPanel::selectStation(int index) {
  if (!active_ || sketching_ || index < 0) return;
  const auto& stations = view_.sketchEditor().stationEditor().lines();
  if (index >= static_cast<int>(stations.size())) return;
  const auto assignment = stations[index].airfoil;
  if (assignment && *assignment < library_.entries().size()) {
    chosen_ = static_cast<int>(*assignment); group_->button(chosen_)->setChecked(true);
  } else if (chosen_ >= 0) view_.sketchEditor().stationEditor().assignSelectedAirfoil(chosen_);
}
void AirfoilPanel::toggleSketch(bool enabled) {
  if (!enabled) { finishSketch(); return; }
  bool ok = false;
  const auto name = QInputDialog::getText(this, "Sketch Airfoil", "Airfoil name:", QLineEdit::Normal, {}, &ok).trimmed();
  if (!ok || name.isEmpty()) { updateControls(); return; }
  draftName_ = name;
  auto& editor = view_.airfoilSketchEditor();
  draft_ = static_cast<int>(editor.layers().size());
  if (draft_ == 1 && editor.layers().front().curves.empty()) draft_ = 0;
  else editor.setLayerCount(draft_ + 1);
  editor.setActiveLayer(draft_); editor.setTool(SketchTool::None);
  view_.sketchEditor().stationEditor().setSelectionEnabled(false);
  sketching_ = true; editor.setEditing(true); updateControls();
  for (auto* button : tools_->findChildren<QPushButton*>()) button->setChecked(false);
}
void AirfoilPanel::finishSketch(bool warn) {
  if (!sketching_) return;
  sketching_ = false;
  auto& editor = view_.airfoilSketchEditor(); editor.finish(); editor.setEditing(false);
  QString error;
  const bool valid = library_.addSketch(draftName_, editor.layers().at(draft_), error);
  if (!valid) {
    if (draft_ == 0) editor.reset(); else editor.setLayerCount(draft_);
  }
  updateControls();
  view_.sketchEditor().stationEditor().setSelectionEnabled(active_);
  if (valid) addLibraryButton();
  else if (warn) QMessageBox::warning(this, "Invalid Airfoil Sketch", error);
}
bool AirfoilPanel::allStationsAssigned() const {
  const auto& stations = view_.sketchEditor().stationEditor().lines();
  if (!view_.sketchEditor().stationEditor().allPanelsDefined()) return false;
  for (const auto& station : stations)
    if (!station.airfoil || *station.airfoil >= library_.entries().size()) return false;
  return true;
}
void AirfoilPanel::reset() {
  sketching_ = false; active_ = false; view_.airfoilSketchEditor().reset(); library_.clear(); chosen_ = -1;
  draft_=0;draftName_.clear();panelChoices_.assign(1,-1);panel_=0;
  {QSignalBlocker block{tabs_};while(tabs_->count()>1)tabs_->removeTab(tabs_->count()-1);tabs_->setCurrentIndex(0);}
  for (auto* button : group_->buttons()) { group_->removeButton(button); delete button; }
  updateControls(); emit libraryChanged();
}
void AirfoilPanel::restoreState(const AirfoilState& state) {
  for(auto* button:group_->buttons()) {group_->removeButton(button);delete button;}
  setPanelCount(static_cast<int>(view_.sketchEditor().layers().size()));
  panelChoices_=state.panelChoices;panelChoices_.resize(view_.sketchEditor().layers().size(),state.chosen);
  panel_=std::clamp(state.panel,0,static_cast<int>(panelChoices_.size())-1);
  {QSignalBlocker block{tabs_};tabs_->setCurrentIndex(panel_);}
  library_.restore(state.entries); chosen_=state.chosen; draft_=state.draft;
  sketching_=state.sketching; draftName_=state.draftName;
  for(int i=0;i<static_cast<int>(state.entries.size());++i) {
    auto* button=new QRadioButton{state.entries[i].name,list_};
    button->setObjectName(QString{"airfoilChoice%1"}.arg(i)); group_->addButton(button,i);
    listLayout_->insertWidget(listLayout_->count()-1,button); button->setChecked(i==chosen_);
  }
  for(auto* button:tools_->findChildren<QPushButton*>())
    button->setChecked(button->property("sketchTool").toInt()==static_cast<int>(view_.airfoilSketchEditor().tool()));
  updateControls();
}
}

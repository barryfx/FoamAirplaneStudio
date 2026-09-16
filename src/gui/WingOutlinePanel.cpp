#include "gui/WingOutlinePanel.h"
#include "gui/SketchEditor.h"
#include <QSpinBox>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <algorithm>
namespace designrc::gui {
WingOutlinePanel::WingOutlinePanel(SketchEditor& editor, QWidget* parent)
    : QWidget{parent}, editor_{editor}, count_{new QSpinBox{this}}, tabs_{new QTabWidget{this}} {
  setObjectName("wingOutlinePanel");
  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0, 0, 0, 0);
  auto* label = new QLabel{"Number of wing panels", this};
  count_->setObjectName("wingPanelCount"); count_->setRange(1, 100); label->setBuddy(count_);
  count_->setToolTip("Reducing the count removes the highest-numbered panels and their outlines.");
  layout->addWidget(label); layout->addWidget(count_); layout->addWidget(tabs_, 1);
  tabs_->setObjectName("wingPanelTabs"); tabs_->setUsesScrollButtons(true);
  connect(count_, &QSpinBox::valueChanged, this, [this](int n) {
    editor_.setLayerCount(n); rebuildTabs(n);
  });
  connect(tabs_, &QTabWidget::currentChanged, this, [this](int i) {
    editor_.setActiveLayer(i);
    for (auto* button : tabs_->findChildren<QPushButton*>()) {
      QSignalBlocker block{button};
      button->setChecked(button->property("sketchTool").toInt() == static_cast<int>(editor_.tool()));
    }
  });
  rebuildTabs(1);
}
void WingOutlinePanel::rebuildTabs(int count) {
  const int selected = std::clamp(tabs_->currentIndex(), 0, count - 1);
  QSignalBlocker block{tabs_};
  while (tabs_->count()) { auto* page = tabs_->widget(0); tabs_->removeTab(0); delete page; }
  for (int i = 0; i < count; ++i) {
    auto* page = new QWidget{tabs_};
    auto* layout = new QVBoxLayout{page};
    auto* text = new QLabel{"Create the outline of the right half wing panel planform including only the leading and trailing edges connected by the tip contour for the last panel.", page};
    text->setWordWrap(true); layout->addWidget(text);
    auto* buttons = new QHBoxLayout;
    for (auto tool : {SketchTool::Line, SketchTool::Spline}) {
      auto* button = new QPushButton{tool == SketchTool::Line ? "Line" : "Spline", page};
      button->setCheckable(true); button->setProperty("sketchTool", static_cast<int>(tool));
      button->setChecked(editor_.tool() == tool); buttons->addWidget(button);
      connect(button, &QPushButton::clicked, this, [this, tool](bool checked) {
        editor_.setTool(checked ? tool : SketchTool::None);
        for (auto* other : tabs_->findChildren<QPushButton*>()) {
          QSignalBlocker guard{other};
          other->setChecked(other->property("sketchTool").toInt() == static_cast<int>(editor_.tool()));
        }
      });
    }
    layout->addLayout(buttons);
    auto* hint = new QLabel{"Escape finishes a spline. With both tools off, click a curve to select it, then press Delete to remove it. Drag a point to move it.", page};
    hint->setWordWrap(true); layout->addWidget(hint); layout->addStretch();
    tabs_->addTab(page, QString::number(i + 1));
  }
  tabs_->setCurrentIndex(selected); editor_.setActiveLayer(selected);
}
void WingOutlinePanel::reset() {
  editor_.reset(); QSignalBlocker block{count_}; count_->setValue(1); rebuildTabs(1);
}
void WingOutlinePanel::restoreControls() {
  const auto saved=editor_.state();
  QSignalBlocker blockCount{count_},blockTabs{tabs_};
  count_->setValue(static_cast<int>(saved.layers.size())); rebuildTabs(count_->value());
  tabs_->setCurrentIndex(saved.active); editor_.restoreState(saved);
}
bool WingOutlinePanel::outlinesDefined() const {
  for (int i = 0; i < static_cast<int>(editor_.layers().size()); ++i)
    if (!panelOutlineDefined(i)) return false;
  return !editor_.layers().empty();
}
bool WingOutlinePanel::panelOutlineDefined(int index) const {
  const auto& layers = editor_.layers();
  if (index < 0 || index >= static_cast<int>(layers.size())) return false;
    const auto& layer = layers[index];
    if (layer.curves.empty()) return false;
    std::vector<std::vector<std::size_t>> adjacent(layer.points.size());
    for (const auto& curve : layer.curves)
      for (std::size_t j = 1; j < curve.points.size(); ++j) {
        const auto a = curve.points[j - 1], b = curve.points[j];
        adjacent[a].push_back(b); adjacent[b].push_back(a);
      }
    int components = 0;
    std::vector<bool> visited(adjacent.size());
    for (std::size_t j = 0; j < adjacent.size(); ++j) {
      if (adjacent[j].size() > 2 || adjacent[j].empty()) return false;
      if (visited[j]) continue;
      ++components;
      int ends = 0;
      std::vector<std::size_t> stack{j};
      while (!stack.empty()) {
        const auto node = stack.back(); stack.pop_back();
        if (visited[node]) continue;
        visited[node] = true;
        if (adjacent[node].size() == 1) ++ends;
        for (auto next : adjacent[node]) if (!visited[next]) stack.push_back(next);
      }
      // Each independent LE/TE series must itself be an open chain.
      if (ends != 2) return false;
    }
    // Inner panels have separate leading/trailing chains; the last has a tip
    // joining them. This checks connectivity, not aerodynamic validity.
    const bool last = index + 1 == static_cast<int>(layers.size());
    return components == (last ? 1 : 2) && (!last || layer.points.size() >= 3);
}
}

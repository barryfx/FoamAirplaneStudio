#include "gui/StabilizerOutlinePanel.h"
#include "gui/SketchEditor.h"
#include "gui/SketchBoundary.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace designrc::gui {
QString stabilizerOutlineWarning(const SketchLayer& layer) {
  if (layer.curves.empty()) return {};
  const QString chainWarning = "Join the line/spline segments into one continuous open outline with two ends. "
      "Do not close the root edge.";
  std::vector<std::vector<std::size_t>> adjacent(layer.points.size());
  for (const auto& curve : layer.curves) {
    if (curve.points.size() < 2) return chainWarning;
    for (std::size_t i = 1; i < curve.points.size(); ++i) {
      const auto a = curve.points[i - 1], b = curve.points[i];
      if (a >= adjacent.size() || b >= adjacent.size() || a == b) return chainWarning;
      adjacent[a].push_back(b); adjacent[b].push_back(a);
    }
  }
  std::vector<std::size_t> ends;
  std::size_t used = 0;
  for (std::size_t i = 0; i < adjacent.size(); ++i) {
    if (adjacent[i].size() > 2) return chainWarning;
    if (!adjacent[i].empty()) ++used;
    if (adjacent[i].size() == 1) ends.push_back(i);
  }
  if (ends.size() != 2) return chainWarning;
  std::vector<bool> visited(adjacent.size());
  std::vector<std::size_t> stack{ends.front()};
  std::size_t count = 0;
  while (!stack.empty()) {
    const auto node = stack.back(); stack.pop_back();
    if (visited[node]) continue;
    visited[node] = true; ++count;
    for (auto next : adjacent[node]) if (!visited[next]) stack.push_back(next);
  }
  if (count != used) return chainWarning;
  const auto delta = layer.points[ends[1]] - layer.points[ends[0]];
  const double dx = std::abs(delta.x()), dy = std::abs(delta.y());
  if (dx == 0 && dy == 0)
    return "The outline endpoints coincide. Move them apart to define an open root edge.";
  // Fold into the nearest axis: endpoint order, scale and 90-degree rotations
  // do not affect the result. Roundoff allowance includes the exact 10-degree limit.
  const double angle = std::atan2(std::min(dx, dy), std::max(dx, dy));
  if (angle > 10.0 * std::numbers::pi / 180.0 + 1e-12)
    return "The line through the outline endpoints must be within 10 degrees of horizontal or vertical "
        "on the reference drawing. Return to Outline and adjust the endpoints.";
  return {};
}

bool stabilizerOutlineDefined(const SketchLayer& layer) {
  if (layer.curves.empty() || !stabilizerOutlineWarning(layer).isEmpty() ||
      !layer.leadingEdge || !isSketchEndpoint(layer,*layer.leadingEdge)) return false;
  std::vector<int> degree(layer.points.size());
  for (const auto& curve : layer.curves)
    for (std::size_t i = 1; i < curve.points.size(); ++i) { ++degree[curve.points[i-1]]; ++degree[curve.points[i]]; }
  std::vector<std::size_t> ends;
  for (std::size_t i = 0; i < degree.size(); ++i) if (degree[i] == 1) ends.push_back(i);
  if (ends.size() != 2) return false;
  auto closed = layer; closed.curves.push_back({SketchTool::Line, {ends[0], ends[1]}});
  return closedSketchBoundary(closed).has_value();
}

StabilizerOutlinePanel::StabilizerOutlinePanel(SketchEditor& editor, bool horizontal, QWidget* parent)
    : QWidget{parent}, editor_{editor}, component_{horizontal ? "Horizontal stabilizer" : "Vertical stabilizer"} {
  setObjectName(horizontal ? "horizontalStabilizerOutlinePanel" : "verticalStabilizerOutlinePanel");
  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0, 0, 0, 0);
  const QString componentText = horizontal
      ? "Trace the right half of the horizontal stabilizer, including the elevator. The right half will be mirrored "
        "to create the left half. Cuts can be made later for rudder clearance in the elevators or other openings. "
      : "Trace the complete vertical stabilizer, including the rudder. The vertical stabilizer will not be mirrored. "
        "Cuts can be made later for clearance or other openings. ";
  auto* instructions = new QLabel{componentText +
      "Create one long, continuous open outline using a fitted spline or connected line/spline segments, "
      "following the leading edge, tip and trailing edge. Leave the root edge open. The line through both endpoints must be "
      "within 10 degrees of horizontal or vertical on the reference drawing, so drawings rotated 90 degrees are supported. "
      "Choose Line for two-point segments or Spline for a curve through several points. Escape finishes a spline. "
      "Tools stay on until clicked again. With both tools off, drag a point to move it, or click a curve and press "
      "Delete to remove it. Nearby points snap together; Escape clears selection. The outline is checked when "
      "leaving Outline or entering 3D. Click Select Leading Edge End Point, then click the open root endpoint "
      "on the leading-edge side. This selection is required before the outline is valid; the selected end is marked LE.", this};
  instructions->setObjectName("stabilizerOutlineInstructions");
  instructions->setWordWrap(true); layout->addWidget(instructions);
  auto* row = new QHBoxLayout;
  for (auto tool : {SketchTool::Line, SketchTool::Spline}) {
    auto* button = new QPushButton{tool == SketchTool::Line ? "Line" : "Spline", this};
    button->setCheckable(true); button->setProperty("sketchTool", static_cast<int>(tool));
    row->addWidget(button);
    connect(button, &QPushButton::clicked, this, [this, tool](bool checked) {
      editor_.setTool(checked ? tool : SketchTool::None); restoreControls();
    });
  }
  layout->addLayout(row);
  auto* leading = new QPushButton{"Select Leading Edge End Point",this};
  leading->setObjectName("selectStabilizerLeadingEdge");leading->setCheckable(true);
  layout->addWidget(leading);
  connect(leading,&QPushButton::clicked,this,[this](bool checked){editor_.setSelectingLeadingEdge(checked);restoreControls();});
  connect(&editor_,&SketchEditor::changed,this,[this]{restoreControls();});
  layout->addStretch();
}
void StabilizerOutlinePanel::restoreControls() {
  for (auto* button : findChildren<QPushButton*>()) {
    QSignalBlocker block{button};
    button->setChecked(button->objectName()=="selectStabilizerLeadingEdge" ? editor_.selectingLeadingEdge()
        : button->property("sketchTool").toInt() == static_cast<int>(editor_.tool()));
  }
}
void StabilizerOutlinePanel::setActive(bool active, bool warn) {
  if (!active && !active_ && !editor_.state().editing) return;
  const bool leaving = active_ && !active;
  active_ = active;
  editor_.setEditing(active); restoreControls();
  if (leaving && warn) {
    const auto& layer=editor_.layers().front();
    auto warning = stabilizerOutlineWarning(layer);
    if(warning.isEmpty() && !layer.curves.empty() && (!layer.leadingEdge || !isSketchEndpoint(layer,*layer.leadingEdge)))
      warning="Click Select Leading Edge End Point, then click the open endpoint on the leading-edge side.";
    if (!warning.isEmpty()) QMessageBox::warning(this, component_ + " outline",
        warning + " Your sketch has been retained.");
  }
}
void StabilizerOutlinePanel::reset() {
  active_ = false; editor_.reset(); restoreControls();
}
}

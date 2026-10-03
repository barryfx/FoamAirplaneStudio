#pragma once

class QToolBar;

namespace designrc::gui {

// Completion comes from the component model/editor, never from clicking a tool.
struct WingDefinitionState {
  bool outlineDefined{};
  bool stationsDefined{};
  bool airfoilsDefined{};
  bool dihedralDefined{};
};

// Configure the Wing actions. Earlier steps remain editable; every later
// step requires the complete prerequisite chain. Returns the selected tool.
int applyWingWorkflow(QToolBar& toolbar, const WingDefinitionState& state);
}

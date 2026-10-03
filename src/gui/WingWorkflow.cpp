#include "gui/WingWorkflow.h"
#include <QAction>
#include <QActionGroup>
#include <QToolBar>
#include <array>

namespace designrc::gui {
int applyWingWorkflow(QToolBar& toolbar, const WingDefinitionState& state) {
  const auto actions = toolbar.actions();
  if (actions.size() != 7 && actions.size() != 8) return -1;
  const bool outline = state.outlineDefined;
  const bool stations = outline && state.stationsDefined;
  const bool airfoils = stations && state.airfoilsDefined;
  const bool tip = airfoils && state.dihedralDefined;
  const std::array<bool, 8> enabled{true, outline, stations, airfoils, tip, tip, tip,outline};
  auto* group = toolbar.findChild<QActionGroup*>("wingToolSelection", Qt::FindDirectChildrenOnly);
  if (!group) {
    group = new QActionGroup{&toolbar};
    group->setObjectName("wingToolSelection");
    group->setExclusive(true);
  }
  for (int i = 0; i < actions.size(); ++i) {
    auto* action = actions[i];
    group->addAction(action);
    action->setCheckable(true);
    action->setEnabled(enabled[i]);
    if (!enabled[i]) action->setChecked(false);
  }
  // Follow the sequential prerequisites; after Dihedral all three optional
  // tools become available together, without choosing one on the user's behalf.
  const int selected = !outline ? 0 : !stations ? 1 : !airfoils ? 2 : 3;
  actions[selected]->setChecked(true);
  return selected;
}
}

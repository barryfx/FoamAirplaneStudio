#include "gui/WingWorkflow.h"
#include <QApplication>
#include <QAction>
#include <QToolBar>
#include <array>
#include "TestCheck.h"

int main(int argc, char** argv) {
  QApplication app{argc, argv};
  using namespace designrc::gui;
  QToolBar toolbar;
  auto populate = [&] {
    toolbar.clear();
    for (const auto* name : {"Outline", "Airfoil Stations", "Airfoils", "Dihedral",
                             "Ailerons/Flaps", "Spars", "Lightening"})
      toolbar.addAction(name);
  };
  populate();
  auto check = [&](WingDefinitionState state, std::array<bool, 7> expected, int selected) {
    TEST_CHECK(applyWingWorkflow(toolbar, state) == selected);
    for (int i = 0; i < 7; ++i) {
      TEST_CHECK(toolbar.actions()[i]->isEnabled() == expected[i]);
      TEST_CHECK(toolbar.actions()[i]->isChecked() == (i == selected));
    }
  };
  check({}, {true,false,false,false,false,false,false}, 0);
  // Selecting a tool does not define geometry or unlock a dependent tool.
  toolbar.actions()[0]->trigger();
  TEST_CHECK(!toolbar.actions()[1]->isEnabled());
  check({true}, {true,true,false,false,false,false,false}, 1);
  check({true,true}, {true,true,true,false,false,false,false}, 2);
  check({true,true,true}, {true,true,true,true,false,false,false}, 3);
  check({true,true,true,true}, {true,true,true,true,true,true,true}, 3);
  for (int i : {4,5,6}) {
    toolbar.actions()[i]->trigger();
    TEST_CHECK(toolbar.actions()[i]->isChecked());
    TEST_CHECK(!toolbar.actions()[3]->isChecked());
  }
  // Upstream removal locks every downstream action even if stale flags remain.
  check({false,true,true,true}, {true,false,false,false,false,false,false}, 0);
  check({true,false,true,true}, {true,true,false,false,false,false,false}, 1);
  // Rebuilding the toolbar after a workspace switch retains model completion.
  populate();
  check({true,true}, {true,true,true,false,false,false,false}, 2);
  check({}, {true,false,false,false,false,false,false}, 0);
}

#pragma once
#include <QWidget>
class QSpinBox;
class QTabWidget;
namespace designrc::gui {
class SketchEditor;
class WingOutlinePanel final : public QWidget {
public:
  explicit WingOutlinePanel(SketchEditor& editor, QWidget* parent = nullptr);
  void reset();
  void restoreControls();
  bool outlinesDefined() const;
  bool panelOutlineDefined(int index) const;
private:
  void rebuildTabs(int count);
  SketchEditor& editor_;
  QSpinBox* count_;
  QTabWidget* tabs_;
};
}

#pragma once
#include <QWidget>
#include <QString>

namespace designrc::gui {
class SketchEditor;
struct SketchLayer;

// Both components use the same open-chain drawing and endpoint checks.
bool stabilizerOutlineDefined(const SketchLayer& layer);
QString stabilizerOutlineWarning(const SketchLayer& layer);

class StabilizerOutlinePanel final : public QWidget {
public:
  StabilizerOutlinePanel(SketchEditor& editor, bool horizontal, QWidget* parent = nullptr);
  void setActive(bool active, bool warn = true);
  void restoreControls();
  void reset();
private:
  SketchEditor& editor_;
  QString component_;
  bool active_ = false;
};
}

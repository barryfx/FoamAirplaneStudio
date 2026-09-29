#pragma once
#include <QWidget>
#include <QStringList>
#include <array>
#include <functional>
class QPushButton;
class QRadioButton;
namespace designrc::gui {
class SketchEditor;
class FuselageOutlinePanel final : public QWidget {
public:
  explicit FuselageOutlinePanel(SketchEditor& editor, QWidget* parent = nullptr);
  void setActive(bool active, bool warn = true);
  void restoreControls(int view);
  void reset();
  void setEnds(bool noseOpen,bool tailOpen);
  bool noseOpen() const;
  bool tailOpen() const;
  std::function<void()> endsChanged;
  int selectedView() const { return view_; }
  bool outlinesDefined() const;
  QStringList invalidViews() const;
  std::function<void()> drawingRequested;
private:
  void syncControls();
  SketchEditor& editor_;
  std::array<QPushButton*,2> views_{};
  std::array<QWidget*,2> tools_{};
  std::array<QRadioButton*,2> openEnds_{}, closedEnds_{};
  int view_ = -1;
  bool active_ = false;
};
}

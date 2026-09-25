#pragma once
#include "gui/AirfoilLibrary.h"
#include <QWidget>
class QPushButton;
class QButtonGroup;
class QVBoxLayout;
class QShortcut;
class QTabBar;
namespace designrc::gui {
class PlanViewport;
struct AirfoilState {
  std::vector<LibraryAirfoil> entries;
  int chosen{-1}, draft{};
  bool sketching{};
  QString draftName;
  int panel=0;
  std::vector<int> panelChoices{-1};
};
class AirfoilPanel final : public QWidget {
  Q_OBJECT
public:
  explicit AirfoilPanel(PlanViewport& view, QWidget* parent = nullptr);
  void setActive(bool active);
  void reset();
  AirfoilState state() const;
  void setPanelCount(int count);
  void restoreState(const AirfoilState& state);
  const AirfoilLibrary& library() const { return library_; }
  bool allStationsAssigned() const;
  bool loadAirfoil(const QString& path, QString& error);
signals:
  void libraryChanged();
private:
  void toggleSketch(bool enabled);
  void finishSketch(bool warn = true);
  void addLibraryButton();
  void selectStation(int index);
  void updateControls();
  PlanViewport& view_;
  AirfoilLibrary library_;
  QTabBar* tabs_{};
  std::vector<int> panelChoices_{-1};
  int panel_{};
  QButtonGroup* group_{};
  QWidget* description_{};
  QPushButton* load_{};
  QPushButton* export_{};
  QPushButton* smooth_{};
  QPushButton* sketchButton_{};
  QWidget* tools_{};
  QWidget* list_{};
  QVBoxLayout* listLayout_{};
  QShortcut* escape_{};
  bool active_{}, sketching_{};
  int chosen_{-1}, draft_{};
  QString draftName_;
};
}

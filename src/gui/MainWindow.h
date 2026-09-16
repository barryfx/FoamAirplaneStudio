#pragma once

#include <QMainWindow>
#include <QPointer>
#include "processing/BackgroundJob.h"
#include <TopoDS_Shape.hxx>
#include "gui/WingWorkflow.h"
#include "gui/ProjectDocument.h"

class QTabWidget;
class QTabBar;
class QToolBar;
class QWidget;
class QCloseEvent;
class QAction;
class QPushButton;

namespace designrc::gui {

class OcctViewport;
class PlanViewport;
class ReferencePanel;
class WingOutlinePanel;
class AirfoilPanel;
class ControlSurfacePanel;
class SparPanel;
class DihedralPanel;
class LighteningPanel;
struct ProjectReference;
enum class CameraView;

class MainWindow final : public QMainWindow {
public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;
  const ProjectReference& projectReference() const;
  void setWingDefinitionState(const WingDefinitionState& state);
  ProjectDocument projectDocument() const;
  bool saveProjectFile(const QString& path,QString& error);
  bool openProjectFile(const QString& path,QString& error);
  bool projectModified() const;

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void buildMenus();
  void buildToolBars();
  void selectWorkspace(int index);
  void updateEditorVisibility();
  void updateWorkspaceAvailability();
  void invalidateWing();
  void updateWingModel();
  void pollModelJob();
  void setModelProcessing(bool active);
  void setCameraView(CameraView cameraView);
  void newProject();
  void resetProject();
  void closeProject();
  void openProject();
  bool saveProject(bool saveAs=false);
  bool maybeSaveProject();
  void restoreProject(const ProjectDocument& project);
  QByteArray projectFingerprint() const;
  QByteArray wingFingerprint() const;
  void updateProjectTitle();
  void openHelp();
  void showAbout();
  void copyFocusedText();
  void pasteFocusedText();

  WingDefinitionState wingDefinitions_;
  QWidget* dataPanel_{};
  QWidget* dataContents_{};
  QPushButton* cancelProcessing_{};
  std::unique_ptr<processing::BackgroundJob<TopoDS_Shape>> modelJob_;
  std::size_t projectEpoch_=0,jobEpoch_=0;
  bool closingAfterProcessing_=false;
  std::vector<std::pair<QPointer<QAction>,bool>> processingActions_;
  ReferencePanel* referencePanel_{};
  WingOutlinePanel* wingOutlinePanel_{};
  QWidget* stationPanel_{};
  QTabBar* stationTabs_{};
  void updatePanelCounts();
  AirfoilPanel* airfoilPanel_{};
  DihedralPanel* dihedralPanel_{};
  LighteningPanel* lighteningPanel_{};
  ControlSurfacePanel* controlSurfacePanel_{};
  SparPanel* sparPanel_{};
  bool wingDirty_ = true, wingHasView_ = false;
  bool restoringProject_ = false, projectOpen_ = true;
  QString projectPath_;
  QByteArray savedFingerprint_, builtWingFingerprint_, jobFingerprint_;
  QAction *saveAction_{}, *saveAsAction_{}, *closeAction_{};
  std::optional<CameraState> restoredWingCamera_;
  QTabWidget* graphicsTabs_{};
  QToolBar* workspaceToolBar_{};
  QToolBar* componentToolBar_{};
  OcctViewport* viewport_{};
  PlanViewport* planViewport_{};
};

} // namespace designrc::gui


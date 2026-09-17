#pragma once

#include <QMainWindow>
#include <QPointer>
#include "processing/BackgroundJob.h"
#include <TopoDS_Shape.hxx>
#include "gui/WingWorkflow.h"
#include "gui/ProjectDocument.h"
#include "geometry/FuselageSolidBuilder.h"

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
class FuselageOutlinePanel;
class FuselageProfilePanel;
class FuselageThickenPanel;
class FuselageCutPanel;
class ServoTrayPanel;
class FormerPanel;
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
  const TopoDS_Shape& servoTrayTopFaces() const { return servoTrayTopFaces_; }
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
  void updateFuselageModel();
  void pollFuselageJob();
  QByteArray fuselageFingerprint() const;
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
  FuselageOutlinePanel* fuselageOutlinePanel_{};
  FuselageProfilePanel* fuselageProfilePanel_{};
  FuselageThickenPanel* fuselageThickenPanel_{};
  FuselageCutPanel* fuselageCutPanel_{};
  ServoTrayPanel* servoTrayPanel_{};
  FormerPanel* formerPanel_{};
  double fuselageWingLeadingEdge() const;
  std::unique_ptr<processing::BackgroundJob<geometry::FuselageBuildResult>> fuselageJob_;
  QByteArray builtFuselageFingerprint_, fuselageJobFingerprint_;
  std::size_t fuselageJobEpoch_{};
  TopoDS_Shape wingShape_, fuselageShape_, servoTrayTopFaces_;
  int displayedComponent_=-1;
  QWidget* fuselageStationPanel_{};
  void updateFuselageStationMode();
  void updateFuselageProgress();
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
  std::optional<CameraState> restoredWingCamera_, restoredFuselageCamera_;
  QTabWidget* graphicsTabs_{};
  QToolBar* workspaceToolBar_{};
  QToolBar* componentToolBar_{};
  OcctViewport* viewport_{};
  PlanViewport* planViewport_{};
};

} // namespace designrc::gui


#pragma once
#include "gui/ReferenceImage.h"
#include "gui/SparState.h"
#include "gui/LighteningState.h"
#include "gui/AirfoilPanel.h"
#include "gui/PlanViewport.h"
#include "gui/OcctViewport.h"
#include <QJsonObject>
#include "gui/AssemblyState.h"
namespace designrc::gui {
struct ProjectDocument {
  AssemblyState assembly;
  ProjectReference reference;
  QString wingspanText, fuselageText;
  SketchState wing, airfoilSketch;
  std::array<SketchState, 2> stabilizerOutlines;
  std::array<SketchState,2> stabilizerHinges;
  std::array<SketchState,2> stabilizerCuts;
  std::array<HingeCut,2> stabilizerHingeCuts{HingeCut::Tape,HingeCut::Tape};
  std::array<std::optional<domain::AirfoilProfile>, 2> stabilizerAirfoils;
  SketchState fuselage{std::vector<SketchLayer>(2)};
  SketchState fuselageProfiles;
  SketchState fuselageCuts{std::vector<SketchLayer>(2)};
  SketchState fuselageHoles{std::vector<SketchLayer>(4)};
  ServoTrayState servoTray;
  FormerState formers;
  bool fuselageThickening=false;
  int fuselageView=-1;
  StationState stations, fuselageStations;
  AirfoilState airfoils;
  ControlSurfaceState controls;
  PanelSpars spars{1};
  LighteningState lightening;
  int selectedSparPanel=0, selectedStationPanel=0;
  std::vector<double> dihedralDegrees{0};
  int selectedDihedralPanel=0;
  int workspace{}, viewport{};
  QString tool;
  PlanViewState plan;
  std::optional<CameraState> camera;
  std::vector<int> splitterSizes;
};
// Complete, versioned files. A lightweight representation is used only for
// in-process dirty comparison; it is never written as a project file.
QJsonObject encodeProject(const ProjectDocument& project, bool embedImages=true);
ProjectDocument decodeProject(const QJsonObject& json);
bool writeProject(const QString& path,const ProjectDocument& project,QString& error);
std::optional<ProjectDocument> readProject(const QString& path,QString& error);
}

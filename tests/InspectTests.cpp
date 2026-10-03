#include "LegacyProject.h"
#include "gui/MainWindow.h"
#include "gui/InspectPanel.h"
#include "gui/ReferencePanel.h"
#include "WaitForModel.h"
#include "gui/ExportPanel.h"
#include "gui/FileSelectionDialog.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <STEPControl_Reader.hxx>
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>
#include <QScreen>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <iostream>
#include <stdexcept>
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__));}while(false)
namespace designrc::gui {
static TopoDS_Shape box(double x,double y,double z,double dx,double dy,double dz) {
  auto s=BRepPrimAPI_MakeBox{gp_Pnt{x,y,z},dx,dy,dz}.Shape();BRepMesh_IncrementalMesh mesh{s,.1};return s;
}
static SketchLayer rectangle(double x,double y,double width,double height) {
  return {{{x,y},{x+width,y},{x+width,y+height},{x,y+height}},{{SketchTool::Line,{0,1}},{SketchTool::Line,{1,2}},{SketchTool::Line,{2,3}},{SketchTool::Line,{3,0}}}};
}
class InspectTest {
public:
  static void run(const QString& capture) {
    MainWindow w;w.resize(1200,760);w.move(w.screen()->availableGeometry().topLeft()+QPoint{10,10});w.show();QApplication::processEvents();
    CHECK(w.workspaceToolBar_->actions()[6]->text()=="Inspect");
    w.workspaceToolBar_->actions()[6]->trigger();QApplication::processEvents();
    CHECK(w.inspectPanel_->isVisible());CHECK(w.graphicsTabs_->currentIndex()==1);CHECK(!w.graphicsTabs_->isTabEnabled(0));
    CHECK(w.inspectPanel_->visibleShapes().empty());CHECK(!w.modelJob_&&!w.fuselageJob_&&!w.assemblyPrepareJob_);
    // A cached wing has a root datum for its Assembly rotation pivot even
    // when the rest of the airplane has not been defined. Keep airfoils unassigned
    // so this synthetic-cache fixture does not trigger wing generation.
    auto fixture=w.projectDocument();fixture.reference.wingspanMm=200;fixture.wingspanText="200";
    fixture.wing.layers[0]=rectangle(10,10,100,30);fixture.wing.layers[0].curves.pop_back();
    fixture.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,40}},LineAlignment::Vertical,std::nullopt},
        {{0,0,1,{110,10}},{0,2,0,{110,40}},LineAlignment::Vertical,std::nullopt}};
    w.restoreProject(fixture);
    // Individual generated caches can be inspected without a complete Assembly.
    w.wingShape_=box(60,-80,10,40,160,8);w.builtWingFingerprint_=w.wingFingerprint();w.updateInspect(true);
    CHECK(w.inspectPanel_->visibleShapes().size()==1);CHECK(!w.exportAssemblyParts());
    auto* edit=w.findChild<QLineEdit*>("inspectName0");CHECK(edit->text()=="Wing");
    edit->setText("Main Wing");QMetaObject::invokeMethod(edit,"editingFinished");CHECK(w.projectModified());
    const auto wingId=edit->property("componentId").toString();
    CHECK(w.inspectPanel_->names().value(wingId)=="Main Wing");
    w.builtWingFingerprint_="stale";w.updateInspect();CHECK(w.inspectPanel_->visibleShapes().empty());
    w.builtWingFingerprint_=w.wingFingerprint();w.updateInspect();
    CHECK(w.findChild<QLineEdit*>("inspectName0")->text()=="Main Wing");
    // Synthetic Assembly snapshot: no component generation is invoked.
    auto document=w.projectDocument();document.reference.toScale=true;
    document.reference.image.physicalSizeMm=QSizeF{400,100};
    document.reference.image.pages.push_back({QImage{400,100,QImage::Format_RGB32},QSizeF{400,100}});
    document.reference.image.pages[0].pixels.fill(Qt::white);
    document.fuselage.layers={rectangle(0,0,200,40),rectangle(0,60,200,40)};
    w.restoreProject(document);
    geometry::AssemblyParts sample;
    sample.fuselage=box(0,-20,-20,200,40,40);sample.wing=box(60,-80,10,40,160,8);
    sample.horizontal=box(165,-45,5,25,90,4);sample.vertical=box(165,-2,5,25,4,35);
    sample.inserts.push_back({"Former 1",box(20,-18,-18,2,36,36),gp_Pln{gp_Pnt{21,0,0},gp_Dir{1,0,0}},"Former/0"});
    sample.inserts.push_back({"Servo Tray",box(70,-15,-5,35,30,3),{},"Servo Tray"});
    w.assemblyOriginals_=sample;w.assemblyState_.positioned=true;w.assemblySourceFingerprint_=w.assemblyFingerprint();
    w.updateWorkspaceAvailability();w.workspaceToolBar_->actions()[6]->trigger();QApplication::processEvents();
    const auto count=geometry::assemblyExportParts(sample).size();CHECK(count==6);
    CHECK(w.inspectPanel_->visibleShapes().size()==count);CHECK(w.viewport_->displayedShapes_.size()==count);
    CHECK(w.viewport_->property("assemblyReferencePages").toInt()==0);
    auto camera=w.viewport_->cameraState();CHECK(camera);
    auto* check=w.findChild<QCheckBox*>("inspectVisible0");CHECK(check->isChecked());check->click();
    CHECK(w.viewport_->displayedShapes_.size()==count-1);
    CHECK(w.viewport_->cameraState()->eye==camera->eye);CHECK(w.viewport_->cameraState()->scale==camera->scale);
    for(auto* item:w.inspectPanel_->findChildren<QCheckBox*>())item->setChecked(false);
    CHECK(w.viewport_->displayedShapes_.empty());
    for(auto* item:w.inspectPanel_->findChildren<QCheckBox*>())item->setChecked(true);
    CHECK(w.viewport_->displayedShapes_.size()==count);
    edit=w.findChild<QLineEdit*>("inspectName0");CHECK(edit->text()=="Former 1");
    edit->setText("Nose Former");QMetaObject::invokeMethod(edit,"editingFinished");
    for(const auto& invalid:{QString{},QString{"../bad"},QString{"CON"},QString{"Main Wing"}}) {
      edit->setText(invalid);QMetaObject::invokeMethod(edit,"editingFinished");CHECK(edit->text()=="Nose Former");
    }
    // Checkboxes are transient; names are project data and survive save/open.
    QTemporaryDir output;QString error;const auto project=output.filePath("Test Aircraft.foam");
    CHECK(w.saveProjectFile(project,error));CHECK(!w.projectModified());
    check->click();CHECK(!w.projectModified());check->click();
    const auto source=w.assemblySourceFingerprint_;CHECK(source==w.assemblyFingerprint());
    auto* controls=w.findChild<QToolBar*>("viewportViewControls");CHECK(controls->isVisible());controls->actions()[2]->trigger();controls->actions()[1]->trigger();
    CHECK(!w.graphicsTabs_->isTabEnabled(0));
    QMetaObject::invokeMethod(edit,"editingFinished"); // Clear the deliberate validation error before capture.
    if(!capture.isEmpty()) {
      QApplication::processEvents();CHECK(w.screen()->grabWindow(w.winId()).save(capture));
      auto* fuselageCheck=w.findChild<QCheckBox*>("inspectVisible1");fuselageCheck->click();
      QApplication::processEvents();CHECK(w.screen()->grabWindow(w.winId()).save(capture+"-hidden.png"));fuselageCheck->click();
    }
    w.workspaceToolBar_->actions()[8]->trigger();QApplication::processEvents();
    CHECK(w.viewport_->displayedShapes_.size()>0);CHECK(!w.inspectPanel_->isVisible());
    CHECK(w.findChild<QCheckBox*>("exportAll")->isChecked());const auto selected=w.exportPanel_->selectedParts();
    CHECK(selected.size()==count);CHECK(selected[0].name=="Nose Former");
    bool renamed=false;for(const auto& part:selected)if(part.id==wingId.toStdString()){CHECK(part.name=="Main Wing");renamed=true;}CHECK(renamed);
    const auto filenames=geometry::exportFileNames(selected,geometry::FormerExportFormat::Dxf,geometry::ComponentExportFormat::Stl,w.exportProjectName());
    CHECK(std::find(filenames.begin(),filenames.end(),"Main Wing.stl")!=filenames.end());
    CHECK(std::find(filenames.begin(),filenames.end(),"Nose Former.dxf")!=filenames.end());
    // Exercise the real folder dialog, combined STEP writer and project filename.
    QTimer accept;QString exportError;
    QObject::connect(&accept,&QTimer::timeout,[&]{
      if(auto* message=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){exportError=message->text();message->accept();}
      if(auto* dialog=dynamic_cast<FileSelectionDialog*>(QApplication::activeModalWidget())){
        dialog->setDirectory(output.path());dialog->selectFile(output.path());QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
      }
    });accept.start(10);w.exportComponents();accept.stop();CHECK(exportError.isEmpty());
    QFile step{output.filePath("Test Aircraft.step")};CHECK(step.open(QIODevice::ReadOnly));const auto text=step.readAll();
    CHECK(text.contains("Main Wing")&&text.contains("Nose Former")&&text.contains("Test Aircraft"));
    CHECK(!QFile::exists(output.filePath("Components.step")));
    STEPControl_Reader reader;CHECK(reader.ReadFile(step.fileName().toStdString().c_str())==IFSelect_RetDone);CHECK(reader.TransferRoots()>0);
    geometry::writeComponentExports(selected,geometry::FormerExportFormat::Dxf,geometry::ComponentExportFormat::Stl,std::filesystem::path{output.path().toStdWString()},w.exportProjectName());
    CHECK(QFile::exists(output.filePath("Nose Former.dxf")));CHECK(QFile::exists(output.filePath("Main Wing.stl")));
    w.workspaceToolBar_->actions()[6]->trigger();CHECK(w.findChild<QLineEdit*>("inspectName0")->text()=="Nose Former");
    CHECK(w.openProjectFile(project,error));CHECK(!w.projectModified());CHECK(w.inspectPanel_->names().value(wingId)=="Main Wing");
    CHECK(!w.assemblyPrepareJob_&&!w.modelJob_&&!w.fuselageJob_);CHECK(!w.exportAssemblyParts());
    w.resetProject();CHECK(w.inspectPanel_->names().empty());CHECK(w.exportProjectName()=="Untitled");
    // Inspect uses the existing worker to refresh a changed Wing, without requiring
    // Fuselage or stabilizer definitions or running their generation suites.
    if(QApplication::arguments().contains("--gui-only"))return;
    ProjectDocument wingProject;wingProject.reference.wingspanMm=200;wingProject.wingspanText="200";
    wingProject.wing.layers[0]=rectangle(10,10,100,30);wingProject.wing.layers[0].curves.pop_back();
    wingProject.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,40}},LineAlignment::Vertical,0},
        {{0,0,1,{110,10}},{0,2,0,{110,40}},LineAlignment::Vertical,0}};
    wingProject.airfoils.entries.push_back({"NACA",domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
    w.restoreProject(wingProject);w.workspaceToolBar_->actions()[6]->trigger();CHECK(w.assemblyPrepareJob_);waitForModel(w);
    CHECK(!w.wingShape_.IsNull());CHECK(w.builtWingFingerprint_==w.wingFingerprint());
    CHECK(w.inspectPanel_->visibleShapes().size()>0);CHECK(w.fuselageShape_.IsNull());
    const auto firstWing=w.wingShape_;w.updateInspect();CHECK(!w.assemblyPrepareJob_);CHECK(w.wingShape_.IsSame(firstWing));
    auto* generatedName=w.findChild<QLineEdit*>("inspectName0");generatedName->setText("Renamed generated wing");QMetaObject::invokeMethod(generatedName,"editingFinished");
    auto reference=w.projectReference();reference.wingspanMm=220;w.referencePanel_->restoreReference(reference);
    w.updateInspect(true);CHECK(w.assemblyPrepareJob_);w.assemblyPrepareJob_->cancel();waitForModel(w);
    CHECK(w.wingShape_.IsSame(firstWing));CHECK(!QApplication::overrideCursor());
    w.updateInspect(true);CHECK(w.assemblyPrepareJob_);waitForModel(w);
    CHECK(!w.wingShape_.IsSame(firstWing));CHECK(w.builtWingFingerprint_==w.wingFingerprint());
    CHECK(w.findChild<QLineEdit*>("inspectName0")->text()=="Renamed generated wing");
    CHECK(w.inspectPanel_->isVisible()&&!w.graphicsTabs_->isTabEnabled(0));
    w.resetProject();
    auto json=encodeProject(document);CHECK(json["version"]==33);json["version"]=26;legacyCutViews(json);json.remove("componentNames");
    auto ui=json["ui"].toObject();ui["workspace"]=0;json["ui"]=ui;
    CHECK(decodeProject(json).componentNames.empty());
    auto invalid=encodeProject(document);invalid["componentNames"]=QJsonObject{{"Wing/solid/0","../escape"}};
    bool rejected=false;try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    auto duplicate=selected;duplicate[1].name=duplicate[0].name;rejected=false;
    try{geometry::exportFileNames(duplicate,geometry::FormerExportFormat::Step,geometry::ComponentExportFormat::Step,"Plane");}
    catch(const std::exception&){rejected=true;}CHECK(rejected);
  }
};
}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QTemporaryDir settings;QSettings::setDefaultFormat(QSettings::IniFormat);
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());app.setOrganizationName("InspectTests");app.setApplicationName("InspectTests");
  try {designrc::gui::InspectTest::run(argc>1?QString::fromLocal8Bit(argv[1]):QString{});std::cout<<"Inspect and renamed export checks passed\n";return 0;}
  catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

#pragma once
#include "gui/ExportPanel.h"
#include "gui/FileSelectionDialog.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <STEPControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <QCheckBox>
#include <QRadioButton>
#include <QDir>
#include <QElapsedTimer>
#include <QThread>
#include <fstream>
#include <numbers>
#include <cstring>

namespace designrc::gui {
class ExportWorkflowTest {
public:
  static void run(const QString& evidence) {
    QTemporaryDir outputs;CHECK(outputs.isValid());
    MainWindow w;w.resize(1400,900);w.show();QApplication::processEvents();
    CHECK(!w.workspaceToolBar_->actions()[6]->isEnabled());
    auto document=fixture();
    // Deliberately add the aft former first. At .25 mm/scene unit their
    // centers are model X=35 and X=10. The first has a rotated local plane.
    document.formers.rectangles={{336,193,8,64},{236,193,8,64}};
    document.formers.rotationDegrees={25,0};
    w.restoreProject(document);
    auto sample=parts();sample.rudder=box(100,-2,5,5,4,15);
    AssemblyWorkflowTest::install(w,sample);
    gp_Trsf rotation;rotation.SetRotation(gp_Ax1{gp_Pnt{35,0,0},gp_Dir{0,1,0}},25*std::numbers::pi/180.);
    const auto aft=BRepBuilderAPI_Transform{box(34,-8,-8,2,16,16),rotation,true}.Shape();
    const auto nose=box(9,-8,-8,2,16,16);
    const auto tray=box(40,-8,6,8,16,2);
    w.fuselageModel_.formers={aft,nose};
    w.fuselageModel_.servoTray=tray;
    BRep_Builder builder;TopoDS_Compound all;builder.MakeCompound(all);
    builder.Add(all,sample.fuselage);builder.Add(all,aft);builder.Add(all,nose);
    builder.Add(all,tray);
    w.fuselageModel_.shape=all;w.fuselageShape_=all;
    CHECK(!w.workspaceToolBar_->actions()[6]->isEnabled()); // Cached components alone are insufficient.
    w.workspaceToolBar_->actions()[5]->trigger();waitForModel(w);
    CHECK(w.workspaceToolBar_->actions()[6]->isEnabled());
    CHECK(w.exportAssemblyParts()->fuselage.IsSame(sample.fuselage));
    CHECK(w.exportAssemblyParts()->inserts.size()==3);
    auto catalog=geometry::assemblyExportParts(*w.exportAssemblyParts());
    CHECK(catalog[0].name=="Former 1"&&catalog[1].name=="Former 2");
    CHECK(catalog[0].shape.IsSame(nose)&&catalog[1].shape.IsSame(aft));
    CHECK(std::abs(catalog[0].formerPlane->Location().X()-10)<1e-9);
    w.assemblyState_.offsets={};w.assemblyCutButton_->click();waitForModel(w);
    CHECK(w.assemblyState_.cuts);
    catalog=geometry::assemblyExportParts(*w.exportAssemblyParts());
    CHECK(catalog[0].shape.IsSame(nose)&&catalog[1].shape.IsSame(aft));
    CHECK(w.exportAssemblyParts()->inserts.size()==3);
    int trays=0;
    for(const auto& part:catalog)if(part.name=="Servo Tray") {++trays;CHECK(part.shape.IsSame(tray));}
    CHECK(trays==1); // Seat cutters overlap inserts but must never modify or join them.
    CHECK(volume(w.exportAssemblyParts()->fuselage)<volume(sample.fuselage));
    CHECK(volume(w.fuselageModel_.formers[0])==volume(aft));
    w.workspaceToolBar_->actions()[6]->trigger();QApplication::processEvents();
    CHECK(w.exportPanel_->isVisible()&&!w.graphicsTabs_->isTabEnabled(0));
    CHECK(w.findChild<QRadioButton*>("formersStep")->isChecked());
    CHECK(w.exportPanel_->formerFormat()==geometry::FormerExportFormat::Step);
    CHECK(!w.findChild<QRadioButton*>("formersDxf")->isChecked());
    CHECK(w.componentToolBar_->isHidden());
    auto* exportButton=w.findChild<QPushButton*>("exportComponents");CHECK(!exportButton->isEnabled());
    auto* allBox=w.findChild<QCheckBox*>("exportAll");allBox->click();
    CHECK(w.exportPanel_->selectedParts().size()==catalog.size()&&exportButton->isEnabled());
    w.findChild<QCheckBox*>("exportPart0")->click();CHECK(!allBox->isChecked());
    CHECK(w.exportPanel_->selectedParts().size()==catalog.size()-1);
    allBox->click();allBox->click();CHECK(w.exportPanel_->selectedParts().empty());allBox->click();
    w.findChild<QRadioButton*>("formersStl")->click();
    CHECK(!w.findChild<QRadioButton*>("formersDxf")->isChecked());
    CHECK(w.findChild<QRadioButton*>("componentsStep")->isChecked());
    w.findChild<QRadioButton*>("componentsStl")->click();
    CHECK(!w.findChild<QRadioButton*>("componentsStep")->isChecked());
    w.findChild<QRadioButton*>("formersStep")->click();
    CHECK(w.exportPanel_->formerFormat()==geometry::FormerExportFormat::Step);
    CHECK(!w.findChild<QRadioButton*>("formersDxf")->isChecked());
    CHECK(!w.findChild<QRadioButton*>("formersStl")->isChecked());
    CHECK(w.findChild<QRadioButton*>("componentsStl")->isChecked());
    w.findChild<QRadioButton*>("formersDxf")->click();w.findChild<QRadioButton*>("componentsStep")->click();
    // Drive the real directory dialog and exporter. Test settings are isolated.
    QTimer accept;bool accepted=false;QString exportError;
    QObject::connect(&accept,&QTimer::timeout,[&]{
      if(auto* message=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
        exportError=message->text();message->accept();return;
      }
      if(auto* dialog=dynamic_cast<FileSelectionDialog*>(QApplication::activeModalWidget())) {
        dialog->setDirectory(outputs.path());dialog->selectFile(outputs.path());
        QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);accepted=true;
      }
    });accept.start(10);w.exportComponents();accept.stop();
    if(!exportError.isEmpty())throw std::runtime_error(exportError.toStdString());CHECK(accepted);
    CHECK(QFile::exists(outputs.path()+"/Components.step"));
    CHECK(QFile::exists(outputs.path()+"/Former 1.dxf")&&QFile::exists(outputs.path()+"/Former 2.dxf"));
    CHECK(QDir{outputs.path()}.entryList(QDir::Files).size()==3);
    FileSelectionDialog remembered{&w,"componentExportDirectory","Export Components",QFileDialog::Directory};
    CHECK(remembered.directory().absolutePath()==QDir{outputs.path()}.absolutePath());
    QFile stepFile{outputs.path()+"/Components.step"};CHECK(stepFile.open(QIODevice::ReadOnly));
    const auto stepText=stepFile.readAll();CHECK(stepText.contains("FoamAirplaneStudio Assembly"));
    CHECK(stepText.contains("Fuselage 1")&&stepText.contains("Horizontal Stabilizer"));
    CHECK(stepText.contains("Servo Tray"));
    CHECK(!stepText.contains("Former 1"));
    STEPControl_Reader reader;CHECK(reader.ReadFile((outputs.path()+"/Components.step").toStdString().c_str())==IFSelect_RetDone);
    CHECK(reader.TransferRoots()>0);CHECK(BRepCheck_Analyzer{reader.OneShape()}.IsValid());
    double expected=0;int expectedSolids=0;
    for(const auto& part:catalog)if(!part.formerPlane){expected+=volume(part.shape);for(TopExp_Explorer e{part.shape,TopAbs_SOLID};e.More();e.Next())++expectedSolids;}
    CHECK(std::abs(volume(reader.OneShape())-expected)<1e-4);
    int actualSolids=0;for(TopExp_Explorer e{reader.OneShape(),TopAbs_SOLID};e.More();e.Next())++actualSolids;
    CHECK(actualSolids==expectedSolids);
    QFile dxf{outputs.path()+"/Former 1.dxf"};CHECK(dxf.open(QIODevice::ReadOnly));
    const auto dxfText=dxf.readAll();CHECK(dxfText.contains("LWPOLYLINE")&&dxfText.contains("$INSUNITS"));
    // Closed inner and outer contours, and rotation-independent physical area.
    auto plate=box(-1,-10,-15,2,20,30);
    auto hole=BRepPrimAPI_MakeCylinder{gp_Ax2{gp_Pnt{-2,0,0},gp_Dir{1,0,0}},3,4}.Shape();
    plate=BRepAlgoAPI_Cut{plate,hole}.Shape();
    gp_Trsf tilt;tilt.SetRotation(gp_Ax1{gp_Pnt{},gp_Dir{0,1,0}},-.4);
    gp_Pln plane{gp_Ax3{gp_Pnt{},gp_Dir{1,0,0},gp_Dir{0,1,0}}};plane.Transform(tilt);
    auto drawing=geometry::formerDrawing({"Former",BRepBuilderAPI_Transform{plate,tilt,true}.Shape(),plane});
    CHECK(drawing.paths.size()==2);double outer=0,inner=1e9;
    for(const auto& path:drawing.paths) {
      CHECK(path.closed);double area=0;
      for(std::size_t i=0;i<path.points.size();++i){auto a=path.points[i],b=path.points[(i+1)%path.points.size()];area+=a.x*b.y-b.x*a.y;}
      area=std::abs(area)/2;outer=std::max(outer,area);inner=std::min(inner,area);
    }
    CHECK(std::abs(outer-600)<1e-5&&std::abs(inner-9*std::numbers::pi)<.5);
    geometry::writeComponentExports(catalog,geometry::FormerExportFormat::Stl,geometry::ComponentExportFormat::Stl,
        std::filesystem::path{outputs.path().toStdWString()});
    for(const auto& part:catalog) {
      QFile stl{outputs.path()+"/"+QString::fromStdString(part.name)+".stl"};CHECK(stl.open(QIODevice::ReadOnly));
      const auto bytes=stl.readAll();CHECK(bytes.size()>84);std::uint32_t count;
      std::memcpy(&count,bytes.constData()+80,4);CHECK(count>0&&bytes.size()==84+50LL*count);
      double meshVolume=0;
      for(std::uint32_t i=0;i<count;++i) {
        float xyz[9];std::memcpy(xyz,bytes.constData()+84+50LL*i+12,sizeof xyz);
        const gp_Vec a{xyz[0],xyz[1],xyz[2]},b{xyz[3],xyz[4],xyz[5]},c{xyz[6],xyz[7],xyz[8]};
        meshVolume+=a.Dot(b.Crossed(c))/6;
      }
      CHECK(std::abs(std::abs(meshVolume)-volume(part.shape))<.01);
    }
    // Partial selection writes only its requested file; no component STEP.
    QTemporaryDir partial;
    geometry::writeComponentExports({catalog.front()},geometry::FormerExportFormat::Dxf,geometry::ComponentExportFormat::Step,
        std::filesystem::path{partial.path().toStdWString()});
    CHECK(QDir{partial.path()}.entryList(QDir::Files)==QStringList{"Former 1.dxf"});
    // STEP formers share one file with STEP components, or coexist with
    // individual component STLs. Also cover selecting only formers/components.
    using geometry::FormerExportFormat;using geometry::ComponentExportFormat;
    for(const auto format:{ComponentExportFormat::Step,ComponentExportFormat::Stl}) {
      for(int selection=0;selection<3;++selection) {
        std::vector<geometry::ExportPart> selected;
        for(const auto& part:catalog)
          if(selection==0||(selection==1&&part.formerPlane)||(selection==2&&!part.formerPlane))selected.push_back(part);
        QTemporaryDir folder;CHECK(folder.isValid());
        const auto names=geometry::exportFileNames(selected,FormerExportFormat::Step,format);
        geometry::writeComponentExports(selected,FormerExportFormat::Step,format,
            std::filesystem::path{folder.path().toStdWString()});
        QStringList expectedFiles;double expectedVolume=0;int expectedCount=0;
        for(const auto& name:names)expectedFiles.append(QString::fromStdString(name));
        expectedFiles.sort();CHECK(QDir{folder.path()}.entryList(QDir::Files,QDir::Name)==expectedFiles);
        for(const auto& part:selected)if(part.formerPlane||format==ComponentExportFormat::Step) {
          expectedVolume+=volume(part.shape);
          for(TopExp_Explorer e{part.shape,TopAbs_SOLID};e.More();e.Next())++expectedCount;
        }
        if(expectedCount) {
          STEPControl_Reader combined;
          CHECK(combined.ReadFile((folder.path()+"/Components.step").toStdString().c_str())==IFSelect_RetDone);
          CHECK(combined.TransferRoots()>0);CHECK(BRepCheck_Analyzer{combined.OneShape()}.IsValid());
          CHECK(std::abs(volume(combined.OneShape())-expectedVolume)<1e-4);
          int count=0;for(TopExp_Explorer e{combined.OneShape(),TopAbs_SOLID};e.More();e.Next())++count;
          CHECK(count==expectedCount);
          QFile file{folder.path()+"/Components.step"};CHECK(file.open(QIODevice::ReadOnly));
          const auto content=file.readAll();
          for(const auto& part:selected)if(part.formerPlane||format==ComponentExportFormat::Step)
            CHECK(content.contains(QByteArray::fromStdString(part.name)));
        }
      }
    }
    // Directory cancellation creates no files and changes no history.
    QTimer cancel;QObject::connect(&cancel,&QTimer::timeout,[&]{if(auto* dialog=dynamic_cast<FileSelectionDialog*>(QApplication::activeModalWidget()))dialog->reject();});
    cancel.start(10);w.exportComponents();cancel.stop();
    QString error;CHECK(w.saveProjectFile(outputs.path()+"/export.foam",error));
    QApplication::processEvents();
    if(auto* screen=w.screen())CHECK(screen->grabWindow(w.winId()).save(evidence+"/export-panel.png"));
    // Saved Export mode opens in 2D without rebuilding or enabling stale exports.
    CHECK(w.openProjectFile(outputs.path()+"/export.foam",error));CHECK(w.graphicsTabs_->currentIndex()==0);
    CHECK(!w.workspaceToolBar_->actions()[6]->isEnabled()&&!w.property("modelProcessing").toBool());
    AssemblyWorkflowTest::install(w,sample);w.assemblyOriginals_=sample;w.assemblyState_.cuts=false;
    w.assemblySourceFingerprint_=w.assemblyFingerprint();
    w.updateExportAvailability();CHECK(w.workspaceToolBar_->actions()[6]->isEnabled());
    w.assemblySourceFingerprint_="obsolete";w.invalidateAssembly();CHECK(!w.workspaceToolBar_->actions()[6]->isEnabled());
    w.resetProject();CHECK(!w.workspaceToolBar_->actions()[6]->isEnabled());
  }
};
}

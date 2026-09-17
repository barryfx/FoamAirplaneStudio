#include "gui/MainWindow.h"
#include "gui/FuselageThickenPanel.h"
#include "geometry/FuselageSolidBuilder.h"
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <QApplication>
#include <QAction>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include "gui/LengthEntry.h"
#include <QJsonArray>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolBar>
#include <QScreen>
#include <iostream>
#include <stdexcept>
using namespace designrc;
using namespace designrc::gui;
#define CHECK(c) do {if(!(c))throw std::runtime_error(std::string{#c}+" at "+std::to_string(__LINE__));}while(false)
SketchLayer polygon(std::vector<QPointF> p) {SketchLayer l{p,{}};for(std::size_t i=0;i<p.size();++i)l.curves.push_back({SketchTool::Line,{i,(i+1)%p.size()}});return l;}
SketchLayer rectangle(double x,double y,double w,double h){return polygon({{x,y},{x+w,y},{x+w,y+h},{x,y+h}});}
bool inside(const TopoDS_Shape& shape,double x,double y,double z){return BRepClass3d_SolidClassifier{shape,gp_Pnt{x,y,z},1e-6}.State()==TopAbs_IN;}
int main(int argc,char** argv) {
  QApplication app{argc,argv};QTemporaryDir dir;
  QCoreApplication::setOrganizationName("FoamThickenTests");QCoreApplication::setApplicationName("FoamThickenTests");
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
  try {
    ProjectDocument p;p.reference.units=ProjectUnits::Inches;p.reference.wingspanMm=1000;p.reference.fuselageLengthMm=700;p.wingspanText="1000 mm";p.fuselageText="700 mm";
    p.wing.layers[0]=rectangle(10,10,90,70);p.wing.layers[0].curves.pop_back();
    p.stations.lines={{{0,0,0,{10,10}},{0,2,1,{10,80}},LineAlignment::Vertical,0},{{0,0,1,{100,10}},{0,2,0,{100,80}},LineAlignment::Vertical,0}};
    p.airfoils.entries.push_back({"NACA",domain::AirfoilProfile::nacaSymmetric(.12),{},{}});
    p.fuselage.layers={rectangle(200,150,400,100),polygon({{200,350},{350,350},{420,350},{600,350},{600,430},{200,430}})};
    p.fuselageStations.lines={{{1,0,1,{350,350}},{1,4,.625,{350,430}},LineAlignment::Vertical},{{1,2,4./9,{500,350}},{1,4,.25,{500,430}},LineAlignment::Vertical}};
    p.fuselageStations.lines.push_back({{1,0,1./3,{250,350}},{1,4,.875,{250,430}},LineAlignment::Vertical});
    p.fuselageProfiles.layers={rectangle(680,140,100,80),rectangle(680,300,100,80),rectangle(810,140,60,60)};
    p.fuselageStations.lines[2].profile=2;
    p.fuselageStations.lines[0].profile=0;p.fuselageStations.lines[1].profile=1;p.workspace=2;p.tool="Edit Profiles";
    QString error;const auto file=dir.filePath("thicken.foam");CHECK(writeProject(file,p,error));
    MainWindow window;window.show();app.processEvents();CHECK(window.openProjectFile(file,error));
    auto* tabs=window.findChild<QTabWidget*>("viewportTabs");auto* view=static_cast<PlanViewport*>(tabs->widget(0));
    auto* toolbar=window.findChild<QToolBar*>("componentToolBar");toolbar->actions()[3]->trigger();app.processEvents();app.processEvents();
    CHECK(window.projectDocument().fuselageThickening&&window.projectModified());
    auto field=[&](int index){return window.findChild<QLineEdit*>(QString{"fuselageThickness%1"}.arg(index));};
    auto commit=[&](QString text){auto* edit=field(0);edit->setText(text);edit->setModified(true);QMetaObject::invokeMethod(edit,"editingFinished");app.processEvents();app.processEvents();};
    CHECK(field(0)&&field(1)&&field(2)&&field(0)->isVisible());
    auto snapshot=window.projectDocument();
    CHECK(snapshot.fuselageStations.lines[0].thicknessMm==8);
    CHECK(snapshot.fuselageStations.lines[1].thicknessMm==5);
    CHECK(snapshot.fuselageStations.lines[2].thicknessMm==8);
    CHECK(field(0)->text()==formattedLength(8,ProjectUnits::Inches));
    auto* panel=window.findChild<QWidget*>("fuselageThickenPanel");
    auto* rows=panel->findChild<QFormLayout*>();CHECK(rows&&rows->rowCount()==3);
    for(int i=0;i<3;++i)CHECK(qobject_cast<QLabel*>(rows->itemAt(i,QFormLayout::LabelRole)->widget())->text()==QString{"Station %1"}.arg(i+1));
    CHECK(rows->itemAt(0,QFormLayout::FieldRole)->widget()==field(2));
    CHECK(rows->itemAt(1,QFormLayout::FieldRole)->widget()==field(0));
    CHECK(rows->itemAt(2,QFormLayout::FieldRole)->widget()==field(1));
    commit("0.25");CHECK(std::abs(*window.projectDocument().fuselageStations.lines[0].thicknessMm-6.35)<1e-9);
    commit("8 mm");CHECK(window.projectDocument().fuselageStations.lines[0].thicknessMm==8);
    auto* units=window.findChild<QComboBox*>("projectUnits");CHECK(units);units->setCurrentIndex(0);app.processEvents();
    CHECK(field(0)->text()=="8 mm");
    commit(".5 in");CHECK(window.projectDocument().fuselageStations.lines[0].thicknessMm==12.7);
    commit("7.5");CHECK(window.projectDocument().fuselageStations.lines[0].thicknessMm==7.5);
    for(const auto& invalid:{"bad","0 mm","-1 in","10001 mm"}){commit(invalid);CHECK(window.projectDocument().fuselageStations.lines[0].thicknessMm==7.5);}
    units->setCurrentIndex(1);app.processEvents();CHECK(field(0)->text()==formattedLength(7.5,ProjectUnits::Inches));
    commit("8.25 mm");CHECK(window.projectDocument().fuselageStations.lines[0].thicknessMm==8.25);
    auto visibleProfiles=[&] {
      view->fitInView(QRectF{0,0,900,500},Qt::KeepAspectRatio);app.processEvents();const auto pixels=view->viewport()->grab().toImage();
      for(auto point:{QPointF{730,140},QPointF{730,300}}) {
        const auto local=view->mapFromScene(point);const QPoint at{qRound(local.x()*pixels.devicePixelRatio()),qRound(local.y()*pixels.devicePixelRatio())};bool blue=false;
        for(int dy=-3;dy<=3;++dy)for(int dx=-3;dx<=3;++dx)if(pixels.rect().contains(at+QPoint{dx,dy})) {
          const auto c=pixels.pixelColor(at+QPoint{dx,dy});if(c.blue()>c.red()+40&&c.green()>c.red()+30)blue=true;
        }
        if(!blue){window.grab().save("build/debug/fuselage-visibility-failure.png");std::cerr<<"mapped "<<at.x()<<","<<at.y()<<" image "<<pixels.width()<<","<<pixels.height()<<std::endl;}CHECK(blue);
      }
    };
    visibleProfiles();toolbar->actions()[4]->trigger();visibleProfiles();toolbar->actions()[2]->trigger();visibleProfiles();
    CHECK(window.findChild<QLabel*>("fuselageSelectedStation")->text()=="Station 2 selected");
    CHECK(window.projectDocument().fuselageThickening);CHECK(window.saveProjectFile(file,error));CHECK(!window.projectModified());
    toolbar->actions()[3]->trigger();CHECK(!window.projectModified());CHECK(window.openProjectFile(file,error));
    CHECK(window.projectDocument().fuselageThickening&&window.projectDocument().fuselageStations.lines[0].thicknessMm==8.25);
    app.processEvents();app.processEvents();CHECK(field(0)->text()==formattedLength(8.25,ProjectUnits::Inches));
    auto& source=view->fuselageSketchEditor();source.mapPoints([](QPointF pt){return pt+QPointF{1,0};});
    CHECK(source.stationEditor().lines()[0].thicknessMm==8.25);CHECK(window.saveProjectFile(file,error));
    toolbar=window.findChild<QToolBar*>("componentToolBar");toolbar->actions()[3]->trigger();app.processEvents();app.processEvents();
    const auto capture=qEnvironmentVariable("FOAM_THICKEN_CAPTURE");if(!capture.isEmpty()){app.processEvents();CHECK(window.grab().save(capture));}
    auto encoded=encodeProject(window.projectDocument());CHECK(encoded["version"]==15);
    auto legacy=encoded;legacy["version"]=11;legacy.remove("fuselageThickening");CHECK(!decodeProject(legacy).fuselageThickening);CHECK(!decodeProject(legacy).fuselageStations.lines[0].thicknessMm);
    auto invalid=encoded;auto stations=invalid["fuselageStations"].toObject();auto lines=stations["lines"].toArray();auto record=lines[0].toObject();record["thicknessMm"]=-1;lines[0]=record;stations["lines"]=lines;invalid["fuselageStations"]=stations;
    bool rejected=false;try{decodeProject(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    if(app.arguments().contains("--ui-only")) {
      // A station just aft of a traced seat corner still represents the LE.
      p.fuselageStations.lines[0].first={1,1,1./70,{351,350}};
      p.fuselageStations.lines[0].second={1,4,249./400,{351,430}};
      CHECK(window.saveProjectFile(file,error));CHECK(writeProject(file,p,error));CHECK(window.openProjectFile(file,error));
      window.findChild<QToolBar*>("componentToolBar")->actions()[3]->trigger();
      CHECK(window.projectDocument().fuselageStations.lines[0].thicknessMm==8);
      // Outside the registration tolerance, an aft station gets 5 mm.
      p.fuselageStations.lines[0].first={1,1,3./70,{353,350}};
      p.fuselageStations.lines[0].second={1,4,247./400,{353,430}};
      CHECK(window.saveProjectFile(file,error));CHECK(writeProject(file,p,error));CHECK(window.openProjectFile(file,error));
      window.findChild<QToolBar*>("componentToolBar")->actions()[3]->trigger();
      CHECK(window.projectDocument().fuselageStations.lines[0].thicknessMm==5);
      const auto gentle=qEnvironmentVariable("FOAM_GENTLE_DEFAULTS_PROJECT");
      if(!gentle.isEmpty()) {
        CHECK(window.saveProjectFile(file,error));CHECK(window.openProjectFile(gentle,error));
        window.findChild<QToolBar*>("componentToolBar")->actions()[3]->trigger();
        const auto actual=window.projectDocument();
        CHECK(actual.fuselageStations.lines[0].thicknessMm==8);
        CHECK(actual.fuselageStations.lines[1].thicknessMm==8);
        CHECK(actual.fuselageStations.lines[2].thicknessMm==5);
        std::cout<<"GentleLady defaults: 8/8/5 mm; source file not saved\n";
      }
      std::cout<<"Fuselage inclusive LE defaults, station ordering, unit entry, visibility and persistence passed\n";return 0;
    }
    geometry::FuselageSolidInput input{{rectangle(0,0,200,40),rectangle(0,0,200,30)}, {},{rectangle(0,0,40,30)},200,true};
    ConstrainedLine a,b;a.first.position={50,0};a.profile=0;a.thicknessMm=8;b.first.position={150,0};b.profile=0;b.thicknessMm=5;auto nose=a;nose.first.position={0,0};auto tail=b;tail.first.position={200,0};input.stations={nose,a,b,tail};
    std::cout<<"Building variable wall fixture..."<<std::endl;
    auto shape=geometry::buildFuselageSolid(input,[](const char* m){std::cout<<m<<std::endl;});
    for(const auto& [x,wall]:std::vector<std::pair<double,double>>{{50,8},{100,6.5},{150,5}}) {
      CHECK(inside(shape,x,-20+wall-.2,0));CHECK(!inside(shape,x,-20+wall+.2,0));
    }
    CHECK(!inside(shape,.1,0,0));CHECK(!inside(shape,199.9,0,0));
    GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass);CHECK(mass.Mass()>0&&mass.Mass()<240000);
    std::cout<<"Building closed-end wall fixture..."<<std::endl;
    input.stations={a,b};shape=geometry::buildFuselageSolid(input);
    CHECK(inside(shape,1,0,0));CHECK(inside(shape,199,0,0));CHECK(!inside(shape,100,0,0));
    std::cout<<"Building open-nose, closed-tail fixture..."<<std::endl;
    input.stations={nose,a,b};shape=geometry::buildFuselageSolid(input);
    CHECK(!inside(shape,.1,0,0));CHECK(inside(shape,199,0,0));
    std::cout<<"Building closed-nose, open-tail fixture..."<<std::endl;
    input.stations={a,b,tail};shape=geometry::buildFuselageSolid(input);
    CHECK(inside(shape,1,0,0));CHECK(!inside(shape,199.9,0,0));
    if(argc>1) {
      const auto actual=readProject(QString::fromLocal8Bit(argv[1]),error);CHECK(actual);
      geometry::FuselageSolidInput gentle{actual->fuselage.layers,actual->fuselageStations.lines,actual->fuselageProfiles.layers,actual->reference.toScale?std::nullopt:actual->reference.fuselageLengthMm,true};
      for(auto& station:gentle.stations)station.thicknessMm=station.first.position.x()<253.173265724253?8.:5.;
      shape=geometry::buildFuselageSolid(gentle,[](const char* m){std::cout<<m<<std::endl;});
      OcctViewport viewer;viewer.resize(1200,800);viewer.show();app.processEvents();viewer.displayShape(shape);app.processEvents();
      if(argc>2)CHECK(viewer.screen()->grabWindow(viewer.winId()).save(QString::fromLocal8Bit(argv[2])));
    }
    std::cout<<"Profile visibility, wall defaults, persistence and smoothly varying inward walls passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

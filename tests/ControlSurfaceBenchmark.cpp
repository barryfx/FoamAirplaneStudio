// Manual benchmark: control_surface_benchmark OUTPUT_DIR [BASELINE_DIR].
// Geometry creation, serialization and parity checks are outside measured cuts.
#include "geometry/ControlSurfaceCut.h"
#include "geometry/WingSolidBuilder.h"
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace designrc;
int countSolids(const TopoDS_Shape& shape) {int n=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++n;return n;}
double volume(const TopoDS_Shape& shape) {GProp_GProps props;BRepGProp::VolumeProperties(shape,props,1e-7);return props.Mass();}
TopoDS_Shape read(const QString& path) {
  TopoDS_Shape s;BRep_Builder b;if(!BRepTools::Read(s,path.toLocal8Bit().constData(),b))throw std::runtime_error("Cannot read baseline BREP");return s;
}
int main(int argc,char** argv) {
  try {
    if(argc<2)throw std::runtime_error("Usage: control_surface_benchmark OUTPUT_DIR [BASELINE_DIR]");
    QDir out{QString::fromLocal8Bit(argv[1])};QDir{}.mkpath(out.path());
    QJsonArray results;
    for(int fixture=0;fixture<3;++fixture) {
      const QString name=fixture==0?"rectangular-tape":fixture==1?"rectangular-standard":"tapered-mixed";
      geometry::WingSolidInput wing;
      wing.panels={{{{0,0},{500,0},{500,200},{0,200}},
        {{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}}};
      wing.stations={{{0,0,0,{0,0}},{0,2,1,{0,200}},gui::LineAlignment::Vertical,0},
        {{0,0,1,{500,0}},{0,2,0,{500,200}},gui::LineAlignment::Vertical,0}};
      wing.airfoils.push_back({"NACA0012",domain::AirfoilProfile::nacaSymmetric(0.12),{}, {}});
      std::array<gui::ControlSurface,2> controls;
      controls[0]={true,fixture==1?gui::HingeCut::Standard:gui::HingeCut::Tape,QRectF{100,140,300,100}};
      if(fixture==2) {
        wing.panels[0].points[1]={500,30};wing.panels[0].points[2]={500,170};
        wing.stations[1].first.position={500,30};wing.stations[1].second.position={500,170};
        controls[0].rectangle=QRectF{280,140,190,100};
        controls[1]={true,gui::HingeCut::Standard,QRectF{30,140,200,100}};
      }
      const QString input=out.filePath(name+"-input.brep");
      if(argc>2) {
        if(!QFile::copy(QDir{QString::fromLocal8Bit(argv[2])}.filePath(name+"-input.brep"),input))
          throw std::runtime_error("Cannot copy baseline input; use a new output directory");
      } else {
        auto whole=geometry::buildWingSolid(wing);auto half=TopExp_Explorer{whole,TopAbs_SOLID}.Current();
        BRepTools::Clean(half);BRepTools::Write(half,input.toLocal8Bit().constData());
      }
      QJsonArray milliseconds;TopoDS_Shape result;
      for(int trial=0;trial<3;++trial) {
        auto half=read(input);
        const auto start=std::chrono::steady_clock::now();
        result=geometry::cutControlSurfaces(half,controls,{0,1},{1,0},0,1,0); // Exclude intentional end-clearance geometry changes from parity.
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        milliseconds.append(ms);std::cout<<name.toStdString()<<" trial "<<trial+1<<": "<<ms<<" ms"<<std::endl;
        if(!BRepCheck_Analyzer{result}.IsValid() || countSolids(result)!=(fixture==2?3:2))throw std::runtime_error("Invalid result bodies");
      }
      const double mass=volume(result);double difference=0;
      BRepTools::Write(result,out.filePath(name+"-result.brep").toLocal8Bit().constData());
      bool identical=false,tighterOnly=false;
      if(argc>2) {
        QFile baselineFile{QDir{QString::fromLocal8Bit(argv[2])}.filePath(name+"-result.brep")};
        QFile resultFile{out.filePath(name+"-result.brep")};
        if(!baselineFile.open(QIODevice::ReadOnly) || !resultFile.open(QIODevice::ReadOnly))throw std::runtime_error("Cannot compare BREP files");
        const auto before=baselineFile.readAll().split('\n'),after=resultFile.readAll().split('\n');
        identical=before==after;
        tighterOnly=before.size()==after.size();
        for(qsizetype i=0;tighterOnly && i<before.size();++i) {
          if(before[i]==after[i])continue;
          bool oldOk=false,newOk=false;
          const double oldTolerance=before[i].toDouble(&oldOk),newTolerance=after[i].toDouble(&newOk);
          tighterOnly=i>0 && before[i-1].trimmed()=="Ve" && oldOk && newOk && newTolerance>0 && newTolerance<=oldTolerance;
        }
      }
      if(argc>2 && !identical && !tighterOnly) {
        auto baseline=read(QDir{QString::fromLocal8Bit(argv[2])}.filePath(name+"-result.brep"));
        BRepAlgoAPI_Cut missing{baseline,result},extra{result,baseline};
        if(!missing.IsDone() || !extra.IsDone())throw std::runtime_error("Parity Boolean failed");
        difference=std::abs(volume(missing.Shape()))+std::abs(volume(extra.Shape()));
        if(difference>mass*1e-7 || std::abs(mass-volume(baseline))>mass*1e-7)throw std::runtime_error("Baseline volume/shape mismatch");
      }
      results.append(QJsonObject{{"case",name},{"milliseconds",milliseconds},{"solids",countSolids(result)},
        {"volumeMm3",mass},{"identicalBrep",identical},{"sameGeometryWithTighterVertexTolerances",tighterOnly},{"symmetricDifferenceMm3",difference}});
      QFile json{out.filePath("results.json")};if(!json.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot write results");
      json.write(QJsonDocument{results}.toJson());
    }
  }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

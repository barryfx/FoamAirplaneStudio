#include "geometry/SparCut.h"
#include "gui/ProjectDocument.h"
#include "gui/WingCalibration.h"
#include "gui/SketchBoundary.h"
#include "geometry/FuselageSolidBuilder.h"
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Standard_Failure.hxx>
#include <iostream>
#include <chrono>
#include <BRepTools.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <iomanip>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <BRep_Builder.hxx>
#include <algorithm>
#include <set>
#include <thread>
#include <stdexcept>
#include <cmath>
#include <condition_variable>
#include <mutex>
using namespace designrc;
#define CHECK(c) do{if(!(c))throw std::runtime_error(std::string{#c}+" line "+std::to_string(__LINE__));}while(false)
std::vector<TopoDS_Solid> bodies(const TopoDS_Shape& shape) {
  CHECK(BRepCheck_Analyzer{shape}.IsValid());std::vector<TopoDS_Solid> r;
  for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())r.push_back(TopoDS::Solid(e.Current()));return r;
}
int faces(const TopoDS_Shape& shape){int n=0;for(TopExp_Explorer e{shape,TopAbs_FACE};e.More();e.Next())++n;return n;}
double volume(const TopoDS_Shape& s){GProp_GProps p;BRepGProp::VolumeProperties(s,p);return p.Mass();}
int main(int argc,char** argv) {
  try {
    if(argc<2)throw std::runtime_error("Specify a .foam benchmark fixture.");
      QString error;auto project=gui::readProject(QString::fromLocal8Bit(argv[1]),error);
      if(!project)throw std::runtime_error(error.toStdString());
      std::optional<double> length;
      if(!project->reference.toScale) {
        const auto side=gui::closedSketchBoundary(project->fuselage.layers.at(1));
        CHECK(side);
        double left=side->front().x(),right=left;
        for(auto point:*side){left=std::min(left,point.x());right=std::max(right,point.x());}
        CHECK(project->reference.wingspanMm);
        length=(right-left)*gui::wingCalibration(project->wing.layers,project->stations.lines,
            project->reference.wingspanMm).scale;
      }
      geometry::FuselageSolidInput input{project->fuselage.layers,project->fuselageStations.lines,project->fuselageProfiles.layers,
        length,
        project->fuselageThickening,project->fuselageCuts.layers,project->servoTray.rectangle,project->formers.rectangles,project->formers.rotationDegrees,project->fuselageHoles.layers};
      input.stiffeners=project->stiffeners;
      input.noseOpen=project->fuselageNoseOpen;input.tailOpen=project->fuselageTailOpen;
      input.mirrorConstruction=!qEnvironmentVariableIsSet("FOAM_BENCH_FULL_SYMMETRIC");
      std::cout<<"Mirrored construction: "<<input.mirrorConstruction<<std::endl;
      const bool parallel=!qEnvironmentVariableIsSet("FOAM_BENCH_SERIAL");
      std::cout<<"Fuselage parallel mode: "<<parallel<<std::endl;
      const auto start=std::chrono::steady_clock::now();std::stop_source stop;
      const auto cancelStage=qgetenv("FOAM_BENCH_CANCEL_STAGE").toStdString();
      const int cancelDelay=qEnvironmentVariableIntValue("FOAM_BENCH_CANCEL_DELAY_MS");
      std::jthread canceller;std::chrono::steady_clock::time_point requested{};
      TopoDS_Shape shape;
      try {
        shape=geometry::buildFuselageModel(input,[&](const char* p) {
          std::cout<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"s "<<p<<std::endl;
          if(!cancelStage.empty()&&!canceller.joinable()&&std::string{p}.find(cancelStage)!=std::string::npos)
            canceller=std::jthread{[&](std::stop_token done) {
              std::mutex mutex;std::condition_variable_any wake;std::unique_lock lock{mutex};
              wake.wait_for(lock,done,std::chrono::milliseconds{std::max(0,cancelDelay)},[]{return false;});
              if(!done.stop_requested()){requested=std::chrono::steady_clock::now();stop.request_stop();}
            }};
        },{stop.get_token(),parallel}).shape;
      } catch(const geometry::ProcessingCancelled&) {
        if(cancelStage.empty())throw;
        if(canceller.joinable())canceller.join();
        CHECK(stop.stop_requested());
        std::cout<<"Cancellation acknowledged after "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-requested).count()<<" seconds; no result published."<<std::endl;
        return 0;
      }
      CHECK(cancelStage.empty());
      std::cout<<"Build seconds: "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<std::endl;

      Bnd_Box box;BRepBndLib::AddOptimal(shape,box,false,false);double a,b,c,d,e,f;box.Get(a,b,c,d,e,f);
      const double mass=volume(shape);
      std::cout<<std::setprecision(16)<<"Volume: "<<mass<<" Bounds: "<<a<<","<<b<<","<<c<<","<<d<<","<<e<<","<<f<<std::endl;
      if(const auto path=qgetenv("FOAM_BENCH_REFERENCE");!path.isEmpty()) {
        TopoDS_Shape reference;BRep_Builder builder;CHECK(BRepTools::Read(reference,path.constData(),builder));
        std::cout<<"Face counts baseline/current: "<<faces(reference)<<" / "<<faces(shape)<<std::endl;
        CHECK(bodies(reference).size()==bodies(shape).size());
        const double referenceMass=volume(reference);CHECK(std::abs(referenceMass-mass)<std::abs(referenceMass)*1e-6);
        Bnd_Box previous;BRepBndLib::AddOptimal(reference,previous,false,false);double bounds[6];previous.Get(bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5]);
        const double current[]{a,b,c,d,e,f};for(int n=0;n<6;++n)CHECK(std::abs(current[n]-bounds[n])<1e-5);
        IntCurvesFace_ShapeIntersector oldRay,newRay;oldRay.Load(reference,1e-7);newRay.Load(shape,1e-7);
        const auto hits=[](auto& ray,const gp_Lin& line,double extent) {
          ray.Perform(line,0,extent);CHECK(ray.IsDone());std::vector<double> values;
          for(int n=1;n<=ray.NbPnt();++n)values.push_back(ray.WParameter(n));
          std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end(),[](double u,double v){return std::abs(u-v)<1e-5;}),values.end());return values;
        };
        for(int axis=0;axis<3;++axis)for(int i=1;i<12;++i)for(int j=1;j<14;++j) {
          double point[]{a,b,c};const int u=(axis+1)%3,v=(axis+2)%3;
          point[axis]-=1;point[u]+=(current[u+3]-current[u])*(i-.17)/12;point[v]+=(current[v+3]-current[v])*(j-.23)/14;
          double direction[]{0,0,0};direction[axis]=1;
          gp_Lin line{gp_Pnt{point[0],point[1],point[2]},gp_Dir{direction[0],direction[1],direction[2]}};
          const auto oldHits=hits(oldRay,line,current[axis+3]-current[axis]+2),newHits=hits(newRay,line,current[axis+3]-current[axis]+2);
          CHECK(oldHits.size()==newHits.size());for(std::size_t n=0;n<oldHits.size();++n)CHECK(std::abs(oldHits[n]-newHits[n])<1e-5);
        }
        std::cout<<"Parity passed: solid count, volume, bounds, and 429 rays on three axes."<<std::endl;
      }
      if(const auto path=qgetenv("FOAM_BENCH_BREP");!path.isEmpty())BRepTools::Write(shape,path.constData());
      std::cout<<"Valid project solids: "<<bodies(shape).size()<<std::endl;return 0;
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

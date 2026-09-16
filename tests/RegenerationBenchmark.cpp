#include "geometry/SparCut.h"
#include "gui/ProjectDocument.h"
#include "geometry/WingSolidBuilder.h"
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
using namespace designrc;
#define CHECK(c) do{if(!(c))throw std::runtime_error(std::string{#c}+" line "+std::to_string(__LINE__));}while(false)
std::vector<TopoDS_Solid> bodies(const TopoDS_Shape& shape) {
  CHECK(BRepCheck_Analyzer{shape}.IsValid());std::vector<TopoDS_Solid> r;
  for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())r.push_back(TopoDS::Solid(e.Current()));return r;
}
double volume(const TopoDS_Shape& s){GProp_GProps p;BRepGProp::VolumeProperties(s,p);return p.Mass();}
int main(int argc,char** argv) {
  try {
    if(argc<2)throw std::runtime_error("Specify a .foam benchmark fixture.");
      QString error;auto project=gui::readProject(QString::fromLocal8Bit(argv[1]),error);
      if(!project)throw std::runtime_error(error.toStdString());
      geometry::WingSolidInput input{project->wing.layers,project->stations.lines,project->airfoils.entries,
        project->reference.toScale?std::nullopt:project->reference.wingspanMm,
        project->dihedralDegrees,project->controls.panels,project->spars,project->lightening};
      if(argc>2 && std::string{argv[2]}=="--surface-only")for(auto& panel:input.spars)panel[2].enabled=false;
      std::set<std::thread::id> panelThreads;
      const auto start=std::chrono::steady_clock::now();
      geometry::WingBuildOptions options;options.maxPanelThreads=qEnvironmentVariableIsSet("FOAM_BENCH_PARALLEL")?0:1;
      const auto shape=geometry::buildWingSolid(input,[&](const char* p){if(std::string_view{p}.starts_with("Wing panel "))panelThreads.insert(std::this_thread::get_id());std::cout<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"s "<<p<<std::endl;},options);
      std::cout<<"Build seconds: "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<std::endl;
      std::cout<<"Panel worker threads: "<<panelThreads.size()<<std::endl;
      Bnd_Box box;BRepBndLib::AddOptimal(shape,box,false,false);double a,b,c,d,e,f;box.Get(a,b,c,d,e,f);
      const double mass=volume(shape);
      std::cout<<std::setprecision(16)<<"Volume: "<<mass<<" Bounds: "<<a<<","<<b<<","<<c<<","<<d<<","<<e<<","<<f<<std::endl;
      if(const auto path=qgetenv("FOAM_BENCH_REFERENCE");!path.isEmpty()) {
        TopoDS_Shape reference;BRep_Builder builder;CHECK(BRepTools::Read(reference,path.constData(),builder));
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

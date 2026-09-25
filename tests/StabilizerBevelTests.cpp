#include "geometry/StabilizerSolidBuilder.h"
#include "gui/ProjectDocument.h"
#include "gui/WingCalibration.h"
#include <BRepCheck_Analyzer.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace designrc;
#define CHECK(c) do { if(!(c))throw std::runtime_error(#c); } while(false)

gui::SketchLayer layer(const QJsonObject& json) {
  gui::SketchLayer result;
  for(auto value:json["points"].toArray()){const auto p=value.toArray();result.points.push_back({p[0].toDouble(),p[1].toDouble()});}
  for(auto value:json["curves"].toArray()) {
    const auto c=value.toObject();gui::SketchCurve curve{static_cast<gui::SketchTool>(c["type"].toInt()),{}};
    for(auto id:c["points"].toArray())curve.points.push_back(id.toInt());result.curves.push_back(curve);
  }
  if(json.contains("leadingEdge"))result.leadingEdge=json["leadingEdge"].toInt();
  return result;
}

// Measure the generated surface, independently of the bevel-tool construction.
// Model XY is chord/span; a vertical fin maps (x,y,z) to (x,-z,y).
void checkBevel(const geometry::StabilizerSolidInput& input,const geometry::StabilizerBuildResult& result) {
  CHECK(!result.fixed.IsNull()&&!result.control.IsNull()&&BRepCheck_Analyzer{result.shape}.IsValid());
  const auto& outline=input.outline;std::vector<int> degree(outline.points.size());
  for(const auto& c:outline.curves)for(std::size_t i=1;i<c.points.size();++i){++degree[c.points[i-1]];++degree[c.points[i]];}
  const auto origin=outline.points[*outline.leadingEdge];QPointF chord;
  for(std::size_t i=0;i<degree.size();++i)if(degree[i]==1&&i!=*outline.leadingEdge)chord=outline.points[i]-origin;
  chord/=std::hypot(chord.x(),chord.y());QPointF span{-chord.y(),chord.x()};double extent=0;
  for(auto p:outline.points){const double d=QPointF::dotProduct(p-origin,span);if(std::abs(d)>std::abs(extent))extent=d;}
  if(extent<0)span=-span;
  const auto map=[&](QPointF p){p-=origin;return QPointF{QPointF::dotProduct(p,chord),QPointF::dotProduct(p,span)}*input.millimetersPerSceneUnit;};
  QPointF start,end;double best=-1;
  for(const auto& c:input.hingeLines.curves) {
    const auto a=map(input.hingeLines.points[c.points[0]]),b=map(input.hingeLines.points[c.points[1]]);
    const double length=QLineF{a,b}.length(),score=input.horizontal?length:std::abs(b.y()-a.y())/length;
    if(score>best){best=score;start=a;end=b;}
  }
  const auto tangent=(end-start)/QLineF{start,end}.length();QPointF inward{-tangent.y(),tangent.x()};if(inward.x()<0)inward=-inward;
  IntCurvesFace_ShapeIntersector fixed,control;fixed.Load(result.fixed,1e-7);control.Load(result.control,1e-7);
  const auto heights=[&](IntCurvesFace_ShapeIntersector& ray,QPointF q) {
    ray.Perform(input.horizontal?gp_Lin{gp_Pnt{q.x(),q.y(),-10000},gp_Dir{0,0,1}}:
      gp_Lin{gp_Pnt{q.x(),10000,q.y()},gp_Dir{0,-1,0}},0,20000);
    CHECK(ray.IsDone()&&ray.NbPnt()>=2);
    double low=10000,high=-10000;
    for(int i=1;i<=ray.NbPnt();++i){const double z=input.horizontal?ray.Pnt(i).Z():-ray.Pnt(i).Y();low=std::min(low,z);high=std::max(high,z);}
    CHECK(high>low);return std::pair{low,high};
  };
  for(double t:{.25,.5,.75}) {
    const auto p=start+(end-start)*t;
    const auto [low,high]=heights(fixed,p-inward*.01);
    const double thickness=high-low,d1=.15*thickness,d2=.3*thickness;
    const auto a=heights(control,p+inward*d1),b=heights(control,p+inward*d2);
    const double anchor=input.hingeCut==gui::HingeCut::Tape?high:(low+high)*.5;
    std::cout<<"  t="<<t<<" lower slope="<<(a.first-b.first)/(d2-d1)
      <<" lower anchor error="<<a.first+d1-anchor<<std::endl;
    CHECK(std::abs((a.first-b.first)/(d2-d1)-1)<.04);
    CHECK(std::abs(a.first+d1-anchor)<.04);
    if(input.hingeCut==gui::HingeCut::Standard) {
      std::cout<<"  upper slope="<<(b.second-a.second)/(d2-d1)<<std::endl;
      CHECK(std::abs((b.second-a.second)/(d2-d1)-1)<.04);
      CHECK(std::abs(a.second-d1-anchor)<.04);
    }
    // Check the mirrored elevator too, rather than just its original half.
    if(input.horizontal) {
      const auto q=p+inward*d1;const auto mirrored=heights(control,{q.x(),-q.y()});
      CHECK(std::abs(mirrored.first-a.first)<1e-5&&std::abs(mirrored.second-a.second)<1e-5);
    }
  }
}

int main(int argc,char** argv) {
  QApplication app{argc,argv};
  try {
    CHECK(argc==2);std::vector<geometry::StabilizerSolidInput> inputs;
    const QString path=QString::fromLocal8Bit(argv[1]);
    if(path.endsWith(".foam")) {
      QString error;auto document=gui::readProject(path,error);CHECK(document);const auto& p=*document;
      const double scale=p.reference.toScale?1.:gui::wingCalibration(p.wing.layers,p.stations.lines,p.reference.wingspanMm).scale;
      for(int i=0;i<2;++i)inputs.push_back({p.stabilizerOutlines[i].layers.front(),
        p.stabilizerAirfoils[i].value_or(domain::AirfoilProfile::nacaSymmetric(.06)),scale,i==0,
        p.stabilizerHinges[i].layers.front(),gui::HingeCut::Tape,p.stabilizerCuts[i].layers});
    } else {
      QFile file{path};CHECK(file.open(QIODevice::ReadOnly));const auto components=QJsonDocument::fromJson(file.readAll()).object()["components"].toArray();
      CHECK(components.size()==2);
      for(int i=0;i<2;++i) {
        const auto component=components[i].toObject();std::stringstream dat;dat<<"BabyBuzzard NACA 0006\n";
        for(auto value:component["airfoil"].toObject()["coordinates"].toArray()){auto p=value.toArray();dat<<p[0].toDouble()<<' '<<p[1].toDouble()<<'\n';}
        geometry::StabilizerSolidInput input{layer(component["outline"].toObject()),domain::AirfoilProfile::fromDat(dat),1,i==0,layer(component["hinge"].toObject())};
        for(auto cut:component["cuts"].toArray())input.cutShapes.push_back(layer(cut.toObject()));inputs.push_back(std::move(input));
      }
    }
    for(auto input:inputs)for(auto style:{gui::HingeCut::Tape,gui::HingeCut::Standard}) {
      input.hingeCut=style;
      std::cout<<(input.horizontal?"Horizontal":"Vertical")<<' '<<(style==gui::HingeCut::Tape?"Tape":"Standard")<<std::endl;
      const auto result=geometry::buildStabilizerModel(input,[](const char* stage){std::cout<<stage<<std::endl;});
      checkBevel(input,result);
    }
    std::cout<<"Both stabilizers: Tape and Standard 45-degree bevels verified\n";
  } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

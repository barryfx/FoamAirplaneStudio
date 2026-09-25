#pragma once
#include "geometry/ProcessingControl.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <IntCurvesFace_Intersector.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <utility>
#include <vector>

namespace designrc::geometry {
// Exact oriented ray crossings supply the same material intervals used by the
// pin rejection filter. The final cylinder/stock Boolean remains authoritative.
// Avoid whole-solid classifiers: they eagerly allocate an intersector for every
// face, duplicating the ray cache and making cancellation cleanup very expensive.
class FuselageMaterialSpans {
public:
  FuselageMaterialSpans(const TopoDS_Shape& shape,double minimum,double maximum,
                        const ProcessingControl& processing)
      : minimum_(minimum),maximum_(maximum) {
    for(TopExp_Explorer solid{shape,TopAbs_SOLID};solid.More();solid.Next(),++solids_)
      for(TopExp_Explorer face{solid.Current(),TopAbs_FACE};face.More();face.Next()) {
        processing.checkpoint();const auto f=TopoDS::Face(face.Current());
        if(f.Orientation()!=TopAbs_FORWARD&&f.Orientation()!=TopAbs_REVERSED)continue;
        Bnd_Box box;BRepBndLib::AddOptimal(f,box,false,false);box.Enlarge(tolerance);
        double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
        faces_.push_back({f,solids_,x0,y0,x1,y1,{}});
      }
    processing.checkpoint();
  }
  std::vector<std::pair<double,double>> at(double x,double y,const ProcessingControl& processing) {
    struct Hit {double z;std::size_t solid;IntCurveSurface_TransitionOnCurve transition;};
    std::vector<Hit> hits;
    const gp_Lin ray{gp_Pnt{x,y,minimum_},gp_Dir{0,0,1}};
    for(auto& face:faces_) {
      processing.checkpoint();
      if(x<face.x0||x>face.x1||y<face.y0||y>face.y1)continue;
      // Each search owns its lazy intersectors. Bounds only reject impossible
      // hits; the unchanged analytic face intersection decides actual crossings.
      if(face.intersector.IsNull())face.intersector=new IntCurvesFace_Intersector{face.shape,tolerance};
      processing.checkpoint();face.intersector->Perform(ray,0,maximum_-minimum_);processing.checkpoint();
      if(!face.intersector->IsDone())throw std::runtime_error("Could not inspect the fuselage mating surfaces.");
      for(int i=1;i<=face.intersector->NbPnt();++i)
        hits.push_back({face.intersector->Pnt(i).Z(),face.solid,face.intersector->Transition(i)});
    }
    std::sort(hits.begin(),hits.end(),[](const Hit& a,const Hit& b){return a.z<b.z;});
    std::vector<bool> inside(solids_,false),enter(solids_),leave(solids_);
    std::vector<std::pair<double,double>> spans;
    double previous=0;
    for(std::size_t i=0;i<hits.size();) {
      processing.checkpoint();const double height=hits[i].z;
      if(i&&std::any_of(inside.begin(),inside.end(),[](bool value){return value;}))
        spans.emplace_back(previous,height);
      std::fill(enter.begin(),enter.end(),false);std::fill(leave.begin(),leave.end(),false);
      std::size_t next=i;
      for(;next<hits.size()&&std::abs(hits[next].z-height)<tolerance;++next) {
        const auto& hit=hits[next];
        if(hit.transition==IntCurveSurface_In)enter[hit.solid]=true;
        if(hit.transition==IntCurveSurface_Out)leave[hit.solid]=true;
      }
      // Dedupe coincident face-edge hits per solid. Opposite crossings at one
      // height are a tangency (or touching intervals), not a parity toggle.
      for(std::size_t solid=0;solid<solids_;++solid)
        if(enter[solid]!=leave[solid])inside[solid]=enter[solid];
      previous=height;i=next;
    }
    processing.checkpoint();return spans;
  }
private:
  static constexpr double tolerance=1e-7;
  struct Face {
    TopoDS_Face shape;std::size_t solid;double x0,y0,x1,y1;
    Handle(IntCurvesFace_Intersector) intersector;
  };
  std::vector<Face> faces_;
  std::size_t solids_=0;
  double minimum_,maximum_;
};
}

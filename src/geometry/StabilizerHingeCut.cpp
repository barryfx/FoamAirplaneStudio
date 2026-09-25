#include "geometry/StabilizerHingeCut.h"
#include <BRepAlgoAPI_Splitter.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepLib.hxx>
#include <array>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
TopoDS_Shape cutStabilizerHinge(const TopoDS_Shape& solid,const gui::SketchLayer& lines,
    gui::HingeCut hinge,const ProcessingControl& processing,bool preferSpan) {
  processing.checkpoint();if(lines.curves.empty())return solid;
  auto fail=[](){throw std::runtime_error("Draw one connected, non-branching hinge polyline with two ends on or beyond the stabilizer boundary.");};
  std::vector<std::vector<std::size_t>> adjacent(lines.points.size());
  std::size_t longest=0;double length=0,bestScore=-1;
  for(std::size_t c=0;c<lines.curves.size();++c) {
    const auto& ids=lines.curves[c].points;
    if(lines.curves[c].type!=gui::SketchTool::Line || ids.size()!=2 || ids[0]>=lines.points.size() || ids[1]>=lines.points.size())fail();
    const double d=QLineF{lines.points[ids[0]],lines.points[ids[1]]}.length();if(d<1e-6)fail();
    // Model Y is span, which becomes vertical when the fin is oriented upright.
    // A long return segment must not steal the rudder's bevel.
    const double score=preferSpan?std::abs(lines.points[ids[1]].y()-lines.points[ids[0]].y())/d:d;
    if(score>bestScore+1e-12 || (std::abs(score-bestScore)<=1e-12&&d>length)) {length=d;longest=c;bestScore=score;}
    adjacent[ids[0]].push_back(ids[1]);adjacent[ids[1]].push_back(ids[0]);
  }
  for(std::size_t i=0;i<lines.curves.size();++i)for(std::size_t j=i+1;j<lines.curves.size();++j) {
    const auto& a=lines.curves[i].points;const auto& b=lines.curves[j].points;
    if(a[0]==b[0] || a[0]==b[1] || a[1]==b[0] || a[1]==b[1])continue;
    QPointF intersection;
    if(QLineF{lines.points[a[0]],lines.points[a[1]]}.intersects(QLineF{lines.points[b[0]],lines.points[b[1]]},&intersection)==QLineF::BoundedIntersection)
      throw std::runtime_error("The hinge polyline must not cross itself.");
  }
  int ends=0,used=0;for(const auto& neighbors:adjacent){if(neighbors.size()>2)fail();ends+=neighbors.size()==1;used+=!neighbors.empty();}
  if(ends!=2)fail();
  std::vector<bool> seen(lines.points.size());std::vector<std::size_t> stack{lines.curves[0].points[0]};int visited=0;
  while(!stack.empty()){auto n=stack.back();stack.pop_back();if(seen[n])continue;seen[n]=true;++visited;for(auto next:adjacent[n])stack.push_back(next);}
  if(visited!=used)fail();
  const auto& ids=lines.curves[longest].points;const auto start=lines.points[ids[0]],end=lines.points[ids[1]];
  const auto tangent=(end-start)/length;QPointF inward{-tangent.y(),tangent.x()};if(inward.x()<0)inward=-inward;
  if(inward.x()<1e-6)throw std::runtime_error("The bevel hinge segment must run across the span, with a trailing-edge side.");
  Bnd_Box bounds;BRepBndLib::Add(solid,bounds);double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
  const double margin=2*std::max({x1-x0,y1-y0,z1-z0,1.0})+std::max(std::abs(z0),std::abs(z1))+1;
  auto point=[](QPointF p,double z){return gp_Pnt{p.x(),p.y(),z};};
  TopTools_ListOfShape tools,args;args.Append(solid);
  for(const auto& curve:lines.curves) {
    processing.checkpoint();const auto a=lines.points[curve.points[0]],b=lines.points[curve.points[1]];
    BRepBuilderAPI_MakePolygon polygon;
    for(const auto p:{point(a,-margin),point(b,-margin),point(b,margin),point(a,margin)})polygon.Add(p);
    polygon.Close();tools.Append(BRepBuilderAPI_MakeFace{polygon.Wire()}.Face());
  }
  BRepAlgoAPI_Splitter split;split.SetArguments(args);split.SetTools(tools);split.Build(processing.range());processing.checkpoint();
  if(!split.IsDone()||split.HasErrors())throw std::runtime_error("Could not split the stabilizer at the hinge line.");
  std::vector<TopoDS_Shape> bodies;for(TopExp_Explorer e{split.Shape(),TopAbs_SOLID};e.More();e.Next())bodies.push_back(e.Current());
  if(bodies.size()!=2)throw std::runtime_error("The hinge line must separate the stabilizer into exactly two connected bodies. Extend its ends to or beyond the boundary.");
  IntCurvesFace_ShapeIntersector ray;ray.Load(solid,1e-7);
  auto heights=[&](QPointF p)->std::optional<std::pair<double,double>> {
    processing.checkpoint();ray.Perform(gp_Lin{point(p,-margin),gp_Dir{0,0,1}},0,2*margin);
    if(!ray.IsDone() || ray.NbPnt()<2)return {};
    double low=margin,high=-margin;for(int i=1;i<=ray.NbPnt();++i){low=std::min(low,ray.Pnt(i).Z());high=std::max(high,ray.Pnt(i).Z());}
    if(high-low<1e-7)return {};return std::pair{low,high};
  };
  struct Sample{double t,anchor;};std::vector<Sample> samples;int moving=-1;
  for(int i=0;i<=32;++i) {
    const double t=i/32.;const auto p=start+t*(end-start);const auto h=heights(p);if(!h)continue;
    samples.push_back({t,hinge==gui::HingeCut::Tape?h->second:(h->first+h->second)*.5});
    // Probe just inside the trailing side; classify split bodies rather than
    // assuming a centroid stays on one side of a bent separation polyline.
    const auto probe=point(p+inward*1e-4,(h->first+h->second)*.5);
    for(int n=0;n<2;++n) {
      BRepClass3d_SolidClassifier classifier{bodies[n],probe,1e-7};
      if(classifier.State()==TopAbs_IN) {
        if(moving>=0 && moving!=n)throw std::runtime_error("The hinge polyline folds across its bevel segment.");
        moving=n;
      }
    }
  }
  if(samples.size()<2 || moving<0)throw std::runtime_error("Place the bevel hinge segment inside the stabilizer.");
  if(samples.front().t>0)samples.insert(samples.begin(),{0,samples.front().anchor});
  if(samples.back().t<1)samples.push_back({1,samples.back().anchor});
  for(std::size_t i=1;i+1<samples.size();) {
    const auto a=samples[i-1],b=samples[i],c=samples[i+1];
    if(std::abs(a.anchor+(c.anchor-a.anchor)*(b.t-a.t)/(c.t-a.t)-b.anchor)<1e-6)samples.erase(samples.begin()+i);else ++i;
  }
  auto part=bodies[moving];
  // Same 33 sampled heights and 45-degree relief profiles as wing controls.
  // Only the chosen finite segment is beveled; connected return cuts stay square.
  auto bevel=[&](bool upper) {
    // Bound the tool by thickness, not aircraft span. Every band is planar:
    // construct those planes explicitly. A generic loft of varying sections
    // can validate yet leave curved-airfoil stock uncut in the Boolean.
    const double reliefExtent=std::max({std::abs(z0),std::abs(z1),z1-z0,1.})+1.;
    std::vector<std::array<gp_Pnt,4>> sections;
    for(const auto sample:samples) {
      const auto at=start+(end-start)*sample.t;
      auto p=[&](double d,double z){return point(at+inward*d,z);};
      if(!upper)sections.push_back({p(-reliefExtent,-reliefExtent),p(sample.anchor+reliefExtent,-reliefExtent),p(0,sample.anchor),p(-reliefExtent,sample.anchor)});
      else sections.push_back({p(-reliefExtent,sample.anchor),p(0,sample.anchor),p(reliefExtent-sample.anchor,reliefExtent),p(-reliefExtent,reliefExtent)});
    }
    BRepBuilderAPI_Sewing sewing{1e-7};
    const auto face=[&](const std::array<gp_Pnt,4>& vertices) {
      processing.checkpoint();BRepBuilderAPI_MakePolygon polygon;
      for(const auto& vertex:vertices)polygon.Add(vertex);polygon.Close();
      BRepBuilderAPI_MakeFace plane{polygon.Wire(),true};
      if(!plane.IsDone())throw std::runtime_error("Could not form stabilizer hinge bevel face.");
      sewing.Add(plane.Face());
    };
    face(sections.front());face(sections.back());
    for(std::size_t s=1;s<sections.size();++s)for(std::size_t v=0;v<4;++v)
      face({sections[s-1][v],sections[s-1][(v+1)%4],sections[s][(v+1)%4],sections[s][v]});
    sewing.Perform(processing.range());processing.checkpoint();
    if(sewing.SewedShape().ShapeType()!=TopAbs_SHELL)throw std::runtime_error("Could not close stabilizer hinge bevel tool.");
    auto tool=BRepBuilderAPI_MakeSolid{TopoDS::Shell(sewing.SewedShape())}.Solid();
    if(!BRepLib::OrientClosedSolid(tool)||!BRepCheck_Analyzer{tool}.IsValid())throw std::runtime_error("Invalid stabilizer hinge bevel tool.");
    BRepAlgoAPI_Cut cut{part,tool,processing.range()};processing.checkpoint();
    if(!cut.IsDone()||cut.HasErrors())throw std::runtime_error("Could not cut stabilizer hinge bevel.");part=cut.Shape();
  };
  bevel(false);if(hinge==gui::HingeCut::Standard)bevel(true);
  int count=0;for(TopExp_Explorer e{part,TopAbs_SOLID};e.More();e.Next())++count;
  if(count!=1 || !BRepCheck_Analyzer{part}.IsValid() || !BRepCheck_Analyzer{bodies[1-moving]}.IsValid())
    throw std::runtime_error("The hinge bevel consumes or disconnects the control surface. Move the hinge forward.");
  BRep_Builder builder;TopoDS_Compound result;builder.MakeCompound(result);builder.Add(result,bodies[1-moving]);builder.Add(result,part);return result;
}
}

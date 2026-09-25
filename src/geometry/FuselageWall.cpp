#include "geometry/FuselageWall.h"
#include "geometry/FuselageProcessing.h"
#include "geometry/FuselageSymmetry.h"
#include "processing/IndexedTasks.h"
#include "geometry/FuselageTopology.h"
#include "geometry/FuselageSolidBuilder.h"
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <Standard_Failure.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepLib.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <QPolygonF>
#include <QLineF>
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
namespace designrc::geometry {
namespace {
double cross(QPointF a,QPointF b){return a.x()*b.y()-a.y()*b.x();}
// Intersect adjacent inward-shifted lines when the planar offset kernel fails
// on a nearly straight, mirrored polygon. Accept only a simple loop that meets
// the original wall clearance; never bridge a collapsed narrow neck.
std::vector<QPointF> miterInset(const std::vector<QPointF>& p,double wall,double area,const ProcessingControl& processing) {
  struct Line {QPointF origin,direction;};
  std::vector<Line> lines;
  const double sign=area>0?1.:-1.;
  for(std::size_t i=0;i<p.size();++i) {
    processing.checkpoint();
    auto direction=p[(i+1)%p.size()]-p[i];
    const double length=std::hypot(direction.x(),direction.y());
    if(length<1e-9)return {};
    direction/=length;lines.push_back({p[i]+QPointF{-direction.y(),direction.x()}*(sign*wall),direction});
  }
  std::vector<QPointF> result;
  while(lines.size()>=3) {
    processing.checkpoint();
    result.clear();
    for(std::size_t i=0;i<lines.size();++i) {
      const auto& a=lines[(i+lines.size()-1)%lines.size()];const auto& b=lines[i];
      const double determinant=cross(a.direction,b.direction);
      if(std::abs(determinant)<1e-12) {
        if(QPointF::dotProduct(a.direction,b.direction)<0)return {};
        result.push_back(b.origin);
      } else result.push_back(a.origin+a.direction*(cross(b.origin-a.origin,b.direction)/determinant));
    }
    bool consumed=false;
    for(std::size_t i=0;i<lines.size();++i) {
      if(QPointF::dotProduct(result[(i+1)%lines.size()]-result[i],lines[i].direction)>1e-9)continue;
      // Only convex corner edges may disappear. A collapsed concave neck can
      // split the cavity and is not repaired by this single-loop fallback.
      if(sign*cross(lines[(i+lines.size()-1)%lines.size()].direction,lines[i].direction)<-1e-12||
         sign*cross(lines[i].direction,lines[(i+1)%lines.size()].direction)<-1e-12)return {};
      lines.erase(lines.begin()+i);consumed=true;break;
    }
    if(!consumed)break;
  }
  if(lines.size()<3)return {};
  const auto distance=[](QPointF p,QPointF a,QPointF b) {
    const auto d=b-a;const double length=QPointF::dotProduct(d,d);
    return QLineF{p,a+d*(length>0?std::clamp(QPointF::dotProduct(p-a,d)/length,0.,1.):0.)}.length();
  };
  const auto crosses=[](QPointF a,QPointF b,QPointF c,QPointF d) {
    return cross(b-a,c-a)*cross(b-a,d-a)<=0&&cross(d-c,a-c)*cross(d-c,b-c)<=0&&
      std::max(std::min(a.x(),b.x()),std::min(c.x(),d.x()))<=std::min(std::max(a.x(),b.x()),std::max(c.x(),d.x()))&&
      std::max(std::min(a.y(),b.y()),std::min(c.y(),d.y()))<=std::min(std::max(a.y(),b.y()),std::max(c.y(),d.y()));
  };
  for(std::size_t i=0;i<result.size();++i) {
    processing.checkpoint();
    const auto a=result[i],b=result[(i+1)%result.size()];
    if(!std::isfinite(a.x())||!std::isfinite(a.y())||QLineF{a,b}.length()<1e-9)return {};
    for(std::size_t j=0;j<p.size();++j) {
      const auto c=p[j],d=p[(j+1)%p.size()];
      if(crosses(a,b,c,d)||std::min({distance(a,c,d),distance(b,c,d),distance(c,a,b),distance(d,a,b)})<wall-1e-6)return {};
    }
    for(std::size_t j=i+2;j<result.size();++j)
      if(!(i==0&&j+1==result.size())&&crosses(a,b,result[j],result[(j+1)%result.size()]))return {};
  }
  return result;
}
}
std::optional<std::vector<QPointF>> insetFuselageSection(const FuselageWallSection& section,const ProcessingControl& processing) {
  processing.checkpoint();
  const auto& p=section.perimeter;const double wall=section.thickness;
  if(p.size()<3)return {};
  double area=0;for(std::size_t i=0;i<p.size();++i)area+=cross(p[i],p[(i+1)%p.size()]);
  // Kernel planar offset removes consumed corner edges instead of inverting
  // short tessellation segments when the wall exceeds their length.
  BRepBuilderAPI_MakePolygon polygonBuilder;
  if(area>0)for(auto point:p)polygonBuilder.Add(gp_Pnt{point.x(),point.y(),0});
  else for(auto it=p.rbegin();it!=p.rend();++it)polygonBuilder.Add(gp_Pnt{it->x(),it->y(),0});
  polygonBuilder.Close();if(!polygonBuilder.IsDone())return {};
  std::vector<QPointF> result;
  bool kernelFailed=true;
  const auto kernelOffset=[&]() -> std::vector<QPointF> {
   try {
    std::vector<QPointF> result;
    BRepBuilderAPI_MakeFace face{polygonBuilder.Wire(),true};
    BRepOffsetAPI_MakeOffset offset{face.Face(),GeomAbs_Intersection};processing.checkpoint();offset.Perform(-wall);processing.checkpoint();
    if(!offset.IsDone()||offset.Shape().IsNull())return {};
    kernelFailed=false; // Multiple/empty offset loops are real topology results.
    TopExp_Explorer wires{offset.Shape(),TopAbs_WIRE};if(!wires.More())return {};
    const auto wire=TopoDS::Wire(wires.Current());wires.Next();if(wires.More())return {};
    for(BRepTools_WireExplorer e{wire};e.More();e.Next()) {
      BRepAdaptor_Curve curve{e.Current()};const bool reverse=e.Current().Orientation()==TopAbs_REVERSED;
      const int count=curve.GetType()==GeomAbs_Line?1:32;
      for(int i=0;i<count;++i) {
        const double t=reverse?1.-double(i)/count:double(i)/count;
        const auto point=curve.Value(curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*t);
        result.push_back({point.X(),point.Y()});
      }
    }
    return result;
   } catch(const Standard_Failure&){return {};}
  };
  result=kernelOffset();
  if(result.empty()&&kernelFailed)result=miterInset(p,wall,area,processing);
  if(result.size()<3)return {};
  QPolygonF polygon;for(auto point:p)polygon<<point;
  double innerArea=0;
  for(std::size_t i=0;i<result.size();++i) {
    processing.checkpoint();
    const auto point=result[i];if(!polygon.containsPoint(point,Qt::OddEvenFill))return {};
    innerArea+=cross(point,result[(i+1)%result.size()]);
    for(std::size_t j=0;j<p.size();++j) {
      const auto a=p[j],d=p[(j+1)%p.size()]-a;const double squared=QPointF::dotProduct(d,d);
      const auto closest=a+d*std::clamp(QPointF::dotProduct(point-a,d)/squared,0.,1.);
      if(QLineF{point,closest}.length()<wall-1e-6)return {};
    }

  }
  if(std::abs(innerArea)<=1e-8)return {};
  double ymin=result[0].x(),ymax=ymin,zmin=result[0].y(),zmax=zmin;
  gui::SketchLayer sketch;
  for(auto point:result) {
    ymin=std::min(ymin,point.x());ymax=std::max(ymax,point.x());zmin=std::min(zmin,point.y());zmax=std::max(zmax,point.y());
    sketch.points.push_back({point.x(),-point.y()});
  }
  for(std::size_t i=0;i<result.size();++i)sketch.curves.push_back({gui::SketchTool::Line,{i,(i+1)%result.size()}});
  const auto sampled=sampleFuselageProfile(sketch);result.clear();
  for(auto point:sampled)result.push_back({ymin+point.x()*(ymax-ymin),zmax-point.y()*(zmax-zmin)});
  return result;
}
namespace {
TopoDS_Wire wireAt(double x,const std::vector<QPointF>& points) {
  BRepBuilderAPI_MakePolygon polygon;for(auto p:points)polygon.Add(gp_Pnt{x,p.x(),p.y()});polygon.Close();
  if(!polygon.IsDone())throw std::runtime_error("Could not construct the inner fuselage wall.");return polygon.Wire();
}
bool endCap(const TopoDS_Shape& face,double x) {
  Bnd_Box box;BRepBndLib::AddOptimal(face,box,false,false);double a,b,c,d,e,f;box.Get(a,b,c,d,e,f);
  return std::abs(a-x)<1e-6&&std::abs(d-x)<1e-6;
}
}
TopoDS_Shape hollowFuselage(const TopoDS_Shape& outside,const std::vector<FuselageWallSection>& sections,bool openNose,bool openTail,
    const std::function<void(const char*)>& progress,const ProcessingControl& processing,TopoDS_Shape* innerCavity,bool mirrorConstruction) {
  if(sections.size()<2)throw std::runtime_error("Fuselage needs enough length for hollowing.");
  if(progress)progress("Fuselage: offsetting station walls inward...");
  std::vector<std::pair<std::size_t,std::vector<QPointF>>> inner;
  const double tailLimit=sections.back().x-(openTail?0:sections.back().thickness);
  std::vector<std::optional<std::vector<QPointF>>> offsets(sections.size());
  // Every offset constructs private OCCT topology from numeric section points.
  // Workers write separate slots; cavity order/closure checks stay sequential.
  processing::runIndexedTasks(sections.size(),[&](std::size_t i,std::stop_token stop) {
    ProcessingControl{stop}.checkpoint();
    if(sections[i].x>tailLimit+1e-8)return;
    if(!openNose&&sections[i].x-sections.front().x<sections.front().thickness-1e-8)return;
    offsets[i]=insetFuselageSection(sections[i],{stop,processing.parallel});
  },processing.stop,processing.parallel?0u:1u);
  processing.checkpoint();
  bool ended=false;
  for(std::size_t i=0;i<sections.size();++i) {
    processing.checkpoint();if(sections[i].x>tailLimit+1e-8)break;
    if(!openNose&&sections[i].x-sections.front().x<sections.front().thickness-1e-8)continue;
    auto loop=std::move(offsets[i]);
    if(i==0&&openNose&&!loop)throw std::runtime_error("The nose profile is too small for its wall thickness. Reduce the foremost station thickness.");
    if(!loop){
      if(openTail&&i+1==sections.size())throw std::runtime_error("The tail profile is too small for its wall thickness. Reduce the rearmost station thickness.");
      if(!inner.empty())ended=true;continue;
    }

    if(ended)throw std::runtime_error("Wall thickness closes the cavity inside the fuselage. Reduce thickness near the narrow section.");
    inner.emplace_back(i,std::move(*loop));
  }
  if(inner.size()<2)throw std::runtime_error("The selected thickness leaves no usable fuselage cavity. Reduce station thicknesses.");
  if(openTail&&inner.back().first+1!=sections.size())throw std::runtime_error("The rearmost profile could not provide an open tail.");
  if(openNose&&inner.front().first!=0)throw std::runtime_error("The foremost profile could not provide an open nose.");
  BRepOffsetAPI_ThruSections cavity{true,true,1e-7};cavity.CheckCompatibility(false);
  for(const auto& [i,loop]:inner){processing.checkpoint();cavity.AddWire(wireAt(sections[i].x,mirrorConstruction?rightFuselageSection(loop):loop));}
  if(progress)progress("Fuselage: lofting the smoothly varying inner wall...");
  {auto range=processing.range();cavity.Build(range);}processing.checkpoint();
  if(!cavity.IsDone())throw std::runtime_error("Inner wall loft failed.");
  if(progress)progress("Fuselage: merging coincident inner loft faces...");
  auto cavityShape=simplifyFuselageTopology(cavity.Shape(),processing);
  if(!fuselageValid(cavityShape,processing))
    throw std::runtime_error("Inner wall loft failed. Reduce thickness or simplify the profile corners.");
  if(mirrorConstruction) {
    // Both solids end on Y=0. Cutting the half-cavity leaves the real mating
    // faces without constructing or subsequently splitting a full outer body.
    if(progress)progress("Fuselage: hollowing the right half...");
    BRepAlgoAPI_Cut cut;NCollection_List<TopoDS_Shape> args,tools;
    args.Append(outside);tools.Append(cavityShape);cut.SetArguments(args);cut.SetTools(tools);
    cut.SetNonDestructive(true);cut.SetRunParallel(processing.parallel);cut.SetFuzzyValue(1e-7);
    {auto range=processing.range();cut.Build(range);}processing.checkpoint();
    if(!cut.IsDone()||cut.HasErrors())throw std::runtime_error("Could not hollow the right fuselage half.");
    TopExp_Explorer solids{cut.Shape(),TopAbs_SOLID};
    if(!solids.More())throw std::runtime_error("Hollowing removed the right fuselage half.");
    auto result=solids.Current();solids.Next();
    if(solids.More()||!fuselageValid(result,processing))
      throw std::runtime_error("The hollow right fuselage half is disconnected or invalid.");
    GProp_GProps before,after;fuselageVolumeProperties(outside,before,processing);fuselageVolumeProperties(result,after,processing);
    if(after.Mass()<=0||after.Mass()>=std::abs(before.Mass()))throw std::runtime_error("Fuselage wall did not produce a positive hollow half.");
    if(innerCavity) {
      if(progress)progress("Fuselage: preparing the whole cavity for removable inserts and holes...");
      *innerCavity=joinMirroredFuselage(cavityShape,processing);
    }
    return result;
  }
  TopoDS_Solid solid;
  if(openNose||openTail) {
    BRepBuilderAPI_Sewing sewing{1e-6};const double nose=sections.front().x,tail=sections.back().x;
    auto openCap=[&](const TopoDS_Shape& face){return (openNose&&endCap(face,nose))||(openTail&&endCap(face,tail));};
    for(TopExp_Explorer e{outside,TopAbs_FACE};e.More();e.Next())if(!openCap(e.Current()))sewing.Add(e.Current());
    for(TopExp_Explorer e{cavityShape,TopAbs_FACE};e.More();e.Next())if(!openCap(e.Current()))sewing.Add(e.Current().Reversed());
    auto addRim=[&](const FuselageWallSection& section,const std::vector<QPointF>& loop,bool atTail) {
      BRepBuilderAPI_MakeFace rim{wireAt(section.x,section.perimeter),true};
      rim.Add(TopoDS::Wire(wireAt(section.x,loop).Reversed()));
      if(atTail)sewing.Add(rim.Face().Reversed());else sewing.Add(rim.Face());
    };
    if(openNose)addRim(sections.front(),inner.front().second,false);
    if(openTail)addRim(sections.back(),inner.back().second,true);
    {auto range=processing.range();sewing.Perform(range);}processing.checkpoint();
    TopExp_Explorer shells{sewing.SewedShape(),TopAbs_SHELL};
    if(!shells.More())throw std::runtime_error("Could not join the fuselage end rims.");
    const auto shell=TopoDS::Shell(shells.Current());shells.Next();
    if(shells.More())throw std::runtime_error("Fuselage wall contains disconnected shells.");
    solid=BRepBuilderAPI_MakeSolid{shell}.Solid();
  } else {
    BRepBuilderAPI_MakeSolid body;
    for(TopExp_Explorer e{outside,TopAbs_SHELL};e.More();e.Next())body.Add(TopoDS::Shell(e.Current()));
    for(TopExp_Explorer e{cavityShape,TopAbs_SHELL};e.More();e.Next())body.Add(TopoDS::Shell(e.Current().Reversed()));
    solid=body.Solid();
    if(progress)progress("Fuselage: retaining solid ends beyond the outermost profiles...");
  }
  BRepLib::OrientClosedSolid(solid);processing.checkpoint();
  if(!fuselageValid(solid,processing))throw std::runtime_error("The thickened fuselage is not a valid solid.");
  GProp_GProps before,after;fuselageVolumeProperties(outside,before,processing);fuselageVolumeProperties(solid,after,processing);
  if(after.Mass()<=0||after.Mass()>=std::abs(before.Mass()))throw std::runtime_error("Fuselage wall did not produce a positive hollow solid.");
  if(innerCavity)*innerCavity=cavityShape;
  return solid;
}
}

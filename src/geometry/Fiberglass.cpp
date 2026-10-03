#include "geometry/Fiberglass.h"
#include "processing/IndexedTasks.h"
#include "gui/SketchPaths.h"
#include "gui/WingCalibration.h"
#include "geometry/FuselageSolidBuilder.h"
#include "geometry/MeshOrientation.h"
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <QPainterPathStroker>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace designrc::geometry {
namespace {
using gui::SketchLayer;using gui::SketchTool;
std::vector<std::size_t> endpoints(const SketchLayer& layer) {
  std::vector<int> degree(layer.points.size());
  for(const auto& c:layer.curves){if(c.points.size()<2)throw std::runtime_error("Incomplete fiberglass curve.");++degree.at(c.points.front());++degree.at(c.points.back());}
  std::vector<std::size_t> ends;for(std::size_t i=0;i<degree.size();++i){if(degree[i]==1)ends.push_back(i);else if(degree[i]>2)throw std::runtime_error("Fiberglass paths must not branch.");}return ends;
}
QPainterPath outlinePath(SketchLayer layer,QPointF span={}) {
  auto ends=endpoints(layer);
  std::sort(ends.begin(),ends.end(),[&](auto a,auto b){return QPointF::dotProduct(layer.points[a],span)<QPointF::dotProduct(layer.points[b],span);});
  if(ends.size()%2)throw std::runtime_error("Incomplete component outline for fiberglass.");
  for(std::size_t i=0;i<ends.size();i+=2)layer.curves.push_back({SketchTool::Line,{ends[i],ends[i+1]}});
  auto boundary=gui::closedSketchBoundary(layer);if(!boundary)throw std::runtime_error("Complete the component outline before calculating fiberglass.");
  return gui::sketchPolygon(*boundary);
}
gp_Pnt average(const gp_Pnt& a,const gp_Pnt& b,const gp_Pnt& c) {return gp_Pnt{(a.XYZ()+b.XYZ()+c.XYZ())/3};}
}
QPainterPath fiberglassRegion(SketchLayer layer,const QPainterPath& outline) {
  if(layer.curves.empty())return {};
  if(auto loop=gui::closedSketchBoundary(layer))return gui::sketchPolygon(*loop).intersected(outline);
  const auto ends=endpoints(layer);
  if(ends.size()!=2)throw std::runtime_error("Use one connected loop or open chain per fiberglass shape.");
  const auto a=layer.points[ends[0]],b=layer.points[ends[1]];
  QPainterPath closure;closure.moveTo(a);closure.lineTo(b);
  QPainterPathStroker stroke;stroke.setWidth(1e-6);
  if(outline.contains(a)||outline.contains(b)||outline.intersects(stroke.createStroke(closure)))
    throw std::runtime_error("Close this fiberglass shape, or move both ends so their closing segment stays outside the component outline.");
  layer.curves.push_back({SketchTool::Line,{ends[0],ends[1]}});
  const auto loop=gui::closedSketchBoundary(layer);if(!loop)throw std::runtime_error("Fiberglass shape does not enclose an area.");
  return gui::sketchPolygon(*loop).intersected(outline);
}

gui::FoamMassProperties::Covering measureFiberglass(const TopoDS_Shape& source,
    const QPainterPath& region,const FiberglassProjection& projection,bool wrap,bool parallelMesh) {
  gui::FoamMassProperties::Covering result;if(source.IsNull()||region.isEmpty())return result;
  // Meshing must never modify cached/displayed Assembly topology or meshes.
  const auto shape=BRepBuilderAPI_Copy{source,true,false}.Shape();
  BRepMesh_IncrementalMesh mesh{shape,0.1,false,0.15,parallelMesh};
  if(!mesh.IsDone())throw std::runtime_error("Could not mesh fiberglass surface.");
  alignMeshOrientation(shape);
  IntCurvesFace_ShapeIntersector visibility;visibility.Load(shape,1e-7);
  gp_XYZ moment{0,0,0};
  for(TopExp_Explorer e{shape,TopAbs_FACE};e.More();e.Next()) {
    const auto face=TopoDS::Face(e.Current());TopLoc_Location location;
    const auto triangles=BRep_Tool::Triangulation(face,location);if(triangles.IsNull())continue;
    for(int i=1;i<=triangles->NbTriangles();++i) {
      int ia,ib,ic;triangles->Triangle(i).Get(ia,ib,ic);
      const auto a=triangles->Node(ia).Transformed(location.Transformation()),b=triangles->Node(ib).Transformed(location.Transformation()),c=triangles->Node(ic).Transformed(location.Transformation());
      auto normal=gp_Vec{a,b}.Crossed(gp_Vec{a,c});const double doubleArea=normal.Magnitude();if(doubleArea<1e-12)continue;
      normal/=doubleArea;if(face.Orientation()==TopAbs_REVERSED)normal.Reverse();
      if(projection.prepareTriangle)projection.prepareTriangle(average(a,b,c));
      const auto direction=projection.outward(average(a,b,c));const double facing=normal.Dot(gp_Vec{direction});
      if(!wrap&&facing<=1e-8)continue;
      const auto add=[&](double area,const gp_Pnt& center) {
        if(area<=1e-12)return;
        gp_Dir ray=direction;
        if(wrap)ray=std::abs(facing)<1e-8?gp_Dir{normal}:facing<0?direction.Reversed():direction;
        visibility.Perform(gp_Lin{center,ray},1e-6,1e7);
        if(!visibility.IsDone())throw std::runtime_error("Could not classify fiberglass exterior surface.");
        for(int hit=1;hit<=visibility.NbPnt();++hit) {
          // A planar mesh chord can sit just inside its curved source face.
          if(visibility.Face(hit).IsSame(face)&&visibility.WParameter(hit)<0.3)continue;
          if(visibility.WParameter(hit)>1e-5)return; // Internal walls and mating faces receive no covering.
        }
        result.areaMm2+=area;moment+=center.XYZ()*area;
      };
      const auto pa=projection.point(a),pb=projection.point(b),pc=projection.point(c);
      QTransform map{pb.x()-pa.x(),pb.y()-pa.y(),pc.x()-pa.x(),pc.y()-pa.y(),pa.x(),pa.y()};
      bool invertible=false;const auto inverse=map.inverted(&invertible);
      if(invertible) {
        QPainterPath triangle;triangle.moveTo(0,0);triangle.lineTo(1,0);triangle.lineTo(0,1);triangle.closeSubpath();
        const auto clipped=inverse.map(region).intersected(triangle);
        for(const auto& polygon:clipped.toFillPolygons()) {
          double twice=0,cx=0,cy=0;
          for(int n=0;n<polygon.size();++n){const auto p=polygon[n],q=polygon[(n+1)%polygon.size()];const double cross=p.x()*q.y()-q.x()*p.y();twice+=cross;cx+=(p.x()+q.x())*cross;cy+=(p.y()+q.y())*cross;}
          if(std::abs(twice)<1e-14)continue;
          const double u=cx/(3*twice),v=cy/(3*twice);
          add(std::abs(twice)*doubleArea*.5,gp_Pnt{a.XYZ()+(b.XYZ()-a.XYZ())*u+(c.XYZ()-a.XYZ())*v});
        }
      } else if(wrap) {
        // An edge-on triangle projects to an interval. Intersect that interval
        // with the mask, then clip in barycentric coordinates: no lost sidewall
        // area and no recursive sampling of long, thin triangles.
        const auto length2=[](QPointF p){return QPointF::dotProduct(p,p);};
        QPointF start=pa,end=pb;
        for(const auto& pair:std::array<std::pair<QPointF,QPointF>,3>{{{pa,pb},{pa,pc},{pb,pc}}})if(length2(pair.second-pair.first)>length2(end-start)){start=pair.first;end=pair.second;}
        const auto axis=end-start;const double squared=length2(axis);
        if(squared<1e-20){if(region.contains(start))add(doubleArea*.5,average(a,b,c));continue;}
        const auto cross=[](QPointF x,QPointF y){return x.x()*y.y()-x.y()*y.x();};
        std::vector<double> limits{0,1};
        for(const auto& polygon:region.toSubpathPolygons())for(int n=0;n<polygon.size();++n) {
          const auto p=polygon[n],d=polygon[(n+1)%polygon.size()]-p;const double determinant=cross(axis,d);if(std::abs(determinant)<1e-15)continue;
          const double t=cross(p-start,d)/determinant,u=cross(p-start,axis)/determinant;
          if(t>0&&t<1&&u>=0&&u<=1)limits.push_back(t);
        }
        std::sort(limits.begin(),limits.end());
        const double ta=QPointF::dotProduct(pa-start,axis)/squared,tb=QPointF::dotProduct(pb-start,axis)/squared,tc=QPointF::dotProduct(pc-start,axis)/squared;
        const auto parameter=[&](QPointF p){return ta+(tb-ta)*p.x()+(tc-ta)*p.y();};
        for(std::size_t n=1;n<limits.size();++n) {
          const double low=limits[n-1],high=limits[n];if(high-low<1e-12)continue;
          const auto midpoint=start+axis*((low+high)*.5);
          // Include the outline edge itself as well as its interior.
          const QPointF epsilon{-axis.y()/std::sqrt(squared)*1e-6,axis.x()/std::sqrt(squared)*1e-6};
          if(!region.contains(midpoint)&&!region.contains(midpoint+epsilon)&&!region.contains(midpoint-epsilon))continue;
          std::vector<QPointF> polygon{{0,0},{1,0},{0,1}};
          for(int side=0;side<2;++side) {
            std::vector<QPointF> clipped;
            for(std::size_t j=0;j<polygon.size();++j) {
              const auto p=polygon[j],q=polygon[(j+1)%polygon.size()];const double dp=side?high-parameter(p):parameter(p)-low,dq=side?high-parameter(q):parameter(q)-low;
              if(dp>=0)clipped.push_back(p);if((dp>=0)!=(dq>=0))clipped.push_back(p+(q-p)*(dp/(dp-dq)));
            }polygon=std::move(clipped);
          }
          double twice=0,cx=0,cy=0;
          for(std::size_t j=0;j<polygon.size();++j){const auto p=polygon[j],q=polygon[(j+1)%polygon.size()];const double v=cross(p,q);twice+=v;cx+=(p.x()+q.x())*v;cy+=(p.y()+q.y())*v;}
          if(std::abs(twice)>1e-14)add(std::abs(twice)*doubleArea*.5,gp_Pnt{a.XYZ()+(b.XYZ()-a.XYZ())*(cx/(3*twice))+(c.XYZ()-a.XYZ())*(cy/(3*twice))});
        }
      }
    }
  }
  if(result.areaMm2>0){const auto center=moment/result.areaMm2;result.centroidMm={center.X(),center.Z()};}
  return result;
}

std::vector<gui::FoamMassProperties::Covering> fiberglassMassProperties(
    const gui::ProjectDocument& project,const AssemblyParts& originals,const AssemblyParts& placed,unsigned workers) {
  std::vector<gui::FoamMassProperties::Covering> result;
  bool any=false;for(const auto& component:project.fiberglass){any=any||!component.sketch.pending.empty();for(const auto& layer:component.sketch.layers)any=any||!layer.curves.empty();}
  if(!any)return result;
  const auto calibration=gui::wingCalibration(project.wing.layers,project.stations.lines,project.reference.toScale?std::nullopt:project.reference.wingspanMm);
  const double scale=calibration.scale;const QString names[]{"Wing","Fuselage","Horiz Stab","Vert Stab"};
  std::array<std::vector<gui::FoamMassProperties::Covering>,4> measured;
  // Each component owns its projection, copied mesh and visibility classifier.
  // Keep OCCT meshing serial inside workers to bound nested thread counts.
  processing::runIndexedTasks(4,[&](std::size_t component,std::stop_token) {
    const auto& state=project.fiberglass[component];
    bool present=!state.sketch.pending.empty();for(const auto& layer:state.sketch.layers)present=present||!layer.curves.empty();
    if(!present)return;
    if(!state.sketch.pending.empty())throw std::runtime_error((names[component]+": finish the pending fiberglass curve first.").toStdString());
    gp_Trsf placement;if(component!=1)placement=assemblyComponentPlacement(originals,project.assembly,component==0?0:component-1);
    const auto inverse=placement.Inverted();
    BRep_Builder builder;TopoDS_Compound shape;builder.MakeCompound(shape);
    const auto append=[&](const TopoDS_Shape& part){if(!part.IsNull())builder.Add(shape,part);};
    if(component==0)append(placed.wing);else if(component==1)append(placed.fuselage);
    else if(component==2){append(placed.horizontal);append(placed.elevator);}else{append(placed.vertical);append(placed.rudder);}
    if(!TopExp_Explorer{shape,TopAbs_FACE}.More())throw std::runtime_error((names[component]+": generate this component in Assembly before calculating its fiberglass.").toStdString());
    QPainterPath outline;QPointF chord,span,origin;
    struct WingFrame {double sceneRoot,length,y,z,angle;};std::vector<WingFrame> frames;
    if(component==0) {
      span=calibration.spanDirection;chord={-span.y(),span.x()};
      if(QPointF::dotProduct(chord,project.stations.lines.front().second.position-project.stations.lines.front().first.position)<0)chord=-chord;
      double y=0,z=0,angle=0;
      for(std::size_t i=0;i<project.wing.layers.size();++i) {
        const auto& layer=project.wing.layers[i];const auto path=outlinePath(layer,span);outline=outline.united(path);
        auto ends=endpoints(layer);double root=1e100,tip=-1e100;
        for(auto id:ends)root=std::min(root,QPointF::dotProduct(layer.points[id],span));
        for(const auto& polygon:path.toSubpathPolygons())for(auto p:polygon)tip=std::max(tip,QPointF::dotProduct(p,span));
        // Match the builder's near-terminal inner-panel truncation.
        const gui::ConstrainedLine* last=nullptr;for(const auto& station:project.stations.lines)if(station.first.layer==i&&(!last||QPointF::dotProduct(station.first.position+station.second.position,span)>QPointF::dotProduct(last->first.position+last->second.position,span)))last=&station;
        if(last&&i+1<project.wing.layers.size()){const double a=QPointF::dotProduct(last->first.position,span),b=QPointF::dotProduct(last->second.position,span);if(std::max(std::abs(tip-a),std::abs(tip-b))<=(tip-root)*.01&&std::min(a,b)>root+1e-7)tip=std::min(a,b);}
        angle+=project.dihedralDegrees.at(i)*std::numbers::pi/180;const double length=(tip-root)*scale;
        const bool raised=std::any_of(project.dihedralDegrees.begin(),project.dihedralDegrees.end(),[](double a){return a!=0;});
        if(!raised)y=frames.empty()?0:(root-frames.front().sceneRoot)*scale;
        frames.push_back({root,length,y,z,angle});y+=length*std::cos(angle);z+=length*std::sin(angle);
      }
    } else if(component>=2) {
      const auto& layer=project.stabilizerOutlines[component-2].layers[0];auto ends=endpoints(layer);
      if(ends.size()!=2||!layer.leadingEdge)throw std::runtime_error("Define the stabilizer root before fiberglass calculation.");
      if(ends[1]==*layer.leadingEdge)std::swap(ends[0],ends[1]);origin=layer.points[ends[0]];
      chord=layer.points[ends[1]]-origin;chord/=std::hypot(chord.x(),chord.y());span={-chord.y(),chord.x()};outline=outlinePath(layer);
      double extent=0;for(const auto& polygon:outline.toSubpathPolygons())for(auto point:polygon){const double d=QPointF::dotProduct(point-origin,span);if(std::abs(d)>std::abs(extent))extent=d;}if(extent<0)span=-span;
    }
    const auto frameAt=[frames](gp_Pnt p) {
      WingFrame best=frames.front();double distance=1e100;
      for(const auto& frame:frames){const double y=std::abs(p.Y())-frame.y,z=p.Z()-frame.z;const double u=y*std::cos(frame.angle)+z*std::sin(frame.angle),v=-y*std::sin(frame.angle)+z*std::cos(frame.angle);const double d=std::hypot(u-std::clamp(u,0.,frame.length),v);if(d<distance){distance=d;best=frame;}}return best;
    };
    for(std::size_t i=0;i<state.patches.size();++i) {
      const auto& patch=state.patches[i];if(state.sketch.layers[i].curves.empty())continue;
      try {
        FiberglassProjection projection;
        WingFrame currentFrame{};double mirrorSign=1;
        const bool top=patch.side==gui::CoverSide::Top||patch.side==gui::CoverSide::Bottom;
        const double sign=patch.side==gui::CoverSide::Bottom||patch.side==gui::CoverSide::Left?-1:1;
        if(component==1) {
          const auto& layer=project.fuselage.layers[top?0:1];outline=outlinePath(layer);
          const auto sideOutline=outlinePath(project.fuselage.layers[1]);
          const auto transform=fuselageSideTransform(layer,sideOutline.boundingRect().width()*scale);
          projection.point=[=](const gp_Pnt& p){return QPointF{transform.left+p.X()/transform.scale,transform.verticalOrigin+(top?p.Y():-p.Z())/transform.scale};};
          projection.outward=[=](const gp_Pnt&){return top?gp_Dir{0,0,sign}:gp_Dir{0,sign,0};};
        } else if(component==0) {
          // One affine panel frame per triangle. Choosing a mirror independently
          // for each vertex folds dihedral root vertices across Y=0 and distorts
          // the area even for patches far from the root.
          projection.prepareTriangle=[&](const gp_Pnt& placedPoint){const auto p=placedPoint.Transformed(inverse);currentFrame=frameAt(p);mirrorSign=p.Y()<0?-1:1;};
          projection.point=[=,&currentFrame,&mirrorSign](const gp_Pnt& placedPoint){const auto p=placedPoint.Transformed(inverse);const auto& frame=currentFrame;const double u=(p.Y()*mirrorSign-frame.y)*std::cos(frame.angle)+(p.Z()-frame.z)*std::sin(frame.angle);return chord*(p.X()/scale)+span*(frame.sceneRoot+u/scale);};
          projection.outward=[=,&currentFrame,&mirrorSign](const gp_Pnt&){return gp_Dir{0,-std::sin(currentFrame.angle)*mirrorSign*sign,std::cos(currentFrame.angle)*sign}.Transformed(placement);};
        } else {
          projection.point=[=](const gp_Pnt& placedPoint){const auto p=placedPoint.Transformed(inverse);return origin+chord*(p.X()/scale)+span*((component==2?std::abs(p.Y()):p.Z())/scale);};
          projection.outward=[=](const gp_Pnt&){return (component==2?gp_Dir{0,0,sign}:gp_Dir{0,sign,0}).Transformed(placement);};
        }
        const auto region=fiberglassRegion(state.sketch.layers[i],outline);
        auto covered=measureFiberglass(shape,region,projection,patch.wrap,false);covered.name=names[component]+" / "+patch.name;
        covered.clothGrams=covered.areaMm2*patch.clothGm2*1e-6;covered.resinVolumeMm3=covered.areaMm2*gui::resinThickness(patch);measured[component].push_back(covered);
      } catch(const std::exception& error){throw std::runtime_error((names[component]+" / "+patch.name+": "+error.what()).toStdString());}
    }
  },{},workers);
  for(auto& component:measured)for(auto& patch:component)result.push_back(std::move(patch));
  return result;
}
}

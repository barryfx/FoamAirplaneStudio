#include "geometry/LighteningCut.h"
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace designrc::geometry {
namespace {
struct Triangle {std::array<gp_Pnt,3> p;double x0,y0,x1,y1;};
// Clip a triangulated boundary to a padded XY column. Taking the extrema of
// the clipped polygon bounds every surface point over that whole column.
std::vector<gp_Pnt> clip(std::vector<gp_Pnt> p,int axis,double limit,bool greater) {
  std::vector<gp_Pnt> result;if(p.empty())return result;
  auto value=[&](const gp_Pnt& v){return axis==0?v.X():v.Y();};
  auto a=p.back();bool inA=greater?value(a)>=limit:value(a)<=limit;
  for(const auto& b:p) {
    const bool inB=greater?value(b)>=limit:value(b)<=limit;
    if(inA!=inB){const double t=(limit-value(a))/(value(b)-value(a));result.emplace_back(a.X()+t*(b.X()-a.X()),a.Y()+t*(b.Y()-a.Y()),a.Z()+t*(b.Z()-a.Z()));}
    if(inB)result.push_back(b);a=b;inA=inB;
  }return result;
}
}
TopoDS_Shape lighteningCavities(const TopoDS_Shape& main,double wall,
    const std::vector<std::pair<double,double>>& bays,
    const std::function<std::pair<double,double>(double,double)>& splitRange,const ProcessingControl& control) {
  control.checkpoint();
  BRep_Builder builder;TopoDS_Compound result;builder.MakeCompound(result);
  if(bays.empty())return result;
  // The extra mesh allowance and L-infinity envelope are conservative: a
  // surface must be separated along at least one axis by the requested wall.
  // They deliberately leave more material at steep skins and narrow features.
  constexpr double deflection=.01;
  IMeshTools_Parameters parameters;parameters.Deflection=deflection;parameters.Angle=.08;parameters.InParallel=true;
  BRepMesh_IncrementalMesh mesh{main,parameters,control.range()};control.checkpoint();
  if(!mesh.IsDone())throw std::runtime_error("Could not sample wing skins for lightening.");
  const double clearance=wall+2*deflection+1e-5;
  std::vector<Triangle> triangles;
  for(TopExp_Explorer e{main,TopAbs_FACE};e.More();e.Next()) {
    TopLoc_Location location;const auto t=BRep_Tool::Triangulation(TopoDS::Face(e.Current()),location);
    if(t.IsNull())throw std::runtime_error("A wing face could not be sampled for lightening.");
    for(int i=1;i<=t->NbTriangles();++i) {
      int a,b,c;t->Triangle(i).Get(a,b,c);Triangle v{{t->Node(a).Transformed(location.Transformation()),t->Node(b).Transformed(location.Transformation()),t->Node(c).Transformed(location.Transformation())}};
      v.x0=std::min({v.p[0].X(),v.p[1].X(),v.p[2].X()});v.x1=std::max({v.p[0].X(),v.p[1].X(),v.p[2].X()});
      v.y0=std::min({v.p[0].Y(),v.p[1].Y(),v.p[2].Y()});v.y1=std::max({v.p[0].Y(),v.p[1].Y(),v.p[2].Y()});triangles.push_back(v);
    }
  }
  Bnd_Box box;BRepBndLib::AddOptimal(main,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
  const double margin=std::max({x1-x0,y1-y0,z1-z0,1.0})*2;
  IntCurvesFace_ShapeIntersector ray;ray.Load(main,1e-7);
  const double dx=(x1-x0)/24;
  for(const auto& [begin,end]:bays) {
    const double start=std::max(begin,y0+clearance+1e-4),stop=std::min(end,y1-clearance-1e-4);
    if(stop<=start)continue;
    const int slices=std::max(1,static_cast<int>(std::ceil((stop-start)/std::max(25.0,(y1-y0)/12))));
    for(int j=0;j<slices;++j) {
      control.checkpoint();
      const double a=start+(stop-start)*j/slices,b=start+(stop-start)*(j+1)/slices;
      const auto [splitLow,splitHigh]=splitRange(a,b);const double reference=(splitLow+splitHigh)*.5;
      // One stepped profile per contiguous run avoids coincident box faces
      // inside the boolean tools. Adjacent span slices meet along their caps.
      struct Column {double left,right,low,high;};std::vector<Column> run;
      auto flush=[&]{
        if(run.empty())return;
        std::vector<gp_Pnt> points;
        for(const auto& c:run){points.emplace_back(c.left,a,c.low);points.emplace_back(c.right,a,c.low);}
        for(auto it=run.rbegin();it!=run.rend();++it){points.emplace_back(it->right,a,it->high);points.emplace_back(it->left,a,it->high);}
        points.erase(std::unique(points.begin(),points.end(),[](const auto& p,const auto& q){return p.Distance(q)<1e-8;}),points.end());
        for(std::size_t k=1;k+1<points.size();) {
          const gp_Vec u{points[k-1],points[k]},v{points[k],points[k+1]};
          if(u.Crossed(v).Magnitude()<1e-9 && u.Dot(v)>0)points.erase(points.begin()+k);else ++k;
        }
        BRepBuilderAPI_MakePolygon polygon;for(const auto& p:points)polygon.Add(p);polygon.Close();
        const auto face=BRepBuilderAPI_MakeFace{polygon.Wire()}.Face();
        builder.Add(result,BRepPrimAPI_MakePrism{face,gp_Vec{0,b-a,0}}.Shape());run.clear();
      };
      for(int i=0;i<24;++i) {
        control.checkpoint();
        const double left=x0+i*dx,right=left+dx;
        const double x=(left+right)*.5,y=(a+b)*.5;
        ray.Perform(gp_Lin{gp_Pnt{x,y,z0-margin},gp_Dir{0,0,1}},0,z1-z0+2*margin);
        std::vector<double> hits;if(ray.IsDone())for(int n=1;n<=ray.NbPnt();++n)hits.push_back(ray.Pnt(n).Z());
        std::sort(hits.begin(),hits.end());hits.erase(std::unique(hits.begin(),hits.end(),[](double u,double v){return std::abs(u-v)<1e-7;}),hits.end());
        bool inside=false;for(std::size_t k=0;k+1<hits.size();k+=2)if(hits[k]<reference && hits[k+1]>reference)inside=true;
        double low=z0-margin,high=z1+margin;
        if(inside)for(const auto& t:triangles) {
          if(t.x1<=left-clearance || t.x0>=right+clearance || t.y1<=a-clearance || t.y0>=b+clearance)continue;
          std::vector<gp_Pnt> polygon(t.p.begin(),t.p.end());
          polygon=clip(std::move(polygon),0,left-clearance,true);polygon=clip(std::move(polygon),0,right+clearance,false);
          polygon=clip(std::move(polygon),1,a-clearance,true);polygon=clip(std::move(polygon),1,b+clearance,false);
          if(polygon.empty())continue;
          double lo=1e100,hi=-1e100;for(const auto& p:polygon){lo=std::min(lo,p.Z());hi=std::max(hi,p.Z());}
          if(hi<reference)low=std::max(low,hi+clearance);
          else if(lo>reference)high=std::min(high,lo-clearance);
          else {inside=false;break;}
        }
        // Each pocket crosses the complete local split, so it is open for access.
        if(!inside || low>=splitLow-1e-5 || high<=splitHigh+1e-5 || low<z0-margin*.5 || high>z1+margin*.5){flush();continue;}
        run.push_back({left,right,low,high});
      }flush();
    }
  }
  // Boolean tools must be a union, not a compound of touching prisms:
  // retained coincident caps would otherwise become zero-thickness membranes.
  NCollection_List<TopoDS_Shape> args,tools;
  for(TopExp_Explorer e{result,TopAbs_SOLID};e.More();e.Next()) {
    if(args.IsEmpty())args.Append(e.Current());else tools.Append(e.Current());
  }
  if(tools.IsEmpty())return result;
  BRepAlgoAPI_Fuse unite;unite.SetArguments(args);unite.SetTools(tools);
  unite.SetNonDestructive(true);unite.SetRunParallel(true);unite.SetUseOBB(true);unite.Build(control.range());control.checkpoint();
  if(!unite.IsDone())throw std::runtime_error("Could not join lightening pocket sections.");
  unite.SimplifyResult(true,true);
  if(!BRepCheck_Analyzer{unite.Shape()}.IsValid())throw std::runtime_error("Invalid joined lightening pockets.");
  return unite.Shape();
}
}

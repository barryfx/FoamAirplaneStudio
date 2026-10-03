#include "geometry/FuselageStiffeners.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <gp_Circ.hxx>
#include <algorithm>
#include <numbers>
#include <stdexcept>
namespace designrc::geometry {
namespace {
FuselageWallSection sectionAt(const std::vector<FuselageWallSection>& sections,double x) {
  auto upper=std::upper_bound(sections.begin(),sections.end(),x,[](double value,const auto& s){return value<s.x;});
  const auto& a=upper==sections.begin()?sections.front():*(upper-1);const auto& b=upper==sections.end()?sections.back():*upper;
  if(a.perimeter.size()<4||a.perimeter.size()!=b.perimeter.size())throw std::runtime_error("Stiffener reaches a pointed end; shorten its Start/Stop range.");
  const double t=b.x>a.x?std::clamp((x-a.x)/(b.x-a.x),0.,1.):0;
  FuselageWallSection result{x,a.thickness*(1-t)+b.thickness*t,{}};
  for(std::size_t i=0;i<a.perimeter.size();++i)result.perimeter.push_back(a.perimeter[i]*(1-t)+b.perimeter[i]*t);return result;
}
double skinY(const FuselageWallSection& section,double z) {
  double y=-1;
  for(std::size_t i=0;i<section.perimeter.size();++i){auto a=section.perimeter[i],b=section.perimeter[(i+1)%section.perimeter.size()];
    if(std::abs(b.y()-a.y())<1e-10)continue;const double t=(z-a.y())/(b.y()-a.y());if(t>=0&&t<=1)y=std::max(y,a.x()+t*(b.x()-a.x()));}
  if(y<=0)throw std::runtime_error("Stiffener does not fit on the side; reduce count/size or shorten its range.");return y;
}
int solids(const TopoDS_Shape& shape){int n=0;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())++n;return n;}
}
TopoDS_Shape cutFuselageStiffeners(const TopoDS_Shape& body,const std::vector<FuselageWallSection>& sections,
    double length,const gui::StiffenerState& settings,std::vector<SparMaterial>& materials,const ProcessingControl& control,bool rightHalf) {
  gui::validateStiffeners(settings);if(!settings.count)return body;
  if(body.IsNull()||sections.size()<2||!std::isfinite(length)||length<=0)throw std::runtime_error("Stiffeners require a complete fuselage.");
  const bool round=settings.shape==gui::SparShape::Round;const double width=round?settings.diameterMm:settings.widthMm;
  const double depth=round?settings.diameterMm/2:settings.heightMm;
  const double start=length*settings.startPercent/100,stop=length*settings.stopPercent/100;
  NCollection_List<TopoDS_Shape> tools;std::vector<SparMaterial> stock;
  for(int number=0;number<settings.count;++number) {
    std::vector<gp_Pnt> centers;
    for(int j=0;j<=64;++j) {
      control.checkpoint();const double x=start+(stop-start)*j/64.;const auto section=sectionAt(sections,x);
      double low=section.perimeter.front().y(),high=low;for(auto point:section.perimeter){low=std::min(low,point.y());high=std::max(high,point.y());}
      const double spacing=(high-low)/(settings.count+1),z=low+spacing*(number+1);
      if(spacing<=width+1e-4)throw std::runtime_error("Stiffeners are too close to each other or the top/bottom; reduce their count or width.");
      if(section.thickness>0&&depth>=section.thickness-1e-4)throw std::runtime_error("Stiffener groove would reach the cavity; reduce depth/diameter or increase wall thickness.");
      const double y=skinY(section,z);centers.emplace_back(x,y,z);
      // Check the deepest edge across the width against the actual body, not
      // just nominal wall thickness (rounded profiles have oblique sidewalls).
      const double sampleX=std::clamp(x,start+std::min(.001,(stop-start)/100),stop-std::min(.001,(stop-start)/100));
      for(double dz:{-width*.49,0.,width*.49}) {
        const double inward=round?std::sqrt(std::max(0.,depth*depth-dz*dz)):depth;
        const gp_Pnt probe{sampleX,y-inward*.999,z+dz};bool inside=false;
        for(TopExp_Explorer e{body,TopAbs_SOLID};e.More();e.Next()){BRepClass3d_SolidClassifier classifier{TopoDS::Solid(e.Current()),probe,1e-6};if(classifier.State()==TopAbs_IN||classifier.State()==TopAbs_ON){inside=true;break;}}
        if(!inside)throw std::runtime_error("Stiffener crosses an opening or leaves the side wall; adjust its range, width or depth.");
      }
    }
    for(std::size_t i=2;i<centers.size();++i) {
      const gp_Vec a{centers[i-2],centers[i-1]},b{centers[i-1],centers[i]};
      if(a.Angle(b)>15*std::numbers::pi/180)throw std::runtime_error("Stiffener route bends abruptly; move Start/Stop onto a smoother portion of the boom.");
    }
    // A smooth cubic loft avoids an angular joint at each sampled section.
    // Remove exactly collinear samples so straight booms create simple tools.
    for(std::size_t i=1;i+1<centers.size();) {
      const auto& a=centers[i-1];const auto& b=centers[i];const auto& c=centers[i+1];const double t=(b.X()-a.X())/(c.X()-a.X());
      if(gp_Pnt{a.XYZ()+(c.XYZ()-a.XYZ())*t}.Distance(b)<1e-8)centers.erase(centers.begin()+i);else ++i;
    }
    for(int side:{1,-1}) {
      if(rightHalf&&side<0)break;
      const auto loft=[&](bool cutting) {
        BRepOffsetAPI_ThruSections result{true,false,1e-7};result.CheckCompatibility(false);result.SetMaxDegree(3);
        for(auto center:centers) {
          center.SetY(center.Y()*side);control.checkpoint();
          if(round){gp_Circ circle{gp_Ax2{center,gp_Dir{1,0,0},gp_Dir{0,1,0}},settings.diameterMm/2};result.AddWire(BRepBuilderAPI_MakeWire{BRepBuilderAPI_MakeEdge{circle}.Edge()}.Wire());}
          else {const double inner=center.Y()-side*depth,outer=center.Y()+side*(cutting?std::max(width,depth):0.);BRepBuilderAPI_MakePolygon wire;
            for(auto p:{gp_Pnt{center.X(),inner,center.Z()-width/2},gp_Pnt{center.X(),outer,center.Z()-width/2},gp_Pnt{center.X(),outer,center.Z()+width/2},gp_Pnt{center.X(),inner,center.Z()+width/2}})wire.Add(p);wire.Close();result.AddWire(wire.Wire());}
        }
        result.Build(control.range());control.checkpoint();if(!result.IsDone()||!BRepCheck_Analyzer{result.Shape()}.IsValid())throw std::runtime_error("Could not create a smooth stiffener groove; adjust its range.");return result.Shape();
      };
      const auto material=loft(false);GProp_GProps mass;BRepGProp::VolumeProperties(material,mass,1e-7);
      if(!std::isfinite(mass.Mass())||std::abs(mass.Mass())<1e-9)throw std::runtime_error("Stiffener has no measurable volume.");
      stock.push_back({std::string{"Fuselage stiffener "}+std::to_string(number+1)+(side==1?" / Right":" / Left"),std::abs(mass.Mass()),mass.CentreOfMass(),true});
      tools.Append(round?material:loft(true));
    }
  }
  control.checkpoint();BRepAlgoAPI_Cut cut;NCollection_List<TopoDS_Shape> args;args.Append(body);cut.SetArguments(args);cut.SetTools(tools);cut.SetNonDestructive(true);cut.SetRunParallel(control.parallel);cut.Build(control.range());control.checkpoint();
  if(!cut.IsDone()||cut.HasErrors()||!BRepCheck_Analyzer{cut.Shape(),true,control.parallel}.IsValid()||solids(cut.Shape())!=solids(body))throw std::runtime_error("Stiffener grooves disconnect or invalidate the fuselage; reduce their size or change the range.");
  materials.insert(materials.end(),stock.begin(),stock.end());return cut.Shape();
}
}

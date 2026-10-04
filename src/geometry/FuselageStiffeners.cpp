#include "geometry/FuselageStiffeners.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <sstream>
#include <iomanip>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Circ.hxx>
#include <algorithm>
#include <numbers>
#include <memory>
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
std::string location(double first,double last,double length) {
  std::ostringstream out;out<<std::fixed<<std::setprecision(2)<<100*first/length;
  if(std::abs(last-first)>1e-7)out<<"–"<<100*last/length;
  return out.str()+"% from the nose";
}
std::string shapeLocation(const TopoDS_Shape& shape,double length,const ProcessingControl& control) {
  control.checkpoint();Bnd_Box box;BRepBndLib::AddOptimal(shape,box,false,false);control.checkpoint();
  if(box.IsVoid())return "location unavailable";
  return location(box.CornerMin().X(),box.CornerMax().X(),length);
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
  // The body is immutable until all clearance checks finish. Load each solid
  // once and reuse its classifier for successive probes. Classifiers hold
  // mutable query state, so ownership stays local to this generation call.
  std::vector<std::unique_ptr<BRepClass3d_SolidClassifier>> clearance;
  for(TopExp_Explorer e{body,TopAbs_SOLID};e.More();e.Next()) {
    control.checkpoint();clearance.push_back(std::make_unique<BRepClass3d_SolidClassifier>(e.Current()));
  }
  for(int number=0;number<settings.count;++number) {
    std::vector<gp_Pnt> centers;
    for(int j=0;j<=64;++j) {
      control.checkpoint();const double x=start+(stop-start)*j/64.;
      auto fail=[&](const std::string& reason){throw std::runtime_error("Stiffener "+std::to_string(number+1)+" at "+location(x,x,length)+": "+reason);};
      FuselageWallSection section;try{section=sectionAt(sections,x);}catch(const std::runtime_error& e){fail(e.what());}
      double low=section.perimeter.front().y(),high=low;for(auto point:section.perimeter){low=std::min(low,point.y());high=std::max(high,point.y());}
      const double spacing=(high-low)/(settings.count+1),z=low+spacing*(number+1);
      if(spacing<=width+1e-4)fail("Stiffeners are too close to each other or the top/bottom; reduce their count or width.");
      if(section.thickness>0&&depth>=section.thickness-1e-4)fail("Stiffener groove would reach the cavity; reduce depth/diameter or increase wall thickness.");
      double y=0;try{y=skinY(section,z);}catch(const std::runtime_error& e){fail(e.what());}centers.emplace_back(x,y,z);
      // Check the deepest edge across the width against the actual body, not
      // just nominal wall thickness (rounded profiles have oblique sidewalls).
      const double sampleX=std::clamp(x,start+std::min(.001,(stop-start)/100),stop-std::min(.001,(stop-start)/100));
      for(double dz:{-width*.49,0.,width*.49}) {
        const double inward=round?std::sqrt(std::max(0.,depth*depth-dz*dz)):depth;
        const gp_Pnt probe{sampleX,y-inward*.999,z+dz};bool inside=false;
        for(const auto& classifier:clearance){classifier->Perform(probe,1e-6);if(classifier->State()==TopAbs_IN||classifier->State()==TopAbs_ON){inside=true;break;}}
        if(!inside)fail("Stiffener crosses an opening or leaves the side wall; adjust its range, width or depth.");
      }
    }
    for(std::size_t i=2;i<centers.size();++i) {
      const gp_Vec a{centers[i-2],centers[i-1]},b{centers[i-1],centers[i]};
      if(a.Angle(b)>15*std::numbers::pi/180)throw std::runtime_error("Stiffener "+std::to_string(number+1)+" at "+location(centers[i-1].X(),centers[i-1].X(),length)+": route bends more than 15 degrees between samples; move Start/Stop onto a smoother portion of the boom.");
    }
    // Ruled spans stay between their endpoint sections. A global smooth loft
    // can fold back or overshoot despite valid samples (see ADR-0055). Remove
    // only collinear samples so straight booms still create simple tools.
    for(std::size_t i=1;i+1<centers.size();) {
      const auto& a=centers[i-1];const auto& b=centers[i];const auto& c=centers[i+1];const double t=(b.X()-a.X())/(c.X()-a.X());
      if(gp_Pnt{a.XYZ()+(c.XYZ()-a.XYZ())*t}.Distance(b)<1e-8)centers.erase(centers.begin()+i);else ++i;
    }
    for(int side:{1,-1}) {
      if(rightHalf&&side<0)break;
      const auto loft=[&](bool cutting) {
        BRepOffsetAPI_ThruSections result{true,true,1e-7};result.CheckCompatibility(false);
        for(auto center:centers) {
          center.SetY(center.Y()*side);control.checkpoint();
          if(round){gp_Circ circle{gp_Ax2{center,gp_Dir{1,0,0},gp_Dir{0,1,0}},settings.diameterMm/2};result.AddWire(BRepBuilderAPI_MakeWire{BRepBuilderAPI_MakeEdge{circle}.Edge()}.Wire());}
          else {const double inner=center.Y()-side*depth,outer=center.Y()+side*(cutting?std::max(width,depth):0.);BRepBuilderAPI_MakePolygon wire;
            for(auto p:{gp_Pnt{center.X(),inner,center.Z()-width/2},gp_Pnt{center.X(),outer,center.Z()-width/2},gp_Pnt{center.X(),outer,center.Z()+width/2},gp_Pnt{center.X(),inner,center.Z()+width/2}})wire.Add(p);wire.Close();result.AddWire(wire.Wire());}
        }
        result.Build(control.range());control.checkpoint();if(!result.IsDone()||!BRepCheck_Analyzer{result.Shape()}.IsValid())throw std::runtime_error("Could not create a valid tool for stiffener "+std::to_string(number+1)+(side==1?" / Right":" / Left")+" over "+location(start,stop,length)+". Exact failure position is unavailable; adjust its range.");return result.Shape();
      };
      const auto material=loft(false);GProp_GProps mass;BRepGProp::VolumeProperties(material,mass,1e-7);
      if(!std::isfinite(mass.Mass())||std::abs(mass.Mass())<1e-9)throw std::runtime_error("Stiffener has no measurable volume.");
      stock.push_back({std::string{"Fuselage stiffener "}+std::to_string(number+1)+(side==1?" / Right":" / Left"),std::abs(mass.Mass()),mass.CentreOfMass(),true});
      tools.Append(round?material:loft(true));
    }
  }
  control.checkpoint();BRepAlgoAPI_Cut cut;NCollection_List<TopoDS_Shape> args;args.Append(body);cut.SetArguments(args);cut.SetTools(tools);cut.SetNonDestructive(true);cut.SetRunParallel(control.parallel);cut.Build(control.range());control.checkpoint();
  if(!cut.IsDone()||cut.HasErrors()) {
    std::ostringstream errors;cut.DumpErrors(errors);
    throw std::runtime_error("Stiffener Boolean cut failed over "+location(start,stop,length)+
      ". Exact failure position is unavailable. OCCT: "+errors.str());
  }
  BRepCheck_Analyzer validation{cut.Shape(),true,control.parallel};
  if(!validation.IsValid()) {
    std::string at="requested range "+location(start,stop,length)+"; exact failure position unavailable";
    for(TopExp_Explorer face{cut.Shape(),TopAbs_FACE};face.More();face.Next()) {
      control.checkpoint();if(!validation.IsValid(face.Current())) {at="invalid face spans "+shapeLocation(face.Current(),length,control);break;}
    }
    throw std::runtime_error("Stiffener cut produced invalid topology: "+at+". Reduce groove size or change Start/Stop.");
  }
  const int expected=solids(body),actual=solids(cut.Shape());
  if(actual!=expected) {
    std::vector<std::pair<double,TopoDS_Shape>> pieces;
    for(TopExp_Explorer e{cut.Shape(),TopAbs_SOLID};e.More();e.Next()) {
      control.checkpoint();GProp_GProps mass;BRepGProp::VolumeProperties(e.Current(),mass);
      pieces.emplace_back(std::abs(mass.Mass()),e.Current());
    }
    std::sort(pieces.begin(),pieces.end(),[](const auto& a,const auto& b){return a.first>b.first;});
    std::ostringstream message;message<<"Stiffener cut produced "<<actual<<" solids; expected "<<expected<<". ";
    if(actual>expected) {
      message<<"The body disconnected or the Boolean created fragments. ";
      for(int i=expected;i<actual&&i<expected+3;++i)
        message<<"Extra piece "<<i-expected+1<<": "<<std::setprecision(6)<<pieces[i].first<<" mm^3, spans "
          <<shapeLocation(pieces[i].second,length,control)<<". ";
      message<<"These spans locate the extra pieces, not an exact break point. ";
    } else message<<"Material was removed or solids merged over "<<location(start,stop,length)<<"; exact failure position unavailable. ";
    message<<"Reduce groove width/depth or change Start/Stop.";throw std::runtime_error(message.str());
  }
  materials.insert(materials.end(),stock.begin(),stock.end());return cut.Shape();
}
}

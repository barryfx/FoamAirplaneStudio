#include "geometry/SparCut.h"
#include "geometry/LighteningCut.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepAlgoAPI_Splitter.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Circ.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <Standard_Failure.hxx>
#include <vector>
namespace designrc::geometry {
namespace {
void valid(const TopoDS_Shape& s,const char* message,bool parallel) {
  int n=0;for(TopExp_Explorer e{s,TopAbs_SOLID};e.More();e.Next())++n;
  if(n!=1)throw std::runtime_error(std::string{message}+" ("+std::to_string(n)+" solids)");
  if(!BRepCheck_Analyzer{s,true,parallel}.IsValid())throw std::runtime_error(std::string{message}+" (invalid topology)");
}
TopoDS_Shape cut(const TopoDS_Shape& s,const TopoDS_Shape& tool,const ProcessingControl& control) {
  control.checkpoint();
  BRepAlgoAPI_Cut op;NCollection_List<TopoDS_Shape> args,tools;args.Append(s);tools.Append(tool);
  op.SetArguments(args);op.SetTools(tools);op.SetNonDestructive(true);op.SetRunParallel(control.parallel);
  op.Build(control.range());control.checkpoint();if(!op.IsDone())throw std::runtime_error("Could not cut spar geometry.");return op.Shape();
}

}
TopoDS_Shape cutSpars(const TopoDS_Shape& half,const gui::SparState& spars,double halfSpan,
    const std::function<std::pair<double,double>(double)>& chordAtSpan,
    const std::function<void(const char*)>& progress, double rootInset,double tipInset,bool mitered,double lighteningWall,const std::vector<std::pair<double,double>>& bays,const ProcessingControl& control,std::vector<SparMaterial>* materials) {
  control.checkpoint();
  const bool lighten=lighteningWall>0;const bool split=spars[2].enabled || lighten;
  if(!lighten && std::none_of(spars.begin(),spars.end(),[](const auto& s){return s.enabled;}))return half;
  if(!std::isfinite(halfSpan) || halfSpan<=0)throw std::runtime_error("Invalid spar span.");
  if(!std::isfinite(rootInset) || !std::isfinite(tipInset) || rootInset<0 || tipInset<0 || rootInset+tipInset>=halfSpan*(1-2e-6))
    throw std::runtime_error("The oblique root leaves no usable span for the spar.");
  std::vector<TopoDS_Shape> bodies;
  for(TopExp_Explorer e{half,TopAbs_SOLID};e.More();e.Next())bodies.push_back(e.Current());
  if(bodies.empty())throw std::runtime_error("Spars require a solid wing.");
  auto original=bodies.front();
  if(lighten) {
    if(progress)progress("Preparing wing faces for lightening...");
    // Remove redundant coplanar loft seams before the numerous pocket cuts.
    // Keep geometry and topology validation; this is not an approximation.
    ShapeUpgrade_UnifySameDomain unify{original,true,true,false};unify.Build();
    valid(unify.Shape(),"Could not prepare the main wing for lightening.",control.parallel);original=unify.Shape();
  }
  auto fixed=original;
  Bnd_Box bounds;BRepBndLib::Add(original,bounds);
  double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
  const double margin=2*std::max({x1-x0,y1-y0,z1-z0,1.0})+std::max(std::abs(z0),std::abs(z1))+1;
  IntCurvesFace_ShapeIntersector ray;ray.Load(original,1e-7);
  auto heights=[&](double x,double y) {
    control.checkpoint();ray.Perform(gp_Lin{gp_Pnt{x,y,-margin},gp_Dir{0,0,1}},0,2*margin);
    if(!ray.IsDone() || ray.NbPnt()<2)throw std::runtime_error("Spar or alignment feature is outside the fixed wing; change its chord location or length.");
    double low=margin,high=-margin;
    for(int i=1;i<=ray.NbPnt();++i){low=std::min(low,ray.Pnt(i).Z());high=std::max(high,ray.Pnt(i).Z());}
    return std::pair{low,high};
  };
  auto position=[&](const gui::Spar& s,double y) {
    const auto [le,te]=chordAtSpan(y);return le+(te-le)*s.chordPercent/100;
  };
  auto splitLocation=spars[2];if(!spars[2].enabled)splitLocation.chordPercent=30;
  std::vector<gp_Pnt> midCenters;std::array<TopoDS_Shape,3> sparTools;
  for(int index=0;index<3;++index) {
    const auto& spar=spars[index];if(!spar.enabled)continue;
    const std::string name=index==0?"Top spar":index==1?"Bottom spar":"Mid spar";
    try {
    if(!std::isfinite(spar.chordPercent)||spar.chordPercent<0||spar.chordPercent>100 ||
       !std::isfinite(spar.lengthPercent)||spar.lengthPercent<=0||spar.lengthPercent>100 ||
       !std::isfinite(spar.sizeMm)||spar.sizeMm<=0 || !std::isfinite(spar.heightMm)||spar.heightMm<=0 ||
       (spar.shape!=gui::SparShape::Round && spar.shape!=gui::SparShape::Strip) || (index==2 && (spar.shape!=gui::SparShape::Round || !std::isfinite(spar.insideDiameterMm) || spar.insideDiameterMm<0 || spar.insideDiameterMm>=spar.sizeMm)))
      throw std::runtime_error("Invalid spar dimensions or shape.");
    if(progress)progress(index==0?"Cutting top spar groove...":index==1?"Cutting bottom spar groove...":"Cutting mid-height spar hole...");
    const double end=halfSpan*spar.lengthPercent/100;
    BRepOffsetAPI_ThruSections tool{true,index==2,1e-7};tool.CheckCompatibility(false);tool.SetMaxDegree(3);
    std::vector<gp_Pnt> centers,materialCenters;
    // Sample the actual solid skin, including the selected wing-tip treatment.
    // All profiles lie in parallel chord/height planes so sizes remain explicit.
    std::vector<double> sampleSpans;
    for(int n=0;n<=32;++n)sampleSpans.push_back(end*n/32.0);
    if(index==2)for(double fraction:{.2,.8})if(halfSpan*fraction<end)sampleSpans.push_back(halfSpan*fraction);
    std::sort(sampleSpans.begin(),sampleSpans.end());
    sampleSpans.erase(std::unique(sampleSpans.begin(),sampleSpans.end(),[](double a,double b){return std::abs(a-b)<1e-8;}),sampleSpans.end());
    for(const double y:sampleSpans) {
      // At an oblique root, sample beyond the complete cap, then extend the
      // groove tool to the root. The solid cap trims away its outside overrun.
      const double sampleY=std::clamp(y,rootInset+halfSpan*(rootInset>0?1e-6:1e-8),halfSpan*(1-1e-8)-tipInset);
      const double x=position(spar,sampleY);const auto [low,high]=heights(x,sampleY);
      const double z=index==0?high:index==1?low:(low+high)*0.5;
      const double extent=spar.shape==gui::SparShape::Round?spar.sizeMm/2:spar.heightMm;
      if((index==0 && z-extent<=low+1e-6) || (index==1 && z+extent>=high-1e-6) ||
         (index==2 && y<=end+1e-8 && high-low<=spar.sizeMm+1e-6))
      {
        std::ostringstream message;message<<std::fixed<<std::setprecision(3);
        message<<"At "<<100*y/halfSpan<<"% panel span from its root ("<<y<<" mm), ";
        if(index==2) {
          message<<"the "<<spar.sizeMm<<" mm diameter hole needs "<<extent
                 <<" mm above and below the 50%-thickness center, but local thickness is "
                 <<high-low<<" mm. Reduce diameter or change the chord location/length.";
        } else {
          message<<"the groove needs "<<extent<<" mm inward depth, but local wing thickness is "
                 <<high-low<<" mm. Reduce the "<<(spar.shape==gui::SparShape::Round?"diameter":"height")<<" or change its chord location/length.";
        }
        throw std::runtime_error(message.str());
      }
      // A slight root overrun avoids a coincident end face at the mirror plane.
      const double planeY=y==0?(mitered?std::min(0.0,y0):0)-1e-5:
          mitered && spar.lengthPercent==100 && y==end?std::max(end,y1)+1e-5:y;
      centers.emplace_back(x,planeY,z);
      if(materials)materialCenters.emplace_back(x,y,z);
    }
    if(materials) {
      // Use the same sampled skin and section orientation as the groove, but
      // stop at nominal panel span/length (not the boolean tool's cap overruns).
      // Surface strips occupy only their entered inward depth. Tube bores are
      // subtracted as volume/moments, avoiding another CAD boolean operation.
      const auto measure=[&](double diameter) {
        BRepOffsetAPI_ThruSections stock{true,index==2,1e-7};stock.CheckCompatibility(false);stock.SetMaxDegree(3);
        for(const auto& center:materialCenters) {
          control.checkpoint();
          if(spar.shape==gui::SparShape::Round) {
            const gp_Circ circle{gp_Ax2{center,gp_Dir{0,1,0},gp_Dir{1,0,0}},diameter/2};
            stock.AddWire(BRepBuilderAPI_MakeWire{BRepBuilderAPI_MakeEdge{circle}.Edge()}.Wire());
          } else {
            const double bottom=index==0?center.Z()-spar.heightMm:center.Z();
            BRepBuilderAPI_MakePolygon wire;
            for(auto p:{gp_Pnt{center.X()-spar.sizeMm/2,center.Y(),bottom},gp_Pnt{center.X()+spar.sizeMm/2,center.Y(),bottom},
                gp_Pnt{center.X()+spar.sizeMm/2,center.Y(),bottom+spar.heightMm},gp_Pnt{center.X()-spar.sizeMm/2,center.Y(),bottom+spar.heightMm}})wire.Add(p);
            wire.Close();stock.AddWire(wire.Wire());
          }
        }
        stock.Build(control.range());control.checkpoint();
        if(!stock.IsDone())throw std::runtime_error("Could not measure carbon fiber spar stock.");
        GProp_GProps props;BRepGProp::VolumeProperties(stock.Shape(),props,1e-7);control.checkpoint();return props;
      };
      const auto outer=measure(spar.sizeMm);double volume=outer.Mass();gp_XYZ moment=outer.CentreOfMass().XYZ()*volume;
      if(index==2 && spar.insideDiameterMm>0) {
        const auto inner=measure(spar.insideDiameterMm);volume-=inner.Mass();moment-=inner.CentreOfMass().XYZ()*inner.Mass();
      }
      if(!std::isfinite(volume)||volume<=0)throw std::runtime_error("Carbon fiber spar has no measurable material volume.");
      materials->push_back({name,volume,gp_Pnt{moment/volume}});
    }
    if(index==2) {
      midCenters=centers;
      // Beyond the physical spar, continue the last split height to the tip.
      // Tip shaping must not turn a stopped spar into an unnecessary curved joint.
      if(end<halfSpan)midCenters.emplace_back(position(spar,halfSpan*(1-1e-8)),halfSpan,midCenters.back().Z());
    }
    // Keep shape changes, but avoid dozens of coincident cylindrical/planar
    // sections on straight spar runs. Smooth lofts avoid an edge at every sample.
    for(std::size_t i=1;i+1<centers.size();) {
      const auto& a=centers[i-1];const auto& b=centers[i];const auto& c=centers[i+1];
      const double t=(b.Y()-a.Y())/(c.Y()-a.Y());
      const gp_Pnt linear{a.X()+(c.X()-a.X())*t,b.Y(),a.Z()+(c.Z()-a.Z())*t};
      if(linear.Distance(b)<1e-7)centers.erase(centers.begin()+i);else ++i;
    }
    for(const auto& center:centers) {
      const double x=center.X(),planeY=center.Y(),z=center.Z();
      if(spar.shape==gui::SparShape::Round) {
        const gp_Circ circle{gp_Ax2{gp_Pnt{x,planeY,z},gp_Dir{0,1,0},gp_Dir{1,0,0}},spar.sizeMm/2};
        tool.AddWire(BRepBuilderAPI_MakeWire{BRepBuilderAPI_MakeEdge{circle}.Edge()}.Wire());
      } else {
        BRepBuilderAPI_MakePolygon wire;
        const double bottom=index==0?z-spar.heightMm:-margin;
        const double top=index==0?margin:z+spar.heightMm;
        for(auto p:{gp_Pnt{x-spar.sizeMm/2,planeY,bottom},gp_Pnt{x+spar.sizeMm/2,planeY,bottom},
                    gp_Pnt{x+spar.sizeMm/2,planeY,top},gp_Pnt{x-spar.sizeMm/2,planeY,top}})wire.Add(p);
        wire.Close();tool.AddWire(wire.Wire());
      }
    }
    tool.Build(control.range());control.checkpoint();if(!tool.IsDone())throw std::runtime_error("Could not construct the spar groove.");
    sparTools[index]=tool.Shape();if(!split)fixed=cut(fixed,tool.Shape(),control);valid(fixed,"The spar cuts disconnect the fixed wing; change their sizes or locations.",control.parallel);
    } catch(const Standard_Failure& e) {throw std::runtime_error(name+": "+e.what());}
      catch(const std::exception& e) {throw std::runtime_error(name+": "+e.what());}
  }
  if(lighten && !spars[2].enabled) {
    gui::Spar location;location.chordPercent=30;
    for(int i=0;i<=32;++i) {
      const double y=halfSpan*i/32.0;
      const double sampleY=std::clamp(y,rootInset+halfSpan*1e-6,halfSpan*(1-1e-6)-tipInset);
      const auto [low,high]=heights(position(location,sampleY),sampleY);
      midCenters.emplace_back(position(location,sampleY),y,(low+high)*.5);
    }
  }
  if(split) {
    try {
    if(progress)progress(spars[2].enabled?"Splitting main wing panel at the mid-spar height...":"Splitting main wing panel for lightening access...");
    // The common boundary is chordwise level at each span station, passing
    // through the mid-spar center along its length, then level to the tip.
    // A single split operation creates a shared boundary before cutting grooves.
    for(std::size_t i=1;i+1<midCenters.size();) {
      const auto& a=midCenters[i-1];const auto& b=midCenters[i];const auto& c=midCenters[i+1];
      const double t=(b.Y()-a.Y())/(c.Y()-a.Y());
      if(std::abs(a.Z()+(c.Z()-a.Z())*t-b.Z())<1e-7)midCenters.erase(midCenters.begin()+i);else ++i;
    }
    auto splitHeight=[&](double y) {
      auto upper=std::upper_bound(midCenters.begin(),midCenters.end(),y,[](double v,const auto& p){return v<p.Y();});
      if(upper==midCenters.begin())return upper->Z();if(upper==midCenters.end())return midCenters.back().Z();
      const auto& a=*std::prev(upper);return a.Z()+(upper->Z()-a.Z())*(y-a.Y())/(upper->Y()-a.Y());
    };
    // Extend the cutting sheet beyond all wing bounds to avoid coincident caps.
    BRepBuilderAPI_MakePolygon boundary;
    boundary.Add(gp_Pnt{x0-margin,y0-margin,midCenters.front().Z()});
    for(const auto& point:midCenters)boundary.Add(gp_Pnt{x0-margin,point.Y(),point.Z()});
    boundary.Add(gp_Pnt{x0-margin,y1+margin,midCenters.back().Z()});
    const auto sheet=BRepPrimAPI_MakePrism{boundary.Wire(),gp_Vec{x1-x0+2*margin,0,0}}.Shape();
    BRepAlgoAPI_Splitter splitter;NCollection_List<TopoDS_Shape> args,tools;args.Append(original);tools.Append(sheet);
    splitter.SetArguments(args);splitter.SetTools(tools);splitter.SetRunParallel(control.parallel);splitter.Build(control.range());control.checkpoint();
    if(!splitter.IsDone())throw std::runtime_error("Could not split the main wing panel at mid-spar height.");
    std::vector<TopoDS_Shape> splitBodies;
    for(TopExp_Explorer e{splitter.Shape(),TopAbs_SOLID};e.More();e.Next())splitBodies.push_back(e.Current());
    if(splitBodies.size()!=2)throw std::runtime_error("The mid-spar sheet must split the main wing into two bodies: "+std::to_string(splitBodies.size()));
    const double probeY=std::clamp(halfSpan*.1,rootInset+halfSpan*1e-6,halfSpan*(1-1e-6)-tipInset),probeX=position(splitLocation,probeY);
    const double probeHigh=heights(probeX,probeY).second;
    const gp_Pnt probe{probeX,probeY,(splitHeight(probeY)+probeHigh)*.5};
    BRepClass3d_SolidClassifier classify{splitBodies[0],probe,1e-7};
    const bool firstTop=classify.State()==TopAbs_IN || classify.State()==TopAbs_ON;
    auto top=splitBodies[firstTop?0:1],bottom=splitBodies[firstTop?1:0];
    if(progress)progress("Validating split wing halves...");
    valid(top,"The upper wing half is disconnected.",control.parallel);valid(bottom,"The lower wing half is disconnected.",control.parallel);
    if(progress)progress("Checking split volume conservation...");
    GProp_GProps wholeMass,topMass,bottomMass;
    BRepGProp::VolumeProperties(original,wholeMass,1e-9);control.checkpoint();BRepGProp::VolumeProperties(top,topMass,1e-9);control.checkpoint();BRepGProp::VolumeProperties(bottom,bottomMass,1e-9);control.checkpoint();
    if(std::abs(topMass.Mass()+bottomMass.Mass()-wholeMass.Mass())>std::max(1e-3,std::abs(wholeMass.Mass())*1e-6))
      throw std::runtime_error("The wing split did not preserve the main panel volume: "+std::to_string(wholeMass.Mass())+" -> "+std::to_string(topMass.Mass()+bottomMass.Mass())+" mm3.");
    if(progress)progress("Applying spar tools to split halves...");
    for(int i:{2,0,1})if(spars[i].enabled){top=cut(top,sparTools[i],control);bottom=cut(bottom,sparTools[i],control);}
    valid(top,"The mid spar disconnects the upper wing half.",control.parallel);valid(bottom,"The mid spar disconnects the lower wing half.",control.parallel);
    for(int i=0;i<2;++i)if(spars[i].enabled)fixed=cut(fixed,sparTools[i],control);
    TopoDS_Shape cavities;
    if(lighten) {
      if(progress)progress("Constructing lightening bays and wall clearances...");
      auto finished=fixed;if(spars[2].enabled)finished=cut(finished,sparTools[2],control);
      const auto range=[&](double a,double b) {
        double low=std::min(splitHeight(a),splitHeight(b)),high=std::max(splitHeight(a),splitHeight(b));
        for(const auto& p:midCenters)if(p.Y()>a && p.Y()<b){low=std::min(low,p.Z());high=std::max(high,p.Z());}
        return std::pair{low,high};
      };
      cavities=lighteningCavities(finished,lighteningWall,bays,range,control);
    }
    // Circular alignment tabs: 3 mm diameter, 2 mm engagement, 0.1 mm radial fit.
    constexpr double radius=1.5,height=2,clearance=0.1;
    // Vertical intersection intervals directly verify a continuous material
    // column through the mating plane; avoid arbitrary-direction classification
    // of every point against the trimmed curved wing faces.
    IntCurvesFace_ShapeIntersector alignmentRay;alignmentRay.Load(fixed,1e-7);
    auto containsColumn=[&](double x,double y,double from,double to) {
      control.checkpoint();alignmentRay.Perform(gp_Lin{gp_Pnt{x,y,-margin},gp_Dir{0,0,1}},0,2*margin);
      if(!alignmentRay.IsDone())return false;
      std::vector<double> hits;
      for(int i=1;i<=alignmentRay.NbPnt();++i)hits.push_back(alignmentRay.Pnt(i).Z());
      std::sort(hits.begin(),hits.end());
      hits.erase(std::unique(hits.begin(),hits.end(),[](double a,double b){return std::abs(a-b)<1e-7;}),hits.end());
      for(std::size_t i=0;i+1<hits.size();i+=2)
        if(hits[i]<from && hits[i+1]>to)return true;
      return false;
    };
    if(progress && spars[2].enabled)progress("Checking alignment tab clearances...");
    BRep_Builder builder;TopoDS_Compound pegs,holes;builder.MakeCompound(pegs);builder.MakeCompound(holes);
    if(spars[2].enabled)for(double fraction:{0.2,0.8}) for(double side:{-1.0,1.0}) {
      const double y=halfSpan*fraction;
      const double x=position(spars[2],y)+side*(spars[2].sizeMm/2+3+radius);
      double minSplit=std::min(splitHeight(y-radius-clearance),splitHeight(y+radius+clearance));
      double maxSplit=std::max(splitHeight(y-radius-clearance),splitHeight(y+radius+clearance));
      for(const auto& point:midCenters) if(std::abs(point.Y()-y)<=radius+clearance) {
        minSplit=std::min(minSplit,point.Z());maxSplit=std::max(maxSplit,point.Z());
      }
      const double pegBottom=minSplit-.5,pegTop=maxSplit+height;
      // Check the full peg/hole footprint against the already-grooved halves.
      // Reject thin edges or clashes rather than silently omitting alignment.
      for(int n=0;n<16;++n) {
        const double angle=n*2*3.14159265358979323846/16;
        const double px=x+(radius+clearance)*std::cos(angle),py=y+(radius+clearance)*std::sin(angle);
        if(!containsColumn(px,py,pegBottom,pegTop+clearance+.2))
          throw std::runtime_error("Insufficient material for alignment tabs at 20%/80% panel span; move or resize the spars.");
      }
      const auto peg=BRepPrimAPI_MakeCylinder{gp_Ax2{gp_Pnt{x,y,pegBottom},gp_Dir{0,0,1}},radius,pegTop-pegBottom}.Shape();
      builder.Add(pegs,peg);
      const auto hole=BRepPrimAPI_MakeCylinder{gp_Ax2{gp_Pnt{x,y,minSplit-.01},gp_Dir{0,0,1}},radius+clearance,pegTop+clearance-minSplit+.01}.Shape();
      builder.Add(holes,hole);
      if(lighten) {
        // Protect the larger socket as well as its pin. A full-height column
        // retains a connection to both skins, not an isolated pin island.
        const auto support=BRepPrimAPI_MakeCylinder{gp_Ax2{gp_Pnt{x,y,z0-margin},gp_Dir{0,0,1}},radius+clearance+lighteningWall,z1-z0+2*margin}.Shape();
        cavities=cut(cavities,support,control);
      }
    }
    if(lighten && TopExp_Explorer{cavities,TopAbs_SOLID}.More()) {
      if(progress)progress("Hollowing upper and lower main-wing halves...");
      top=cut(top,cavities,control);bottom=cut(bottom,cavities,control);
      valid(top,"Lightening disconnects the upper wing half; increase retained material.",control.parallel);
      valid(bottom,"Lightening disconnects the lower wing half; increase retained material.",control.parallel);
    }
    if(spars[2].enabled) {
    if(progress)progress("Joining alignment tabs to lower wing...");
    BRepAlgoAPI_Fuse join;NCollection_List<TopoDS_Shape> joinArgs,joinTools;joinArgs.Append(bottom);joinTools.Append(pegs);
    join.SetArguments(joinArgs);join.SetTools(joinTools);join.SetRunParallel(control.parallel);join.Build(control.range());control.checkpoint();if(!join.IsDone())throw std::runtime_error("Could not attach alignment tabs.");bottom=join.Shape();
    if(progress)progress("Cutting matching alignment holes in upper wing...");
    top=cut(top,holes,control);
    }
    valid(top,"Invalid upper spar half.",control.parallel);valid(bottom,"Invalid lower spar half.",control.parallel);
    bodies[0]=top;bodies.insert(bodies.begin()+1,bottom);
    } catch(const Standard_Failure& e) {throw std::runtime_error(std::string{"Wing alignment/split/lightening: "}+e.what());}
      catch(const std::exception& e) {throw std::runtime_error(std::string{"Wing alignment/split/lightening: "}+e.what());}
  } else bodies[0]=fixed;
  TopoDS_Compound result;BRep_Builder builder;builder.MakeCompound(result);
  for(const auto& body:bodies)builder.Add(result,body);
  return result;
}
}





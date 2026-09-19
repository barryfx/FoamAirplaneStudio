#include "geometry/WingSolidBuilder.h"
#include "processing/IndexedTasks.h"
#include "geometry/MeshOrientation.h"
#include <mutex>
#include <Standard_Failure.hxx>
#include "geometry/ControlSurfaceCut.h"
#include "geometry/SparCut.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Builder.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax1.hxx>
#include <numbers>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
namespace designrc::geometry {
namespace {
double dot(QPointF a, QPointF b) { return QPointF::dotProduct(a,b); }
double length(QPointF a) { return std::hypot(a.x(),a.y()); }
std::vector<QPointF> openEnds(const gui::SketchLayer& panel, QPointF spanAxis) {
  std::vector<int> degree(panel.points.size());
  for (const auto& curve : panel.curves) {
    if (curve.points.size()<2) continue;
    ++degree.at(curve.points.front()); ++degree.at(curve.points.back());
  }
  std::vector<QPointF> ends;
  for (std::size_t i=0;i<degree.size();++i) if (degree[i]==1) ends.push_back(panel.points[i]);
  std::sort(ends.begin(),ends.end(),[&](auto a,auto b){return dot(a,spanAxis)<dot(b,spanAxis);});
  if (ends.size()!=2 && ends.size()!=4)
    throw std::runtime_error("Each panel needs open leading and trailing edges at its root.");
  return ends;
}
domain::AirfoilProfile profile(const gui::LibraryAirfoil& foil) {
  if (foil.imported) return *foil.imported;
  // Traces use image axes: LE left, TE right, positive Y down. Rotate the
  // closed traversal to the trailing edge before passing it to the DAT reader.
  auto points = foil.boundary;
  if (points.size() < 5) throw std::runtime_error("The traced airfoil has too few points.");
  if (length(points.front()-points.back()) < 1e-8) points.pop_back();
  const auto trailing = std::max_element(points.begin(),points.end(),[](auto a, auto b){return a.x()<b.x();});
  std::rotate(points.begin(),trailing,points.end());
  const auto leading = *std::min_element(points.begin(),points.end(),[](auto a,auto b){return a.x()<b.x();});
  const auto te = points.front();
  const double chord = te.x()-leading.x();
  if (chord <= 1e-8) throw std::runtime_error("The traced airfoil has no horizontal chord.");
  points.push_back(points.front());
  std::ostringstream dat; dat.precision(17); dat << foil.name.toStdString() << '\n';
  for (auto p : points) {
    const double x = (p.x()-leading.x())/chord;
    dat << x << ' ' << (leading.y()+x*(te.y()-leading.y())-p.y())/chord << '\n';
  }
  std::istringstream stream{dat.str()}; return domain::AirfoilProfile::fromDat(stream);
}
struct PanelMiter {
  bool enabled=false;
  double rootSlope=0,tipSlope=0,span=1;
  double shift(double y,double z) const {
    return enabled?z*(rootSlope+(tipSlope-rootSlope)*std::clamp(y/span,0.0,1.0)):0;
  }
};
TopoDS_Wire sectionWire(const std::vector<domain::Point2>& foil, QPointF le, QPointF te,
                        QPointF chordAxis, QPointF spanAxis, double root, double scale,
                        bool roundedTip, const std::function<double(QPointF)>& tipDistance,
                        const PanelMiter& miter={},std::optional<double> capSpan={}) {
  const double chord = length(te-le);
  auto point = [&](std::size_t i) {
    const auto p = le + foil[i].x*(te-le);
    double height = foil[i].y;
    // Keep one tiny TE closure edge for consistent topology between mixed DATs.
    if (i==0) height = std::max(height, 1e-6);
    if (i+1==foil.size()) height = std::min(height,-1e-6);
    double z = height*chord*scale;
    if (roundedTip) {
      // Cosine resampling pairs upper/lower ordinates at the same chord fraction.
      // Move only Z: the traced planform survives the rounded profile. A tiny edge
      // thickness keeps closed section topology at the outer boundary.
      const double other = foil[foil.size()-1-i].y*chord*scale;
      const double low = std::min(z,other), high = std::max(z,other);
      const double thickness = high-low;
      if (thickness > 1e-8) {
        const double radius=thickness*.5;
        const double distance=std::clamp(tipDistance(p)*scale/radius,0.0,1.0);
        const double retained=std::max(1e-3,std::sqrt(std::max(0.0,1-(1-distance)*(1-distance))));
        const double center=(low+high)*.5;
        z=center+(z-center)*retained;
        if(i==0)z=std::max(z,center+1e-5);
        if(i+1==foil.size())z=std::min(z,center-1e-5);
      }
    }
    const double y=capSpan.value_or((dot(p,spanAxis)-root)*scale);
    return gp_Pnt{dot(p,chordAxis)*scale,y+miter.shift(y,z),z};
  };
  const std::size_t mid = foil.size()/2;
  BRepBuilderAPI_MakeWire wire;
  for (auto range : {std::pair<std::size_t,std::size_t>{0,mid},{mid,foil.size()-1}}) {
    Handle(TColgp_HArray1OfPnt) nodes = new TColgp_HArray1OfPnt(1,static_cast<int>(range.second-range.first+1));
    for (std::size_t i=range.first;i<=range.second;++i) nodes->SetValue(static_cast<int>(i-range.first+1),point(i));
    GeomAPI_Interpolate fit{nodes,false,1e-8}; fit.Perform();
    if (!fit.IsDone()) throw std::runtime_error("Airfoil curve interpolation failed.");
    wire.Add(BRepBuilderAPI_MakeEdge{fit.Curve()}.Edge());
  }
  wire.Add(BRepBuilderAPI_MakeEdge{point(foil.size()-1),point(0)}.Edge());
  return wire.Wire();
}
}
struct WingFrame {QPointF chord,span;double scale;};
static TopoDS_Shape buildWingPanel(const WingSolidInput& input, const std::function<void(const char*)>& progress,
                                 std::optional<WingFrame> frame={}, bool internalPanel=false, PanelMiter miter={},double* generatedSpan=nullptr, const std::vector<std::pair<double,double>>& lighteningBays={},const ProcessingControl& control={}) {
  const auto report = [&](const char* message) { control.checkpoint();if(progress) progress(message); };
  if(gui::controlSurfacesOverlap(input.controls))
    throw std::runtime_error("Aileron and flap rectangles must not overlap. Edit their rectangles in Ailerons/Flaps mode.");
  report("Sampling wing outline and airfoil stations...");
  if (input.stations.size()<2 || input.panels.empty()) throw std::runtime_error("Define an outline and at least two assigned stations.");
  struct Station { gui::ConstrainedLine line; domain::AirfoilProfile foil; double span; };
  const auto initialChord = input.stations.front().second.position-input.stations.front().first.position;
  const bool horizontalSpan = std::abs(initialChord.y()) >= std::abs(initialChord.x());
  const auto& first = *std::min_element(input.stations.begin(),input.stations.end(),[&](const auto& a,const auto& b) {
    const auto midpointA=(a.first.position+a.second.position)*0.5;
    const auto midpointB=(b.first.position+b.second.position)*0.5;
    return horizontalSpan ? midpointA.x()<midpointB.x() : midpointA.y()<midpointB.y();
  });
  auto chordAxis = first.second.position-first.first.position;
  if (length(chordAxis)<1e-8) throw std::runtime_error("A station has zero chord.");
  chordAxis /= length(chordAxis);
  QPointF spanAxis{chordAxis.y(),-chordAxis.x()};
  if (spanAxis.x() < -1e-8 || (std::abs(spanAxis.x())<1e-8 && spanAxis.y()<0)) spanAxis=-spanAxis;
  // The sketch deliberately omits the root contour. Its two open ends define
  // the root section, even when the first station is angled or placed outboard.
  if(frame){chordAxis=frame->chord;spanAxis=frame->span;}
  const auto rootEnds=openEnds(input.panels.front(),spanAxis);
  auto rootChord=rootEnds[1]-rootEnds[0];
  if (dot(rootChord,chordAxis)<0) rootChord=-rootChord;
  if (length(rootChord)<1e-8) throw std::runtime_error("The root has zero chord.");
  if(!frame){chordAxis=rootChord/length(rootChord);spanAxis={chordAxis.y(),-chordAxis.x()};}
  // Reject coincident sections, independently of the order they were placed.
  QPointF far = first.first.position; double separation=0;
  for (const auto& s : input.stations) if (std::abs(dot(s.first.position-first.first.position,spanAxis))>separation) {
    far=s.first.position; separation=std::abs(dot(far-first.first.position,spanAxis));
  }
  // The right half is drawn toward positive image X (or positive Y for a vertical wing).
  if (spanAxis.x() < -1e-8 || (std::abs(spanAxis.x())<1e-8 && spanAxis.y()<0)) spanAxis=-spanAxis;
  if (separation<1e-8) throw std::runtime_error("Airfoil stations must be separated along the span.");
  std::vector<Station> stations;
  for (const auto& s : input.stations) {
    if (!s.airfoil || *s.airfoil>=input.airfoils.size()) throw std::runtime_error("Assign an airfoil to every station.");
    stations.push_back({s,profile(input.airfoils[*s.airfoil]),dot((s.first.position+s.second.position)*0.5,spanAxis)});
  }
  std::sort(stations.begin(),stations.end(),[](const auto& a,const auto& b){return a.span<b.span;});
  std::vector<std::pair<QPointF,QPointF>> edges;
  std::vector<double> sections;
  double root=1e100,tip=-1e100;
  for (const auto& panel : input.panels) for (const auto& curve : panel.curves) {
    std::vector<QPointF> points; for (auto id:curve.points) points.push_back(panel.points.at(id));
    const auto path=gui::SketchEditor::fittedPath(points,curve.type);
    for (int i=1;i<path.elementCount();++i) {
      QPointF a{path.elementAt(i-1).x,path.elementAt(i-1).y}, b{path.elementAt(i).x,path.elementAt(i).y};
      edges.emplace_back(a,b);
      sections.push_back(dot(a,spanAxis)); sections.push_back(dot(b,spanAxis));
      root=std::min({root,dot(a,spanAxis),dot(b,spanAxis)}); tip=std::max({tip,dot(a,spanAxis),dot(b,spanAxis)});
    }
    if (!points.empty()) { sections.push_back(dot(points.front(),spanAxis)); sections.push_back(dot(points.back(),spanAxis)); }
  }
  // Close each panel only for section intersection; these construction edges
  // never alter the user's sketch or its outline-completion rules.
  for (const auto& panel : input.panels) {
    const auto ends=openEnds(panel,spanAxis);
    for (std::size_t i=0;i<ends.size();i+=2) edges.emplace_back(ends[i],ends[i+1]);
  }
  root=dot(rootEnds[0],spanAxis);
  if (!(tip-root>1e-7)) throw std::runtime_error("The outline has no span.");
  // Internal construction ends may differ slightly in traced length. Stop at
  // the inboard side of the final station only when both ends are within 1% of
  // the panel span. The outermost wing tip is deliberately never truncated.
  if(internalPanel) {
    const auto& last=stations.back().line;
    const double a=dot(last.first.position,spanAxis),b=dot(last.second.position,spanAxis);
    const double tolerance=(tip-root)*0.01;
    if(std::max(std::abs(tip-a),std::abs(tip-b))<=tolerance && std::min(a,b)>root+1e-7) {
      tip=std::min(a,b);
      report("Truncating internal panel at its last airfoil station...");
    }
  }
  const double scale=frame?frame->scale:input.wingspanMm ? *input.wingspanMm/(2*(tip-root)) : 1.0;
  if (!std::isfinite(scale)||scale<=0) throw std::runtime_error("Wingspan must be positive.");
  miter.span=(tip-root)*scale;
  if(generatedSpan)*generatedSpan=miter.span;
  for (int i=0;i<=32;++i) sections.push_back(root+(tip-root)*i/32.0);
  for (const auto& s:stations) sections.push_back(s.span);
  if(!internalPanel) {
    double reach=0;
    for(const auto& station:stations) {
      double low=0,high=0;for(const auto& p:station.foil.outline()){low=std::min(low,p.y);high=std::max(high,p.y);}
      reach=std::max(reach,(high-low)*length(station.line.second.position-station.line.first.position)*.5);
    }
    // Resolve the circular tip profile even when ordinary span sections are
    // farther apart than the tip radius. Cosine spacing resolves its closure.
    for(int i=1;i<=16;++i)sections.push_back(tip-reach*(1-std::cos(i*std::numbers::pi/32)));
  }
  std::erase_if(sections,[&](double span){return span<root-1e-7 || span>tip+1e-7;});
  std::sort(sections.begin(),sections.end());
  sections.erase(std::unique(sections.begin(),sections.end(),[](double a,double b){return std::abs(a-b)<1e-7;}),sections.end());
  BRepOffsetAPI_ThruSections loft{true,true,1e-7};
  loft.CheckCompatibility(false);
  // Intersect an outboard ray with the actual contour at each chord location.
  // Unlike a global clipping plane, this follows rounded and swept tip outlines.
  const auto tipDistance = [&](QPointF point) {
    const double chordPosition = dot(point,chordAxis);
    double outer = dot(point,spanAxis);
    for (const auto& [a,b] : edges) {
      const double delta = dot(b-a,chordAxis);
      if (std::abs(delta)<1e-10) {
        if (std::abs(dot(a,chordAxis)-chordPosition)<1e-7)
          outer=std::max({outer,dot(a,spanAxis),dot(b,spanAxis)});
        continue;
      }
      const double t=(chordPosition-dot(a,chordAxis))/delta;
      if (t>=-1e-8 && t<=1+1e-8) outer=std::max(outer,dot(a+std::clamp(t,0.0,1.0)*(b-a),spanAxis));
    }
    return std::max(0.0,outer-dot(point,spanAxis));
  };
  const double rootFullSpan=std::max(dot(rootEnds[0],spanAxis),dot(rootEnds[1],spanAxis));
  const bool obliqueRoot=rootFullSpan-root>1e-7;
  Bnd_Box rootCapBounds,tipCapBounds;
  if(obliqueRoot || miter.enabled) {
    auto le=rootEnds[0],te=rootEnds[1];if(dot(te-le,chordAxis)<0)std::swap(le,te);
    const auto foil=domain::AirfoilProfile::interpolate(stations.front().foil,stations.front().foil,0,25).outline();
    const auto cap=sectionWire(foil,le,te,chordAxis,spanAxis,root,scale,false,tipDistance,miter,miter.enabled?std::optional<double>{0}:std::nullopt);
    loft.AddWire(cap);BRepBndLib::AddOptimal(cap,rootCapBounds,false,false);
  }
  report("Building airfoil sections and contour-following wing tip...");
  for (const double span:sections) {
    control.checkpoint();
    // An oblique root is a full airfoil cap, not a sequence of shrinking chords.
    if((obliqueRoot || miter.enabled) && span<=rootFullSpan+1e-7)continue;
    std::vector<QPointF> hits;
    for (const auto& [a,b]:edges) {
      const double delta=dot(b-a,spanAxis);
      if (std::abs(delta)<1e-10) {
        if(std::abs(dot(a,spanAxis)-span)<1e-7) {hits.push_back(a);hits.push_back(b);} continue;
      }
      const double t=(span-dot(a,spanAxis))/delta;
      if(t>=-1e-8&&t<=1+1e-8) hits.push_back(a+std::clamp(t,0.0,1.0)*(b-a));
    }
    if(hits.empty()) throw std::runtime_error("The wing panels have a gap along the span.");
    auto bounds=std::minmax_element(hits.begin(),hits.end(),[&](auto a,auto b){return dot(a,chordAxis)<dot(b,chordAxis);});
    QPointF le=*bounds.first,te=*bounds.second;
    // Use the exact user section, including its oblique chord, at each station.
    for (const auto& s:stations) if(std::abs(s.span-span)<1e-7) {
      const double a=dot(s.line.first.position,spanAxis),b=dot(s.line.second.position,spanAxis);
      // An oblique station touching the root cap overlaps it. Use this span's
      // ordinary full-chord section instead of adding a crossing loft wire.
      if(!miter.enabled && (std::abs(a-b)<=1e-7 || std::min(a,b)>rootFullSpan+1e-7)) {
        le=s.line.first.position;te=s.line.second.position;
      }
      break;
    }
    if(miter.enabled && internalPanel && std::abs(span-tip)<1e-7 && rootEnds.size()==4) {
      le=rootEnds[2];te=rootEnds[3];if(dot(te-le,chordAxis)<0)std::swap(le,te);
    }
    const double chord=length(te-le);
    if(chord*scale<1e-5) {
      if(std::abs(span-tip)>1e-7) throw std::runtime_error("The outline closes before the wing tip.");
      loft.AddVertex(BRepBuilderAPI_MakeVertex{gp_Pnt{dot(le,chordAxis)*scale,(span-root)*scale,0}}.Vertex()); continue;
    }
    auto upper=std::upper_bound(stations.begin(),stations.end(),span,[](double s,const auto& station){return s<station.span;});
    auto left=upper==stations.begin()?upper:std::prev(upper);
    auto right=upper==stations.end()?std::prev(upper):upper;
    const double fraction=right==left?0:std::clamp((span-left->span)/(right->span-left->span),0.0,1.0);
    const auto foil=domain::AirfoilProfile::interpolate(left->foil,right->foil,fraction,25).outline();
    const bool terminal=std::abs(span-tip)<1e-7;
    const auto section=sectionWire(foil,le,te,chordAxis,spanAxis,root,scale,!internalPanel,tipDistance,miter,
        miter.enabled && terminal?std::optional<double>{miter.span}:std::nullopt);
    loft.AddWire(section);
    if(terminal)BRepBndLib::AddOptimal(section,tipCapBounds,false,false);
  }
  report("Lofting wing surfaces...");
  loft.Build(control.range());control.checkpoint(); if(!loft.IsDone()) throw std::runtime_error("OCCT could not loft the wing sections.");
  TopoDS_Shape half=loft.Shape();
  report("Validating wing solid...");
  if(half.IsNull()||!TopExp_Explorer{half,TopAbs_SOLID}.More()||!BRepCheck_Analyzer{half,true,control.parallel}.IsValid())
    throw std::runtime_error("The wing loft is not a valid solid; check outline and station placement.");
  if(std::any_of(input.controls.front().begin(),input.controls.front().end(),[](const auto& c){return c.enabled && c.rectangle;})) {
    report("Separating ailerons/flaps and cutting hinge bevels...");
    half=cutControlSurfaces(half,input.controls.front(),chordAxis,spanAxis,root,scale,25.4/16.0,control);
  }
  if(input.lightening.enabled || std::any_of(input.spars.begin(),input.spars.end(),[](const auto& p){return std::any_of(p.begin(),p.end(),[](const auto& s){return s.enabled;});})) {
    const auto chordAtSpan=[&](double y) {
      const double span=root+y/scale;double le=1e100,te=-1e100;
      for(const auto& [a,b]:edges) {
        const double delta=dot(b-a,spanAxis);
        if(std::abs(delta)<1e-10) {
          if(std::abs(dot(a,spanAxis)-span)<1e-7) {le=std::min({le,dot(a,chordAxis),dot(b,chordAxis)});te=std::max({te,dot(a,chordAxis),dot(b,chordAxis)});}
          continue;
        }
        const double t=(span-dot(a,spanAxis))/delta;
        if(t>=-1e-8 && t<=1+1e-8) {const double x=dot(a+std::clamp(t,0.0,1.0)*(b-a),chordAxis);le=std::min(le,x);te=std::max(te,x);}
      }
      if(te<le)throw std::runtime_error("No wing chord at the spar location.");
      return std::pair{le*scale,te*scale};
    };
    if(input.spars.size()!=input.panels.size())throw std::runtime_error("Spar settings must match the number of wing panels.");
    std::vector<TopoDS_Shape> bodies;
    for(TopExp_Explorer e{half,TopAbs_SOLID};e.More();e.Next())bodies.push_back(e.Current());
    const auto fixed=bodies.front();
    std::vector<double> boundaries{0};
    for(std::size_t i=1;i<input.panels.size();++i) {
      const auto ends=openEnds(input.panels[i],spanAxis);
      boundaries.push_back((dot((ends[0]+ends[1])*0.5,spanAxis)-root)*scale);
    }
    boundaries.push_back((tip-root)*scale);
    Bnd_Box box;BRepBndLib::Add(fixed,box);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    const double margin=2*std::max({x1-x0,y1-y0,z1-z0,1.0});
    BRep_Builder builder;TopoDS_Compound panels;builder.MakeCompound(panels);
    for(std::size_t i=0;i<input.panels.size();++i) {
      const double begin=boundaries[i],end=boundaries[i+1];
      if(end-begin<1e-6)throw std::runtime_error("Wing panels must be ordered from root to tip with distinct spans.");
      try {
        auto piece=fixed;
        if(input.panels.size()>1) {
          const double lower=i==0?y0-margin:begin,upper=i+1==input.panels.size()?y1+margin:end;
          BRepAlgoAPI_Common clip{fixed,BRepPrimAPI_MakeBox{gp_Pnt{x0-margin,lower,z0-margin},x1-x0+2*margin,upper-lower,z1-z0+2*margin}.Shape()};
          if(!clip.IsDone())throw std::runtime_error("Could not separate the main wing panel.");piece=clip.Shape();
        }
        // SparCut receives panel-local span coordinates. Separate aileron/flap
        // bodies are deliberately excluded from both panel and height splitting.
        gp_Trsf local;local.SetTranslation(gp_Vec{0,-begin,0});
        if(begin!=0)piece=BRepBuilderAPI_Transform{piece,local,true}.Shape();
        const auto panelChord=[&](double y){return chordAtSpan(y+begin);};
        double rootInset=obliqueRoot?(rootFullSpan-root)*scale:0,tipInset=0;
        if(miter.enabled) {
          double a,b,c,d,e,f;
          if(!rootCapBounds.IsVoid()){rootCapBounds.Get(a,b,c,d,e,f);rootInset=std::max(rootInset,e);}
          if(!tipCapBounds.IsVoid()){tipCapBounds.Get(a,b,c,d,e,f);tipInset=std::max(0.0,miter.span-b);}
        }
        auto result=cutSpars(piece,input.spars[i],end-begin,panelChord,progress,rootInset,tipInset,miter.enabled,input.lightening.enabled?input.lightening.wallMm:0,lighteningBays,control);
        local.SetTranslation(gp_Vec{0,begin,0});
        if(begin!=0)result=BRepBuilderAPI_Transform{result,local,true}.Shape();
        builder.Add(panels,result);
      } catch(const std::exception& e) {throw std::runtime_error((frame?std::string{}:"Panel "+std::to_string(i+1)+": ")+e.what());}
    }
    for(std::size_t i=1;i<bodies.size();++i)builder.Add(panels,bodies[i]);
    half=panels;
  }
  if(frame)return half;
  report("Mirroring wing at the root...");
  gp_Trsf mirror; mirror.SetMirror(gp_Ax2{gp_Pnt{0,0,0},gp_Dir{0,1,0}});
  TopoDS_Compound result; BRep_Builder builder;builder.MakeCompound(result);builder.Add(result,half);
  builder.Add(result,BRepBuilderAPI_Transform{half,mirror,true}.Shape());
  report("Meshing wing for display...");
  BRepMesh_IncrementalMesh mesh{result,0.1,false,0.25,true};
  return result;
}
TopoDS_Shape buildWingSolid(const WingSolidInput& input,const std::function<void(const char*)>& progress,const WingBuildOptions& options) {
  options.processing.checkpoint();
  if(input.panels.empty() || input.controls.size()!=input.panels.size())throw std::runtime_error("Control settings must match the wing panels.");
  if(gui::controlSurfacesOverlap(input.controls))throw std::runtime_error("Control rectangles must not overlap, including rectangles on different panels.");
  for(const auto& station:input.stations)if(station.first.layer!=station.second.layer)throw std::runtime_error("Each station must connect leading and trailing edges on the same panel.");
  if(!input.stations.empty())for(std::size_t i=1;i<input.stations.size();++i)
    if(!gui::stationDirectionsAgree(input.stations.front(),input.stations[i]))
      throw std::runtime_error("Conflicting LE/TE orientation: station "+std::to_string(i+1)+
        " on panel "+std::to_string(input.stations[i].first.layer+1)+
        " disagrees with station 1 on panel "+std::to_string(input.stations.front().first.layer+1)+
        ". In Airfoil Stations, delete and redraw the reversed station, clicking LE first, then TE.");

  if(input.stations.empty() || input.spars.size()!=input.panels.size())throw std::runtime_error("Define stations and spar settings for each panel.");
  auto chord=input.stations.front().second.position-input.stations.front().first.position;
  if(length(chord)<1e-8)throw std::runtime_error("A station has zero chord.");chord/=length(chord);
  QPointF span{chord.y(),-chord.x()};if(span.x()<0 || (std::abs(span.x())<1e-8 && span.y()<0))span=-span;
  auto ends=openEnds(input.panels.front(),span);auto rootChord=ends[1]-ends[0];
  if(length(rootChord)<1e-8)throw std::runtime_error("The root has zero chord.");
  if(dot(rootChord,chord)<0)rootChord=-rootChord;chord=rootChord/length(rootChord);
  span={chord.y(),-chord.x()};if(span.x()<0 || (std::abs(span.x())<1e-8 && span.y()<0))span=-span;
  const double root=dot(ends[0],span);double tip=root;
  for(const auto& panel:input.panels)for(const auto& curve:panel.curves) {
    std::vector<QPointF> points;for(auto id:curve.points)points.push_back(panel.points.at(id));
    const auto path=gui::SketchEditor::fittedPath(points,curve.type);
    for(int n=0;n<path.elementCount();++n)tip=std::max(tip,dot(QPointF{path.elementAt(n).x,path.elementAt(n).y},span));
  }
  if(tip-root<1e-8)throw std::runtime_error("The outline has no span.");
  const double scale=input.wingspanMm?*input.wingspanMm/(2*(tip-root)):1;
  auto angles=input.dihedralDegrees;
  if(angles.empty())angles.assign(input.panels.size(),0);
  if(angles.size()!=input.panels.size())throw std::runtime_error("Dihedral settings must match the wing panels.");
  bool raised=false;double cumulative=0;
  for(double angle:angles) {
    cumulative+=angle;
    if(!std::isfinite(angle)||std::abs(angle)>80 || std::abs(cumulative)>=85)
      throw std::runtime_error("Root dihedral must be between -80 and 80 degrees, with cumulative panel angles between -85 and 85 degrees.");
    raised=raised || angle!=0;
  }
  // Measure lightening in the assembled, unfolded span, including internal
  // near-terminal truncation. Dihedral changes placement, never these lengths.
  std::vector<double> unfoldedStarts;double unfolded=0,lastStation=-1e100;
  for(std::size_t i=0;i<input.panels.size();++i) {
    options.processing.checkpoint();
    const auto e=openEnds(input.panels[i],span);const double r=dot(e[0],span);double t=r;
    for(const auto& c:input.panels[i].curves) {
      std::vector<QPointF> points;for(auto id:c.points)points.push_back(input.panels[i].points.at(id));
      const auto path=gui::SketchEditor::fittedPath(points,c.type);
      for(int n=0;n<path.elementCount();++n)t=std::max(t,dot(QPointF{path.elementAt(n).x,path.elementAt(n).y},span));
    }
    const gui::ConstrainedLine* last=nullptr;
    for(const auto& station:input.stations)if(station.first.layer==static_cast<int>(i)) {
      if(!last || dot(station.first.position+station.second.position,span)>dot(last->first.position+last->second.position,span))last=&station;
    }
    if(last) {
      const double a=dot(last->first.position,span),b=dot(last->second.position,span);
      if(i+1<input.panels.size() && std::max(std::abs(t-a),std::abs(t-b))<=(t-r)*.01 && std::min(a,b)>r+1e-7)t=std::min(a,b);
      if(i+1==input.panels.size())lastStation=unfolded+(std::min(a,b)-r)*scale;
    }
    unfoldedStarts.push_back(unfolded);unfolded+=(t-r)*scale;
  }
  std::vector<std::pair<double,double>> globalBays;
  if(input.lightening.enabled) {
    const auto& l=input.lightening;
    for(double v:{l.wallMm,l.ribMm})if(!std::isfinite(v)||v<.0001||v>10000)throw std::runtime_error("Lightening wall and crossmember thickness must be positive and at most 10000 mm.");
    for(double v:{l.startMm,l.stopMm})if(!std::isfinite(v)||v<0||v>10000)throw std::runtime_error("Lightening start and stop distances must be 0 to 10000 mm.");
    if(l.crossmembers<0 || l.crossmembers>100)throw std::runtime_error("Lightening crossmember count must be 0 to 100.");
    // Use the inboard endpoint of the last station, so neither edge of an
    // oblique terminal profile can lie inside the hollowed region.
    const double start=l.startMm,end=lastStation-l.stopMm;
    if(end<=start)throw std::runtime_error("Lightening start and stop distances leave no hollowing range before the last station.");
    const double pitch=(end-start)/(l.crossmembers+1);
    if(l.crossmembers && pitch<=l.ribMm)throw std::runtime_error("Lightening crossmembers are too thick or too numerous for the available span.");
    double a=start;
    for(int i=1;i<=l.crossmembers;++i){const double center=start+i*pitch;globalBays.emplace_back(a,center-l.ribMm*.5);a=center+l.ribMm*.5;}
    globalBays.emplace_back(a,end);
  }
  struct PanelResult {TopoDS_Shape shape;double span=0;};
  std::vector<PanelResult> results(input.panels.size());std::mutex progressMutex;
  processing::runIndexedTasks(input.panels.size(),[&](std::size_t panel,std::stop_token token) {
    const ProcessingControl control{options.maxPanelThreads==1 && !options.processing.stop.stop_possible()?std::stop_token{}:token,options.processing.parallel};
    const auto report=[&](const char* message) {
      control.checkpoint();
      if(progress) {
        const auto text="Wing panel "+std::to_string(panel+1)+": "+message;
        std::lock_guard lock{progressMutex};progress(text.c_str());
      }
    };
    try {
      WingSolidInput local;local.panels={input.panels[panel]};local.airfoils=input.airfoils;
      local.controls={input.controls[panel]};local.spars={input.spars[panel]};local.lightening=input.lightening;
      for(auto station:input.stations)if(station.first.layer==static_cast<int>(panel) && station.second.layer==static_cast<int>(panel)) {
        station.first.layer=station.second.layer=0;local.stations.push_back(station);
      }
      if(local.stations.size()<2)throw std::runtime_error("Define at least two stations on this panel and assign their airfoils.");
      PanelMiter miter;miter.enabled=raised;
      const double increment=angles[panel]*std::numbers::pi/180;
      miter.rootSlope=std::tan(panel==0?increment:increment*.5);
      miter.tipSlope=panel+1<angles.size()?-std::tan(angles[panel+1]*std::numbers::pi/360):0;
      std::vector<std::pair<double,double>> panelBays;
      for(auto [a,b]:globalBays)panelBays.emplace_back(a-unfoldedStarts[panel],b-unfoldedStarts[panel]);
      results[panel].shape=buildWingPanel(local,report,WingFrame{chord,span,scale},panel+1<input.panels.size(),miter,&results[panel].span,panelBays,control);
      report("Complete.");
    } catch(const ProcessingCancelled&) {throw;}
      catch(const Standard_Failure& e) {control.checkpoint();throw std::runtime_error("Panel "+std::to_string(panel+1)+": "+e.what());}
      catch(const std::exception& e) {control.checkpoint();throw std::runtime_error("Panel "+std::to_string(panel+1)+": "+e.what());}
  },options.processing.stop,options.maxPanelThreads);
  options.processing.checkpoint();
  double orientation=0,originY=0,originZ=0;
  BRep_Builder builder;TopoDS_Compound half;builder.MakeCompound(half);
  for(std::size_t panel=0;panel<results.size();++panel) {
    options.processing.checkpoint();orientation+=angles[panel]*std::numbers::pi/180;
    const auto rootEnds=openEnds(input.panels[panel],span);gp_Trsf placement;
    if(raised) {
      placement.SetRotation(gp_Ax1{gp_Pnt{0,0,0},gp_Dir{1,0,0}},orientation);
      placement.SetTranslationPart(gp_Vec{0,originY,originZ});
      originY+=results[panel].span*std::cos(orientation);originZ+=results[panel].span*std::sin(orientation);
    } else placement.SetTranslation(gp_Vec{0,(dot(rootEnds[0],span)-root)*scale,0});
    builder.Add(half,BRepBuilderAPI_Transform{results[panel].shape,placement,true}.Shape());
  }
  if(progress)progress("Meshing right wing panels for display...");
  options.processing.checkpoint();
  IMeshTools_Parameters parameters;parameters.Deflection=.1;parameters.Angle=.25;parameters.InParallel=true;
  BRepMesh_IncrementalMesh mesh{half,parameters,options.processing.range()};options.processing.checkpoint();
  if(!mesh.IsDone())throw std::runtime_error("Could not mesh wing panels for display.");
  if(progress)progress("Mirroring wing panels and their display meshes at the root...");
  gp_Trsf mirror;mirror.SetMirror(gp_Ax2{gp_Pnt{0,0,0},gp_Dir{0,1,0}});
  TopoDS_Compound result;builder.MakeCompound(result);builder.Add(result,half);
  // Reflection preserves distances: reuse a transformed copy of the existing
  // triangulation instead of tessellating the identical left wing again.
  const auto reflected=BRepBuilderAPI_Transform{half,mirror,true,true}.Shape();
  alignMeshOrientation(reflected,options.processing);builder.Add(result,reflected);
  options.processing.checkpoint();return result;
}
}

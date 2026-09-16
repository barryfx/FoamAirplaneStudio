#include "geometry/WingSolidBuilder.h"
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <TopoDS.hxx>
#include <numbers>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <cassert>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <Standard_Failure.hxx>
#include <stdexcept>
#undef assert
#define assert(condition) do { if (!(condition)) throw std::runtime_error(#condition); } while (false)
using namespace designrc;
geometry::WingSolidInput rectangular() {
  geometry::WingSolidInput input;
  gui::SketchLayer panel;
  panel.points={{0,0},{500,0},{500,200},{0,200}};
  panel.curves={{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}};
  input.panels.push_back(panel);
  for (double x:{0.,500.}) input.stations.push_back({{0,0,0,{x,0}},{0,2,0,{x,200}},gui::LineAlignment::Vertical,0});
  input.airfoils.push_back({"NACA0012",domain::AirfoilProfile::nacaSymmetric(0.12),{}, {}});
  input.wingspanMm=1000; return input;
}
double validate(const TopoDS_Shape& shape,int count=2) {
  assert(!shape.IsNull() && BRepCheck_Analyzer{shape}.IsValid());
  int solids=0; for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next()) ++solids;
  assert(solids==count);
  GProp_GProps props; BRepGProp::VolumeProperties(shape,props,1e-8); assert(props.Mass()>0);
  assert(std::abs(props.CentreOfMass().Y())<1e-5);
  return props.Mass();
}
void reachesOutline(const TopoDS_Shape& shape, QPointF point) {
  // Cast vertically just inside the traced planform. A bevel may change Z,
  // but must never remove the wing at this XY location.
  IntCurvesFace_ShapeIntersector ray;
  ray.Load(shape,1e-6);
  ray.Perform(gp_Lin{gp_Pnt{point.y(),point.x()-0.05,-100},gp_Dir{0,0,1}},0,200);
  assert(ray.IsDone() && ray.NbPnt()>0);
}
void checkNearTerminalPanels() {
  double x0,y0,z0,x1,y1,z1;
    // Slightly skewed root/join with a nearly terminal internal station.
    // Full-length surface spars must retain a full airfoil at the panel end.
    auto nearEnd=rectangular();nearEnd.wingspanMm.reset();
    nearEnd.panels={
      {{{0,0},{250,0},{250,200},{.4,200}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{2,3}}}},
      {{{250,0},{500,20},{500,180},{250,200}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}}};
    nearEnd.stations={{{0,0,0,{.4,0}},{0,1,1,{.4,200}},gui::LineAlignment::Vertical,0},
      {{0,0,1,{249.99,0}},{0,1,0,{249.99,200}},gui::LineAlignment::Vertical,0},
      {{1,0,0,{250,0}},{1,2,1,{250,200}},gui::LineAlignment::Vertical,0},
      {{1,0,1,{499,19.92}},{1,2,0,{499,180.08}},gui::LineAlignment::Vertical,0}};
    nearEnd.controls.resize(2);nearEnd.spars.resize(2);
    for(int i=0;i<2;++i)for(int j=0;j<2;++j)nearEnd.spars[i][j]={true,gui::SparShape::Strip,25,i==0?100.0:50.0,5,1};
    const auto cropped=geometry::buildWingSolid(nearEnd);validate(cropped,4);
    TopExp_Explorer firstPanel{cropped,TopAbs_SOLID};Bnd_Box innerBounds;BRepBndLib::AddOptimal(firstPanel.Current(),innerBounds,false,false);
    innerBounds.Get(x0,y0,z0,x1,y1,z1);
    const double cutoff=(249.99*200-200*.4)/std::hypot(200.,.4);
    std::cout<<"Internal panel end "<<y1<<" mm; cutoff "<<cutoff<<" mm\n";
    assert(std::abs(y1-cutoff)<.02);
    // Even a near-end station on the outermost panel must leave its tip intact.
    Bnd_Box fullBounds;BRepBndLib::Add(cropped,fullBounds);fullBounds.Get(x0,y0,z0,x1,y1,z1);
    assert(y1>499.8);
    // A deliberate internal setback beyond the tolerance preserves the outline.
    nearEnd.panels[0].points[3]={0,200};
    nearEnd.stations[1].first.position={240,0};nearEnd.stations[1].second.position={240,200};
    for(auto& panel:nearEnd.spars)for(auto& spar:panel)spar.enabled=false;
    const auto untrimmed=geometry::buildWingSolid(nearEnd);validate(untrimmed,4);
    TopExp_Explorer firstUntrimmed{untrimmed,TopAbs_SOLID};Bnd_Box retained;
    BRepBndLib::AddOptimal(firstUntrimmed.Current(),retained,false,false);retained.Get(x0,y0,z0,x1,y1,z1);
    assert(std::abs(y1-250)<.02);
 }
void checkDihedral() {
  auto input=rectangular();input.wingspanMm.reset();
  input.panels={
    {{{0,0},{250,0},{250,200},{0,200}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{2,3}}}},
    {{{250,0},{500,0},{500,200},{250,200}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}}};
  input.controls.resize(2);input.spars.resize(2);input.dihedralDegrees={3,5};
  input.stations={{{0,0,0,{0,0}},{0,1,1,{0,200}},gui::LineAlignment::Vertical,0},
    {{0,0,1,{250,0}},{0,1,0,{250,200}},gui::LineAlignment::Vertical,0},
    {{1,0,0,{250,0}},{1,2,1,{250,200}},gui::LineAlignment::Vertical,0},
    {{1,0,1,{500,0}},{1,2,0,{500,200}},gui::LineAlignment::Vertical,0}};
  const auto shape=geometry::buildWingSolid(input);validate(shape,4);
  std::vector<TopoDS_Shape> bodies;for(TopExp_Explorer e{shape,TopAbs_SOLID};e.More();e.Next())bodies.push_back(e.Current());
  auto contains=[](const TopoDS_Shape& body,gp_Pnt p){BRepClass3d_SolidClassifier c{body,p,1e-6};return c.State()==TopAbs_IN || c.State()==TopAbs_ON;};
  const double rad=std::numbers::pi/180,a=3*rad,b=8*rad,joint=5.5*rad;
  const double baseY=250*std::cos(a),baseZ=250*std::sin(a);
  for(double x:{40.,100.,160.})for(double w:{-2.,0.,2.}) {
    const double y=baseY-std::sin(joint)*w/std::cos(2.5*rad),z=baseZ+std::cos(joint)*w/std::cos(2.5*rad);
    assert(contains(bodies[0],{x,y-0.02*std::cos(joint),z-0.02*std::sin(joint)}));
    assert(contains(bodies[1],{x,y+0.02*std::cos(joint),z+0.02*std::sin(joint)}));
    assert(!contains(bodies[0],{x,y+0.02*std::cos(joint),z+0.02*std::sin(joint)}));
    assert(!contains(bodies[1],{x,y-0.02*std::cos(joint),z-0.02*std::sin(joint)}));
    assert(contains(bodies[0],{x,.02,w/std::cos(a)}));assert(contains(bodies[2],{x,-.02,w/std::cos(a)}));
    assert(!contains(bodies[0],{x,-.02,w/std::cos(a)}));assert(!contains(bodies[2],{x,.02,w/std::cos(a)}));
  }
  const double tipY=baseY+250*std::cos(b),tipZ=baseZ+250*std::sin(b);
  assert(contains(bodies[1],{100,tipY-.1*std::cos(b),tipZ-.1*std::sin(b)}));
  assert(!contains(bodies[1],{100,tipY+.1*std::cos(b),tipZ+.1*std::sin(b)}));
  assert(contains(bodies[3],{100,-tipY+.1*std::cos(b),tipZ-.1*std::sin(b)}));
  std::cout<<"Dihedral: inner 3 degrees, outer 8 degrees; tip Y="<<tipY<<", Z="<<tipZ<<" mm. Mating faces verified.\n";
  input.dihedralDegrees={80,10};bool rejected=false;
  try{geometry::buildWingSolid(input);}catch(const std::exception&){rejected=true;}assert(rejected);
}
int main(int argc,char** argv) {
  try {
    if(argc>1){if(std::string{argv[1]}=="--dihedral")checkDihedral();else checkNearTerminalPanels();return 0;}
    auto input=rectangular(); const auto shape=geometry::buildWingSolid(input); const double base=validate(shape);
    Bnd_Box box; BRepBndLib::Add(shape,box); double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    assert(std::abs(y0+500)<0.2 && std::abs(y1-500)<0.2);
    assert(std::abs(x0)<0.2 && std::abs(x1-200)<0.2);
    // Rounded tips keep the planform while reducing thickness at the contour.
    for(double chord : {20.,60.,100.,160.,180.}) reachesOutline(shape,{500,chord});
    IntCurvesFace_ShapeIntersector tipRay;tipRay.Load(shape,1e-7);
    auto tipThickness=[&](double y){tipRay.Perform(gp_Lin{gp_Pnt{100,y,-100},gp_Dir{0,0,1}},0,200);double lo=1e9,hi=-1e9;for(int i=1;i<=tipRay.NbPnt();++i){lo=std::min(lo,tipRay.Pnt(i).Z());hi=std::max(hi,tipRay.Pnt(i).Z());}assert(tipRay.NbPnt()>=2);return hi-lo;};
    const double roundRatio=tipThickness(495)/tipThickness(480);assert(roundRatio>.7 && roundRatio<.95);
    input.wingspanMm=2000;
    assert(std::abs(validate(geometry::buildWingSolid(input))/base-8)<1e-4);
    input.wingspanMm.reset(); assert(std::abs(validate(geometry::buildWingSolid(input))-base)/base<1e-5);
    // Two independent inner chains, tapered outer panel, distinct interpolated profiles.
    input=rectangular(); input.panels[0].points={{0,0},{250,0},{250,200},{0,200}};
    input.panels[0].curves={{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{2,3}}};
    input.panels.push_back({{{250,0},{500,50},{500,150},{250,200}},
      {{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}});
    input.stations[1].first.position={500,50};input.stations[1].second.position={500,150};
    input.airfoils.push_back({"NACA0008",domain::AirfoilProfile::nacaSymmetric(0.08),{}, {}});
    input.stations={{{0,0,0,{0,0}},{0,1,1,{0,200}},gui::LineAlignment::Vertical,0},
      {{0,0,1,{250,0}},{0,1,0,{250,200}},gui::LineAlignment::Vertical,0},
      {{1,0,0,{250,0}},{1,2,1,{250,200}},gui::LineAlignment::Vertical,1},
      {{1,0,1,{500,50}},{1,2,0,{500,150}},gui::LineAlignment::Vertical,1}};
    input.controls.resize(2);input.spars.resize(2);
    const auto independent=geometry::buildWingSolid(input);assert(validate(independent,4)<base);
    IntCurvesFace_ShapeIntersector ray;ray.Load(independent,1e-7);
    auto thickness=[&](double y){ray.Perform(gp_Lin{gp_Pnt{60,y,-100},gp_Dir{0,0,1}},0,200);double lo=1e9,hi=-1e9;for(int i=1;i<=ray.NbPnt();++i){lo=std::min(lo,ray.Pnt(i).Z());hi=std::max(hi,ray.Pnt(i).Z());}return hi-lo;};
    assert(thickness(240)>thickness(260)*1.3);
    input.controls[0][1]={true,gui::HingeCut::Tape,QRectF{30,140,150,100}};
    input.controls[1][1]={true,gui::HingeCut::Standard,QRectF{300,130,130,100}};
    validate(geometry::buildWingSolid(input),8);
    input.stations.pop_back();bool panelRejected=false;
    try{geometry::buildWingSolid(input);}catch(const std::exception& e){panelRejected=std::string{e.what()}.find("Panel 2")!=std::string::npos;}assert(panelRejected);
    input=rectangular(); input.stations[1].first.position={400,0}; input.stations[1].second.position={420,200};
    const double oblique=validate(geometry::buildWingSolid(input));
    std::reverse(input.stations.begin(),input.stations.end());
    assert(std::abs(validate(geometry::buildWingSolid(input))-oblique)/oblique<1e-6);
    input=rectangular();
    input.panels[0].points={{0,0},{450,0},{500,100},{450,200},{0,200}};
    input.panels[0].curves={{gui::SketchTool::Line,{0,1}},
      {gui::SketchTool::Spline,{1,2,3}},{gui::SketchTool::Line,{3,4}}};
    input.stations[1].first.position={400,0}; input.stations[1].second.position={400,200};
    const auto tipPath=gui::SketchEditor::fittedPath({{450,0},{500,100},{450,200}},gui::SketchTool::Spline);
    {
      const auto curved=geometry::buildWingSolid(input); validate(curved);
      for(int i=tipPath.elementCount()/4;i<3*tipPath.elementCount()/4;i+=tipPath.elementCount()/8) {
        const auto point=tipPath.elementAt(i); reachesOutline(curved,{point.x,point.y});
      }
    }
    // Open root endpoints need not share an image X coordinate, and the first
    // station may be placed slightly outboard of that root.
    input=rectangular(); input.panels[0].points[3]={8,200};
    input.stations[0].first.position={20,0}; input.stations[0].second.position={20,200};
    validate(geometry::buildWingSolid(input));
    input=rectangular();
    input.stations[0].first.position={20,0}; input.stations[0].second.position={28,200};
    validate(geometry::buildWingSolid(input));
    // An actual interior pinch must still be rejected rather than skipped.
    input=rectangular();
    input.panels[0].points={{0,0},{250,100},{500,0},{500,200},{250,100},{0,200}};
    input.panels[0].curves={{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},
      {gui::SketchTool::Line,{2,3}},{gui::SketchTool::Line,{3,4}},{gui::SketchTool::Line,{4,5}}};
    bool pinchRejected=false;
    try { geometry::buildWingSolid(input); }
    catch(const std::runtime_error& e) { pinchRejected=std::string{e.what()}.find("The outline closes before the wing tip.")!=std::string::npos; }
    assert(pinchRejected);
    // A traced closed contour can begin anywhere and use screen Y-down coordinates.
    input=rectangular(); input.airfoils[0].imported.reset();
    input.airfoils[0].boundary={{100,100},{150,90},{200,100},{150,110},{100,100}};
    validate(geometry::buildWingSolid(input));
    // Saved inconsistent stations must not silently create a twisted wing.
    auto reversed=rectangular();std::swap(reversed.stations[1].first,reversed.stations[1].second);
    bool orientationRejected=false;
    try{geometry::buildWingSolid(reversed);}catch(const std::exception& e){orientationRejected=std::string{e.what()}.find("Conflicting LE/TE orientation")!=std::string::npos;}
    assert(orientationRejected);
    checkNearTerminalPanels();checkDihedral();
    input.stations[0].airfoil.reset(); bool rejected=false;
    try { geometry::buildWingSolid(input); } catch(const std::exception&) { rejected=true; } assert(rejected);
    std::cout << "Wing solids: mirroring, scale, rounded tips, panels, interpolation, traces and invalid assignments passed.\n";
  } catch(const Standard_Failure& e) {std::cerr<<e.GetMessageString()<<'\n';return 1;}
    catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}

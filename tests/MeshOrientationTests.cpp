#include "geometry/MeshOrientation.h"
#include "geometry/WingSolidBuilder.h"
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <Poly_Triangulation.hxx>
#include <gp_Vec.hxx>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <Standard_Failure.hxx>
using namespace designrc;
#define CHECK(c) do{if(!(c))throw std::runtime_error(#c);}while(false)
void check(const TopoDS_Shape& shape) {
  int triangles=0,unmeshed=0;
  for(TopExp_Explorer e{shape,TopAbs_FACE};e.More();e.Next()) {
    const auto face=TopoDS::Face(e.Current());TopLoc_Location location,surfaceLocation;
    const auto mesh=BRep_Tool::Triangulation(face,location);const auto surface=BRep_Tool::Surface(face,surfaceLocation);
    if(mesh.IsNull()){++unmeshed;continue;}
    CHECK(mesh->HasUVNodes());
    // Tiny tip triangles can straddle a surface singularity; use the full
    // face area-weighted agreement, independently of the repair sampling.
    double agreement=0,area=0;
    for(int i=1;i<=mesh->NbTriangles();++i) {
      int a,b,c;mesh->Triangle(i).Get(a,b,c);const auto uv=(mesh->UVNode(a).XY()+mesh->UVNode(b).XY()+mesh->UVNode(c).XY())/3;
      gp_Pnt point;gp_Vec du,dv;surface->D1(uv.X(),uv.Y(),point,du,dv);du.Transform(surfaceLocation.Transformation());dv.Transform(surfaceLocation.Transformation());
      const auto p=mesh->Node(a).Transformed(location.Transformation()),q=mesh->Node(b).Transformed(location.Transformation()),r=mesh->Node(c).Transformed(location.Transformation());
      const auto cross=gp_Vec{p,q}.Crossed(gp_Vec{p,r});
      if(cross.SquareMagnitude()>1e-20 && du.Crossed(dv).SquareMagnitude()>1e-20){agreement+=cross.Dot(du.Crossed(dv))/du.Crossed(dv).Magnitude();area+=cross.Magnitude();++triangles;}
    }
    if(area>1e-10)CHECK(agreement>0);
  }CHECK(triangles>0);
  std::cout<<"Checked "<<triangles<<" triangles; "<<unmeshed<<" faces have no cached triangulation."<<std::endl;
}
int main(int argc,char** argv) {
  try {
    if(argc==3) {
      TopoDS_Shape shape;BRep_Builder builder;CHECK(BRepTools::Read(shape,argv[1],builder));
      const auto start=std::chrono::steady_clock::now();const auto corrected=geometry::alignMeshOrientation(shape);
      std::cout<<"Corrected faces: "<<corrected<<" in "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" seconds."<<std::endl;
      check(shape);CHECK(geometry::alignMeshOrientation(shape)==0);CHECK(BRepTools::Write(shape,argv[2]));return 0;
    }
    geometry::WingSolidInput wing;
    wing.panels={{{{0,0},{200,0},{200,100},{0,100}},{{gui::SketchTool::Line,{0,1}},{gui::SketchTool::Line,{1,2}},{gui::SketchTool::Line,{2,3}}}}};
    wing.stations={{{0,0,0,{0,0}},{0,2,1,{0,100}},gui::LineAlignment::Vertical,0},{{0,0,1,{200,0}},{0,2,0,{200,100}},gui::LineAlignment::Vertical,0}};
    wing.airfoils.push_back({"NACA0012",domain::AirfoilProfile::nacaSymmetric(.12),{}, {}});wing.dihedralDegrees={5};
    auto shape=geometry::buildWingSolid(wing);check(shape);CHECK(geometry::alignMeshOrientation(shape)==0);
    // Reverse one face deliberately; the utility must detect and repair it.
    auto face=TopoDS::Face(TopExp_Explorer{shape,TopAbs_FACE}.Current());TopLoc_Location location;auto mesh=BRep_Tool::Triangulation(face,location);
    for(int i=1;i<=mesh->NbTriangles();++i){int a,b,c;mesh->Triangle(i).Get(a,b,c);mesh->SetTriangle(i,Poly_Triangle{a,c,b});}
    CHECK(geometry::alignMeshOrientation(shape)==1);check(shape);
    std::cout<<"Mirrored curved-wing mesh winding, normalization and idempotence passed.\n";
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<'\n';return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

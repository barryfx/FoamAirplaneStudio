#include "geometry/MeshOrientation.h"
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <Poly_Triangulation.hxx>
#include <gp_Vec.hxx>
#include <algorithm>
namespace designrc::geometry {
std::size_t alignMeshOrientation(const TopoDS_Shape& shape,const ProcessingControl& control) {
  TopTools_IndexedMapOfShape faces;TopExp::MapShapes(shape,TopAbs_FACE,faces);std::size_t corrected=0;
  for(int faceIndex=1;faceIndex<=faces.Extent();++faceIndex) {
    control.checkpoint();const auto face=TopoDS::Face(faces(faceIndex));
    TopLoc_Location meshLocation,surfaceLocation;
    const auto mesh=BRep_Tool::Triangulation(face,meshLocation);
    const auto surface=BRep_Tool::Surface(face,surfaceLocation);
    if(mesh.IsNull() || surface.IsNull() || !mesh->HasUVNodes())continue;
    // OCCT 8's copied reflection can reverse both the face and its triangles.
    // Test against the surface instead of relying on version-specific behavior:
    // AIS separately reverses triangles for a REVERSED face.
    int vote=0;const int step=std::max(1,mesh->NbTriangles()/16);
    for(int i=1;i<=mesh->NbTriangles();i+=step) {
      int a,b,c;mesh->Triangle(i).Get(a,b,c);
      const auto uv=(mesh->UVNode(a).XY()+mesh->UVNode(b).XY()+mesh->UVNode(c).XY())/3;
      gp_Pnt point;gp_Vec du,dv;surface->D1(uv.X(),uv.Y(),point,du,dv);
      du.Transform(surfaceLocation.Transformation());dv.Transform(surfaceLocation.Transformation());
      const auto p=mesh->Node(a).Transformed(meshLocation.Transformation());
      const auto q=mesh->Node(b).Transformed(meshLocation.Transformation());
      const auto r=mesh->Node(c).Transformed(meshLocation.Transformation());
      const auto triangle=gp_Vec{p,q}.Crossed(gp_Vec{p,r}),normal=du.Crossed(dv);
      const double dot=triangle.Dot(normal),product=triangle.SquareMagnitude()*normal.SquareMagnitude();
      if(product>1e-30 && dot*dot>product*.0625)vote+=dot>0?1:-1;
    }
    if(vote<0) {
      ++corrected;
      for(int i=1;i<=mesh->NbTriangles();++i) {
        if(i%1024==0)control.checkpoint();
        int a,b,c;mesh->Triangle(i).Get(a,b,c);mesh->SetTriangle(i,Poly_Triangle{a,c,b});
      }
    }
    // Recompute normals from the transformed surface when the viewer needs them.
    mesh->RemoveNormals();
  }
  return corrected;
}
}

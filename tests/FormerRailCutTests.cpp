#include "geometry/Formers.h"
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <iostream>
#include <cmath>
#include <stdexcept>
#define CHECK(c) do{if(!(c))throw std::runtime_error(#c);}while(false)
double volume(const TopoDS_Shape& shape) {GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass);return mass.Mass();}
TopoDS_Shape read(const std::string& path) {TopoDS_Shape shape;BRep_Builder b;CHECK(BRepTools::Read(shape,path.c_str(),b));return shape;}
int main(int argc,char** argv) {
  try {
    CHECK(argc==3);const std::string base=argv[1];const double expectedVolume=std::stod(argv[2]);
    const auto pocket=read(base+"-pocket.brep"),clearance=read(base+"-clearance.brep");
    const auto originalVolume=volume(pocket);
    for(bool parallel:{false,true}) {
      const auto rail=designrc::geometry::cutFormerRetainingRail(pocket,clearance,{{},parallel});
      CHECK(BRepCheck_Analyzer{rail}.IsValid());CHECK(std::abs(volume(rail)-expectedVolume)<.01);
      CHECK(std::abs(volume(pocket)-originalVolume)<1e-7);
      BRepClass3d_SolidClassifier source{pocket},tool{clearance},result{rail};
      Bnd_Box box;BRepBndLib::AddOptimal(pocket,box,false,false);
      double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
      int retained=0,removed=0;
      // Independent point-membership oracle: rail = pocket minus clearance.
      // This detects retained plugs and missing wall bands, even if OCCT calls
      // the Boolean successful and its topology valid.
      for(int i=0;i<5;++i)for(int j=0;j<19;++j)for(int k=0;k<21;++k) {
        const gp_Pnt p{x0+(x1-x0)*(i+.37)/5,y0+(y1-y0)*(j+.41)/19,z0+(z1-z0)*(k+.29)/21};
        source.Perform(p,1e-5);tool.Perform(p,1e-5);result.Perform(p,1e-5);
        if(source.State()==TopAbs_ON||tool.State()==TopAbs_ON||result.State()==TopAbs_ON)continue;
        const bool expected=source.State()==TopAbs_IN&&tool.State()==TopAbs_OUT;
        CHECK((result.State()==TopAbs_IN)==expected);
        if(expected)++retained;
        if(source.State()==TopAbs_IN&&tool.State()==TopAbs_IN)++removed;
      }
      CHECK(retained>30&&removed>100);
      std::cout<<"parallel="<<parallel<<" volume="<<volume(rail)<<" retained samples="<<retained<<" cleared samples="<<removed<<std::endl;
    }
    std::stop_source stop;stop.request_stop();bool cancelled=false;
    try{designrc::geometry::cutFormerRetainingRail(pocket,clearance,{stop.get_token()});}
    catch(const designrc::geometry::ProcessingCancelled&){cancelled=true;}CHECK(cancelled);
    return 0;
  }catch(const Standard_Failure& e){std::cerr<<e.what()<<std::endl;return 1;}
   catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

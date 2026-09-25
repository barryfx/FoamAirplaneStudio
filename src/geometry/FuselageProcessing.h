#pragma once
#include "geometry/ProcessingControl.h"
#include <BRepGProp_Face.hxx>
#include <BRepGProp_Domain.hxx>
#include <BRepGProp_Vinert.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

namespace designrc::geometry {
// Keep the same analytic face integrals and common reference point as OCCT's
// volume integration. A whole-body integration has no progress/cancel API.
inline void fuselageVolumeProperties(const TopoDS_Shape& shape,GProp_GProps& result,
                                     const ProcessingControl& processing) {
  processing.checkpoint();gp_Pnt origin;origin.Transform(shape.Location());
  result=GProp_GProps{origin};
  BRepGProp_Vinert integral;integral.SetLocation(origin);
  for(TopExp_Explorer face{shape,TopAbs_FACE};face.More();face.Next()) {
    processing.checkpoint();const auto f=TopoDS::Face(face.Current());
    if(f.Orientation()!=TopAbs_FORWARD&&f.Orientation()!=TopAbs_REVERSED)continue;
    BRepGProp_Face surface{f};
    if(f.NbChildren()==0)integral.Perform(surface);
    else {BRepGProp_Domain domain{f};integral.Perform(surface,domain);}
    result.Add(integral);
  }
  processing.checkpoint();
}
inline bool fuselageValid(const TopoDS_Shape& shape,const ProcessingControl& processing) {
  processing.checkpoint();
  const bool valid=BRepCheck_Analyzer{shape,true,processing.parallel}.IsValid();
  processing.checkpoint();return valid;
}
inline void fuselageBounds(const TopoDS_Shape& shape,Bnd_Box& box,const ProcessingControl& processing) {
  processing.checkpoint();
  for(TopExp_Explorer face{shape,TopAbs_FACE};face.More();face.Next()) {
    processing.checkpoint();BRepBndLib::AddOptimal(face.Current(),box,false,false);
  }
  processing.checkpoint();
}
}

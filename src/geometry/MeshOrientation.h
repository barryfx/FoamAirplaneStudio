#pragma once
#include "geometry/ProcessingControl.h"
#include <TopoDS_Shape.hxx>
#include <cstddef>
namespace designrc::geometry {
// Mutates only cached triangulations of a worker-owned shape, before publication.
// Normalize cached triangle winding to the face's parametric surface, before
// the renderer applies the topological face orientation. CAD geometry is unchanged.
std::size_t alignMeshOrientation(const TopoDS_Shape&,const ProcessingControl& = {});
}

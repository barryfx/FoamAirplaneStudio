#pragma once
#include "geometry/ProcessingControl.h"
#include <QPointF>
#include <TopoDS_Shape.hxx>
#include <vector>

namespace designrc::geometry {
// The loft samples run top, right, bottom, left. Only reflection-symmetric
// sections about the registered Y=0 plane are eligible for the fast path.
bool symmetricFuselageSection(const std::vector<QPointF>& perimeter);
std::vector<QPointF> rightFuselageSection(const std::vector<QPointF>& perimeter);
TopoDS_Shape reflectFuselage(const TopoDS_Shape& shape,const ProcessingControl& processing={});
TopoDS_Shape fuselagePair(const TopoDS_Shape& right,const ProcessingControl& processing={});
// Remove the artificial Y=0 closure, reflect the remaining skin and sew one
// closed solid. This avoids a Boolean union of two coincident seam faces.
TopoDS_Shape joinMirroredFuselage(const TopoDS_Shape& half,
    const ProcessingControl& processing);
}

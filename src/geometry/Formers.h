#pragma once
#include "geometry/ProcessingControl.h"
#include <TopoDS_Shape.hxx>
#include <QRectF>
#include <optional>
#include <vector>
#include <functional>
namespace designrc::geometry {
// Validated rail difference; retries an incorrect OCCT result with a bounded
// 0.0001 mm maximum fuzzy tolerance. Operands remain unchanged and cancellation propagates.
TopoDS_Shape cutFormerRetainingRail(const TopoDS_Shape& pocket,const TopoDS_Shape& clearance,
                                  const ProcessingControl& processing={});
// Integral, full-height side retainers: 4 mm fore/aft, 3 mm into the cavity.
TopoDS_Shape addFormerRetainers(const TopoDS_Shape& body,const TopoDS_Shape& cavity,
    const std::vector<QRectF>& rectangles,const std::vector<TopoDS_Shape>& inserts,
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={},const std::vector<double>& rotationDegrees={},bool rightHalfOnly=false);
// Physical X/Z rectangles: y is lower Z. Cavity clips all remaining dimensions.
std::vector<TopoDS_Shape> buildFormers(const TopoDS_Shape& cavity,const TopoDS_Shape& supportedBody,
    const std::vector<QRectF>& rectangles,const std::optional<QRectF>& tray={},
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={},const std::vector<double>& rotationDegrees={});
}

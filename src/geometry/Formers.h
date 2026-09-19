#pragma once
#include "geometry/ProcessingControl.h"
#include <TopoDS_Shape.hxx>
#include <QRectF>
#include <optional>
#include <vector>
#include <functional>
namespace designrc::geometry {
// Integral, full-height side retainers: 4 mm fore/aft, 3 mm into the cavity.
TopoDS_Shape addFormerRetainers(const TopoDS_Shape& body,const TopoDS_Shape& cavity,
    const std::vector<QRectF>& rectangles,const std::vector<TopoDS_Shape>& inserts,
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={},const std::vector<double>& rotationDegrees={});
// Physical X/Z rectangles: y is lower Z. Cavity clips all remaining dimensions.
std::vector<TopoDS_Shape> buildFormers(const TopoDS_Shape& cavity,const TopoDS_Shape& supportedBody,
    const std::vector<QRectF>& rectangles,const std::optional<QRectF>& tray={},
    const std::function<void(const char*)>& progress={},const ProcessingControl& processing={},const std::vector<double>& rotationDegrees={});
}

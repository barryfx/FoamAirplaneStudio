#pragma once
#include "geometry/ProcessingControl.h"
#include <TopoDS_Shape.hxx>
#include <QRectF>
#include <functional>
namespace designrc::geometry {
struct ServoTrayGeometry {TopoDS_Shape body,tray,topFaces;};
// Rectangle is physical X/Z: left/right set length, y is underside, height is thickness.
ServoTrayGeometry addServoTray(const TopoDS_Shape& body,const TopoDS_Shape& cavity,
    const QRectF& rectangle,const std::function<void(const char*)>& progress={},const ProcessingControl& processing={},bool rightHalfOnly=false);
}

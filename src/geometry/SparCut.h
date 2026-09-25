#pragma once
#include "geometry/SparMaterial.h"
#include "geometry/ProcessingControl.h"
#include "gui/SparState.h"
#include <TopoDS_Shape.hxx>
#include <functional>
#include <vector>
namespace designrc::geometry {
// Coordinates are millimetres: X chord, Y span, Z height; span starts at the panel root.
// Mid uses 50% local thickness and a matching split surface.
// Input is one main wing panel; any appended control bodies remain intact.
TopoDS_Shape cutSpars(const TopoDS_Shape& half,const gui::SparState& spars,
    double halfSpan,const std::function<std::pair<double,double>(double)>& chordAtSpan,
    const std::function<void(const char*)>& progress={}, double rootInset=0,double tipInset=0,bool mitered=false,
    double lighteningWall=0,const std::vector<std::pair<double,double>>& bays={},const ProcessingControl& control={},std::vector<SparMaterial>* materials=nullptr);
}

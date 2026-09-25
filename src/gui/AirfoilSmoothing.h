#pragma once
#include "domain/AirfoilProfile.h"
namespace designrc::gui {
// Separate a short, steep closing cap from coincident trailing-edge endpoints.
domain::AirfoilProfile airfoilSmoothingSource(const domain::AirfoilProfile& original);
// Strength 0..100 increases the curvature penalty; even 0 fits a smooth curve.
domain::AirfoilProfile smoothAirfoil(const domain::AirfoilProfile& original,int strength);
struct AirfoilMetrics { double thickness{},camber{}; };
AirfoilMetrics airfoilMetrics(const domain::AirfoilProfile& profile);
}

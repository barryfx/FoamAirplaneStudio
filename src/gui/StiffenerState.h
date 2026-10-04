#pragma once
#include "gui/LengthUnit.h"
#include "gui/SparState.h"
#include <cmath>
#include <stdexcept>
namespace designrc::gui {
struct StiffenerState {
  int count=0;
  SparShape shape=SparShape::Strip;
  double startPercent=20,stopPercent=90;
  double widthMm=3,heightMm=1,diameterMm=3;
  LengthUnit widthUnit=LengthUnit::Default,heightUnit=LengthUnit::Default,diameterUnit=LengthUnit::Default;
};
inline void validateStiffeners(const StiffenerState& s) {
  if(s.count<0||s.count>3||(s.shape!=SparShape::Strip&&s.shape!=SparShape::Round)||
      !std::isfinite(s.startPercent)||!std::isfinite(s.stopPercent)||s.startPercent<0||s.stopPercent>100||s.startPercent>=s.stopPercent||
      !std::isfinite(s.widthMm)||s.widthMm<.01||s.widthMm>100||!std::isfinite(s.heightMm)||s.heightMm<.01||s.heightMm>100||
      !std::isfinite(s.diameterMm)||s.diameterMm<.01||s.diameterMm>100)
    throw std::runtime_error("Stiffeners require 0-3 per side, Start < Stop within 0-100%, positive dimensions.");
}
}

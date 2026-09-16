#pragma once
#include <array>
#include <vector>
#include <string>
namespace designrc::gui {
enum class SparShape { Round, Strip };
struct Spar {
  bool enabled=false;
  SparShape shape=SparShape::Round;
  double chordPercent=30, lengthPercent=80;
  double sizeMm=3, heightMm=1; // Diameter or strip width; strip depth.
  std::string sizeText, heightText; // Empty until entered; then explicit display units.
};
using SparState=std::array<Spar,3>; // Top, bottom, mid (round only), within one panel.
using PanelSpars=std::vector<SparState>;
}

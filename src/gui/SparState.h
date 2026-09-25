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
  double insideDiameterMm=0; // Mid tube bore; zero represents a solid rod.
  std::string insideDiameterText; // Empty: default bore follows OD minus 1 mm.
};
struct SparState : std::array<Spar,3> { // Top, bottom, mid within one panel.
  SparState() { (*this)[2].sizeMm=6; (*this)[2].insideDiameterMm=5; }
};
using PanelSpars=std::vector<SparState>;
}

#pragma once
#include <array>
#include <string>
namespace designrc::gui {
struct LighteningState {
  bool enabled=false;
  double wallMm=2, ribMm=3, startMm=25, stopMm=25;
  int crossmembers=4;
  std::array<std::string,4> text{};
};
}

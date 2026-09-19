#pragma once
#include <array>
#include <QPointF>

namespace designrc::gui {
// Physical X/Z translations in fuselage coordinates; Y remains on the centerline.
struct AssemblyState {
  bool positioned=false;
  std::array<QPointF,3> offsets{}; // Wing, horizontal stabilizer, vertical stabilizer.
  bool cuts=false;
};
}

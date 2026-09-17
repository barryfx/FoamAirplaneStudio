#pragma once
#include "gui/ConstrainedLineEditor.h"
#include <algorithm>
#include <numeric>
namespace designrc::gui {
// Display numbers follow nose-to-tail position. Keep storage and profile links
// untouched so moving a station never transfers its sketch or wall thickness.
inline std::vector<int> fuselageStationOrder(const std::vector<ConstrainedLine>& stations) {
  std::vector<int> order(stations.size());std::iota(order.begin(),order.end(),0);
  std::stable_sort(order.begin(),order.end(),[&](int a,int b){return stations[a].first.position.x()<stations[b].first.position.x();});
  return order;
}
inline int fuselageStationNumber(const std::vector<ConstrainedLine>& stations,int index) {
  const auto order=fuselageStationOrder(stations);
  const auto found=std::find(order.begin(),order.end(),index);
  return found==order.end()?0:static_cast<int>(found-order.begin())+1;
}
}

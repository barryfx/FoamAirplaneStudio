#pragma once
#include <QRectF>
#include <array>
#include <optional>
#include <vector>
namespace designrc::gui {
enum class HingeCut { Tape, Standard };
struct ControlSurface {
  bool enabled = false;
  HingeCut hinge = HingeCut::Tape;
  std::optional<QRectF> rectangle;
};
inline bool controlSurfacesOverlap(const std::array<ControlSurface,2>& surfaces) {
  return surfaces[0].enabled && surfaces[1].enabled && surfaces[0].rectangle && surfaces[1].rectangle &&
    surfaces[0].rectangle->normalized().intersects(surfaces[1].rectangle->normalized());
}
using PanelControls = std::vector<std::array<ControlSurface,2>>;
inline bool controlSurfacesOverlap(const PanelControls& panels) {
  std::vector<const ControlSurface*> active;
  for(const auto& panel:panels)for(const auto& surface:panel)if(surface.enabled && surface.rectangle)active.push_back(&surface);
  for(std::size_t i=0;i<active.size();++i)for(std::size_t j=i+1;j<active.size();++j)
    if(active[i]->rectangle->normalized().intersects(active[j]->rectangle->normalized()))return true;
  return false;
}
struct ControlSurfaceState {
  PanelControls panels{1};
  int panel=0; // Ailerons, then flaps; right half wing.
  int drawing = -1;
  std::optional<QPointF> first;
};
}

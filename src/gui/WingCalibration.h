#pragma once
#include "gui/SketchEditor.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace designrc::gui {
// Uses the same root frame and mirrored span convention as WingSolidBuilder.
struct WingCalibration { double scale; double leadingEdgeX; };
inline WingCalibration wingCalibration(const std::vector<SketchLayer>& panels,
    const std::vector<ConstrainedLine>& stations,std::optional<double> wingspanMm) {
  if(panels.empty()||stations.empty())throw std::runtime_error("Define the wing root and stations for the balance datum.");
  auto chord=stations.front().second.position-stations.front().first.position;
  if(std::hypot(chord.x(),chord.y())<1e-8)throw std::runtime_error("Wing chord is undefined.");
  chord/=std::hypot(chord.x(),chord.y());
  auto span=QPointF{chord.y(),-chord.x()};
  if(span.x()<0||(std::abs(span.x())<1e-8&&span.y()<0))span=-span;
  const auto dot=[](QPointF a,QPointF b){return a.x()*b.x()+a.y()*b.y();};
  const auto& panel=panels.front();std::vector<int> degrees(panel.points.size());
  for(const auto& curve:panel.curves)if(curve.points.size()>=2) {++degrees.at(curve.points.front());++degrees.at(curve.points.back());}
  std::vector<QPointF> ends;
  for(std::size_t i=0;i<degrees.size();++i)if(degrees[i]==1)ends.push_back(panel.points[i]);
  std::sort(ends.begin(),ends.end(),[&](auto a,auto b){return dot(a,span)<dot(b,span);});
  if(ends.size()<2)throw std::runtime_error("Wing root endpoints are undefined.");
  auto rootChord=ends[1]-ends[0];
  const double chordLength=std::hypot(rootChord.x(),rootChord.y());
  if(chordLength<1e-8)throw std::runtime_error("Wing root chord is zero.");
  if(dot(rootChord,chord)<0)rootChord=-rootChord;
  chord=rootChord/chordLength;span={chord.y(),-chord.x()};
  if(span.x()<0||(std::abs(span.x())<1e-8&&span.y()<0))span=-span;
  const double root=dot(ends[0],span);double tip=root;
  for(const auto& layer:panels)for(const auto& curve:layer.curves) {
    std::vector<QPointF> points;for(auto id:curve.points)points.push_back(layer.points.at(id));
    const auto path=SketchEditor::fittedPath(points,curve.type);
    for(int i=0;i<path.elementCount();++i)tip=std::max(tip,dot({path.elementAt(i).x,path.elementAt(i).y},span));
  }
  if(tip-root<1e-8)throw std::runtime_error("Wing span is zero.");
  const double scale=wingspanMm?*wingspanMm/(2*(tip-root)):1.;
  if(!std::isfinite(scale)||scale<=0)throw std::runtime_error("Wingspan must be positive.");
  return {scale,std::min(dot(ends[0],chord),dot(ends[1],chord))*scale};
}
}

#pragma once
#include "gui/SketchBoundary.h"
#include <QPainterPath>
#include <algorithm>
#include <numeric>
namespace designrc::gui {
struct SketchPath {SketchLayer layer;std::vector<std::size_t> points,curves;};
inline std::vector<SketchPath> sketchPaths(const SketchLayer& layer) {
  std::vector<std::size_t> root(layer.points.size());std::iota(root.begin(),root.end(),0);
  auto find=[&](std::size_t i){while(root[i]!=i)i=root[i];return i;};
  for(const auto& c:layer.curves)for(auto i:c.points)root[find(i)]=find(c.points.front());
  std::vector<SketchPath> paths;std::vector<std::size_t> groups;
  for(std::size_t i=0;i<root.size();++i) {
    auto r=find(i);auto at=std::find(groups.begin(),groups.end(),r);
    if(at==groups.end()){groups.push_back(r);paths.emplace_back();at=groups.end()-1;}
    auto& p=paths[at-groups.begin()];p.points.push_back(i);p.layer.points.push_back(layer.points[i]);
  }
  for(std::size_t i=0;i<layer.curves.size();++i) {
    auto& p=paths[std::find(groups.begin(),groups.end(),find(layer.curves[i].points.front()))-groups.begin()];
    auto c=layer.curves[i];for(auto& id:c.points)id=std::find(p.points.begin(),p.points.end(),id)-p.points.begin();
    p.curves.push_back(i);p.layer.curves.push_back(std::move(c));
  }
  return paths;
}
inline QPainterPath sketchPolygon(const std::vector<QPointF>& points) {
  QPainterPath p;if(points.empty())return p;p.moveTo(points.front());for(auto v:points)p.lineTo(v);p.closeSubpath();return p;
}
inline bool sketchPathInside(const SketchLayer& shape,const SketchLayer& outline) {
  const auto boundary=closedSketchBoundary(outline);if(!boundary)return false;
  const auto outer=sketchPolygon(*boundary);
  for(auto p:shape.points)if(!outer.contains(p))return false;
  for(const auto& curve:shape.curves) {
    std::vector<QPointF> points;for(auto i:curve.points)points.push_back(shape.points[i]);
    const auto path=SketchEditor::fittedPath(points,curve.type);
    if(!outer.contains(path))return false;
  }
  return true;
}
inline void eraseSketchPath(SketchLayer& layer,const SketchPath& remove) {
  SketchLayer kept;std::vector<std::size_t> map(layer.points.size());
  for(std::size_t i=0;i<layer.points.size();++i)if(std::find(remove.points.begin(),remove.points.end(),i)==remove.points.end()){map[i]=kept.points.size();kept.points.push_back(layer.points[i]);}
  for(std::size_t i=0;i<layer.curves.size();++i)if(std::find(remove.curves.begin(),remove.curves.end(),i)==remove.curves.end()){auto c=layer.curves[i];for(auto& id:c.points)id=map[id];kept.curves.push_back(std::move(c));}
  layer=std::move(kept);
}
}

#pragma once
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace designrc::gui {
inline double formerAngle(const std::vector<double>& angles,std::size_t i) {
  return i<angles.size()?angles[i]:0.;
}
// Scene Y points down: positive degrees are clockwise in the Side View.
inline QTransform formerTransform(const QRectF& r,double degrees) {
  QTransform t;t.translate(r.center().x(),r.center().y());t.rotate(degrees);
  t.translate(-r.center().x(),-r.center().y());return t;
}
inline QPolygonF formerPolygon(const QRectF& r,double degrees) {
  return formerTransform(r,degrees).map(QPolygonF{r});
}
// Separating axes of both rectangles reject positive-area overlap, allowing
// touching edges. Unlike bounding boxes this also permits nearby tilted masks.
inline bool formerMasksOverlap(const QRectF& a,double angleA,const QRectF& b,double angleB=0) {
  const auto p=formerPolygon(a,angleA),q=formerPolygon(b,angleB);
  for(const auto* polygon:{&p,&q})for(int i=0;i<2;++i) {
    const auto edge=(*polygon)[i+1]-(*polygon)[i];
    const double length=std::hypot(edge.x(),edge.y());if(length<=0)return false;
    const QPointF axis{-edge.y()/length,edge.x()/length};
    auto interval=[&](const QPolygonF& vertices) {
      double low=std::numeric_limits<double>::infinity(),high=-low;
      for(const auto& v:vertices){const double d=QPointF::dotProduct(v,axis);low=std::min(low,d);high=std::max(high,d);}
      return std::pair{low,high};
    };
    const auto [p0,p1]=interval(p);const auto [q0,q1]=interval(q);
    if(std::min(p1,q1)-std::max(p0,q0)<=1e-7)return false;
  }
  return true;
}
}

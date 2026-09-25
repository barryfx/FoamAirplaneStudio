#pragma once
#include <QRectF>
#include <QString>
#include <vector>
namespace designrc::geometry {
inline QString formerRailLocation(const std::vector<QRectF>& rectangles,std::size_t index,double angle) {
  const auto& r=rectangles.at(index);std::size_t number=1;
  for(std::size_t i=0;i<rectangles.size();++i)
    if(rectangles[i].center().x()<r.center().x()||(rectangles[i].center().x()==r.center().x()&&i<index))++number;
  return QString{"Former %1 (counted nose to tail), center %2 mm / %3 in from the nose, thickness %4 mm, rotation %5 deg"}
    .arg(static_cast<qulonglong>(number)).arg(r.center().x(),0,'f',2).arg(r.center().x()/25.4,0,'f',3)
    .arg(r.width(),0,'f',2).arg(angle,0,'f',3);
}
inline std::string formerRailClearanceError(const std::vector<QRectF>& rectangles,std::size_t index,
                                           double angle,bool ahead,bool right,double volume) {
  return (QString{"Fuselage: %1, %2 rail, %3 side.\n"
    "The CAD cut could not clear the rail's hollow center. Generation stopped to avoid leaving a foam plug.\n"
    "Check this former in Fuselage > Formers. If retrying fails, report the project and this message."}
    .arg(formerRailLocation(rectangles,index,angle).section(',',0,0))
    .arg(ahead?"front":"rear").arg(right?"right":"left")).toStdString();
}
}
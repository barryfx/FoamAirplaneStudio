#include "gui/AirplaneStatistics.h"
#include "gui/ProjectDocument.h"

#include "gui/SketchBoundary.h"
#include "gui/WingCalibration.h"
#include <QStringList>
#include <algorithm>
#include <cmath>
namespace designrc::gui {
namespace {
std::optional<double> area(SketchLayer layer,QPointF span,bool internal=false) {
  if(layer.points.empty()||layer.curves.empty())return {};
  std::vector<int> degree(layer.points.size());
  for(const auto& curve:layer.curves) {
    if(curve.points.size()<2)return {};
    for(std::size_t i=1;i<curve.points.size();++i) {
      if(curve.points[i]>=degree.size()||curve.points[i-1]>=degree.size())return {};
      ++degree[curve.points[i]];++degree[curve.points[i-1]];
    }
  }
  std::vector<std::size_t> ends;
  for(std::size_t i=0;i<degree.size();++i)if(degree[i]==1)ends.push_back(i);else if(degree[i]!=2)return {};
  if(ends.size()!=2 && !(internal&&ends.size()==4))return {};
  const auto dot=[&](QPointF p){return p.x()*span.x()+p.y()*span.y();};
  std::sort(ends.begin(),ends.end(),[&](auto a,auto b){return dot(layer.points[a])<dot(layer.points[b]);});
  // Root and (for internal panels) tip chords close the drawn LE/TE chains.
  for(std::size_t i=0;i<ends.size();i+=2)layer.curves.push_back({SketchTool::Line,{ends[i],ends[i+1]}});
  const auto boundary=closedSketchBoundary(layer);if(!boundary)return {};
  double sum=0;for(std::size_t i=1;i<boundary->size();++i) {
    const auto a=(*boundary)[i-1]-boundary->front(),b=(*boundary)[i]-boundary->front();sum+=a.x()*b.y()-a.y()*b.x();
  }
  return std::abs(sum)*.5;
}
}
AirplaneStatistics outlineStatistics(const ProjectDocument& p) {
  AirplaneStatistics result;
  if(p.reference.toScale) {
    if(p.reference.image.empty()||!p.reference.image.physicalSizeMm)return result;
  } else if(!p.reference.wingspanMm||*p.reference.wingspanMm<=0)return result;
  if(!p.reference.toScale)result.wingspanMm=p.reference.wingspanMm;
  std::optional<double> scale;if(p.reference.toScale)scale=1.;
  try {
    auto stations=p.stations.lines;
    // A complete single-panel outline already identifies its two root ends;
    // area and span need not wait for an airfoil station or assigned airfoil.
    if(stations.empty()&&!p.wing.layers.empty()) {
      const auto& root=p.wing.layers.front();std::vector<int> degree(root.points.size());
      for(const auto& c:root.curves)if(c.points.size()>=2){++degree.at(c.points.front());++degree.at(c.points.back());}
      std::vector<QPointF> ends;for(std::size_t i=0;i<degree.size();++i)if(degree[i]==1)ends.push_back(root.points[i]);
      if(ends.size()==2){ConstrainedLine line;line.first.position=ends[0];line.second.position=ends[1];stations.push_back(line);}
    }
    const auto calibration=wingCalibration(p.wing.layers,stations,p.reference.toScale?std::nullopt:p.reference.wingspanMm);
    scale=calibration.scale;result.rootChordMm=calibration.rootChordMm;result.wingspanMm=calibration.wingspanMm;
    double sum=0;bool complete=true;
    for(std::size_t i=0;i<p.wing.layers.size();++i) {
      const auto value=area(p.wing.layers[i],calibration.spanDirection,i+1<p.wing.layers.size());
      if(!value){complete=false;break;}sum+=*value;
    }
    if(complete&&sum>0) {
      result.wingAreaMm2=2*sum*(*scale)*(*scale);
      result.aspectRatio=(*result.wingspanMm)*(*result.wingspanMm) / *result.wingAreaMm2;
    }
  } catch(const std::exception&) {} // An unfinished outline is normal while editing.
  if(!scale)return result;
  if(p.fuselage.layers.size()>1)if(const auto side=closedSketchBoundary(p.fuselage.layers[1])) {
    const auto [lo,hi]=std::minmax_element(side->begin(),side->end(),[](auto a,auto b){return a.x()<b.x();});
    if(hi->x()>lo->x())result.fuselageLengthMm=(hi->x()-lo->x())*(*scale);
  }
  for(int i=0;i<2;++i)if(!p.stabilizerOutlines[i].layers.empty()) {
    // A stabilizer has a single open root, so its two endpoints need no sorting.
    if(const auto value=area(p.stabilizerOutlines[i].layers.front(),{}))
      (i==0?result.horizontalAreaMm2:result.verticalAreaMm2)=(*value)*(*scale)*(*scale)*(i==0?2:1);
  }
  return result;
}
QString statisticsText(const AirplaneStatistics& s,ProjectUnits units) {
  const bool inches=units==ProjectUnits::Inches;const double unit=inches?25.4:1.;
  const QString suffix=inches?" in":" mm",areaSuffix=inches?" in²":" mm²";
  QStringList rows;
  const auto row=[&](const char* label,std::optional<double> value,double divisor,QString ending,int decimals=2,bool sign=false) {
    if(!value) {
      const QLatin1StringView name{label};
      if(name==u"Weight"||name==u"Wing Loading"||name==u"CG from wing LE")
        rows<<QString{"<tr><td>%1</td><td align=right>—</td></tr>"}.arg(label);
      return;
    }
    rows<<QString{"<tr><td>%1</td><td align=right>%2%3%4</td></tr>"}.arg(label).arg(sign&&*value>=0?"+":"").arg(*value/divisor,0,'f',decimals).arg(ending);
  };
  row("Wingspan",s.wingspanMm,unit,suffix);row("Wing Area",s.wingAreaMm2,unit*unit,areaSuffix);
  row("Root Chord",s.rootChordMm,unit,suffix);row("Aspect Ratio",s.aspectRatio,1,"");
  row("Fuselage Length",s.fuselageLengthMm,unit,suffix);
  row("Horiz Stab Area",s.horizontalAreaMm2,unit*unit,areaSuffix);row("Vert Stab Area",s.verticalAreaMm2,unit*unit,areaSuffix);
  row("Weight",s.weightGrams,inches?gramsPerOunce:1,inches?" oz":" g");
  row("Wing Loading",s.wingLoadingGramsPerDm2,inches?gramsPerOunce*10000/(304.8*304.8):1,inches?" oz/ft²":" g/dm²");
  row("CG from wing LE",s.cgFromLeadingEdgeMm,unit,suffix,3,true);
  if(rows.empty())return {};
  return "<b>Airplane statistics</b><table width='100%' cellspacing='0' cellpadding='0'>"+rows.join("")+"</table>";
}
}

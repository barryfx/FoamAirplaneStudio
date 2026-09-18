#include "gui/ProjectDocument.h"
#include "gui/LengthEntry.h"
#include <QBuffer>
#include <QFile>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QImageReader>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <stdexcept>
namespace designrc::gui {
namespace {
[[noreturn]] void bad(const char* field) {throw std::runtime_error(std::string{"Invalid project field: "}+field);}
QJsonArray array(const QJsonValue& value,const char* name,int maximum=1000000) {
  if(!value.isArray()||value.toArray().size()>maximum) bad(name); return value.toArray();
}
QJsonObject object(const QJsonValue& value,const char* name) {if(!value.isObject()) bad(name);return value.toObject();}
double number(const QJsonValue& value,const char* name,double min=-1e12,double max=1e12) {
  if(!value.isDouble()) bad(name); const double n=value.toDouble();
  if(!std::isfinite(n)||n<min||n>max) bad(name); return n;
}
int integer(const QJsonValue& value,const char* name,int min,int max) {
  const auto n=number(value,name,min,max); if(std::floor(n)!=n) bad(name);return static_cast<int>(n);
}
bool boolean(const QJsonValue& value,const char* name) {if(!value.isBool()) bad(name);return value.toBool();}
QString string(const QJsonValue& value,const char* name,int maximum=4096) {
  if(!value.isString()||value.toString().size()>maximum) bad(name);return value.toString();
}
QJsonArray point(QPointF p) {return {p.x(),p.y()};}
QPointF point(const QJsonValue& value) {
  auto a=array(value,"point",2);if(a.size()!=2) bad("point");return {number(a[0],"point.x"),number(a[1],"point.y")};
}
QJsonArray points(const std::vector<QPointF>& values) {QJsonArray a;for(auto p:values)a.append(point(p));return a;}
std::vector<QPointF> points(const QJsonValue& value) {std::vector<QPointF> out;for(auto p:array(value,"points"))out.push_back(point(p));return out;}
QJsonValue size(const std::optional<QSizeF>& value) {return value?QJsonValue{QJsonArray{value->width(),value->height()}}:QJsonValue{};}
std::optional<QSizeF> size(const QJsonValue& value) {
  if(value.isNull()) return {}; auto p=point(value);if(p.x()<=0||p.y()<=0)bad("physical size");return QSizeF{p.x(),p.y()};
}
QJsonValue length(const std::optional<double>& value) {return value?QJsonValue{*value}:QJsonValue{};}
std::optional<double> length(const QJsonValue& value) {if(value.isNull())return {};return number(value,"length",1e-12,1e12);}
QJsonObject layer(const SketchLayer& value) {
  QJsonArray curves;
  for(const auto& curve:value.curves) {
    QJsonArray ids;for(auto id:curve.points)ids.append(static_cast<qint64>(id));
    curves.append(QJsonObject{{"type",static_cast<int>(curve.type)},{"points",ids}});
  }
  QJsonObject out{{"points",points(value.points)},{"curves",curves}};
  if(value.leadingEdge)out["leadingEdge"]=static_cast<qint64>(*value.leadingEdge);
  return out;
}
SketchLayer layer(const QJsonValue& value) {
  auto o=object(value,"layer");SketchLayer out;out.points=points(o["points"]);
  for(auto v:array(o["curves"],"curves",100000)) {
    auto c=object(v,"curve");SketchCurve curve{static_cast<SketchTool>(integer(c["type"],"curve type",1,2)),{}};
    for(auto id:array(c["points"],"curve points"))curve.points.push_back(integer(id,"point index",0,static_cast<int>(out.points.size())-1));
    if(curve.points.size()<2||(curve.type==SketchTool::Line&&curve.points.size()!=2))bad("curve point count");
    out.curves.push_back(std::move(curve));
  }
  if(o.contains("leadingEdge")) {
    out.leadingEdge=integer(o["leadingEdge"],"leading edge endpoint",0,static_cast<int>(out.points.size())-1);
    if(!isSketchEndpoint(out,*out.leadingEdge))bad("leading edge must be an open endpoint");
  }
  return out;
}
QJsonObject sketch(const SketchState& value) {
  QJsonArray layers;for(const auto& l:value.layers)layers.append(layer(l));
  return {{"layers",layers},{"pending",points(value.pending)},{"tool",static_cast<int>(value.tool)},
    {"active",value.active},{"selected",value.selected},{"editing",value.editing}};
}
SketchState sketch(const QJsonValue& value,int maximumLayers) {
  auto o=object(value,"sketch");SketchState out;out.layers.clear();
  for(auto l:array(o["layers"],"layers",maximumLayers))out.layers.push_back(layer(l));
  if(out.layers.empty())bad("empty sketch layers");out.pending=points(o["pending"]);
  out.tool=static_cast<SketchTool>(integer(o["tool"],"sketch tool",0,2));
  out.active=integer(o["active"],"active layer",0,static_cast<int>(out.layers.size())-1);
  out.selected=integer(o["selected"],"selected curve",-1,static_cast<int>(out.layers[out.active].curves.size())-1);
  out.editing=boolean(o["editing"],"sketch editing");return out;
}
QJsonObject anchor(const CurveAnchor& a) {return {{"layer",a.layer},{"curve",a.curve},{"parameter",a.parameter},{"position",point(a.position)}};}
CurveAnchor anchor(const QJsonValue& value,const SketchState& wing) {
  auto o=object(value,"anchor");CurveAnchor out;
  out.layer=integer(o["layer"],"anchor layer",0,static_cast<int>(wing.layers.size())-1);
  out.curve=integer(o["curve"],"anchor curve",0,static_cast<int>(wing.layers[out.layer].curves.size())-1);
  out.parameter=number(o["parameter"],"anchor parameter",0,1);out.position=point(o["position"]);return out;
}
QJsonArray xyz(const std::array<double,3>& p) {return {p[0],p[1],p[2]};}
std::array<double,3> xyz(const QJsonValue& value) {
  auto a=array(value,"camera vector",3);if(a.size()!=3)bad("camera vector");
  return {number(a[0],"camera vector"),number(a[1],"camera vector"),number(a[2],"camera vector")};
}
}
QJsonObject encodeProject(const ProjectDocument& p,bool embedImages) {
  QJsonArray pages;
  for(const auto& page:p.reference.image.pages) {
    QJsonObject o{{"physicalMm",size(page.physicalSizeMm)}};
    if(embedImages) {
      QByteArray bytes;QBuffer buffer{&bytes};buffer.open(QIODevice::WriteOnly);
      if(!page.pixels.save(&buffer,"PNG"))throw std::runtime_error("Could not encode a reference page.");
      o["png"]=QString::fromLatin1(bytes.toBase64());
    } else o["imageKey"]=QString::number(page.pixels.cacheKey());
    pages.append(o);
  }
  QJsonObject reference{{"filename",p.reference.image.path},{"pages",pages},
    {"physicalMm",size(p.reference.image.physicalSizeMm)},{"nativeUnits",static_cast<int>(p.reference.image.nativeUnits)},
    {"toScale",p.reference.toScale},{"units",static_cast<int>(p.reference.units)},
    {"wingspanMm",length(p.reference.wingspanMm)},{"fuselageLengthMm",length(p.reference.fuselageLengthMm)},
    {"wingspanText",p.wingspanText},{"fuselageText",p.fuselageText}};
  QJsonArray entries;
  for(const auto& entry:p.airfoils.entries) {
    QJsonObject o{{"name",entry.name},{"boundary",points(entry.boundary)}};
    if(entry.imported) {
      QJsonArray coords;for(auto pt:entry.imported->outline())coords.append(QJsonArray{pt.x,pt.y});o["imported"]=coords;
    } else if(entry.sketch) o["sketch"]=layer(*entry.sketch);
    entries.append(o);
  }
  QJsonArray stabilizerAirfoils;
  for (const auto& foil : p.stabilizerAirfoils) {
    if (!foil) {stabilizerAirfoils.append(QJsonValue{});continue;}
    QJsonArray coordinates;
    for (auto pt : foil->outline()) coordinates.append(QJsonArray{pt.x,pt.y});
    stabilizerAirfoils.append(QJsonObject{{"name",QString::fromStdString(foil->name())},{"coordinates",coordinates}});
  }
  QJsonArray choices;for(std::size_t i=0;i<p.wing.layers.size();++i)choices.append(i<p.airfoils.panelChoices.size()?p.airfoils.panelChoices[i]:p.airfoils.chosen);
  QJsonObject airfoils{{"panel",p.airfoils.panel},{"panelChoices",choices},{"entries",entries},{"chosen",p.airfoils.chosen},{"draft",p.airfoils.draft},
    {"sketching",p.airfoils.sketching},{"draftName",p.airfoils.draftName}};
  QJsonArray lines;
  for(const auto& line:p.stations.lines) lines.append(QJsonObject{{"leading",anchor(line.first)},
    {"trailing",anchor(line.second)},{"alignment",static_cast<int>(line.alignment)},
    {"airfoil",line.airfoil?QJsonValue{static_cast<qint64>(*line.airfoil)}:QJsonValue{}}});
  QJsonObject stations{{"lines",lines},{"selected",p.stations.selected},
    {"first",p.stations.first?QJsonValue{anchor(*p.stations.first)}:QJsonValue{}}};
  QJsonArray profileLines;
  for(const auto& line:p.fuselageStations.lines)
    profileLines.append(QJsonObject{{"top",anchor(line.first)},{"bottom",anchor(line.second)},
      {"thicknessMm",line.thicknessMm?QJsonValue{*line.thicknessMm}:QJsonValue{}},
      {"profile",line.profile?QJsonValue{static_cast<qint64>(*line.profile)}:QJsonValue{}}});
  QJsonObject fuselageStations{{"lines",profileLines},{"selected",p.fuselageStations.selected}};
  QJsonValue camera;
  if(p.camera) {const auto& c=*p.camera;camera=QJsonObject{{"eye",xyz(c.eye)},{"center",xyz(c.center)},
    {"up",xyz(c.up)},{"scale",c.scale},{"fov",c.fov},{"projection",c.projection}};}
  QJsonArray controls;
  for(const auto& panel:p.controls.panels) {
  QJsonArray panelEntries;for(const auto& control:panel) {
    QJsonValue rectangle;
    if(control.rectangle) {const auto& r=*control.rectangle;rectangle=QJsonArray{r.x(),r.y(),r.width(),r.height()};}
    panelEntries.append(QJsonObject{{"enabled",control.enabled},{"hinge",static_cast<int>(control.hinge)},{"rectangle",rectangle}});
  }
    controls.append(panelEntries);
  }
  QJsonObject controlState{{"panels",controls},{"panel",p.controls.panel},{"drawing",p.controls.drawing},
    {"first",p.controls.first?QJsonValue{point(*p.controls.first)}:QJsonValue{}}};
  QJsonArray spars;
  for(const auto& panel:p.spars) {
    QJsonArray entries;
    for(const auto& s:panel)entries.append(QJsonObject{{"enabled",s.enabled},{"shape",static_cast<int>(s.shape)},
      {"chordPercent",s.chordPercent},{"lengthPercent",s.lengthPercent},{"sizeMm",s.sizeMm},{"heightMm",s.heightMm},{"sizeText",QString::fromStdString(s.sizeText)},{"heightText",QString::fromStdString(s.heightText)}});
    spars.append(entries);
  }
  QJsonArray lightText;for(const auto& t:p.lightening.text)lightText.append(QString::fromStdString(t));
  const auto& l=p.lightening;
  QJsonObject lightening{{"enabled",l.enabled},{"wallMm",l.wallMm},{"ribMm",l.ribMm},{"startMm",l.startMm},{"stopMm",l.stopMm},{"crossmembers",l.crossmembers},{"text",lightText}};
  QJsonArray dihedral;for(double angle:p.dihedralDegrees)dihedral.append(angle);
  QJsonArray split;for(auto s:p.splitterSizes)split.append(s);
  QJsonValue trayRect;
  if(p.servoTray.rectangle){const auto& r=*p.servoTray.rectangle;trayRect=QJsonArray{r.x(),r.y(),r.width(),r.height()};}
  QJsonObject tray{{"rectangle",trayRect},{"first",p.servoTray.first?QJsonValue{point(*p.servoTray.first)}:QJsonValue{}},{"drawing",p.servoTray.drawing}};
  QJsonArray formerRects;for(const auto& r:p.formers.rectangles)formerRects.append(QJsonArray{r.x(),r.y(),r.width(),r.height()});
  QJsonObject formers{{"rectangles",formerRects},{"thicknessMm",p.formers.thicknessMm}};
  return {{"format","FoamAirplaneStudio"},{"version",20},{"spars",spars},{"controlSurfaces",controlState},{"reference",reference},{"wingOutline",sketch(p.wing)},
    {"stabilizerAirfoils",stabilizerAirfoils},
    {"horizontalStabilizerCuts",sketch(p.stabilizerCuts[0])},{"verticalStabilizerCuts",sketch(p.stabilizerCuts[1])},
    {"horizontalStabilizerHinge",sketch(p.stabilizerHinges[0])},{"verticalStabilizerHinge",sketch(p.stabilizerHinges[1])},
    {"stabilizerHingeCuts",QJsonArray{static_cast<int>(p.stabilizerHingeCuts[0]),static_cast<int>(p.stabilizerHingeCuts[1])}},
    {"horizontalStabilizerOutline",sketch(p.stabilizerOutlines[0])},{"verticalStabilizerOutline",sketch(p.stabilizerOutlines[1])},
    {"formers",formers},{"servoTray",tray},{"fuselageCuts",sketch(p.fuselageCuts)},{"fuselageThickening",p.fuselageThickening},{"fuselageProfiles",sketch(p.fuselageProfiles)},{"fuselageStations",fuselageStations},{"fuselageOutline",sketch(p.fuselage)},{"airfoilSketches",sketch(p.airfoilSketch)},{"stations",stations},{"airfoils",airfoils},
    {"lightening",lightening},{"dihedralDegrees",dihedral},{"ui",QJsonObject{{"fuselageView",p.fuselageView},{"workspace",p.workspace},{"tool",p.tool},{"viewport",p.viewport},{"dihedralPanel",p.selectedDihedralPanel},{"sparPanel",p.selectedSparPanel},{"stationPanel",p.selectedStationPanel},
      {"plan",QJsonObject{{"zoom",p.plan.zoom},{"center",point(p.plan.center)}}},{"camera",camera},{"splitter",split}}}};
}
ProjectDocument decodeProject(const QJsonObject& json) {
  if(json["format"]!="FoamAirplaneStudio")bad("format (expected FoamAirplaneStudio)");
  const int version=integer(json["version"],"version",1,100000);
  if(version>20)throw std::runtime_error("This project version is not supported by this application.");
  ProjectDocument p;
  auto r=object(json["reference"],"reference");
  p.reference.image.path=string(r["filename"],"reference filename");
  p.reference.image.physicalSizeMm=size(r["physicalMm"]);
  p.reference.image.nativeUnits=static_cast<ProjectUnits>(integer(r["nativeUnits"],"native units",0,1));
  qint64 pixelCount=0;
  for(auto value:array(r["pages"],"reference pages",1000)) {
    auto o=object(value,"reference page");
    const auto encoded=string(o["png"],"reference PNG",512*1024*1024).toLatin1();
    const auto decoded=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
    if(!decoded)bad("reference PNG encoding");
    auto bytes=decoded.decoded;QBuffer buffer{&bytes};buffer.open(QIODevice::ReadOnly);
    QImageReader reader{&buffer,"PNG"};const auto dimensions=reader.size();
    if(!dimensions.isValid())bad("reference PNG dimensions");
    pixelCount+=static_cast<qint64>(dimensions.width())*dimensions.height();
    if(pixelCount>200000000)bad("reference image pixel limit");
    QImage image=reader.read();if(image.isNull())bad("reference PNG data");
    p.reference.image.pages.push_back({image,size(o["physicalMm"])});
  }
  p.reference.toScale=boolean(r["toScale"],"reference scale mode");
  p.reference.units=static_cast<ProjectUnits>(integer(r["units"],"project units",0,1));
  p.reference.wingspanMm=length(r["wingspanMm"]);p.reference.fuselageLengthMm=length(r["fuselageLengthMm"]);
  p.wingspanText=string(r["wingspanText"],"wingspan text");p.fuselageText=string(r["fuselageText"],"fuselage text");
  if(p.reference.toScale) {
    if(p.reference.image.empty()||!p.reference.image.physicalSizeMm)bad("physical reference scale");
    for(const auto& page:p.reference.image.pages)if(!page.physicalSizeMm)bad("page physical scale");
    if(p.reference.units!=p.reference.image.nativeUnits)bad("actual-scale units");
  }
  p.wing=sketch(json["wingOutline"],100);p.airfoilSketch=sketch(json["airfoilSketches"],10001);
  p.controls.panels.resize(p.wing.layers.size());
  if(version>=2) {
    const auto controls=object(json["controlSurfaces"],"control surfaces");
    auto readControls=[&](const QJsonValue& value,std::array<ControlSurface,2>& target) {
    const auto surfaces=array(value,"control surfaces",2);
    if(surfaces.size()!=2)bad("control surface count");
    for(int i=0;i<2;++i) {
      const auto o=object(surfaces[i],"control surface");auto& surface=target[i];
      surface.enabled=boolean(o["enabled"],"control surface enabled");
      surface.hinge=static_cast<HingeCut>(integer(o["hinge"],"hinge",0,1));
      if(!o["rectangle"].isNull()) {
        const auto r=array(o["rectangle"],"control rectangle",4);if(r.size()!=4)bad("control rectangle");
        surface.rectangle=QRectF{number(r[0],"rectangle x"),number(r[1],"rectangle y"),
          number(r[2],"rectangle width",1e-6,1e12),number(r[3],"rectangle height",1e-6,1e12)};
      }
    }
    };
    if(version<5)readControls(controls["surfaces"],p.controls.panels[0]);
    else {
      const auto panels=array(controls["panels"],"control panels",100);
      if(panels.size()!=static_cast<int>(p.wing.layers.size()))bad("control panel count");
      for(int i=0;i<panels.size();++i)readControls(panels[i],p.controls.panels[i]);
      p.controls.panel=integer(controls["panel"],"selected control panel",0,panels.size()-1);
    }
    p.controls.drawing=integer(controls["drawing"],"drawing control surface",-1,1);
    if(!controls["first"].isNull())p.controls.first=point(controls["first"]);
  if(p.controls.drawing>=0 && !p.controls.panels[p.controls.panel][p.controls.drawing].enabled)bad("disabled control surface drawing");
    if(p.controls.first && p.controls.drawing<0)bad("control rectangle first point");
  }
  p.spars.resize(p.wing.layers.size());
  if(version>=3) {
    auto readSpars=[&](const QJsonValue& value,SparState& target) {
      const auto entries=array(value,"panel spars",3);if(entries.size()!=3)bad("spar count");
      for(int i=0;i<3;++i) {
        const auto o=object(entries[i],"spar");auto& s=target[i];
        s.enabled=boolean(o["enabled"],"spar enabled");s.shape=static_cast<SparShape>(integer(o["shape"],"spar shape",0,i==2?0:1));
        s.chordPercent=number(o["chordPercent"],"spar chord",0,100);s.lengthPercent=number(o["lengthPercent"],"spar length",0.01,100);
        s.sizeMm=number(o["sizeMm"],"spar size",0.0001,10000);s.heightMm=number(o["heightMm"],"spar height",0.0001,10000);
        if(version>=6) {
          auto display=[&](const char* key,double mm) {
            const auto text=string(o[key],key,256);
            if(!text.isEmpty()) {
              const auto parsed=lengthInMm(text,p.reference.units);
              if(!parsed || std::abs(*parsed-mm)>1.e-8*std::max(1.0,mm))bad("spar display length mismatch");
            }
            return explicitLength(text,p.reference.units).toStdString();
          };
          s.sizeText=display("sizeText",s.sizeMm);s.heightText=display("heightText",s.heightMm);
        }
      }
    };
    if(version==3)readSpars(json["spars"],p.spars.front()); // Preserve former global settings on panel 1.
    else {
      const auto panels=array(json["spars"],"spar panels",100);
      if(panels.size()!=static_cast<int>(p.spars.size()))bad("spar panel count must match outline panels");
      for(int i=0;i<panels.size();++i)readSpars(panels[i],p.spars[i]);
    }
  }
  auto a=object(json["airfoils"],"airfoils");
  for(auto value:array(a["entries"],"airfoil entries",10000)) {
    auto o=object(value,"airfoil");LibraryAirfoil entry;entry.name=string(o["name"],"airfoil name");
    if(entry.name.trimmed().isEmpty())bad("airfoil name");entry.boundary=points(o["boundary"]);
    if(o.contains("imported")==o.contains("sketch"))bad("airfoil source");
    if(o.contains("imported")) {
      std::ostringstream dat;dat.precision(17);dat<<entry.name.toStdString()<<'\n';
      for(auto pt:points(o["imported"]))dat<<pt.x()<<' '<<pt.y()<<'\n';
      std::istringstream stream{dat.str()};entry.imported=domain::AirfoilProfile::fromDat(stream);
      const auto sampled=entry.imported->resampled(25);if(sampled.empty())bad("airfoil samples");
    } else {entry.sketch=layer(o["sketch"]);if(!closedAirfoilBoundary(*entry.sketch))bad("airfoil closed loop");}
    p.airfoils.entries.push_back(std::move(entry));
  }
  p.airfoils.chosen=integer(a["chosen"],"chosen airfoil",-1,static_cast<int>(p.airfoils.entries.size())-1);
  p.airfoils.panelChoices.assign(p.wing.layers.size(),p.airfoils.chosen);
  if(version>=5) {
    p.airfoils.panel=integer(a["panel"],"selected airfoil panel",0,p.wing.layers.size()-1);
    const auto choices=array(a["panelChoices"],"panel airfoil choices",100);
    if(choices.size()!=static_cast<int>(p.wing.layers.size()))bad("panel airfoil choice count");
    for(int i=0;i<choices.size();++i)p.airfoils.panelChoices[i]=integer(choices[i],"panel airfoil choice",-1,static_cast<int>(p.airfoils.entries.size())-1);
  }
  p.airfoils.sketching=boolean(a["sketching"],"airfoil sketch mode");
  p.airfoils.draft=integer(a["draft"],"airfoil draft",0,10000);
  p.airfoils.draftName=string(a["draftName"],"airfoil draft name");
  if(p.airfoils.sketching&&(p.airfoils.draft>=static_cast<int>(p.airfoilSketch.layers.size())||p.airfoils.draftName.trimmed().isEmpty()))bad("active airfoil draft");
  auto s=object(json["stations"],"stations");
  for(auto value:array(s["lines"],"station lines",100000)) {
    auto o=object(value,"station");ConstrainedLine line;
    line.first=anchor(o["leading"],p.wing);line.second=anchor(o["trailing"],p.wing);
    line.alignment=static_cast<LineAlignment>(integer(o["alignment"],"station alignment",0,2));
    if(!o["airfoil"].isNull())line.airfoil=integer(o["airfoil"],"station airfoil",0,static_cast<int>(p.airfoils.entries.size())-1);
    p.stations.lines.push_back(line);
  }
  p.stations.selected=integer(s["selected"],"selected station",-1,static_cast<int>(p.stations.lines.size())-1);
  if(!s["first"].isNull())p.stations.first=anchor(s["first"],p.wing);
  if(version>=8) {
    const auto o=object(json["lightening"],"lightening");auto& l=p.lightening;
    l.enabled=boolean(o["enabled"],"lightening enabled");
    l.wallMm=number(o["wallMm"],"lightening wall thickness",.0001,10000);
    l.ribMm=number(o["ribMm"],"lightening crossmember thickness",.0001,10000);
    l.startMm=number(o["startMm"],"lightening start distance",0,10000);
    l.stopMm=number(o["stopMm"],"lightening stop distance",0,10000);
    l.crossmembers=integer(o["crossmembers"],"lightening crossmember count",0,100);
    const auto texts=array(o["text"],"lightening dimension text",4);if(texts.size()!=4)bad("lightening dimension text count");
    const std::array<double,4> values{l.wallMm,l.ribMm,l.startMm,l.stopMm};
    for(int i=0;i<4;++i) {
      auto t=string(texts[i],"lightening dimension text",256);
      if(!t.isEmpty()) {const auto mm=lengthInMm(t,p.reference.units,i>=2);
        if(!mm || std::abs(*mm-values[i])>std::max(1.0,values[i])*1e-8)bad("lightening dimension text does not match physical size");
        t=explicitLength(t,p.reference.units,i>=2);}
      l.text[i]=t.toStdString();
    }
  }
  p.dihedralDegrees.assign(p.wing.layers.size(),0);
  if(version>=7) {
    const auto angles=array(json["dihedralDegrees"],"dihedral angles",100);
    if(angles.size()!=static_cast<int>(p.dihedralDegrees.size()))bad("dihedral panel count");
    for(int i=0;i<angles.size();++i)p.dihedralDegrees[i]=number(angles[i],"root dihedral",-80,80);
  } else if(json.contains("wingTip")) (void)integer(json["wingTip"],"legacy wing tip",0,2);
  auto ui=object(json["ui"],"UI");p.workspace=integer(ui["workspace"],"workspace",0,6);
  p.tool=string(ui["tool"],"active tool");p.viewport=integer(ui["viewport"],"viewport",0,1);
  if(version>=7)p.selectedDihedralPanel=integer(ui["dihedralPanel"],"selected dihedral panel",0,p.dihedralDegrees.size()-1);
  else if(p.workspace==1 && p.tool=="Wing Tip")p.tool="Dihedral";
  if(version>=4)p.selectedSparPanel=integer(ui["sparPanel"],"selected spar panel",0,static_cast<int>(p.spars.size())-1);
  if(version>=5)p.selectedStationPanel=integer(ui["stationPanel"],"selected station panel",0,p.wing.layers.size()-1);
  if(version<5) {
    p.selectedStationPanel=p.stations.first?p.stations.first->layer:p.stations.selected>=0?p.stations.lines[p.stations.selected].first.layer:0;
    p.airfoils.panel=p.stations.selected>=0?p.stations.lines[p.stations.selected].first.layer:0;
  } else if(p.stations.first && p.stations.first->layer!=p.selectedStationPanel)bad("station draft panel");
  auto plan=object(ui["plan"],"plan view");p.plan.zoom=number(plan["zoom"],"plan zoom",1e-5,1000);p.plan.center=point(plan["center"]);
  for(auto value:array(ui["splitter"],"splitter",2))p.splitterSizes.push_back(integer(value,"splitter size",0,100000));
  if(!ui["camera"].isNull()) {
    auto c=object(ui["camera"],"camera");CameraState camera;
    camera.eye=xyz(c["eye"]);camera.center=xyz(c["center"]);camera.up=xyz(c["up"]);
    camera.scale=number(c["scale"],"camera scale",1e-9,1e12);camera.fov=number(c["fov"],"camera field of view",1e-5,179.99);
    camera.projection=integer(c["projection"],"camera projection",0,4);
    gp_Vec direction{gp_Pnt{camera.eye[0],camera.eye[1],camera.eye[2]},gp_Pnt{camera.center[0],camera.center[1],camera.center[2]}};
    gp_Vec up{camera.up[0],camera.up[1],camera.up[2]};
    if(direction.SquareMagnitude()<1e-16||up.SquareMagnitude()<1e-16||direction.Crossed(up).SquareMagnitude()<1e-16)bad("camera orientation");
    p.camera=camera;
  }
  if (p.workspace == 3 || p.workspace == 4) {
    if (p.tool == "Airfoils") p.tool = "Airfoil";
    if (p.tool == "Airfoil Stations" || p.tool == "Edit") p.tool = "Outline";
  }
  if(version>=20)for(int i=0;i<2;++i) {
    auto& cuts=p.stabilizerCuts[i];cuts=sketch(json[i==0?"horizontalStabilizerCuts":"verticalStabilizerCuts"],1000);
    for(const auto& layer:cuts.layers)if(layer.leadingEdge)bad("cut shape leading edge");
    if(cuts.editing && (p.workspace!=i+3 || p.tool!="Cut" || p.viewport!=0))bad("stabilizer cut workspace");
    if(!cuts.pending.empty() && (!cuts.editing || cuts.tool==SketchTool::None))bad("stabilizer cut draft");
  }
  if(version>=19) {
    const auto cuts=array(json["stabilizerHingeCuts"],"stabilizer hinge cuts",2);
    if(cuts.size()!=2)bad("stabilizer hinge cut count");
    for(int i=0;i<2;++i) {
      p.stabilizerHingeCuts[i]=static_cast<HingeCut>(integer(cuts[i],"stabilizer hinge cut",0,1));
      auto& h=p.stabilizerHinges[i];h=sketch(json[i==0?"horizontalStabilizerHinge":"verticalStabilizerHinge"],1);
      if(h.tool==SketchTool::Spline || h.layers[0].leadingEdge)bad("stabilizer hinge tool");
      for(const auto& curve:h.layers[0].curves)if(curve.type!=SketchTool::Line)bad("stabilizer hinge segment");
      if(h.editing && (p.workspace!=i+3 || p.tool!="Hinge Line" || p.viewport!=0))bad("stabilizer hinge workspace");
      if(!h.pending.empty() && (!h.editing || h.tool!=SketchTool::Line || h.pending.size()!=1))bad("stabilizer hinge draft");
    }
  }
  if (version >= 17) {
    const auto entries = array(json["stabilizerAirfoils"], "stabilizer airfoils", 2);
    if (entries.size() != 2) bad("stabilizer airfoil count");
    for (int i=0;i<2;++i) if (!entries[i].isNull()) {
      const auto entry=object(entries[i],"stabilizer airfoil");
      const auto name=string(entry["name"],"stabilizer airfoil name");
      if(name.trimmed().isEmpty() || name.contains('\n') || name.contains('\r'))bad("stabilizer airfoil name");
      std::ostringstream dat;dat.precision(17);dat<<name.toStdString()<<'\n';
      for(auto pt:points(entry["coordinates"]))dat<<pt.x()<<' '<<pt.y()<<'\n';
      std::istringstream stream{dat.str()};auto foil=domain::AirfoilProfile::fromDat(stream);
      const auto sampled=foil.resampled(41);double low=0,high=0;
      for(auto pt:sampled){if(!std::isfinite(pt.x)||!std::isfinite(pt.y))bad("stabilizer airfoil coordinates");low=std::min(low,pt.y);high=std::max(high,pt.y);}
      if(high-low<1e-9)bad("stabilizer airfoil thickness");
      p.stabilizerAirfoils[i]=std::move(foil);
    }
  }
  if (version >= 16) {
    const std::array<const char*, 2> keys{"horizontalStabilizerOutline", "verticalStabilizerOutline"};
    for (int i = 0; i < 2; ++i) {
      auto& outline = p.stabilizerOutlines[i];
      outline = sketch(json[keys[i]], 1);
      if(version<18)outline.layers[0].leadingEdge.reset();
      if (outline.editing && (p.workspace != i + 3 || p.tool != "Outline" || p.viewport != 0))
        bad("stabilizer outline editing workspace");
      if (!outline.pending.empty() && (!outline.editing || outline.tool == SketchTool::None))
        bad("stabilizer outline draft tool");
    }
  }
  if(version>=9) {
    p.fuselage=sketch(json["fuselageOutline"],2);
    if(p.fuselage.layers.size()!=2)bad("fuselage outline view count");
    p.fuselageView=integer(ui["fuselageView"],"fuselage view",-1,1);
    const bool hasTool=p.fuselage.tool!=SketchTool::None;
    if(p.fuselageView>=0 && p.fuselage.active!=p.fuselageView)bad("fuselage active view");
    if(!p.fuselage.pending.empty() && (!hasTool || !p.fuselage.editing))bad("fuselage draft tool");
    if(p.fuselage.editing && (p.fuselageView<0 || p.workspace!=2 || p.tool!="Outline" || p.viewport!=0))bad("fuselage editing workspace");
  }
  if(p.airfoils.sketching&&(p.workspace!=1||p.tool!="Airfoils"))bad("airfoil draft workspace");
  if(version>=14) {
    const auto tray=object(json["servoTray"],"servo tray");
    if(!tray["rectangle"].isNull()) {
      const auto r=array(tray["rectangle"],"servo tray rectangle",4);if(r.size()!=4)bad("servo tray rectangle");
      p.servoTray.rectangle=QRectF{number(r[0],"tray x"),number(r[1],"tray y"),number(r[2],"tray length",1e-6,1e12),number(r[3],"tray height",0,1e12)};
    }
    if(p.servoTray.rectangle&&p.servoTray.rectangle->height()<=0)bad("zero tray thickness");
    p.servoTray.drawing=boolean(tray["drawing"],"tray drawing");
    if(!tray["first"].isNull())p.servoTray.first=point(tray["first"]);
    if(p.servoTray.first&&(!p.servoTray.drawing||p.workspace!=2||p.tool!="Servo Tray"||p.viewport!=0))bad("servo tray draft workspace");
  }
  if(version>=15) {
    const auto formers=object(json["formers"],"formers");
    p.formers.thicknessMm=number(formers["thicknessMm"],"former thickness",0,10000);
    if(p.formers.thicknessMm<=0)bad("zero former thickness");
    for(auto value:array(formers["rectangles"],"former rectangles",1000)) {
      const auto r=array(value,"former rectangle",4);if(r.size()!=4)bad("former rectangle");
      const QRectF rect{number(r[0],"former x"),number(r[1],"former y"),number(r[2],"former width",0,1e12),number(r[3],"former height",1e-6,1e12)};
      if(rect.width()<=0)bad("zero former thickness");
      for(const auto& other:p.formers.rectangles)if(rectanglesOverlap(rect,other))bad("overlapping formers");
      if(p.servoTray.rectangle&&rectanglesOverlap(rect,*p.servoTray.rectangle))bad("former overlapping servo tray");
      p.formers.rectangles.push_back(rect);
    }
  }
  if(p.workspace==2&&p.tool=="Firewall")p.tool="Formers";
  if(version>=13) {
    p.fuselageCuts=sketch(json["fuselageCuts"],2);
    const auto& cuts=p.fuselageCuts;
    if(cuts.layers.size()!=2)bad("fuselage cut view count");
    if(cuts.editing&&(p.workspace!=2||p.tool!="Cut"||p.viewport!=0))bad("fuselage cut editing workspace");
    if(!cuts.pending.empty()&&(!cuts.editing||cuts.tool==SketchTool::None))bad("fuselage cut draft tool");
  }
  if(version>=12)p.fuselageThickening=boolean(json["fuselageThickening"],"fuselage thickening");
  if(version>=11)p.fuselageProfiles=sketch(json["fuselageProfiles"],100001);
  if(version>=10) {
    auto stations=object(json["fuselageStations"],"fuselage stations");
    for(auto value:array(stations["lines"],"fuselage station lines",100000)) {
      auto o=object(value,"fuselage station");
      ConstrainedLine line{anchor(o["top"],p.fuselage),anchor(o["bottom"],p.fuselage),LineAlignment::Vertical};
      if(line.first.layer!=1 || line.second.layer!=1)bad("fuselage station Side View anchors");
      if(std::abs(line.first.position.x()-line.second.position.x())>1e-7 ||
          line.second.position.y()-line.first.position.y()<1e-7)bad("fuselage vertical station");
      if(std::any_of(p.fuselageStations.lines.begin(),p.fuselageStations.lines.end(),[&](const auto& existing){
          return std::abs(existing.first.position.x()-line.first.position.x())<1e-6;}))bad("duplicate fuselage station");
      if(version>=11&&!o["profile"].isNull())line.profile=integer(o["profile"],"fuselage profile slot",0,static_cast<int>(p.fuselageProfiles.layers.size())-1);
      if(line.profile && std::any_of(p.fuselageStations.lines.begin(),p.fuselageStations.lines.end(),[&](const auto& other){return other.profile==line.profile;}))bad("shared fuselage profile slot");
      if(version>=12&&!o["thicknessMm"].isNull())line.thicknessMm=number(o["thicknessMm"],"fuselage thickness",.001,10000);
      if(p.fuselageThickening&&!line.thicknessMm)bad("missing fuselage thickness");
      p.fuselageStations.lines.push_back(line);
    }
    p.fuselageStations.selected=integer(stations["selected"],"selected fuselage station",-1,static_cast<int>(p.fuselageStations.lines.size())-1);
  }
  if(version>=11) {
    const auto& profiles=p.fuselageProfiles;
    if(profiles.editing) {
      const int selected=p.fuselageStations.selected;
      if(p.workspace!=2 || p.tool!="Edit Profiles" || p.viewport!=0 || selected<0 ||
          p.fuselageStations.lines[selected].profile!=static_cast<std::size_t>(profiles.active))bad("fuselage profile editing station");
    }
    if(!profiles.pending.empty() && (!profiles.editing || profiles.tool==SketchTool::None))bad("fuselage profile draft tool");
  }
  if(p.controls.drawing>=0 && (p.workspace!=1 || p.tool!="Ailerons/Flaps"))bad("control rectangle workspace");
  return p;
}
bool writeProject(const QString& path,const ProjectDocument& project,QString& error) {
  error.clear();
  try {
    const auto json=encodeProject(project); // Complete encoding before opening the destination.
    (void)decodeProject(json); // Never write a snapshot the reader cannot restore.
    const auto bytes=QJsonDocument{json}.toJson(QJsonDocument::Indented);
    if(bytes.size()>512LL*1024*1024)throw std::runtime_error("The project exceeds the 512 MiB file limit.");
    QSaveFile file{path}; // Atomic replacement; a failed write retains the previous file.
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()) {error=file.errorString();return false;}
    return true;
  } catch(const std::exception& e) {error=QString::fromUtf8(e.what());return false;}
}
std::optional<ProjectDocument> readProject(const QString& path,QString& error) {
  error.clear();
  try {
    QFile file{path};if(!file.open(QIODevice::ReadOnly)){error=file.errorString();return {};}
    if(file.size()>512LL*1024*1024)throw std::runtime_error("The project exceeds the 512 MiB file limit.");
    QJsonParseError parse;const auto json=QJsonDocument::fromJson(file.readAll(),&parse);
    if(parse.error!=QJsonParseError::NoError||!json.isObject())throw std::runtime_error("The project is not a valid JSON document.");
    return decodeProject(json.object());
  } catch(const std::exception& e) {error=QString::fromUtf8(e.what());return {};}
}
}

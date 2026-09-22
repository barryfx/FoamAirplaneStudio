#pragma once
#include <QMap>
#include <QRegularExpression>
#include <QString>
namespace designrc::gui {
using ComponentNames=QMap<QString,QString>;
inline bool validComponentName(const QString& name) {
  static const QRegularExpression invalid{R"([\x00-\x1f<>:"/\\|?*])"};
  static const QRegularExpression reserved{R"(^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$))",QRegularExpression::CaseInsensitiveOption};
  return !name.isEmpty()&&name.size()<=120&&name==name.trimmed()&&!name.endsWith('.')&&
      !name.contains(invalid)&&!name.contains(reserved);
}
inline QString exportProjectStem(QString name) {
  name=name.trimmed();name.replace(QRegularExpression{R"([\x00-\x1f<>:"/\\|?*])"},"_");
  while(name.endsWith('.')||name.endsWith(' '))name.chop(1);
  if(name.isEmpty())return "Untitled";
  if(!validComponentName(name))name="Project_"+name.left(110);
  return name;
}
}

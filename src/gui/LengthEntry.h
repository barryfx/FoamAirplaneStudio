#pragma once
#include "gui/ReferenceImage.h"
#include <QLocale>
#include <QRegularExpression>
#include <optional>
#include <cmath>

namespace designrc::gui {
// All physical calculations use millimetres; presentation retains the user's text.
inline QString unitSuffix(ProjectUnits units) {
  return units == ProjectUnits::Inches ? " in" : " mm";
}
inline std::optional<double> lengthInMm(const QString& text, ProjectUnits units, bool allowZero=false) {
  static const QRegularExpression pattern{
      R"(^\s*(\d+(?:\.\d*)?|\.\d+)\s*(mm|in|inches|inch|millimeters|millimetres|")?\s*$)",
      QRegularExpression::CaseInsensitiveOption};
  const auto match=pattern.match(text);
  if(!match.hasMatch())return {};
  const auto suffix=match.captured(2).toLower();
  if(!suffix.isEmpty())units=(suffix=="mm" || suffix.startsWith("milli"))
      ?ProjectUnits::Millimeters:ProjectUnits::Inches;
  bool ok=false;const double value=QLocale::c().toDouble(match.captured(1),&ok);
  const double mm=value*(units==ProjectUnits::Inches?25.4:1.0);
  if(!ok || !std::isfinite(mm) || (allowZero?mm<0:mm<=0) || mm>1.e12)return {};
  return mm;
}
inline QString explicitLength(QString text, ProjectUnits units, bool allowZero=false) {
  if(lengthInMm(text,units,allowZero) && !text.contains(QRegularExpression{"[a-zA-Z\"]"}))
    text+=unitSuffix(units);
  return text;
}
inline QString formattedLength(double mm,ProjectUnits units) {
  return QLocale::c().toString(mm/(units==ProjectUnits::Inches?25.4:1.0),'g',12)+unitSuffix(units);
}
}

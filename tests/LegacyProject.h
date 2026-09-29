#pragma once
#include <QJsonObject>
#include <QJsonArray>
// Downgraded test fixtures must use the historical two-view cut schema too.
inline void legacyCutViews(QJsonObject& project) {
  auto cuts=project["fuselageCuts"].toObject();auto layers=cuts["layers"].toArray();
  if(layers.size()!=4)return;
  cuts["layers"]=QJsonArray{layers[0],layers[2]};
  if(cuts["active"].toInt()==2)cuts["active"]=1;
  project["fuselageCuts"]=cuts;
}

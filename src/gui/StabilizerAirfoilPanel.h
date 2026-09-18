#pragma once
#include "domain/AirfoilProfile.h"
#include <QWidget>
#include <functional>
#include <optional>
class QLabel;
namespace designrc::gui {
domain::AirfoilProfile defaultStabilizerAirfoil();
class StabilizerAirfoilPanel final : public QWidget {
public:
  explicit StabilizerAirfoilPanel(bool horizontal, QWidget* parent = nullptr);
  const domain::AirfoilProfile& airfoil() const { return selected_ ? *selected_ : default_; }
  const std::optional<domain::AirfoilProfile>& selection() const { return selected_; }
  void restore(std::optional<domain::AirfoilProfile> profile);
  bool loadFile(const QString& path, QString& error);
  std::function<void()> changed;
private:
  domain::AirfoilProfile default_;
  std::optional<domain::AirfoilProfile> selected_;
  QLabel* name_{};
};
}

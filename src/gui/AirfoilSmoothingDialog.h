#pragma once
#include "domain/AirfoilProfile.h"
#include <QDialog>
#include <optional>
class QLineEdit;
namespace designrc::gui {
class AirfoilSmoothingDialog final : public QDialog {
public:
  AirfoilSmoothingDialog(const domain::AirfoilProfile& original,const QString& name,QWidget* parent=nullptr);
  const domain::AirfoilProfile& result() const { return *result_; }
  QString copyName() const;
private:
  QLineEdit* name_{};
  std::optional<domain::AirfoilProfile> result_;
};
}

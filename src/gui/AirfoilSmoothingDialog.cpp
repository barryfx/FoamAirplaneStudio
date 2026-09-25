#include "gui/AirfoilSmoothingDialog.h"
#include "gui/AirfoilSmoothing.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
namespace designrc::gui {
namespace {
class Preview final : public QWidget {
public:
  std::vector<domain::Point2> original,smoothed;
  explicit Preview(QWidget* parent):QWidget{parent} {setMinimumSize(700,340);setObjectName("airfoilSmoothingPreview");}
protected:
  void paintEvent(QPaintEvent*) override {
    QPainter painter{this};painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(),palette().base());
    double low=0,high=0;
    for(const auto* curve:{&original,&smoothed})for(auto p:*curve){low=std::min(low,p.y);high=std::max(high,p.y);}
    const double scale=std::min(width()-48.,(height()-64.)/std::max(.05,high-low));
    const QPointF origin{(width()-scale)/2.,height()/2.+(high+low)*scale/2.};
    auto point=[&](domain::Point2 p){return origin+QPointF{p.x*scale,-p.y*scale};};
    painter.setPen(QPen{palette().mid().color(),1,Qt::DotLine});painter.drawLine(point({0,0}),point({1,0}));
    auto draw=[&](const auto& curve,QColor color,Qt::PenStyle style) {
      if(curve.empty())return;
      QPainterPath path;path.moveTo(point(curve.front()));for(auto p:curve)path.lineTo(point(p));
      path.closeSubpath(); // A blunt trailing edge closes with a line, never a spline.
      painter.setPen(QPen{color,2,style});painter.drawPath(path);
    };
    draw(original,QColor{"#287bb5"},Qt::DashLine);draw(smoothed,QColor{"#e07818"},Qt::SolidLine);
    painter.setPen(palette().text().color());painter.drawText(12,22,"Original: blue dashed     Smoothed: orange");
    painter.drawText(12,height()-10,"Normalized chord — equal horizontal and vertical scale");
  }
};
}
AirfoilSmoothingDialog::AirfoilSmoothingDialog(const domain::AirfoilProfile& original,const QString& name,QWidget* parent)
    :QDialog{parent} {
  setWindowTitle("Smooth Airfoil");setObjectName("airfoilSmoothingDialog");resize(1000,680);
  auto* layout=new QVBoxLayout{this};
  auto* description=new QLabel{"Adjust smoothing and compare the outlines. Short, steep trailing-edge closing segments stay separate from the smooth surfaces. Save Copy adds a new airfoil; the original and station assignments stay as they are.",this};
  description->setWordWrap(true);layout->addWidget(description);
  auto* preview=new Preview{this};preview->original=original.resampled(161);layout->addWidget(preview,1);
  auto* strengthLabel=new QLabel{this};layout->addWidget(strengthLabel);
  auto* strength=new QSlider{Qt::Horizontal,this};strength->setObjectName("airfoilSmoothingStrength");
  strength->setRange(0,100);strength->setValue(25);layout->addWidget(strength);
  auto* metrics=new QLabel{this};metrics->setObjectName("airfoilSmoothingMetrics");metrics->setWordWrap(true);layout->addWidget(metrics);
  name_=new QLineEdit{name,this};name_->setObjectName("smoothedAirfoilName");
  auto* nameRow=new QHBoxLayout;nameRow->addWidget(new QLabel{"Copy name:",this});nameRow->addWidget(name_);layout->addLayout(nameRow);
  auto* buttons=new QDialogButtonBox{QDialogButtonBox::Save|QDialogButtonBox::Cancel,this};
  auto* save=buttons->button(QDialogButtonBox::Save);save->setText("Save Copy");save->setObjectName("saveSmoothedAirfoil");layout->addWidget(buttons);
  connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  connect(name_,&QLineEdit::textChanged,this,[this,save]{save->setEnabled(result_.has_value()&&!copyName().isEmpty());});
  auto refresh=[this,original,preview,strength,strengthLabel,metrics,save] {
    strengthLabel->setText(QString{"Smoothing strength: %1% (gentle to strong)"}.arg(strength->value()));
    try {
      result_=smoothAirfoil(original,strength->value());preview->smoothed=result_->outline();
      const auto before=airfoilMetrics(original),after=airfoilMetrics(*result_);
      metrics->setText(QString{"Maximum thickness: %1% → %2% chord\nMaximum camber (signed): %3% → %4% chord"}
        .arg(before.thickness*100,0,'f',2).arg(after.thickness*100,0,'f',2)
        .arg(before.camber*100,0,'f',2).arg(after.camber*100,0,'f',2));
    }catch(const std::exception& e){result_.reset();preview->smoothed.clear();metrics->setText(QString::fromUtf8(e.what()));}
    save->setEnabled(result_.has_value()&&!copyName().isEmpty());preview->update();
  };
  connect(strength,&QSlider::valueChanged,this,[refresh](int){refresh();});refresh();
}
QString AirfoilSmoothingDialog::copyName() const {return name_->text().simplified();}
}

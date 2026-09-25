#include "gui/AirfoilSmoothing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace designrc::gui {
namespace {
constexpr int controls=16,samples=161;
using Coefficients=std::array<double,controls>;
// Clamped cubic B-spline basis on the normalized chord.
Coefficients basis(double x) {
  std::array<double,controls+4> knots{};
  for(int i=0;i<controls+4;++i)knots[i]=std::clamp((i-3.)/(controls-3),0.,1.);
  std::array<double,controls+3> values{};
  if(x==1){Coefficients result{};result.back()=1;return result;}
  for(int i=0;i<controls+3;++i)values[i]=(x>=knots[i]&&x<knots[i+1])?1.:0.;
  for(int degree=1;degree<=3;++degree)for(int i=0;i<controls+3-degree;++i) {
    const double a=knots[i+degree]-knots[i],b=knots[i+degree+1]-knots[i+1];
    values[i]=(a>0?(x-knots[i])/a*values[i]:0)+(b>0?(knots[i+degree+1]-x)/b*values[i+1]:0);
  }
  Coefficients result{};std::copy_n(values.begin(),controls,result.begin());return result;
}
double evaluate(const Coefficients& coefficients,double x) {
  const auto b=basis(x);double result=0;
  for(int i=0;i<controls;++i)result+=coefficients[i]*b[i];
  return result;
}
Coefficients fit(const std::vector<domain::Point2>& points,bool thickness,
                 double leading,double trailing,double penalty) {
  std::array<Coefficients,controls> matrix{};Coefficients rhs{};
  for(int k=1;k<samples-1;++k) {
    const auto upper=points[samples-1-k],lower=points[samples-1+k];const double x=upper.x;
    auto row=basis(x);const double envelope=(thickness?std::sqrt(x):x)*(1-x);
    const double value=thickness?(upper.y-lower.y)*.5:(upper.y+lower.y)*.5;
    const double target=value-(leading*(1-x)+trailing*x);
    for(auto& v:row)v*=envelope;
    for(int i=0;i<controls;++i) {
      rhs[i]+=row[i]*target;
      for(int j=0;j<controls;++j)matrix[i][j]+=row[i]*row[j];
    }
  }
  // Penalize bending of the control polygon, retaining broad camber/thickness.
  for(int k=0;k<controls-2;++k)for(int i=0;i<3;++i)for(int j=0;j<3;++j)
    matrix[k+i][k+j]+=penalty*(i==1?-2.:1.)*(j==1?-2.:1.);
  for(int i=0;i<controls;++i)matrix[i][i]+=1e-12;
  Coefficients result{};
  // Projected coordinate descent solves the convex fit and enforces positive
  // thickness coefficients. B-spline non-negativity then protects the full chord.
  for(int iteration=0;iteration<20000;++iteration) {
    double change=0;
    for(int i=0;i<controls;++i) {
      double value=rhs[i];for(int j=0;j<controls;++j)if(j!=i)value-=matrix[i][j]*result[j];
      value/=matrix[i][i];if(thickness)value=std::max(1e-6,value);
      change=std::max(change,std::abs(value-result[i]));result[i]=value;
    }
    if(change<1e-11)return result;
  }
  throw std::runtime_error("Airfoil smoothing did not converge. Try a lower strength.");
}
}
domain::AirfoilProfile airfoilSmoothingSource(const domain::AirfoilProfile& original) {
  auto points=original.outline();
  if(points.size()<5)return original;
  const auto first=points.front(),last=points.back();
  // Only a repeated closing point can be mistaken for the other surface's TE.
  // Preserve genuine sharp edges and existing open/blunt TE coordinate pairs.
  if(std::hypot(first.x-last.x,first.y-last.y)>1e-9||first.x<1-1e-9)return original;
  auto separate=[&](bool atEnd) {
    const auto tip=atEnd?points.back():points.front();
    const auto near=atEnd?points[points.size()-2]:points[1];
    const auto previous=atEnd?points[points.size()-3]:points[2];
    const double dx=tip.x-near.x,run=near.x-previous.x;
    if(dx< -1e-9||dx>.005||run<1e-7)return false;
    const double slope=(near.y-previous.y)/run,jump=std::abs(tip.y-near.y);
    // Require both a near-vertical segment and an abrupt change from its
    // preceding surface tangent, not merely a steep but smooth wedge.
    if(jump<.001||jump<2*dx||jump<10*std::abs(slope)*dx)return false;
    const domain::Point2 endpoint{1,near.y+slope*dx};
    if(atEnd)points.back()=endpoint;else points.front()=endpoint;
    return true;
  };
  const bool end=separate(true),start=separate(false);
  if(!end&&!start)return original;
  std::ostringstream dat;dat.imbue(std::locale::classic());dat.precision(17);dat<<original.name()<<'\n';
  for(auto p:points)dat<<p.x<<' '<<p.y<<'\n';
  std::istringstream input{dat.str()};input.imbue(std::locale::classic());return domain::AirfoilProfile::fromDat(input);
}
domain::AirfoilProfile smoothAirfoil(const domain::AirfoilProfile& original,int strength) {
  if(strength<0||strength>100)throw std::invalid_argument("Smoothing strength must be 0 to 100.");
  const auto points=airfoilSmoothingSource(original).resampled(samples);
  for(auto p:points)if(!std::isfinite(p.x)||!std::isfinite(p.y))throw std::runtime_error("Airfoil coordinates must be finite.");
  double maximum=0;
  for(int k=1;k<samples;++k) {
    const double thickness=points[samples-1-k].y-points[samples-1+k].y;
    if(thickness< -1e-9)throw std::runtime_error("The airfoil surfaces cross. Correct the outline before smoothing.");
    maximum=std::max(maximum,thickness);
  }
  if(maximum<1e-8)throw std::runtime_error("The airfoil has no thickness.");
  const double le=points[samples-1].y,te=(points.front().y+points.back().y)*.5;
  const double halfGap=std::max(0.,(points.front().y-points.back().y)*.5);
  const double penalty=1e-5*std::pow(10000.,strength/100.);
  const auto camber=fit(points,false,le,te,penalty),thickness=fit(points,true,0,halfGap,penalty);
  std::ostringstream dat;dat.imbue(std::locale::classic());dat.precision(17);dat<<"Smoothed airfoil\n";
  for(std::size_t i=0;i<points.size();++i) {
    const double x=points[i].x;
    const double middle=le*(1-x)+te*x+x*(1-x)*evaluate(camber,x);
    const double half=halfGap*x+std::sqrt(x)*(1-x)*evaluate(thickness,x);
    double y=middle+(i<samples-1?half:-half);
    if(i==0||i==points.size()-1||i==samples-1)y=points[i].y;
    dat<<x<<' '<<y<<'\n';
  }
  std::istringstream input{dat.str()};input.imbue(std::locale::classic());return domain::AirfoilProfile::fromDat(input);
}
AirfoilMetrics airfoilMetrics(const domain::AirfoilProfile& profile) {
  const auto points=profile.resampled(samples);AirfoilMetrics result;
  for(int k=0;k<samples;++k) {
    const double upper=points[samples-1-k].y,lower=points[samples-1+k].y;
    result.thickness=std::max(result.thickness,upper-lower);
    const double camber=(upper+lower)*.5;if(std::abs(camber)>std::abs(result.camber))result.camber=camber;
  }
  return result;
}
}

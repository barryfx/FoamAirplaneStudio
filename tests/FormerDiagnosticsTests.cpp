#include "geometry/FormerDiagnostics.h"
#include <iostream>
#include <stdexcept>
int main() {
  const std::vector<QRectF> rectangles{{100,0,4,30},{23.4,0,4,30}};
  const auto error=designrc::geometry::formerRailClearanceError(rectangles,1,-3,true,true,12.345);
  for(const auto* expected:{"Former 1","front rail","right side","Fuselage > Formers","foam plug"})
    if(error.find(expected)==std::string::npos)throw std::runtime_error(expected);
  const auto other=designrc::geometry::formerRailClearanceError(rectangles,0,0,false,false,1);
  if(error.size()>400||error.find(" mm")!=std::string::npos||error.find(" in from")!=std::string::npos)
    throw std::runtime_error("Diagnostic must be short and omit measurements");
  if(other.find("Former 2")==std::string::npos||other.find("rear rail")==std::string::npos||other.find("left side")==std::string::npos)
    throw std::runtime_error("Wrong former or rail identification");
  std::cout<<"Former diagnostic identity, units, rail and guidance checks passed. No geometry generated.\n";
}

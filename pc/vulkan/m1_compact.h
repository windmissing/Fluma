#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace grs::pc {
inline constexpr int kCompactFirstColumn=24;
inline constexpr int kCompactEndColumn=104;
inline constexpr int kCompactHeight=64;
inline constexpr int kCompactAngleStep=5*256;
inline constexpr double kCompactNozzleHeight=16;
inline constexpr double kCompactSourceRate=32;

inline std::array<float,2> CompactRotation(int angle) {
  const auto radians=angle*(3.14159265358979323846/(180*256));
  return {float(std::cos(radians)),float(std::sin(radians))};
}

inline bool CompactPointer(int px,int py,int width,int height,int angle,double &x,double &y) {
  if (width<=0 || height<=0) return false;
  const auto rotation=CompactRotation(angle);
  const double scale=std::min(width,height)*.008;
  const double sx=(px+.5-width*.5)/scale,sy=(py+.5-height*.5)/scale;
  x=(kCompactFirstColumn+kCompactEndColumn)*.5+rotation[0]*sx+rotation[1]*sy;
  y=kCompactHeight*.5+rotation[1]*sx-rotation[0]*sy;
  if (x<kCompactFirstColumn || x>=kCompactEndColumn || y<0 || y>=kCompactHeight) return false;
  y=std::min(y,kCompactNozzleHeight);
  return true;
}
}

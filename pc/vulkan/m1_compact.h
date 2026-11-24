#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace grs::pc {
inline constexpr int kCompactFirstColumn=24;
inline constexpr int kCompactEndColumn=104;
inline constexpr int kCompactHeight=64;
inline constexpr int kCompactAngleStep=5*256;
inline constexpr double kCompactNozzleHeight=16;
inline constexpr double kCompactSourceRate=32;

struct CompactView {
  uint64_t run=0,geometry=0,serial=0,revision=0,requestSequence=0,nonce=0,commit=0,generation=0;
  uintptr_t window=0,swapchain=0;
  uint32_t clientWidth=0,clientHeight=0,dpi=0,width=0,height=0,slot=0,surfaceTransform=0;
  int32_t requestedAngle=0,angle=0;
  float centerX=0,centerY=0,scale=0;
  std::array<float,2> rotation{1.f,0.f};
  bool mapped=false;
};

inline bool SameCompactMapping(const CompactView &a,const CompactView &b) {
  return a.run==b.run && a.geometry==b.geometry && a.window==b.window && a.swapchain==b.swapchain &&
      a.clientWidth==b.clientWidth && a.clientHeight==b.clientHeight && a.dpi==b.dpi &&
      a.width==b.width && a.height==b.height && a.surfaceTransform==b.surfaceTransform &&
      a.centerX==b.centerX && a.centerY==b.centerY && a.scale==b.scale &&
      a.rotation==b.rotation && a.mapped==b.mapped;
}

inline bool MapCompactPointer(const CompactView &view,int px,int py,double &x,double &y) {
  if (!view.mapped || view.scale<=0 || px<0 || py<0 ||
      uint32_t(px)>=view.clientWidth || uint32_t(py)>=view.clientHeight) return false;
  const double dx=(px+.5-view.centerX)/view.scale,dy=(py+.5-view.centerY)/view.scale;
  const auto c=view.rotation[0],s=view.rotation[1];
  x=(kCompactFirstColumn+kCompactEndColumn)*.5+c*dx+s*dy;
  y=kCompactHeight*.5+s*dx-c*dy;
  return x>=kCompactFirstColumn && x<kCompactEndColumn && y>=0 && y<kCompactHeight;
}

inline std::array<double,2> ProjectCompactPoint(const CompactView &view,double x,double y) {
  const double c=view.rotation[0],s=view.rotation[1];
  x-=(kCompactFirstColumn+kCompactEndColumn)*.5;
  y-=kCompactHeight*.5;
  const double scale=view.scale/(c*c+s*s);
  return {view.centerX+(c*x+s*y)*scale,view.centerY+(s*x-c*y)*scale};
}

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

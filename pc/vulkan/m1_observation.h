#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace grs::pc {
inline constexpr int32_t kM1AngleLimit=30*256, kM1AngleStep=15*256;
inline constexpr bool ValidM1Angle(int32_t angle) { return angle>=-kM1AngleLimit && angle<=kM1AngleLimit; }
struct M1Gravity { uint64_t geometry=0, sequence=0, qpc=0; int32_t angle=0; };
struct M1GravityDisplay {
  uint64_t geometry=0,sequence=0;
  int32_t actual=0;
  bool accepted=false,limited=false,stale=false;
};
inline constexpr bool M1ResetHit(int x,int y,int width,int height) {
  return width>=112 && height>=42 && x>=width-42 && x<width-8 && y>=8 && y<38;
}
struct M1Rect { uint32_t x=0,y=0,width=0,height=0; };
inline constexpr size_t kM1RoiBytes=3*16*16*4;
inline constexpr bool Fits(const M1Rect &r,uint32_t width,uint32_t height) {
  return r.width && r.height && r.width<=16 && r.height<=16 &&
         r.x<=width && r.y<=height && r.width<=width-r.x && r.height<=height-r.y;
}
struct M1Observation {
  uint64_t frame=0;
  std::array<M1Rect,3> regions{};
  std::wstring archivePrefix;
  uint32_t series=0;
  uint32_t packetId=0,packetGeneration=0;
  uint32_t birthFrames=0;
  bool Enabled() const { return frame || birthFrames; }
  bool Includes(uint64_t index) const { return frame && index>=frame && index-frame<(series ? series : 1); }
};
}

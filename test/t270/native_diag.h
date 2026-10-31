#pragma once
#include "clear_water_packet.h"
#include <GLES3/gl3.h>
#include <array>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <cmath>
namespace cwdiag {
struct Sample {std::string token;uint64_t frame=0;int layer=0,x=0,y=0,w=0,h=0;uint32_t fbo=0,raw=0;double ms=0,water=0,ledgerWater=0,sourceCommit=0,physicsCommit=0;std::array<uint8_t,1024> rgba{};};
inline std::mutex mutex;inline std::array<Sample,9> samples{};inline uint64_t first=0;inline int frames=0,x=0,y=0,w=0,h=0;inline size_t count=0;inline uint64_t dropped=0;
inline bool arm(uint64_t frame,int n,int px,int py,int width,int height){if(frame==0||n<1||n>3||px<0||py<0||width<1||height<1||width>16||height>16)return false;std::lock_guard<std::mutex> l(mutex);first=frame;frames=n;x=px;y=py;w=width;h=height;count=0;dropped=0;return true;}
inline void sample(int layer,const p013::ClearWaterPacket& p,GLuint expected){std::lock_guard<std::mutex> l(mutex);uint64_t frame=uint64_t(p.frame.frameId);if(first==0||frame<first||frame-first>=uint64_t(frames)||(layer!=2&&layer!=3&&layer!=4))return;if(count==samples.size()){dropped++;return;}auto begin=std::chrono::steady_clock::now();auto& r=samples[count++];r.token=p.token;r.frame=frame;r.layer=layer;r.x=x;r.y=y;r.w=w;r.h=h;r.fbo=expected;r.raw=0;r.water=0;for(int c=0;c<128;c++)r.water+=p.profile[8*c+1];r.ledgerWater=p.ledger[11];
// T333 (T327 contract §2): Engine header contract (P011ClearWaterEngine.ets :157) is
// h[5]=c.physicsCommit, h[6]=c.sourceCommit. The V1 writer swapped these two labels
// (V1 archives: "sourceCommit" key actually holds physicsCommit and vice versa).
// V2 writes the correct mapping. V1 archive readers MUST interpret keys as
// value-for-key-swapped; V1 archives are never rewritten (immutability).
r.sourceCommit=p.header[6];r.physicsCommit=p.header[5];
 GLint bound=0,pack=0,alignment=0,rowLength=0,skipRows=0,skipPixels=0;glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);glGetIntegerv(GL_PACK_ROW_LENGTH,&rowLength);glGetIntegerv(GL_PACK_SKIP_ROWS,&skipRows);glGetIntegerv(GL_PACK_SKIP_PIXELS,&skipPixels);glGetIntegerv(GL_FRAMEBUFFER_BINDING,&bound);glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&pack);
 r.fbo=uint32_t(bound);if(bound==0||(expected!=0&&GLuint(bound)!=expected)||pack!=0||alignment>4||rowLength!=0||skipRows!=0||skipPixels!=0||x+w>int(p.frame.surfaceWidthPx)||y+h>int(p.frame.surfaceHeightPx))r.raw=0xffffffffu;else{glReadPixels(x,y,w,h,GL_RGBA,GL_UNSIGNED_BYTE,r.rgba.data());r.raw=glGetError();}
 r.ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();}
inline std::string escaped(const std::string& s){std::string r;for(char c:s){if(c=='"'||c=='\\')r+='\\';if(static_cast<unsigned char>(c)<32){const char* h="0123456789abcdef";r+="\\u00";r+=h[(static_cast<unsigned char>(c)>>4)&15];r+=h[static_cast<unsigned char>(c)&15];}else r+=c;}return r;}
inline std::string exportCold(){std::lock_guard<std::mutex> l(mutex);std::ostringstream o;o<<std::setprecision(17)<<"{\"revision\":\"CW_C_DIAG_V2\",\"format\":\"SRGB8_ALPHA8_ENCODED_RGBA8_UNORM\",\"dropped\":"<<dropped<<",\"samples\":[";const char* hex="0123456789abcdef";for(size_t i=0;i<count;i++){const auto& r=samples[i];if(i)o<<',';o<<"{\"frame\":"<<r.frame<<",\"layer\":"<<r.layer<<",\"token\":\""<<escaped(r.token)<<"\",\"fbo\":"<<r.fbo<<",\"roi\":["<<r.x<<','<<r.y<<','<<r.w<<','<<r.h<<"],\"rawGL\":"<<r.raw<<",\"readbackMs\":"<<r.ms<<",\"nativeWater\":"<<r.water<<",\"ledgerWater\":"<<r.ledgerWater<<",\"sourceCommit\":"<<r.sourceCommit<<",\"physicsCommit\":"<<r.physicsCommit<<",\"rgbaHex\":\"";if(r.raw==0)for(int j=0;j<4*r.w*r.h;j++)o<<hex[r.rgba[j]>>4]<<hex[r.rgba[j]&15];o<<"\"}";}o<<"]}";return o.str();}
}

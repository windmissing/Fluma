#include "clear_water_packet.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <cstdlib>
namespace p013 {
namespace {
bool Same(double a,double b){return std::memcmp(&a,&b,8)==0;}
float PX(double x){float q=float(x);return q>=std::floor(x)+1?std::nextafter(q,-INFINITY):q;}
}
Status ValidateClearWater(const ClearWaterPacket& p,const char*& stage){
 stage="CW_V3_IDENTITY";const auto& f=p.frame;const auto& h=p.header;const auto& l=p.ledger;const auto& o=p.optics;
 if(p.token.empty()||p.run.empty()||p.capacity<1||p.capacity>512||p.count>p.capacity||!SafeInteger(p.context)||p.context<1||!std::isfinite(p.dpi)||p.dpi<=0)return Status::INVALID_ARGUMENT;
 const double ids[]={f.sessionId,f.frameId,f.snapshotId,f.surfaceEpoch,f.fieldEpoch,f.geometryEpoch,f.continuityEpoch,f.appearanceConfigRevision,f.lightConfigRevision};for(double x:ids)if(!SafeInteger(x))return Status::INVALID_ARGUMENT;
 if(f.frameId<1||f.capacity!=256||f.T_dataLayoutVersion!=3||f.zOriginVersion!=1||!std::isfinite(f.frameTimeNs)||f.frameTimeNs<0)return Status::VERSION_MISMATCH;
 stage="CW_UNSUPPORTED_GEOMETRY";
 const double geometry[]={f.surfaceWidthPx,f.surfaceHeightPx,f.boxXPx,f.boxYPx,f.boxWidthPx,f.boxHeightPx};for(double x:geometry)if(!std::isfinite(x)||!std::isfinite(float(x)))return Status::INVALID_ARGUMENT;
 if(!SafeInteger(f.surfaceWidthPx)||!SafeInteger(f.surfaceHeightPx)||f.surfaceWidthPx<1||f.surfaceHeightPx<1||f.surfaceWidthPx>INT32_MAX||f.surfaceHeightPx>INT32_MAX||f.boxXPx<0||f.boxYPx<0||f.boxWidthPx<128||f.boxHeightPx<=0||f.boxXPx+f.boxWidthPx>f.surfaceWidthPx||f.boxYPx+f.boxHeightPx>f.surfaceHeightPx)return Status::INVALID_ARGUMENT;
 stage="CW_HEADER_LEDGER";
 for(double x:h)if(!std::isfinite(x))return Status::INVALID_ARGUMENT;for(double x:l)if(!std::isfinite(x))return Status::INVALID_ARGUMENT;
 for(int i=40;i<64;i++)if(!Same(h[i],0.))return Status::INVALID_ARGUMENT;for(int i=27;i<64;i++)if(!Same(l[i],0.))return Status::INVALID_ARGUMENT;
 const int integerSlots[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,16,17,18,19,20,21,22,23,24,29,30,31,32,33,34,35,36,37,38,39};for(int i:integerSlots)if(!SafeInteger(h[i]))return Status::INVALID_ARGUMENT;
 if(h[3]!=h[1]||h[0]<1||h[1]<1||h[2]<1||h[4]<1||h[18]!=p.count||h[19]!=p.capacity||h[20]!=128||h[21]!=256||h[32]!=1||h[33]!=1||h[34]!=1||h[37]!=0||h[7]>2||h[14]>1||h[16]!=h[14]||h[17]!=h[10]||std::abs(h[15])>30||h[11]>h[5]||h[12]>h[6]||h[22]!=1||h[38]<1||h[39]!=1||h[35]!=h[4])return Status::INVALID_ARGUMENT;
 if(f.snapshotId!=h[2]||f.fieldEpoch!=h[3]||f.geometryEpoch!=h[9]||f.appearanceConfigRevision!=h[4]||f.lightConfigRevision!=h[36])return Status::INVALID_ARGUMENT;
 if(l[0]!=h[1]||l[1]!=h[4]||l[2]!=h[6]||l[3]!=h[5]||l[4]!=h[22]||l[5]!=h[23]||l[6]!=h[24]||!Same(l[7],h[25])||!Same(l[8],h[26])||!Same(l[9],h[27])||!Same(l[10],h[28])||l[21]!=h[12]||l[22]!=h[11]||l[23]!=h[13]||l[24]!=h[10]||l[25]!=h[14]||!Same(l[26],h[15]))return Status::INVALID_ARGUMENT;
 if(h[14]==0&&(h[10]!=0||h[11]!=0||h[12]!=0||h[13]!=0||h[15]!=0))return Status::INVALID_ARGUMENT;
 if(l[15]!=0&&l[15]!=1)return Status::INVALID_ARGUMENT;if(l[15]==0)for(int i=16;i<=20;i++)if(!Same(l[i],0.))return Status::INVALID_ARGUMENT;
 stage="CW_OPTICS16";for(double x:o)if(!std::isfinite(x)||!std::isfinite(float(x)))return Status::INVALID_ARGUMENT;
 if(o[0]<0||o[5]!=h[36]||o[6]!=1||!Same(o[7],0.)||o[8]<=0||o[8]>8||o[13]<1||o[13]>64||std::abs(std::hypot(o[14],o[15])-1)>1e-12)return Status::INVALID_ARGUMENT;
 for(int i:{1,2,3,4,9,10,11,12})if(o[i]<0||o[i]>1)return Status::INVALID_ARGUMENT;
 stage="CW_PROOF8";std::array<double,128> fold{};uint32_t last=0;double totalParticle=0;
 for(size_t i=0;i<p.capacity;i++){
  const double* a=p.proof.data()+8*i;
  for(int j=0;j<8;j++)if(!std::isfinite(a[j]))return Status::INVALID_ARGUMENT;
  if(i>=p.count){for(int j=0;j<8;j++)if(!Same(a[j],0.))return Status::INVALID_ARGUMENT;if(!Same(p.origin[i],0.)||p.wait[i]||p.xy[2*i]!=0||p.xy[2*i+1]!=0||p.meta[2*i]||p.meta[2*i+1])return Status::INVALID_ARGUMENT;continue;}
  if(!SafeInteger(a[0])||a[0]<=last||a[0]>UINT32_MAX||a[1]<=0||a[2]<0||a[2]>=128||a[3]<0||a[3]>256||!SafeInteger(a[6])||a[6]<1||a[6]>UINT32_MAX||(a[7]!=256&&a[7]!=512)||!SafeInteger(p.origin[i])||p.origin[i]<1||p.origin[i]>h[6]||p.wait[i]>4)return Status::INVALID_ARGUMENT;
  if(p.meta[2*i]!=a[7]||p.meta[2*i+1]!=a[0]||p.xy[2*i]!=PX(a[2])||p.xy[2*i+1]!=float(a[3]))return Status::INVALID_ARGUMENT;
  int col=int(std::floor(a[2]));if(a[3]<p.profile[8*col])return Status::INVALID_ARGUMENT;fold[col]=fold[col]+a[1];last=uint32_t(a[0]);totalParticle+=a[1];
 }
 stage="CW_PROFILE8";double tw=0,ts=0,tp=0,tr=0;
 for(int c=0;c<128;c++){
  const double* a=p.profile.data()+8*c;for(int j=0;j<8;j++)if(!std::isfinite(a[j])||a[j]<0)return Status::INVALID_ARGUMENT;
  double sand=(a[4]+0.)+a[5],eta=a[0]+a[1],r=a[2]+a[3];
  if(!Same(a[0],sand)||!Same(a[2],eta)||!Same(a[3],fold[c])||!Same(a[6],r)||r>256||a[7]!=(a[1]>0?1:0))return Status::INVALID_ARGUMENT;
  tw+=a[1];ts+=a[0];tp+=a[3];tr+=a[6];
 }
 if(!Same(tw,l[11])||!Same(ts,l[12])||!Same(tp,l[13])||!Same(tr,l[14])||std::abs(tw-(h[27]+h[25]))>1e-6||std::abs((ts+totalParticle)-(h[28]+h[26]))>1e-6)return Status::INVALID_ARGUMENT;
 stage="CW_TOKEN";
 std::istringstream stream(p.token);std::string part;
 if(!std::getline(stream,part,'|')||part!="CW_FRAME_TOKEN_C1"||!std::getline(stream,part,'|')||part!=p.run)return Status::INVALID_ARGUMENT;
 const double binding[]={h[0],h[1],h[5],h[6],h[2],h[10],h[11],h[12],h[13],h[14],f.sessionId,double(p.context),f.surfaceEpoch,f.geometryEpoch,f.surfaceWidthPx,f.surfaceHeightPx,p.dpi,f.visualInverted?1.:0.,f.boxXPx,f.boxYPx,f.boxWidthPx,f.boxHeightPx,h[4],h[35],h[36],h[32],h[33],h[34],h[38],h[39],h[15],h[16],h[17],f.frameId};
 for(double expected:binding){if(!std::getline(stream,part,'|')||part.empty())return Status::INVALID_ARGUMENT;char* end=nullptr;double v=std::strtod(part.c_str(),&end);if(!end||*end||!Same(v,expected))return Status::INVALID_ARGUMENT;}
 if(p.configDigest.empty()||!std::getline(stream,part,'|')||part!=p.configDigest||!std::getline(stream,part,'|')||part!=p.payloadDigest||std::getline(stream,part,'|'))return Status::INVALID_ARGUMENT;
 uint32_t hash=0x811c9dc5;auto bytes=[&](const void* data,size_t count){auto b=static_cast<const uint8_t*>(data);for(size_t i=0;i<count;i++){hash^=b[i];hash*=0x01000193;}};
 bytes(p.profile.data(),8192);bytes(p.proof.data(),p.capacity*64);bytes(p.origin.data(),p.capacity*8);bytes(p.wait.data(),p.capacity);bytes(p.xy.data(),p.capacity*8);bytes(p.meta.data(),p.capacity*8);bytes(h.data(),512);bytes(l.data(),512);bytes(o.data(),128);
 std::ostringstream digest;digest<<"CW_RAW_FNV1A32:"<<std::hex<<hash;if(digest.str()!=p.payloadDigest)return Status::INVALID_ARGUMENT;
 stage="NONE";return Status::OK;
}
double ClearWaterTransmissionAlpha(double S,double W,double y,double sigma){if(W==0||y>S+W)return 0;double path=std::clamp(S+W-std::max(y,S),0.,W);return 1-std::exp(-sigma*path);}
double ClearWaterSurfaceAlpha(const ClearWaterPacket& p,int c,double px,double py){
 const auto& f=p.frame;const auto& o=p.optics;const double* a=p.profile.data()+8*c;if(a[1]<=0)return 0;
 double colWidth=f.boxWidthPx/128,lo=f.boxXPx+c*colWidth,hi=lo+colWidth;
 // Physical upward Y coordinate, then map the clipped interval through mirror.
 double ys=a[0]/256*f.boxHeightPx,ye=a[2]/256*f.boxHeightPx,low=std::max(ys,ye-o[8]);
 double y0=f.boxYPx+f.boxHeightPx-ye,y1=f.boxYPx+f.boxHeightPx-low;
 if(f.visualInverted){lo=f.boxXPx+f.boxWidthPx-(c+1)*colWidth;hi=lo+colWidth;y0=f.boxYPx+low;y1=f.boxYPx+ye;}
 double A=std::max(0.,std::min(px+1,hi)-std::max(px,lo))*std::max(0.,std::min(py+1,y1)-std::max(py,y0));
 bool left=c>0&&p.profile[(c-1)*8+1]>0,right=c<127&&p.profile[(c+1)*8+1]>0;double slope=0;
 if(left&&right)slope=(p.profile[(c+1)*8+2]-p.profile[(c-1)*8+2])/2;else if(left)slope=a[2]-p.profile[(c-1)*8+2];else if(right)slope=p.profile[(c+1)*8+2]-a[2];
 double n=std::hypot(slope,1),nx=-slope/n,ny=1/n,F=o[12]+(1-o[12])*std::pow(1-ny,5),G=std::pow(std::clamp(nx*o[14]+ny*o[15],0.,1.),o[13]);return o[4]*A*(F+(1-F)*G);
}
ClearWaterReceipt ClearWaterAtomic::Draw(const ClearWaterPacket& p,ClearWaterGpu& gpu){
 ClearWaterReceipt r;r.code=ValidateClearWater(p,r.stage);if(r.code!=Status::OK)return r;
 r.stage="CW_STALE_FRAME";
 if(p.frame.frameId<=lastAttempt)return ClearWaterReceipt{Status::STALE_FRAME,r.stage,false,{}};
 if(p.header[0]==lastOwner&&p.header[1]==lastRun&&(p.header[5]<lastCommit||p.header[2]<=lastPublication))return ClearWaterReceipt{Status::STALE_FRAME,r.stage,false,{}};
 lastAttempt=p.frame.frameId;
 r.stage="CW_BEGIN";r.code=gpu.Begin(p);if(r.code!=Status::OK)return r;
 static const char* stages[]={"CW_BACKGROUND","CW_SAND_BED","CW_PARTICLES","CW_TRANSMISSION","CW_WATER_SURFACE","CW_FOREGROUND"};
 for(int i=0;i<6;i++){r.stage=stages[i];r.code=gpu.Layer(i,p);if(r.code!=Status::OK)return r;}
 r.stage="CW_PRESENT";r.code=gpu.Present();if(r.code!=Status::OK)return r;
 r.stage="CW_SWAP";r.code=gpu.Swap(uint64_t(p.frame.surfaceEpoch));if(r.code!=Status::OK)return r;
 gpu.Commit();lastOwner=p.header[0];lastRun=p.header[1];lastCommit=p.header[5];lastPublication=p.header[2];lastGood=p.token;r.rendered=p.token;r.success=true;r.stage="NONE";return r;
}
}

#include "clear_water_packet.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
using namespace p013;
void digest(ClearWaterPacket& p){uint32_t h=0x811c9dc5;auto bytes=[&](const void* data,size_t n){auto b=static_cast<const uint8_t*>(data);for(size_t i=0;i<n;i++){h^=b[i];h*=0x01000193;}};bytes(p.profile.data(),8192);bytes(p.proof.data(),p.capacity*64);bytes(p.origin.data(),p.capacity*8);bytes(p.wait.data(),p.capacity);bytes(p.xy.data(),p.capacity*8);bytes(p.meta.data(),p.capacity*8);bytes(p.header.data(),512);bytes(p.ledger.data(),512);bytes(p.optics.data(),128);std::ostringstream s;s<<"CW_RAW_FNV1A32:"<<std::hex<<h;p.payloadDigest=s.str();}
void token(ClearWaterPacket& p){
 const auto& h=p.header;const auto& f=p.frame;std::ostringstream s;s<<std::setprecision(17)<<"CW_FRAME_TOKEN_C1|"<<p.run;
 const double binding[]={h[0],h[1],h[5],h[6],h[2],h[10],h[11],h[12],h[13],h[14],f.sessionId,double(p.context),f.surfaceEpoch,f.geometryEpoch,f.surfaceWidthPx,f.surfaceHeightPx,p.dpi,f.visualInverted?1.:0.,f.boxXPx,f.boxYPx,f.boxWidthPx,f.boxHeightPx,h[4],h[35],h[36],h[32],h[33],h[34],h[38],h[39],h[15],h[16],h[17],f.frameId};for(double v:binding)s<<'|'<<v;s<<'|'<<p.configDigest<<'|'<<p.payloadDigest;p.token=s.str();
}
struct FaultGpu:ClearWaterGpu {
 int fail=-1,commits=0,calls=0;Status Begin(const ClearWaterPacket&)override{calls++;return fail==0?Status::GL_RESOURCE_FAILED:Status::OK;}
 Status Layer(int i,const ClearWaterPacket&)override{calls++;return fail==i+1?Status::GL_DRAW_FAILED:Status::OK;}
 Status Present()override{calls++;return fail==7?Status::GL_DRAW_FAILED:Status::OK;}
 Status Swap(uint64_t)override{calls++;return fail==8?Status::SWAP_FAILED:Status::OK;}void Commit()override{commits++;}
};
int main(){
 std::ifstream in("test/reports/t226/canonical-frame.bin",std::ios::binary);assert(in);double v[24];in.read((char*)v,sizeof(v));ClearWaterPacket p;auto& f=p.frame;
 f.sessionId=v[0];f.frameId=v[1];f.snapshotId=v[2];f.surfaceEpoch=v[3];f.fieldEpoch=v[4];f.geometryEpoch=v[5];f.continuityEpoch=v[6];f.appearanceConfigRevision=v[7];f.lightConfigRevision=v[8];f.frameTimeNs=v[9];f.surfaceWidthPx=v[10];f.surfaceHeightPx=v[11];f.boxXPx=v[12];f.boxYPx=v[13];f.boxWidthPx=v[14];f.boxHeightPx=v[15];f.capacity=v[16];f.T_dataLayoutVersion=v[17];f.zOriginVersion=v[18];f.visualInverted=v[19];f.diagnosticEnabled=v[20];p.context=v[21];p.count=size_t(v[22]);p.capacity=size_t(v[23]);
 auto read=[&](void* data,size_t n){in.read((char*)data,n);assert(in);};read(p.header.data(),512);read(p.ledger.data(),512);read(p.optics.data(),128);read(p.profile.data(),8192);read(p.proof.data(),p.capacity*64);read(p.origin.data(),p.capacity*8);read(p.wait.data(),p.capacity);read(p.xy.data(),p.capacity*8);read(p.meta.data(),p.capacity*8);
 std::ifstream binding("test/reports/t226/canonical-binding.txt");std::getline(binding,p.run);std::getline(binding,p.payloadDigest);p.configDigest="offline-explicit-fixture";token(p);const char* stage;auto result=ValidateClearWater(p,stage);if(result!=Status::OK){std::cerr<<"canonical rejected "<<stage<<std::endl;return 1;}
 int passed=1;
 for(int mutation=0;mutation<12;mutation++){auto bad=p;switch(mutation){case 0:bad.proof[2]=65.;break;case 1:bad.profile[64*8+3]=0.;break;case 2:bad.xy[0]=65.f;break;case 3:bad.header[40]=-0.;break;case 4:bad.optics[6]=2;break;case 5:bad.frame.boxWidthPx=127;break;case 6:bad.optics[8]=9;break;case 7:bad.ledger[2]++;break;case 8:bad.origin[0]=0;break;case 9:bad.header[15]=31;break;case 10:bad.token="hash-only";break;case 11:bad.proof[8*bad.count]=1;break;}FaultGpu gpu;ClearWaterAtomic atomic;auto r=atomic.Draw(bad,gpu);assert(r.code!=Status::OK&&!r.success&&gpu.calls==0&&gpu.commits==0);passed++;}
 for(int fail=0;fail<=8;fail++){FaultGpu gpu;ClearWaterAtomic atomic;auto good=atomic.Draw(p,gpu);assert(good.success&&atomic.lastGood==p.token&&gpu.commits==1);auto next=p;next.frame.frameId++;next.header[2]++;next.frame.snapshotId++;digest(next);token(next);gpu.fail=fail;auto bad=atomic.Draw(next,gpu);assert(!bad.success&&bad.rendered.empty()&&gpu.commits==1&&atomic.lastGood==p.token);passed++;}
 // Analytic footprint oracle: flat, isolated wet column, vertical light -> F+(1-F)G=1.
 auto q=p;q.profile.fill(0);q.profile[8*10+1]=.25;q.profile[8*10+2]=.25;q.optics[4]=.35;q.optics[8]=1.5;q.optics[12]=.02;q.optics[13]=32;q.optics[14]=0;q.optics[15]=1;
 q.frame.boxWidthPx=256;q.frame.boxHeightPx=512;
 double area=0;for(int y=0;y<512;y++)for(int x=0;x<256;x++)area+=ClearWaterSurfaceAlpha(q,10,x,y)/.35;assert(std::abs(area-1.)<1e-12);passed++;
 q.frame.visualInverted=true;double mirror=0;for(int y=0;y<512;y++)for(int x=0;x<256;x++)mirror+=ClearWaterSurfaceAlpha(q,10,x,y)/.35;assert(std::abs(area-mirror)<1e-12);passed++;
 q.profile[8*10+1]=0;assert(ClearWaterSurfaceAlpha(q,10,0,0)==0);q.profile[8*10+1]=.25;q.optics[4]=0;assert(ClearWaterSurfaceAlpha(q,10,235,0)==0);passed++;
 // Fresnel at flat surface with light tangent has G=0, exact F0 contribution.
 q.frame.visualInverted=false;q.optics[4]=1;q.optics[14]=1;q.optics[15]=0;double alpha=ClearWaterSurfaceAlpha(q,10,20,511);assert(std::abs(alpha-.01)<1e-12);passed++;
 assert(ClearWaterTransmissionAlpha(10,0,10,1)==0);assert(ClearWaterTransmissionAlpha(10,20,31,1)==0);assert(ClearWaterTransmissionAlpha(10,20,15,0)==0);assert(ClearWaterTransmissionAlpha(10,20,5,.2)==1-std::exp(-4.));assert(ClearWaterTransmissionAlpha(10,20,15,.2)==1-std::exp(-3.));passed++;
 // Two adjacent half-width column rectangles contribute to one physical pixel.
 q.profile.fill(0);q.frame.boxXPx=.5;q.frame.boxWidthPx=128;q.optics[14]=0;q.optics[15]=1;q.profile[10*8+1]=q.profile[11*8+1]=1;q.profile[10*8+2]=q.profile[11*8+2]=1;
 double first=ClearWaterSurfaceAlpha(q,10,11,510),second=ClearWaterSurfaceAlpha(q,11,11,510);assert(std::abs(first-.5)<1e-12&&std::abs(second-.5)<1e-12);assert(first+second==1.);passed++;
 // Repeated frame ID / older publication is rejected before touching GPU.
 {FaultGpu gpu;ClearWaterAtomic atomic;assert(atomic.Draw(p,gpu).success);int calls=gpu.calls;auto stale=atomic.Draw(p,gpu);assert(stale.code==Status::STALE_FRAME&&gpu.calls==calls);passed++;}
 std::cout<<"{\"revision\":\"T226_NATIVE_PRODUCTION_PACKET_ATOMIC_ORACLE_V1\",\"passed\":"<<passed<<",\"failed\":0,\"device\":false,\"gpu\":\"NOT_MEASURED\",\"packetBanksBytes\":"<<2*sizeof(ClearWaterPacket)<<"}"<<std::endl;
}

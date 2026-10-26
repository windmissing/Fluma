#include "base_strip_uploader.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace p013;
struct Gpu final:BaseStripGpu {
 uint32_t next=1;int creates=0,deletes=0,uploads=0;size_t bytes=0;
 bool Create(uint32_t& t,int w)override {assert(w==128);t=next++;creates++;return true;}
 bool Upload(uint32_t t,int layer,int x,int pixels,const uint8_t* p)override {
  assert(t&&layer>=0&&layer<3&&x>=0&&x+pixels<=128&&p);uploads++;bytes+=size_t(pixels)*4;return true;
 }
 void Delete(uint32_t& t,bool)override {if(t)deletes++;t=0;}
};
static BaseStripDesc desc(uint64_t serial,const std::vector<uint8_t>& b){
 BaseStripDesc d;d.session=1;d.serial=serial;d.stateEpoch=1;d.commit=1;d.bindingId=1;d.geometry=1;
 d.context=1;d.surface=1;d.width=128;d.height=128;d.boxWidth=128;
 d.token="token"+std::to_string(serial);d.run="run";d.digest=std::string(64,'a');
 d.profileHash=std::string(64,'b');d.paletteHash=std::string(64,'c');
 d.payloadHash=MaterialSha256(b.data(),b.size());return d;
}
int main(){
 Gpu gpu;BaseStripUploader u(gpu);std::vector<uint8_t> b(128*12);
 for(size_t i=0;i<b.size();i+=4){b[i]=172;b[i+1]=1;b[i+2]=2;b[i+3]=1;}
 auto d=desc(1,b);assert(u.Stage(d,b.data(),b.size()).code==0);
 assert(u.Pump(0,1,1).code==400&&u.Inspect().uploadedBytes==0);
 assert(u.Pump(3,1,1).code==0&&u.Inspect().uploadedBytes==0&&gpu.creates==0);
 BaseStripLease l;assert(!u.Acquire(d,l));
 assert(u.Pump(511,1,1).code==0&&u.Inspect().uploadedBytes==508);
 while(u.Inspect().pendingBytes)assert(u.Pump(512,1,1).code==0);
 assert(gpu.bytes==b.size()&&gpu.creates==1&&u.Acquire(d,l));
 auto bad=d;bad.token="wrong";BaseStripLease other;assert(!u.Acquire(bad,other));
 u.Abort(l);assert(u.Inspect().activeSerial==0&&u.Acquire(d,l));u.CommitAfterSwap(l);
 assert(u.Inspect().activeSerial==1&&u.Inspect().pendingSerial==0);
 b[0]=173;auto d2=desc(2,b);assert(u.Stage(d2,b.data(),b.size()).code==0);
 while(u.Inspect().pendingBytes)assert(u.Pump(65536,1,1).code==0);
 assert(gpu.creates==2&&u.Inspect().textureLiveCount==2);
 assert(u.Acquire(d2,l));u.CommitAfterSwap(l);assert(u.Inspect().activeSerial==2);
 auto d3=desc(3,b);b[0]=249;d3.payloadHash=MaterialSha256(b.data(),b.size());
 assert(u.Stage(d3,b.data(),b.size()).code==400&&u.Inspect().activeSerial==2);
 u.ClearSession(true);assert(u.Inspect().textureLiveCount==0&&u.Inspect().cpuBytes==0);
 std::puts("T114 base strip core PASS");
}

// Actual G2 CPU lifecycle + synthetic GPU faults, no device/GL claims.
#include "material_uploader.h"
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <limits>
using namespace p013;
#define REQUIRE(x) do{if(!(x)){std::printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Gpu:MaterialGpu {
 uint32_t next=1;int live=0;bool fail=false;size_t bytes=0;std::vector<uint8_t> first;
 bool Create(std::array<uint32_t,2>& t)override{t={next++,next++};live+=2;return true;}
 bool Upload(uint32_t,int width,int layer,int row,int rows,const uint8_t* p)override{
  if(fail)return false;if(width!=256&&width!=64)throw std::runtime_error("width");
  if(row<0||row+rows>256||layer<0||layer>2)throw std::runtime_error("row/layer");
  if(first.empty())first.assign(p,p+width*4);bytes+=width*4*rows;return true;
 }
 void Delete(std::array<uint32_t,2>& t,bool)override{for(auto v:t)if(v)live--;t={};}
};
int main(){
 REQUIRE(MaterialSha256(nullptr,0)=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 std::vector<uint8_t> million(1000000,97);REQUIRE(MaterialSha256(million.data(),million.size())=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
 const char* abc="abc";REQUIRE(MaterialSha256(reinterpret_cast<const uint8_t*>(abc),3)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 std::vector<uint8_t> grain(kMaterialGrainBytes,128),strata(kMaterialStrataBytes,128);
 for(size_t i=3;i<grain.size();i+=4)grain[i]=255;for(size_t i=3;i<strata.size();i+=4)strata[i]=255;
 MaterialDesc d;d.generation=1;d.revision=8;d.surfaceEpoch=1;d.contextEpoch=1;d.bundleId="SYNTHETIC_TEST_ONLY";d.metadata="owned configuration";
 d.grainHash=MaterialSha256(grain.data(),grain.size());d.strataHash=MaterialSha256(strata.data(),strata.size());
 Gpu gpu;MaterialUploader u(gpu);
 REQUIRE(u.Stage(d,grain.data(),grain.size(),strata.data(),strata.size()).code==401);
 REQUIRE(u.BindConfiguration(1,8,std::string(64,'a')).code==0);
 auto stage=[&](){return u.Stage(d,grain.data(),grain.size(),strata.data(),strata.size());};
 REQUIRE(stage().state=="CPU_STAGED");
 auto original=d;auto originalBinding=u.BindConfiguration(1,8,std::string(64,'a'));
 REQUIRE(originalBinding.bindingId==1&&u.BindConfiguration(1,8,std::string(64,'b')).code==402);
 d.revision=7;REQUIRE(stage().code==402);d=original;
 auto idempotent=u.Pump(1024,1,1);REQUIRE(stage().pendingBytes==idempotent.pendingBytes);
 d.bundleId="different";REQUIRE(stage().errorStage=="identity");d=original;
 d.seed=24;REQUIRE(stage().errorStage=="identity");d=original;
 grain[0]=129;d.grainHash=MaterialSha256(grain.data(),grain.size());REQUIRE(stage().errorStage=="identity");grain[0]=128;d=original;
 // New target changes activation eligibility without destroying pending/active.
 REQUIRE(u.BindConfiguration(1,9,std::string(64,'c')).code==0);REQUIRE(!u.CanActivate(1,1));
 REQUIRE(u.BindConfiguration(1,8,std::string(64,'a')).code==0);
 // A rebound identity cannot be reused as same-generation restage.
 REQUIRE(stage().errorStage=="identity");u.ClearSession();REQUIRE(u.BindConfiguration(1,8,std::string(64,'a')).code==0);REQUIRE(stage().state=="CPU_STAGED");gpu.bytes=0;gpu.first.clear();grain[0]=7;
 auto r=u.Pump(65536,1,1);REQUIRE(r.uploadedBytes==65536&&gpu.first[0]==128);grain[0]=128;
 REQUIRE(!u.CanActivate(1,1));REQUIRE(u.Activate(1,1,"F1").code==401);
 while((r=u.Pump(65536,1,1)).state!="READY")REQUIRE(r.code==0&&r.uploadedBytes<=65536);
 REQUIRE(gpu.bytes==kMaterialBytes&&u.CanActivate(1,1));
 r=u.Activate(1,1,"F1");REQUIRE(r.code==0&&r.activeGeneration==1&&r.frameToken=="F1"&&gpu.live==2);
 std::puts("PASS TC-P013-G2-C01 C03 D01 D02 synchronous ownership and complete-row upload");
 auto invalid=d;invalid.grainHash="bad";REQUIRE(u.Stage(invalid,grain.data(),grain.size(),strata.data(),strata.size()).code==400);REQUIRE(u.ActiveDesc()->generation==1);
 REQUIRE(u.Stage(d,grain.data(),grain.size()-4,strata.data(),strata.size()).errorStage=="byteLength");
 grain[3]=0;invalid=d;invalid.grainHash=MaterialSha256(grain.data(),grain.size());REQUIRE(u.Stage(invalid,grain.data(),grain.size(),strata.data(),strata.size()).errorStage=="alpha");grain[3]=255;
 std::puts("PASS TC-P013-G2-A06 A08 A09 B02 failed stage preserves active");
 d.generation=2;REQUIRE(stage().code==0);u.Pump(1024,1,1);REQUIRE(gpu.live==4);
 d.generation=3;r=stage();REQUIRE(r.discardedGeneration==2&&r.activeGeneration==1&&gpu.live==2);
 for(int i=0;i<6;i++){u.Pump(1024,1,1);REQUIRE(gpu.live<=4);d.generation++;stage();REQUIRE(gpu.live==2);}
 std::puts("PASS TC-P013-G2-B01 B03 pending replacement explicit and bounded");
 auto before=u.Inspect();for(double budget:{0.,-1.,1.5,65537.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){auto bad=u.Pump(budget,99,99);REQUIRE(bad.code==400&&bad.errorStage=="byteBudget"&&bad.uploadedBytes==0&&bad.pumpCount==before.pumpCount&&u.Inspect().receivedBytes==before.receivedBytes&&u.Inspect().state==before.state);}
 REQUIRE(u.Pump(1,1,1).uploadedBytes==0);REQUIRE(u.Pump(1024,1,1).uploadedBytes==1024);
 std::puts("PASS C02 structured invalid budget receipts; pending retained");
 r=u.Pump(65536,2,1);REQUIRE(r.state=="DISCARDED"&&r.uploadedBytes==0&&u.ActiveDesc()->generation==1);
 std::puts("PASS TC-P013-G2-C04 epoch mismatch drops pending");
 d.surfaceEpoch=2;d.contextEpoch=2;d.generation++;stage();gpu.fail=true;r=u.Pump(65536,2,2);REQUIRE(r.code==301&&r.state=="FAILED"&&u.ActiveDesc()->generation==1);gpu.fail=false;u.Discard(d.generation);
 std::puts("PASS TC-P013-G2-D03 GL failure retains active");
 u.Reset(false);REQUIRE(gpu.live==0&&u.Inspect().cpuBytes==kMaterialBytes&&!u.BindActive(1));
 REQUIRE(u.Restore(3,3).state=="CPU_STAGED");REQUIRE(!u.CanActivate(1,1));
 do{r=u.Pump(65536,3,3);REQUIRE(r.code==0);}while(r.state!="READY");
 REQUIRE(u.CanActivate(1,3));REQUIRE(u.Activate(1,3,"F-restored").code==0);REQUIRE(u.BindActive(3)&&!u.BindActive(1));
 REQUIRE(u.ActiveDesc()->revision==8&&u.ActiveDesc()->metadata=="owned configuration");
 std::puts("PASS TC-P013-G2-B04 CPU cache restore and same-generation configuration");
 u.Reset(true);REQUIRE(gpu.live==0);
 { Gpu isolatedGpu;MaterialUploader other(isolatedGpu);auto target=d;target.generation=1;target.surfaceEpoch=1;target.contextEpoch=1;
 REQUIRE(other.BindConfiguration(2,8,std::string(64,'a')).code==0);
 REQUIRE(other.Stage(target,grain.data(),grain.size(),strata.data(),strata.size()).code==0);
 while(other.Pump(65536,1,1).state!="READY"){}
 REQUIRE(other.BindConfiguration(2,9,std::string(64,'b')).code==0);
 REQUIRE(!other.CanActivate(1,1)&&other.Activate(1,1,"reject").code==402);
 auto mismatch=other.Stage(target,grain.data(),grain.size(),strata.data(),strata.size());
 REQUIRE(mismatch.code==402&&mismatch.requestedRevision==8&&mismatch.boundRevision==9);
 REQUIRE(other.Inspect().state=="READY");other.ClearSession();REQUIRE(other.Inspect().cpuBytes==0&&isolatedGpu.live==0);
 }
 std::puts("PASS G2 native core all checks; synthetic bytes/GPU only");return 0;
}

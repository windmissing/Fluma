#include "scene_compositor.h"
#include <cassert>
#include <cstdio>
using namespace p013;
struct AssetGpu final:MaterialGpu {uint32_t n=1;bool Create(std::array<uint32_t,2>& t)override{t={n++,n++};return true;}
 bool Upload(uint32_t,int,int,int,int,const uint8_t*)override{return true;}
 void Delete(std::array<uint32_t,2>& t,bool)override{t={};}};
struct StripGpu final:BaseStripGpu {uint32_t n=100;bool Create(uint32_t& t,int w)override{assert(w==160);t=n++;return true;}
 bool Upload(uint32_t,int,int,int,const uint8_t*)override{return true;}
 void Delete(uint32_t& t,bool)override{t=0;}};
struct DrawGpu final:SceneGpu {bool failSwap=false;uint32_t strip=0;int commits=0;
 void SetBaseStripTexture(uint32_t t)override{strip=t;}
 Status Begin(const ScenePacket&)override{return strip?Status::OK:Status::NOT_READY;}
 Status Layer(int,const ScenePacket&,const SceneConfiguration&,const SceneLease*)override{return Status::OK;}
 Status Present(bool)override{return Status::OK;}
 Status Swap(uint64_t)override{return failSwap?Status::SWAP_FAILED:Status::OK;}
 void CommitTargets()noexcept override{commits++;}};
int main(){
 AssetGpu ag;StripGpu sg;DrawGpu draw;MaterialUploader assets(ag);BaseStripUploader strips(sg);SceneCompositor scene;
 SceneConfiguration c;c.revision=1;c.digest=std::string(64,'a');auto bind=assets.BindConfiguration(1,1,c.digest);c.bindingId=bind.bindingId;
 auto& v=c.values;v[0]=1;v[1]=3;v[3]=24;v[4]=v[5]=1;v[8]=128;v[9]=256;v[10]=v[11]=v[12]=1;
 v[13]=v[14]=256./11;v[15]=.25;v[16]=v[17]=v[18]=v[19]=1;v[50]=20261026;
 const std::array<double,6> palette{145,168,111,216,196,150};for(int i=0;i<6;i++){v[29+i]=palette[i]/255.;v[51+i]=palette[i];}
 double light[]={.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};for(int i=0;i<10;i++)v[35+i]=light[i];
 assert(scene.Register(c));
 std::vector<uint8_t> grain(kMaterialGrainBytes,128),strata(kMaterialStrataBytes,128);
 for(size_t i=3;i<grain.size();i+=4)grain[i]=255;
 for(size_t i=3;i<strata.size();i+=4)strata[i]=255;
 MaterialDesc md;md.generation=md.revision=md.surfaceEpoch=md.contextEpoch=1;md.bundleId="asset";md.metadata="test";
 md.lambdaMaterial=md.lambdaCanvas=256./11;md.warpRange=.25;md.seed=24;md.geometryEpoch=1;md.grainBase=c.Params().grainBaseSrgb;
 md.grainHash=MaterialSha256(grain.data(),grain.size());md.strataHash=MaterialSha256(strata.data(),strata.size());
 assert(assets.Stage(md,grain.data(),grain.size(),strata.data(),strata.size()).code==0);
 while(assets.Pump(65536,1,1).state!="READY"){}
 ScenePacket p;p.baseStripV2=true;p.frame.sessionId=1;p.frame.frameId=1;p.frame.surfaceEpoch=1;
 p.frame.geometryEpoch=p.frame.continuityEpoch=p.frame.appearanceConfigRevision=1;
 p.frame.surfaceWidthPx=160;p.frame.surfaceHeightPx=288;p.frame.boxWidthPx=128;p.frame.boxHeightPx=256;
 p.run="run";p.contextEpoch=p.stateEpoch=p.commit=1;p.requestedGeneration=1;p.capacity=1;p.xy.resize(2);p.meta.resize(2);
 std::vector<uint8_t> bytes(160*12);for(int k=0;k<3;k++)for(int j=0;j<128;j++){
  auto i=(k*160+j)*4;bytes[i]=145;bytes[i+1]=168;bytes[i+2]=111;bytes[i+3]=1;}
 BaseStripDesc d;d.session=1;d.stateEpoch=1;d.commit=1;d.bindingId=c.bindingId;d.geometry=1;d.context=1;d.surface=1;
 d.width=160;d.height=288;d.boxWidth=128;d.spark=true;d.run="run";d.digest=c.digest;
 d.profileHash=std::string(64,'b');d.paletteHash=MaterialSha256(reinterpret_cast<const uint8_t*>(palette.data()),sizeof(palette));
 d.payloadHash=MaterialSha256(bytes.data(),bytes.size());
 auto stage=[&](uint64_t serial,const char* token){d.serial=serial;d.token=token;p.frame.frameId=serial;p.token=token;p.baseStrip=d;
  assert(strips.Stage(d,bytes.data(),bytes.size()).code==0);assert(strips.Pump(65536,1,1).pendingBytes==0);};
 stage(1,"one");draw.failSwap=true;auto r=scene.Draw(p,assets,draw,&strips);
 assert(!r.current&&assets.Inspect().activeGeneration==0&&strips.Inspect().activeSerial==0&&draw.commits==0);
 stage(2,"two");draw.failSwap=false;r=scene.Draw(p,assets,draw,&strips);
 assert(r.current&&r.actualStripSerial==2&&assets.Inspect().activeGeneration==1&&strips.Inspect().activeSerial==2&&draw.commits==1);
 stage(3,"three");p.baseStrip.profileHash="bad";r=scene.Draw(p,assets,draw,&strips);
 assert(!r.current&&r.stage=="BASE_STRIP_NOT_READY"&&strips.Inspect().activeSerial==2);
 strips.ClearSession(true);assets.ClearSession();std::puts("T114 scene dual lease atomic PASS");
}

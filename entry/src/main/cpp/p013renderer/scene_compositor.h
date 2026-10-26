// T56 / T68: all layers are one transaction; no Canvas or borrowed JS data.
#pragma once
#include "material_uploader.h"
#include "material_pass.h"
#include "base_strip_uploader.h"
#include <memory>
namespace p013 {
constexpr size_t kSceneConfigValues=57,kSceneDataFloats=2048;
struct SceneConfiguration {
 std::array<double,kSceneConfigValues> values{};
 uint64_t bindingId=0,revision=0;std::string digest;
 bool Material()const{return values[0]!=0;}
 MaterialParams Params()const;
 LightParams Light()const;
};
struct ScenePacket {
 FrameInput frame;uint64_t contextEpoch=0,stateEpoch=0,commit=0,requestedGeneration=0;
 bool baseStripV2=false;BaseStripDesc baseStrip;
 std::string token,run,ledgerKey,route,medium;
 size_t count=0,capacity=0;bool particleEnabled=true;
 std::array<float,kSceneDataFloats> data{};
 std::array<double,10> motion{}; // offsets[3], bandMin[3], bandMax[3], fogPhase
 std::vector<float> xy;std::vector<uint32_t> meta;
};
struct SceneSource {
 std::string token,run,ledgerKey,pipeline,route,medium;uint64_t generation=0,revision=0,lightRevision=0,frameId=0,fieldEpoch=0,context=0,surface=0,geometry=0,continuity=0,stateEpoch=0,commit=0;
 int width=0,height=0;std::array<double,4> box{};bool invert=false;
 uint64_t stripSerial=0;std::string stripProfileHash,stripPayloadHash;
};
struct SceneReceipt {
 int code=1;std::string state="NO_VALID_FRAME",fault="NOT_READY",stage="NOT_READY";
 SceneSource requested,rendered,lastGood;
 bool current=false,requestedSwap=false,staleSwap=false;uint64_t actualGeneration=0;
 uint64_t actualStripSerial=0;
};
class SceneGpu {
 public:virtual ~SceneGpu()=default;
 virtual const char* FailureStage()const{return "SCENE_TARGETS";}
 virtual Status Begin(const ScenePacket&)=0;
 virtual Status Layer(int,const ScenePacket&,const SceneConfiguration&,const SceneLease*)=0;
 virtual Status Present(bool lastGood)=0;
 virtual Status Swap(uint64_t)=0;
 virtual void SetBaseStripTexture(uint32_t){}
 virtual void CommitTargets() noexcept=0;
};
class SceneCompositor {
 public:
 bool Register(const SceneConfiguration&);
 const SceneConfiguration* Desired()const{return desired_.get();}
 SceneReceipt Draw(const ScenePacket&,MaterialUploader&,SceneGpu&,BaseStripUploader* strip=nullptr);
 SceneReceipt Reject(const ScenePacket&,int,const char*,SceneGpu*,bool replayAllowed);
 void ResetGpu() noexcept {valid_=false;}
 void ClearSession() noexcept {valid_=false;active_.reset();desired_.reset();last_=SceneSource{};lastFrame_=0;}
 bool HasLastGood()const{return valid_;}
 private:
 bool Compatible(const ScenePacket&)const;
 SceneSource Source(const ScenePacket&,uint64_t,uint64_t)const;
 std::shared_ptr<const SceneConfiguration> desired_,active_;
 SceneSource last_;bool valid_=false;uint64_t lastFrame_=0;
};
}

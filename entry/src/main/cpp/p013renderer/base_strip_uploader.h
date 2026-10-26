#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "material_uploader.h"
namespace p013 {
struct BaseStripDesc {
 uint64_t session=0,serial=0,stateEpoch=0,commit=0,bindingId=0,geometry=0,context=0,surface=0;
 int originX=0,originY=0,width=0,height=0;double boxX=0,boxWidth=0;
 bool mirror=false,spark=false;
 std::string token,run,digest,profileHash,paletteHash,payloadHash;
 std::string representation="P013_NATIVE_X_F64_BASEBYTE_V2",evaluator="P1_FROZEN_F64_EVAL_V1",paletteRevision="P1_ENCODED_PALETTE_F64_V1";
};
struct BaseStripReceipt {int code=0;const char* stage="NONE";const char* state="DISCARDED";uint64_t activeSerial=0,pendingSerial=0,discardedSerial=0,restoreCount=0;
 size_t uploadedBytes=0,pendingBytes=0,cpuBytes=0,gpuBytes=0,textureLiveCount=0;};
class BaseStripGpu {
 public: virtual ~BaseStripGpu()=default;
 virtual bool Create(uint32_t&,int)=0;
 virtual bool Upload(uint32_t,int,int,int,const uint8_t*)=0;
 virtual void Delete(uint32_t&,bool)=0;
};
class BaseStripGlGpu final: public BaseStripGpu {
 public: bool Create(uint32_t&,int) override;
 bool Upload(uint32_t,int,int,int,const uint8_t*) override;
 void Delete(uint32_t&,bool) override;
};
struct BaseStripLease {const BaseStripDesc* desc=nullptr;uint32_t texture=0;bool pending=false;};
class BaseStripUploader {
 public:
 explicit BaseStripUploader(BaseStripGpu& gpu):gpu_(gpu){}
 BaseStripReceipt Stage(const BaseStripDesc&,const uint8_t*,size_t);
 BaseStripReceipt Pump(double,uint64_t,uint64_t);
 bool Acquire(const BaseStripDesc&,BaseStripLease&);
 void CommitAfterSwap(BaseStripLease&) noexcept;
 void Abort(BaseStripLease&) noexcept;
 void Reset(bool);void ClearSession(bool);
 BaseStripReceipt Inspect()const;
 const BaseStripDesc* PendingDesc()const{return pending_.valid?&pending_.desc:nullptr;}
 private:
 struct Slot {BaseStripDesc desc;std::vector<uint8_t> bytes;size_t uploaded=0;bool ready=false;bool valid=false;};
 BaseStripReceipt Result(int=0,const char* stage="NONE")const;
 BaseStripGpu& gpu_;Slot active_,pending_;
 uint32_t textures_[2]{};int textureWidth_[2]{};int activeSlot_=-1,pendingSlot_=0;uint64_t context_=0,discarded_=0,restoreCount_=0;
 bool held_=false;
};
}

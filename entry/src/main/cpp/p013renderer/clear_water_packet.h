#pragma once
#include "render_contract.h"
#include <string>
namespace p013 {
// Separate V3 proof namespace, fixed cold capacity; no V1/V2 parser reinterpretation.
struct ClearWaterPacket {
 FrameInput frame;uint64_t context=0;size_t count=0,capacity=0;
 std::string token,run,configDigest,payloadDigest;double dpi=1;
 std::array<double,1024> profile{};std::array<double,4096> proof{};
 std::array<double,512> origin{};std::array<uint8_t,512> wait{};
 std::array<float,1024> xy{};std::array<uint32_t,1024> meta{};
 std::array<double,64> header{},ledger{};std::array<double,16> optics{};
};
Status ValidateClearWater(const ClearWaterPacket&,const char*&);
// Area of physical pixel intersecting the water surface band; CPU oracle helper.
double ClearWaterTransmissionAlpha(double S,double W,double y,double sigma);
double ClearWaterSurfaceAlpha(const ClearWaterPacket&,int column,double pixelX,double pixelY);
class ClearWaterGpu {
 public:virtual ~ClearWaterGpu()=default;
 virtual Status Begin(const ClearWaterPacket&)=0;virtual Status Layer(int,const ClearWaterPacket&)=0;
 virtual Status Present()=0;virtual Status Swap(uint64_t)=0;virtual void Commit()=0;
};
struct ClearWaterReceipt {Status code=Status::NOT_READY;const char* stage="NOT_READY";bool success=false;std::string rendered;};
class ClearWaterAtomic {
 public:ClearWaterReceipt Draw(const ClearWaterPacket&,ClearWaterGpu&);void Reset(){lastGood.clear();lastAttempt=0;lastOwner=lastRun=lastCommit=lastPublication=0;}std::string lastGood;double lastAttempt=0,lastOwner=0,lastRun=0,lastCommit=0,lastPublication=0;
};
}

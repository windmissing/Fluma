// T54 G2 isolated uploader. No page or physics dependencies.
#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace p013 {
enum class MaterialStatus : int {
 MATERIAL_PACKET_INVALID=400, ASSET_NOT_READY=401, ASSET_REVISION_MISMATCH=402,
 COLORSPACE_UNSUPPORTED=403, COMPOSITION_ORDER_BLOCKED=404
};
constexpr size_t kMaterialGrainBytes=786432, kMaterialStrataBytes=196608, kMaterialBytes=983040;
struct MaterialDesc {
 uint64_t generation=0, revision=0, surfaceEpoch=0, contextEpoch=0;
 std::string bundleId, grainHash, strataHash, metadata;
 std::array<double,9> grainBase{};
 double lambdaMaterial=0,lambdaCanvas=0,warpRange=0,seed=0,geometryEpoch=0;
 std::string paletteHash,generatorVersion;
};
struct MaterialConfigReceipt {
 int code=0; uint64_t sessionId=0,bindingId=0,revision=0; std::string digest,errorStage="NONE";
};
struct MaterialReceipt {
 uint64_t requestedRevision=0,boundRevision=0,bindingId=0;std::string configurationDigest;
 int code=0; uint64_t assetGeneration=0, activeGeneration=0, appearanceConfigRevision=0, contextEpoch=0;
 std::string state="DISCARDED", errorStage="NONE", frameToken;
 size_t receivedBytes=0, uploadedBytes=0, totalBytes=kMaterialBytes, pendingBytes=0, cpuBytes=0;
 uint64_t pumpCount=0, discardedGeneration=0, pendingGeneration=0; size_t textureLiveCount=0;
};
class MaterialGpu {
 public:
 virtual ~MaterialGpu()=default;
 virtual bool Create(std::array<uint32_t,2>&)=0;
 virtual bool Upload(uint32_t texture,int width,int layer,int row,int rows,const uint8_t*)=0;
 virtual void Delete(std::array<uint32_t,2>&,bool current)=0;
};
std::string MaterialSha256(const uint8_t*,size_t);
class MaterialGlGpu final: public MaterialGpu {
 public: bool Create(std::array<uint32_t,2>&) override;
 bool Upload(uint32_t,int,int,int,int,const uint8_t*) override;
 void Delete(std::array<uint32_t,2>&,bool) override;
};
struct SceneLease { const MaterialDesc* desc=nullptr; const std::array<uint32_t,2>* textures=nullptr;
 uint64_t bindingId=0;std::string bindingDigest,frameToken,committedState="ACTIVE";bool pending=false; };
class MaterialUploader {
 public:
 explicit MaterialUploader(MaterialGpu& gpu):gpu_(gpu){}
 MaterialConfigReceipt BindConfiguration(uint64_t session,uint64_t revision,const std::string& digest);
 MaterialReceipt CheckConfiguration(const MaterialDesc&) const;
 MaterialReceipt InvalidBudget() const;
 void ClearSession();
 bool AcquireSceneLease(uint64_t generation,uint64_t bindingId,const std::string& token,uint64_t surface,uint64_t context,SceneLease&);
 void CommitSceneLeaseAfterSwap(SceneLease&) noexcept;
 void AbortSceneLease(SceneLease&,const char*) noexcept;
 MaterialReceipt Stage(const MaterialDesc&,const uint8_t*,size_t,const uint8_t*,size_t);
 MaterialReceipt Pump(double,uint64_t surfaceEpoch,uint64_t contextEpoch);
 bool TargetMatches(uint64_t surfaceEpoch,uint64_t contextEpoch) const;
 bool CanActivate(uint64_t generation,uint64_t contextEpoch) const;
 MaterialReceipt Activate(uint64_t generation,uint64_t contextEpoch,const std::string& frameToken);
 MaterialReceipt Discard(uint64_t generation);
 // Serial owner completes each frame before another API call: no old frame is in flight at activation.
 void Reset(bool contextCurrent); // keep CPU active for restoration
 MaterialReceipt Restore(uint64_t surfaceEpoch,uint64_t contextEpoch);
 MaterialReceipt Inspect() const;
 const MaterialDesc* ActiveDesc() const;
 const MaterialDesc* PendingDesc() const;
 const std::array<uint32_t,2>* BindActive(uint64_t contextEpoch) const;
 private:
 struct Generation { MaterialDesc desc; std::vector<uint8_t> grain,strata; std::array<uint32_t,2> textures{};
 uint64_t bindingId=0;std::string fingerprint,bindingDigest;
 size_t uploaded=0; std::string state="CPU_STAGED",token; };
 MaterialReceipt Receipt(const Generation*,int=0,const char* = "NONE",size_t=0) const;
 void Drop(std::unique_ptr<Generation>&,bool=true);
 uint64_t boundSession_=0,bindingId_=0,boundRevision_=0,highWater_=0;std::string boundDigest_;
 bool sceneLeaseHeld_=false;
 MaterialGpu& gpu_; std::unique_ptr<Generation> active_,pending_; uint64_t pumps_=0,discarded_=0;
};
}

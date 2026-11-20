#pragma once
#include "transport.h"
#include "planning_control.h"
#include "quota_reader_v3.h"
#include "input_gate.h"
#include "cold_trace.h"
#include <mutex>
#include <string>
namespace grs::g9dm {
class Controller {
 mutable std::recursive_mutex mutex_;
 StageCapabilityV2 caps_{};Transport transport_;Config config_;const Profile* profile_=nullptr;ControlIdentity identity_{};
 ProgressClockV3 clock_;RunControl run_;PlanningControl planning_{clock_,run_};InputGate inputGate_;
 struct Queued {Input128 input{};SourceIntentKey key{};uint64_t intent=0;};
 Input128 consumedSource_{},consumedImpulse_{};bool hasConsumedSource_=false,hasConsumedImpulse_=false;
 std::array<Queued,16> queue_{};size_t head_=0,size_=0;
 Permit active_{};uint64_t nonce_=0,frame_=0,commit_=0,quanta_=0,generation_=0;
 bool configured_=false,rootLive_=false,lastGood_=false,stale_=false,lease_=false,coldBuilding_=false;
 GLsync sceneFence_=nullptr;Completion256 lastTail_{};
 coldtrace::TraceManager coldTrace_;uint64_t coldEpoch_=1;
 cold503::Scope ColdScopeLocked()const;
 std::array<uint32_t,8> layoutSha_{},shaderSha_{};
 static bool Check(void*,const Permit&,uint64_t);bool RecheckLocked(const Permit&,uint64_t)const;
 ControlResult RevokeLocked(ControlResult);
public:
 Result ConfigureResearch(const Config&,const ControlIdentity&,const SeedFileIdentity&);
 Result OfferInput128(Input128,uint64_t now,uint64_t sourceIntent);
 Result OfferModelCommand(const Input128&,uint64_t now,uint64_t sourceIntent);
 ControlResult Pulse(const PagePulseV3&,uint64_t now);
 ControlResult Target(const ControlIdentity&,int32_t,uint64_t now);
 Result BeginRoot(uint64_t now,uint64_t frame,uint64_t geometry);
 Result PumpBounded(uint64_t now);
 QuotaSnapshotV3 Quota(uint64_t now)const;
 Result Stop();bool Drain();void Release(bool current);
 struct SceneView {bool valid=false,stale=false;GLuint buffer=0;uint64_t generation=0;ControlIdentity identity{};uint32_t k=0;};
 SceneView AcquireScene();bool ReleaseScene(uint64_t,GLsync);bool PollSceneFence();
 // T564: the packet draw reads profile.state[6] for the packet64 byte start.
 const Profile* TransportProfile()const{return profile_;}
 // T564: cold capability record for this controller (compute + scene domains).
 const StageCapabilityV2& Caps()const{return caps_;}
 bool CanPresent(uint64_t,uint64_t now)const;
 Completion256 LastCompletion()const;
#ifdef GRS_G9_BOOT_SOURCE
 bool ExportSourceCold(uint64_t trace, uint64_t now, bool (*write)(void*, const uint32_t*, size_t) noexcept, void*);
 GLenum PrepareSourceCache();
#endif
 std::string ColdCapabilities()const;
 std::string ColdBegin(uint64_t replace,uint64_t now);std::string ColdState(uint64_t,uint64_t now);std::string ColdPage(uint64_t,uint32_t,uint64_t now);std::string ColdEnd(uint64_t,uint64_t now);void BuildCold(uint64_t now);
};
}

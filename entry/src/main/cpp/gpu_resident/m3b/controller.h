#pragma once
#include "transport.h"
#include "root_owner.h"
#include "publication_lease.h"
#include "scene_source.h"
#include "cold_trace.h"
#include <mutex>
namespace grs::m3b {
class ControllerV5:public SceneSourceV5 {
 mutable std::mutex mutex_;
 ProgressClockV3 clock_;LiftInputGate liftInput_;LiftParameters liftParameters_;
 RunControl run_;
 RootOwnerV5 owner_{clock_,run_};
 SourceQueueV5 queue_;
 TransportV5 transport_;
 PublicationLeasesV5 leases_;
 const Profile* profile_=nullptr;
 RootPermitV5 active_;
 ProgressClockV3::Intent sourceIntent_;
 uint64_t frame_=0,control_=0;
 bool configured_=false;
 Words256 sceneHeader_{};
 Identity sceneIdentity_{};
 std::array<PublicationTokenV5,2> drawTokens_{};
 std::array<GLsync,2> releaseFences_{};
 coldtrace::TraceManager coldTrace_;
 bool coldBuilding_=false,coldPrepared_=false;
 uint64_t coldEpoch_=1;
 cold503::Scope ColdScopeLocked()const;
 static bool Authorize(void*,const RootPermitV5&,uint64_t);
 ControlResult Fail(ControlResult);
public:
 uint64_t SourceGeneration()const{std::lock_guard<std::mutex> lock(mutex_);return clock_.sourceControlGeneration;}
 ControlResult Configure(const Profile&,const ControlIdentityV5&,uint64_t geometry,const PhysicalConfigV5&,
                         const std::array<uint32_t,8>& shader,const std::array<uint32_t,8>& angle,const std::array<uint32_t,8>& layout);
 InputResult OfferLiftInput(const LiftInput64&,uint64_t receipt);
 ControlResult ResumeLiftSession(const ControlIdentityV5&);
 ControlResult Pulse(const PagePulseV3&,uint64_t now);
 ControlResult Target(const ControlIdentityV5&,int32_t effective);
 ControlResult Mode(const ControlIdentityV5&,GravityMode);
 ControlResult Offer(const ControlIdentityV5&,const SourceIntentKeyV5&,const SourcePacketV5&);
 ControlResult Advance(uint64_t now,uint64_t frame,uint64_t control);
 TransportResultV5 Pump(uint64_t now);
 QuotaSnapshotV3 Quota(uint64_t now)const;
 ControlResult Stop();
 // Called only on the GL owner after its real old fence/context is retired.
 bool DrainRevoked();
 void Release(bool contextCurrent);
 Words256 SceneHeaderV5()const override;
 ControlCode AcquireV5(uint32_t,U64,PublicationLease&,GLuint&) override;
 GLuint StateBufferV5(const PublicationLease&)const override;
 ControlCode ReleaseV5(uint32_t,U64,GLsync) override;
 bool PollSceneReleases();
 struct View {bool valid=false;uint32_t slot=0;uint64_t generation=0;};
 View PublicationView()const;
 bool CanPresent(uint64_t generation,uint64_t now)const;
 std::string ColdBegin(uint64_t replace,uint64_t now);
 std::string ColdState(uint64_t request,uint64_t now);
 std::string ColdPage(uint64_t trace,uint32_t page,uint64_t now);
 std::string ColdEnd(uint64_t trace,uint64_t now);
 std::string ColdCapabilities()const;
 std::string SourceStats()const;
 void BuildCold(uint64_t now);
 struct RootDiagnostic {RootPermitV5 permit;Words256 completion;NativeRootEvidence evidence;ControlResult fault=ControlResult::OK;};
 // GL owner only; UI cold APIs use the bounded snapshot collector instead.
 RootDiagnostic DiagnosticOwnerThread()const;
};
}

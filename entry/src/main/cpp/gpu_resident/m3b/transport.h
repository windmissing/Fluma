#pragma once
#include "buffer_plan.h"
#include "completion_gate.h"
#include "source_queue_v5.h"
#include "lift_input.h"
#include "../uniform_locations.h"
#include <EGL/egl.h>
#include <thread>
namespace grs::m3b {
struct PhysicalConfigV5 {
 uint32_t omega=0,waterRate=8388608,gravity=1048576,settle=65536,emit=1048576,kAir=0,kFlight=0,kWet=120,soft=524288,flux=1048576;
 bool omegaProvided=false;
 LiftParameters lift;
 bool fullBranchTrace=false;
 bool Valid()const{return lift.Valid()&&omegaProvided&&omega<=1966080&&waterRate<=1006632960&&gravity>=120&&gravity<=1073741824&&settle&&settle<=1073741824&&emit<=1073741824&&kAir<=960&&kFlight<=960&&kWet&&kWet<=960&&soft<605394;}
};
enum class TransportResultV5 {OK,IN_PROGRESS,WOULD_BLOCK,UNSUPPORTED,INVALID,STALE,DEADLINE,GL_FAILED,SHADER_FAILED,CONTEXT_LOST};
class TransportV5 {
public:
 using Authorize=bool(*)(void*,const RootPermitV5&,uint64_t);
 TransportV5()=default;TransportV5(const TransportV5&)=delete;TransportV5& operator=(const TransportV5&)=delete;
 TransportResultV5 Create(const Profile&,const ControlIdentityV5&,uint64_t geometry,const PhysicalConfigV5&,
                           const std::array<uint32_t,8>& shaderSha,const std::array<uint32_t,8>& angleSha,const std::array<uint32_t,8>& layoutSha);
 TransportResultV5 Begin(const RootPermitV5&,int32_t frozenTarget,const SourcePacketV5*,uint32_t sourcePrefix,uint32_t dedupPrefix,Authorize,void*);
 TransportResultV5 Pump();
 // Commit/Reject is called after the single mapped tail passes completion_gate
 // and owner-lock acceptance. GPU workspace remains quarantined until then.
 bool Commit(const RootPermitV5&,const Words256&);
 bool RetireRejected(const RootPermitV5&);
 void Release(bool contextCurrent);
 void SetLiftInput(const LiftInput64& words){liftInput_=words;}
 GLuint StateBuffer(uint32_t bank)const{return bank<2?buffers_[bank]:0;}
 bool Busy()const{return busy_;}
 uint32_t PlannedUpload(uint32_t kind);
 const Words256& Tail()const{return tail_;}
 const NativeRootEvidence& Evidence()const{return evidence_;}
 GLuint Publication(uint32_t slot)const{return slot<2?buffers_[3+slot]:0;}
 size_t PublicationWords()const{return old_?old_->pubWords:0;}
 size_t StorageAlignment()const{return alignment_;}
 bool FullBranchMode()const{return config_.fullBranchTrace;}
 bool FullTraceNeedsFreeze()const{return config_.fullBranchTrace&&fullPlan_!=0&&!fullFrozen_;}
 void TraceFrozen(){fullFrozen_=true;}
 bool HasColdTail()const{return !busy_&&tail_[0]==1381192263&&tail_[1]==4&&tail_[28]!=4;}
 GLenum FirstGlError()const{return rawGl_;}
 bool CaptureColdV5(uint32_t* reserved,size_t capacity,const QuotaSnapshotV3&,size_t& stateWords,size_t& eventWords);
 // T571 (problem T527/T552): FULL-plan capture state — which of the 20 scope
 // pairs and 4 quanta actually executed in this plan, and whether the frozen
 // completion copies exist. BASE anchors have no persistent resident object;
 // their descriptors are honest NOT_EXECUTED unless a future change adds one.
 uint32_t FullScopeMask()const{return fullMask_;}
 uint32_t FullQuantumMask()const{return fullQuantumMask_;}
 bool FullFrozen()const{return fullFrozen_;}
 uint32_t CurrentStateBank()const{return currentBank_;}
 uint32_t SourcePrefix()const{return sourcePrefix_;}
 size_t SourceFrameBytes()const{return profile_?size_t(profile_->k+1)*256:0;}
 size_t CombinedEventBytes()const{return layout_.combinedBytes;}
 EGLint FirstEglError()const{return rawEgl_;}
private:
 struct Program {GLuint id=0;UniformLocations locations;};
 std::array<Program,20> programs_{};
 std::array<GLuint,28> buffers_{};
 std::array<GLuint,21> fullBuffers_{};
 const Profile* profile_=nullptr;const m2::Profile* old_=nullptr;WorkLayout layout_;ResidentBuffersV5 allocation_;
 m2::Profile oldStorage_{};uint32_t headerWord_=0;
 PhysicalConfigV5 config_;RootPermitV5 permit_;int32_t target_=0;
 LiftInput64 liftInput_{};uint64_t auxFrame_=0;uint32_t auxEnd_=0,auxStart_=0;
 Words256 tail_{};NativeRootEvidence evidence_;
 EGLContext context_=EGL_NO_CONTEXT;EGLDisplay display_=EGL_NO_DISPLAY;EGLSurface surface_=EGL_NO_SURFACE;std::thread::id thread_;
 GLsync fence_=nullptr;Authorize authorize_=nullptr;void* authorization_=nullptr;
 uint32_t nextPass_=0,currentBank_=0,sourcePrefix_=0,dedupPrefix_=0,sourceStore_=15,angle_=0;
 uint32_t sandIndex_=0,waterIndex_=0,arrivalIndex_=0,edges_=0,frozen_=0,rows_=0,angles_=0,ledger_=0;
 uint32_t activeProgram_=0;bool ready_=false,busy_=false,mapped_=false;
 uint32_t uploadCounter_=0;bool dryUniforms_=false;
 size_t alignment_=16;
 uint64_t fullPlan_=0;
 uint32_t fullMask_=0,fullQuantumMask_=0,pendingFullMask_=0;
 bool fullFrozen_=true;
 GLenum rawGl_=0;EGLint rawEgl_=EGL_SUCCESS;
 bool Current()const;bool Good();bool Allowed();
 TransportResultV5 Denied()const;
 void Use(uint32_t);void U(const char*,uint32_t);void I(const char*,int32_t);void Pair(const char*,uint64_t);
 void Quad(const char*,const uint32_t*);
 void Common(bool source);void SourcePass(uint32_t);void QuantumPass(uint32_t);
 void Copy(GLuint,size_t,GLuint,size_t,size_t);void Barrier();
};
}

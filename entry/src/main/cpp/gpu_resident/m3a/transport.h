#pragma once
#include "buffer_plan.h"
#include "completion_gate.h"
#include "source_queue_v4.h"
#include "../uniform_locations.h"
#include <EGL/egl.h>
#include <thread>
namespace grs::m3a {
struct PhysicalConfigV4 {
 uint32_t omega=0,waterRate=8388608,gravity=1048576,settle=65536,emit=1048576,kAir=0,kFlight=0,kWet=120,soft=524288,flux=1048576;
 bool omegaProvided=false;
 bool fullBranchTrace=false;
 bool Valid()const{return omegaProvided&&omega<=1966080&&waterRate<=1006632960&&gravity>=120&&gravity<=1073741824&&settle&&settle<=1073741824&&emit<=1073741824&&kAir<=960&&kFlight<=960&&kWet&&kWet<=960&&soft<605394;}
};
enum class TransportResultV4 {OK,IN_PROGRESS,WOULD_BLOCK,UNSUPPORTED,INVALID,STALE,DEADLINE,GL_FAILED,SHADER_FAILED,CONTEXT_LOST};
class TransportV4 {
public:
 using Authorize=bool(*)(void*,const RootPermitV4&,uint64_t);
 TransportV4()=default;TransportV4(const TransportV4&)=delete;TransportV4& operator=(const TransportV4&)=delete;
 TransportResultV4 Create(const Profile&,const ControlIdentityV4&,uint64_t geometry,const PhysicalConfigV4&,
                           const std::array<uint32_t,8>& shaderSha,const std::array<uint32_t,8>& angleSha,const std::array<uint32_t,8>& layoutSha);
 TransportResultV4 Begin(const RootPermitV4&,int32_t frozenTarget,const SourcePacketV4*,uint32_t sourcePrefix,uint32_t dedupPrefix,Authorize,void*);
 TransportResultV4 Pump();
 // Commit/Reject is called after the single mapped tail passes completion_gate
 // and owner-lock acceptance. GPU workspace remains quarantined until then.
 bool Commit(const RootPermitV4&,const Words256&);
 bool RetireRejected(const RootPermitV4&);
 void Release(bool contextCurrent);
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
 bool CaptureColdV4(uint32_t* reserved,size_t capacity,const QuotaSnapshotV3&,size_t& stateWords,size_t& eventWords);
 EGLint FirstEglError()const{return rawEgl_;}
private:
 struct Program {GLuint id=0;UniformLocations locations;};
 std::array<Program,16> programs_{};
 std::array<GLuint,28> buffers_{};
 std::array<GLuint,21> fullBuffers_{};
 const Profile* profile_=nullptr;const m2::Profile* old_=nullptr;WorkLayout layout_;ResidentBuffersV4 allocation_;
 PhysicalConfigV4 config_;RootPermitV4 permit_;int32_t target_=0;
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
 TransportResultV4 Denied()const;
 void Use(uint32_t);void U(const char*,uint32_t);void I(const char*,int32_t);void Pair(const char*,uint64_t);
 void Common(bool source);void SourcePass(uint32_t);void QuantumPass(uint32_t);
 void Copy(GLuint,size_t,GLuint,size_t,size_t);void Barrier();
};
}

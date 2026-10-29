#pragma once
#include "product_wire.h"
#include "resources.h"
#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <thread>
#include <vector>
namespace grs {
// Isolated source-root development transport; no UI or production route calls this yet.
// All methods execute on the same native GL owner/current context. Poll never waits.
enum class TransportCode {OK,NOT_READY,BUSY,WRONG_OWNER,CAPABILITY_UNSUPPORTED,MEMORY_PROFILE_UNSUPPORTED,SHADER_FAILED,GL_FAILED,STATE_LOST,PROTOCOL_REJECTED};
struct SourceWorkspace {
 size_t words=0,spare=0,motion=0,indexA=0,indexB=0,scan=0,edges=0,audit=0,dedup=0,sourceEvents=0,command=0,status=0,angles=0,stats=0,terminal=0;
};
struct SourcePublication {size_t words=0,sand=0,water=0,column=0,header=0;};
bool PlanSourceWorkspace(uint32_t K,size_t alignment,size_t maxBytes,SourceWorkspace&,SourcePublication&);
struct SourceTransportStats {uint64_t coldMappedBytes=0,dispatches=0,barriers=0,fences=0,polls=0,mappedBytes=0,uploadedBytes=0,allocatedBytes=0;uint32_t firstGlError=0;};
class SourceGpuTransport {
 public:
 SourceGpuTransport()=default;SourceGpuTransport(const SourceGpuTransport&)=delete;SourceGpuTransport& operator=(const SourceGpuTransport&)=delete;
 TransportCode Create(uint32_t K,const Identity& coldIdentity);
 TransportCode Submit(const std::array<uint32_t,32>& command,RootRequest request);
 TransportCode SubmitQuantum(RootRequest request,int requestedAngle);
 TransportCode Poll(AcceptedRoot&);
 TransportCode FreezeDiagnostics(bool freeze);TransportCode CaptureAcceptedState(std::vector<uint32_t>& words);
 ControlCode Stop();void ContextLost();TransportCode Release();
 ControlCode Acquire(uint32_t slot,U64 generation,PublicationLease& lease,GLuint& buffer);
 ControlCode ReleasePublication(uint32_t slot,U64 generation,GLsync releaseFence);
 U64 InputGeneration()const{return input;}
 const Words256& LastCompletion()const{return lastCompletion;}
 const SourceTransportStats& Stats()const{return stats;}const RootOwner& Owner()const{return owner;}
 const SourcePublication& PublicationLayout()const{return publication;}
 private:
 TransportCode SubmitRoot(const std::array<uint32_t,32>* command,RootRequest,int requestedAngle);
 bool Current()const;bool GlOK();bool SetUniform(const char*,uint32_t);void SetPair(const char*,U64);void RetireReleases();
 Words256 lastCompletion{};U64 arenaFrames[2]{};uint32_t arenaUsed[2]{};
 uint32_t K=0,pad=0;EGLContext context=EGL_NO_CONTEXT;EGLDisplay display=EGL_NO_DISPLAY;EGLSurface surface=EGL_NO_SURFACE;std::thread::id thread;
 GLuint program=0,banks[2]{},work=0,events[2]{},pubs[2]{},results[2]{};GLsync fence=nullptr,releases[2]{};
 U64 releaseGenerations[2]{};uint32_t pendingSlot=0;U64 input{1,0},lastSourceFrame{},lastQuantumFrame{};uint32_t lastQuantumOrdinal=0;bool fatalFrozen=false,diagnosticFreeze=false,submittedAny=false,ready=false,lost=false;
 SourceWorkspace workspace;SourcePublication publication;StateSegments segments;SourceTransportStats stats;RootOwner owner;Identity identity;RootRequest pending;
};
const char* SourceKernelSource();const char* SourceKernelSha();
}

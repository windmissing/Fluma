#pragma once
#include "source_transport.h"
namespace grs {namespace m1 {
// Candidate values remain PENDING_CALIBRATION (T237); not a validated speed law.
struct QuantumParameters {uint32_t gravity=512u*1048576u,settle=8u*1048576u,kAir=0,kFlight=0,kWet=24,kappa=24,soft=557537,flux=24u*1048576u;int32_t requestedAngle=0;};
// Candidate only. Neither Poll nor Capture changes SourceRootOwner or publishes.
class QuantumCandidate {
public:
 QuantumCandidate()=default;QuantumCandidate(const QuantumCandidate&)=delete;QuantumCandidate& operator=(const QuantumCandidate&)=delete;
 TransportCode Create(uint32_t K,uint32_t Kw);
 TransportCode Submit(SourceCoreTransport&,const QuantumParameters&,RootRequest);
 TransportCode Poll();TransportCode Release();void ContextLost();
 TransportCode Freeze(bool);TransportCode Capture(std::vector<uint32_t>& state,std::vector<uint32_t>& rows);
 const std::array<uint32_t,16>& Status()const{return status;}
 uint64_t AllocatedBytes()const{return allocated;}uint32_t Dispatches()const{return dispatches;}
 uint32_t ExpectedDispatches()const;uint32_t EventCapacity()const{return eventCapacity;}
#ifdef GRS_T279_HOST_TEST
 // Cold fixture upload belongs only to the host executable, never HAP.
 TransportCode Fixture(const std::vector<uint32_t>&,const std::vector<uint32_t>&,const QuantumParameters&,RootRequest,uint32_t capacity=0);
#endif
private:
 friend struct QuantumCommitTestAccess;
 friend struct FullM1HostAccess;
 friend class FullM1Transport;
 bool Current()const;void Set(const char*,uint32_t);void Pass(uint32_t,uint32_t);void Sort(uint32_t,uint32_t);
 TransportCode Enqueue(GLuint,GLuint,size_t,GLuint,uint32_t,const QuantumParameters&,RootRequest);
 GLuint program=0,candidate=0,workspace=0,events=0;GLsync fence=nullptr;SourceCoreTransport* parent=nullptr;
 EGLContext context=EGL_NO_CONTEXT;EGLDisplay display=EGL_NO_DISPLAY;EGLSurface surface=EGL_NO_SURFACE;std::thread::id thread;
 uint32_t K=0,Kw=0,pad=0,water=0,column=0,header=0,words=0,sandIndex=32,waterIndex=0,arrivalIndex=0,edges=0,frozenAt=0,rowsAt=0,angles=0,sourceEvents=0,workWords=0,eventCapacity=0,dispatches=0;
 uint64_t allocated=0;bool frozen=false,ready=false,lost=false;std::array<uint32_t,16> status{};
 bool deferFence=false;
 uint32_t submittedUniformBytes=0;
 TransportTraceRing trace;Identity traceIdentity{};RootRequest traceRoot{};uint64_t traceOutputBytes=0;uint32_t firstGlError=0;
#ifdef GRS_T279_HOST_TEST
 GLuint fixtureBase=0,fixtureLedger=0;
#endif
};
const char* QuantumKernel();const char* QuantumSha();
}}

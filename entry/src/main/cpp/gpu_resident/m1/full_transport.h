#pragma once
#include "quantum_candidate.h"
namespace grs {namespace m1 {
// Full M1 roots, still isolated from product routing/scene. All calls belong
// to the same current native GL owner. Only Poll can accept a sealed root.
class FullM1Transport {
public:
 TransportCode Create(uint32_t K,uint32_t Kw,const Identity&,const QuantumParameters& cold=QuantumParameters{},uint32_t emitQ=16u*1048576u);
 TransportCode SubmitSource(const std::array<uint32_t,32>&,RootRequest);
 TransportCode SubmitQuantum(const QuantumParameters&,RootRequest);
 TransportCode Poll(AcceptedRoot&);ControlCode Stop();void ContextLost();TransportCode Release();
 ControlCode Acquire(uint32_t,U64,PublicationLease&,GLuint&);ControlCode ReleasePublication(uint32_t,U64,GLsync);
 TransportCode Freeze(bool);TransportCode Capture(std::vector<uint32_t>& bank,std::vector<uint32_t>& events);
 const SourceRootOwner& Owner()const{return source.Owner();}const Words256& LastCompletion()const{return tail;}
 uint64_t AllocatedBytes()const{return source.Stats().allocatedBytes+quantum.AllocatedBytes()+extraBytes;}
 SourceTransportStats Stats()const;
 const TransportTraceRing& Trace()const{return quantum.trace;} // last quantum root, reset before submission
private:
 friend struct FullM1HostAccess;
 void TestHook(uint32_t stage){
#ifdef GRS_T280_HOST_TEST
  if(testHook)testHook(*this,stage);
#else
  (void)stage;
#endif
 }
#ifdef GRS_T280_HOST_TEST
 void (*testHook)(FullM1Transport&,uint32_t)=nullptr;
#endif
 SourceCoreTransport source;QuantumCandidate quantum;GLuint eventArenas[2]{},results[2]{};uint64_t extraBytes=0;uint32_t spareAt=0,replayFrozen=0;
 bool active=false,quantumFlight=false,failed=false,lost=false,frozen=false;RootRequest pending{};Words256 tail{};SourceTransportStats stats{};
 U64 lastFrame{},arenaFrame[2]{};uint32_t used[2]{},lastOrdinal=0,lastPublication=2;bool frameHasSource=false;
 QuantumParameters coldParams{};
};
const char* FullSourceKernel();const char* QuantumCommitKernel();const char* FullM1ShaderSha();
}}

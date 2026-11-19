#pragma once
#if __has_include("m5_source_manifest.h")
#include "m5_source_manifest.h"
#endif
#ifndef GRS_M5_SOURCE_MANIFEST_SHA
#define GRS_M5_SOURCE_MANIFEST_SHA "UNRECORDED"
#endif
// T653 / T650: bounded CPU observation only; never a readiness or physics authority.
#include "progress_clock_v3.h"
#include <array>
#include <mutex>
#include <string>
#include <cstddef>
namespace grs::m3b::init650 {
using Digest=std::array<uint32_t,8>;
enum class Life:uint32_t {NONE,REQUEST_QUEUED,WAIT_WINDOW,WAIT_RETIRED,OWNER_CREATE,CONTROLLER_CONFIGURED,SCENE_READY,PUBLISHED_WAIT_PAGE_PULSE,INPUT_READY,PULSE_ACCEPTED,RUNNING,FAILED,CANCELLED};
enum class Code:uint32_t {NONE,CONFIG_REQUEST,WINDOW_RETAIN,RETIRE_PREVIOUS,EGL_READY,CONTROLLER_CONFIGURE,PROGRAM_SOURCE,PROGRAM_INTERMEDIATE,PROGRAM_FINAL,SCENE_PROGRAM,SCENE_TARGETS,PUBLISH_CONTROLLER,SESSION_READ,FRAME_ATTACH,INPUT_OFFER,PULSE_DIRECT,OWNER_OBSERVE_PULSE,PLAN_ADVANCE,RETIRE_CONTROLLER};
enum Known:uint32_t {RAW_GL=1,RAW_EGL=2,API_RETURN=4,SOURCE_SHA=8,ACTUAL_RETURN=16,SCOPE_KNOWN=32,PULSE_PAYLOAD=64};
struct Stage {
 uint64_t seq=0,nativeNs=0,requestTicket=0,configAttempt=0,diagGeneration=0,cancelGeneration=0,ownerTid=0,epoch=0,window=0,context=0;
 std::array<uint64_t,9> scope{};int32_t actualReturn=-1,rawGl=-1,rawEgl=-1,apiReturn=-1;
 std::array<char,32> phase{};std::array<char,8> edge{};std::array<uint32_t,8> sourceOrPulse{};
 uint32_t known=0,op=0,programRole=0,reserved=0;
};
struct Attempt {std::array<uint32_t,128> words{};};
struct Log {std::array<uint32_t,16> header{};std::array<char,1984> raw{};};
struct Payload {std::array<uint32_t,256> control{};std::array<Attempt,4> attempts{};std::array<Stage,32> stages{};std::array<Log,2> logs{};std::array<uint32_t,256> reserved{};};
static_assert(sizeof(Stage)==256&&offsetof(Stage,sourceOrPulse)==208&&offsetof(Stage,known)==240);
static_assert(sizeof(Attempt)==512&&sizeof(Log)==2048&&sizeof(Payload)==16384);
static_assert(offsetof(Payload,attempts)==1024&&offsetof(Payload,stages)==3072&&offsetof(Payload,logs)==11264&&offsetof(Payload,reserved)==15360);
uint64_t Now();uint64_t Tid();Digest Hash(const uint32_t*,size_t);Digest Hex(const char*);
std::array<uint64_t,9> Scope(const ControlIdentityV5&);
struct Binding {class Store* store=nullptr;uint64_t ticket=0;};
struct ProgramWatch {Binding binding;Code code;uint32_t role;Digest source{};};
class Store {
 mutable std::mutex mutex_;Payload payload_{};
 Attempt* Find(uint64_t);const Attempt* Find(uint64_t)const;bool Mutation(uint64_t);void EmitLocked(uint64_t,Stage);
public:
 Store();
 uint64_t Request(const std::array<uint32_t,32>&,uint64_t boot,uint64_t attempt,uint64_t generation,const Digest& source);
 void Bind(uint64_t,uint64_t epoch,uint64_t window,uint64_t context,const ControlIdentityV5* scope=nullptr,bool active=true);
 void Emit(uint64_t,Code,const char* edge,int32_t result=-1,Life life=Life::NONE,const Digest* source=nullptr,uint32_t role=0,int32_t gl=-1,int32_t egl=-1);
 void Pulse(uint64_t,Code,const PagePulseV3&,uint64_t receipt,int32_t result,uint64_t ownerSeen=0);
 void LogBytes(uint64_t,uint32_t role,uint32_t slot,const char*,size_t copied,int64_t reported,const Digest&);
 void Cancel(uint64_t);bool Live(uint64_t)const;std::string Page(uint64_t,uint32_t)const;
};
}

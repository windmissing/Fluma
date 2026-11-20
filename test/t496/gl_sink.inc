// T561 (problem T547 搂3 + manifest): full-process Create sink, single instance,
// ENTER/鍘熸牱璋冪敤/EXIT for every listed GL/EGL call plus the 14 high-level
// cold observer seams. Records carry schema/runId/arm/pid/ownerTid/createAttempt
// Id/callId/functionId/event/wallNs/scriptNs and the watcher-facing ActiveCall
// phases (ENTER_EMIT/IN_API/RETURNED/EXIT_EMIT). Per-record <=4096B,
// per-attempt <=32MiB with EVIDENCE_BUDGET termination; error consumption is
// forward-only (the sink itself never calls glGetError/eglGetError).
// The legacy six-function names (CallSeq/WallNs/Emit) are preserved as thin
// wrappers so the existing gl_forward_sink.cpp six-function wiring keeps
// working unchanged; the full family uses ScopedCall/ScopedCallVoid.
#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#ifdef _WIN32
#include <process.h>
#define T561_GETPID _getpid
#else
#include <unistd.h>
#define T561_GETPID getpid
#endif

#include "exit_diagnostics.h"
#include "../t693/trace.h"
namespace t496sink {
inline std::unique_ptr<t693::Trace>& BinaryTrace(){static std::unique_ptr<t693::Trace> trace;return trace;}
inline void CloseBinaryTrace(){if(BinaryTrace())BinaryTrace()->close();}
// ---- one instance per process; main initializes before the owner starts ----
inline std::string& RunId(){static std::string s;return s;}
inline std::string& Arm(){static std::string s;return s;}
inline std::string& AttemptId(){static std::string s;return s;}
inline std::string& TracePath(){static std::string s;return s;}
inline std::atomic<bool>& Initialized(){static std::atomic<bool> v{false};return v;}
inline std::atomic<bool>& BudgetExceeded(){static std::atomic<bool> v{false};return v;}
inline std::atomic<uint64_t>& CallSeq(){static std::atomic<uint64_t> s{0};return s;}
inline std::atomic<uint64_t>& BudgetBytes(){static std::atomic<uint64_t> s{0};return s;}
inline std::mutex& TraceMu(){static std::mutex m;return m;}
inline std::FILE*& TraceHandleRef(){static std::FILE* f=nullptr;return f;}
// Per-owner atomic ActiveCall: (function, callId, phase), lock-free readable.
inline std::atomic<uint32_t>& ActiveFunction(){static std::atomic<uint32_t> v{0};return v;}
inline std::atomic<uint64_t>& ActiveCallId(){static std::atomic<uint64_t> v{0};return v;}
inline std::atomic<uint32_t>& ActivePhase(){static std::atomic<uint32_t> v{0};return v;}
enum Phase:uint32_t{PHASE_IDLE=0,ENTER_EMIT=1,IN_API=2,RETURNED=3,EXIT_EMIT=4};
inline constexpr uint64_t kAttemptFileBytesMax=33554432ull; // 32MiB test-disk credit
inline constexpr size_t kRecordUtf8Max=4096;

inline uint64_t WallNs(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
inline uint64_t OwnerTid(){
#ifdef _WIN32
 return GetCurrentThreadId();
#else
 return uint64_t(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
}
inline std::string JEsc(const std::string& s){std::string o;for(char c:s){if(c=='"')o+="\\\"";else if(c=='\\')o+="\\\\";else if(static_cast<unsigned char>(c)<0x20){char b[8];std::snprintf(b,sizeof(b),"\\u%04x",c);o+=b;}else o+=c;}return o;}

// Stable function ids, ordered after the T547 manifest apiGroups + seams.
enum FunctionId:uint32_t{
 FN_EGL_GET_PLATFORM_DISPLAY=1,FN_EGL_INITIALIZE,FN_EGL_BIND_API,FN_EGL_CHOOSE_CONFIG,
 FN_EGL_CREATE_CONTEXT,FN_EGL_CREATE_PBUFFER_SURFACE,FN_EGL_MAKE_CURRENT,FN_EGL_QUERY_SURFACE,
 FN_EGL_QUERY_CONTEXT,FN_EGL_SWAP_BUFFERS,FN_EGL_DESTROY_SURFACE,FN_EGL_DESTROY_CONTEXT,
 FN_EGL_TERMINATE,FN_EGL_GET_PROC_ADDRESS,FN_EGL_GET_CURRENT_CONTEXT,FN_EGL_GET_CURRENT_DISPLAY,
 FN_EGL_GET_CURRENT_SURFACE,FN_EGL_GET_ERROR,
 FN_GL_CREATE_SHADER=100,FN_GL_SHADER_SOURCE,FN_GL_COMPILE_SHADER,FN_GL_CREATE_PROGRAM,
 FN_GL_ATTACH_SHADER,FN_GL_LINK_PROGRAM,FN_GL_PROGRAM_PARAMETERI,FN_GL_PROGRAM_BINARY,
 FN_GL_GET_PROGRAM_BINARY,FN_GL_GET_PROGRAMIV,FN_GL_GET_SHADERIV,FN_GL_GET_SHADER_INFO_LOG,
 FN_GL_GET_PROGRAM_INFO_LOG,FN_GL_DELETE_PROGRAM,FN_GL_DELETE_SHADER,
 FN_GL_GET_STRING=200,FN_GL_GET_STRINGI,FN_GL_GET_INTEGERV,FN_GL_GET_INTEGER64V,
 FN_GL_GET_ACTIVE_UNIFORM=300,FN_GL_GET_UNIFORM_LOCATION,FN_GL_GET_PROGRAM_INTERFACEIV,
 FN_GL_GET_PROGRAM_RESOURCE_INDEX,FN_GL_GET_PROGRAM_RESOURCEIV,FN_GL_GET_ACTIVE_ATTRIB,
 FN_GL_GET_ATTRIB_LOCATION,FN_GL_GET_FRAG_DATA_LOCATION,FN_GL_IS_PROGRAM,
 FN_GL_GEN_BUFFERS=400,FN_GL_BIND_BUFFER,FN_GL_BUFFER_DATA,FN_GL_BUFFER_SUB_DATA,
 FN_GL_BIND_BUFFER_BASE,FN_GL_COPY_BUFFER_SUB_DATA,FN_GL_DELETE_BUFFERS,
 FN_GL_GEN_VERTEX_ARRAYS,FN_GL_BIND_VERTEX_ARRAY,FN_GL_DELETE_VERTEX_ARRAYS,
 FN_GL_USE_PROGRAM,FN_GL_DISPATCH_COMPUTE,FN_GL_MEMORY_BARRIER,FN_GL_FLUSH,FN_GL_FINISH,
 FN_GL_FENCE_SYNC,FN_GL_CLIENT_WAIT_SYNC,FN_GL_DELETE_SYNC,FN_GL_MAP_BUFFER_RANGE,
 FN_GL_UNMAP_BUFFER,
 FN_GL_GEN_FRAMEBUFFERS=500,FN_GL_BIND_FRAMEBUFFER,FN_GL_DELETE_FRAMEBUFFERS,
 FN_GL_VIEWPORT,FN_GL_DRAW_ARRAYS,
 FN_SEAM_TRANSPORT_CREATE=900,FN_SEAM_SOURCE_CREATE,FN_SEAM_INTERMEDIATE_CREATE,
 FN_SEAM_FINAL_BUILD,FN_SEAM_REFLECT_PREPARE,FN_SEAM_COLD_PREFIX,FN_SEAM_SCENE_PREPARE,
 FN_SEAM_SCENE_ROLE_BUILD,FN_SEAM_RESOURCE_COMMIT,FN_SEAM_CAPTURE_EPOCH,
 FN_SEAM_SESSION_HANDOFF,FN_SEAM_FIRST_PAGE_PULSE,FN_SEAM_READY,FN_SEAM_RETIRE_RETRY
};

inline bool EmitRecord(uint32_t functionId,const char* event,uint64_t callId,const std::string& detail){
 if(!Initialized().load()||BudgetExceeded().load())return false;
 if (BinaryTrace()) {
  const uint8_t edge = std::strcmp(event,"ENTER")==0 ? 1 : std::strcmp(event,"EXIT")==0 ? 2 :
     std::strcmp(event,"CPU_MAP_RETURN_BYTES")==0 ? 3 : 0;
  if(edge && BinaryTrace()->append(callId,WallNs(),uint32_t(OwnerTid()),functionId,edge,detail,{}))return true;
  BudgetExceeded().store(true);t496diag::EvidenceBudget();return false;
 }
 char record[kRecordUtf8Max];
 const std::string safe=JEsc(detail.substr(0,3000));
 const auto wrote=std::snprintf(record,sizeof(record),
  "{\"schema\":\"T561_CREATE_SINK_V1\",\"runId\":\"%s\",\"arm\":\"%s\",\"pid\":%d,\"ownerTid\":%llu,"
  "\"createAttemptId\":\"%s\",\"callId\":%llu,\"functionId\":%u,\"event\":\"%s\","
  "\"wallNs\":\"%llu\",\"scriptNs\":\"%llu\",\"detail\":\"%s\"}\n",
  JEsc(RunId()).c_str(),JEsc(Arm()).c_str(),int(T561_GETPID()),(unsigned long long)OwnerTid(),
  JEsc(AttemptId()).c_str(),(unsigned long long)callId,functionId,event,
  (unsigned long long)WallNs(),(unsigned long long)WallNs(),safe.c_str());
 if(wrote<=0||size_t(wrote)>=sizeof(record))return false;
 std::lock_guard<std::mutex> lock(TraceMu());
 auto*& f=TraceHandleRef();
 if(!f)return false;
 const size_t n=size_t(wrote);
 if(BudgetBytes().fetch_add(n)+n>kAttemptFileBytesMax){BudgetExceeded().store(true);t496diag::EvidenceBudget();return false;}
 if(std::fwrite(record,1,n,f)!=n){BudgetExceeded().store(true);t496diag::EvidenceBudget();return false;}
 std::fflush(f);
 return true;
}

// Legacy compatibility for the existing six-function gl_forward wiring.
inline void Emit(const char* event,const char* call,uint64_t seq,uint64_t wallNs,const std::string& detail=""){
 // Map the legacy free-form call names onto the new record shape via a
 // function-name hash so old callers keep producing parseable records.
 const uint32_t pseudo=1000u+(uint32_t(std::hash<std::string>{}(std::string(call))&0x3FFFFFFFu));
 EmitRecord(pseudo,event,seq,detail.empty()?std::string("call=")+call:detail+" call="+call);
 (void)wallNs;
}

// ENTER persisted -> IN_API -> original call -> RETURNED -> EXIT persisted.
template<typename Call>
auto ScopedCall(uint32_t functionId,const char* name,const std::string& enterDetail,Call&& call)->decltype(call()){
 const auto seq=CallSeq().fetch_add(1);
 ActiveFunction().store(functionId);ActiveCallId().store(seq);
 ActivePhase().store(ENTER_EMIT);t496diag::Sample(ENTER_EMIT,ActiveFunction(),ActiveCallId(),WallNs());
 EmitRecord(functionId,"ENTER",seq,enterDetail);
 ActivePhase().store(IN_API);t496diag::Sample(IN_API,ActiveFunction(),ActiveCallId(),WallNs());
 auto value=call();
 ActivePhase().store(RETURNED);t496diag::Sample(RETURNED,ActiveFunction(),ActiveCallId(),WallNs());
 ActivePhase().store(EXIT_EMIT);t496diag::Sample(EXIT_EMIT,ActiveFunction(),ActiveCallId(),WallNs());
 EmitRecord(functionId,"EXIT",seq,std::string("name=")+name+" return="+std::to_string((uintptr_t)value));
 ActivePhase().store(PHASE_IDLE);
 return value;
}
template<typename Call>
void ScopedCallVoid(uint32_t functionId,const char* name,const std::string& enterDetail,Call&& call){
 const auto seq=CallSeq().fetch_add(1);
 ActiveFunction().store(functionId);ActiveCallId().store(seq);
 ActivePhase().store(ENTER_EMIT);t496diag::Sample(ENTER_EMIT,ActiveFunction(),ActiveCallId(),WallNs());
 EmitRecord(functionId,"ENTER",seq,enterDetail);
 ActivePhase().store(IN_API);t496diag::Sample(IN_API,ActiveFunction(),ActiveCallId(),WallNs());
 call();
 ActivePhase().store(RETURNED);t496diag::Sample(RETURNED,ActiveFunction(),ActiveCallId(),WallNs());
 ActivePhase().store(EXIT_EMIT);t496diag::Sample(EXIT_EMIT,ActiveFunction(),ActiveCallId(),WallNs());
 EmitRecord(functionId,"EXIT",seq,std::string("name=")+name);
 ActivePhase().store(PHASE_IDLE);
}
// High-level seams: the host calls SeamEnter at the real business cut point and
// SeamExit with the actual return evidence; no forwarding happens here.
inline uint64_t SeamEnter(uint32_t functionId,const std::string& detail){
 const auto seq=CallSeq().fetch_add(1);
 ActiveFunction().store(functionId);ActiveCallId().store(seq);
 ActivePhase().store(ENTER_EMIT);t496diag::Sample(ENTER_EMIT,ActiveFunction(),ActiveCallId(),WallNs());
 if(!EmitRecord(functionId,"ENTER",seq,detail)){ActivePhase().store(PHASE_IDLE);return 0;}
 ActivePhase().store(IN_API);t496diag::Sample(IN_API,ActiveFunction(),ActiveCallId(),WallNs());
 return seq;
}
inline bool SeamExit(uint64_t seq,uint32_t functionId,const std::string& result){
 if(!seq)return false;
 ActivePhase().store(RETURNED);t496diag::Sample(RETURNED,ActiveFunction(),ActiveCallId(),WallNs());
 const bool persisted=EmitRecord(functionId,"EXIT",seq,result);
 ActivePhase().store(PHASE_IDLE);
 return persisted;
}
} // namespace t496sink

// C ABI: the main TU initializes the single process sink before the owner
// thread starts (T547 搂3 cross-TU single instance). Reusing an attempt file
// is rejected (create-new rule).
extern "C" inline bool t496_sink_initialize(const char* runId,const char* arm,const char* attemptId){
 bool expect=false;
 if(!t496sink::Initialized().compare_exchange_strong(expect,true))return false;
 t496sink::RunId()=runId?runId:"";t496sink::Arm()=arm?arm:"";t496sink::AttemptId()=attemptId?attemptId:"";
 t496sink::TracePath()=std::string(".codex/local-test/t496/gltrace-")+t496sink::AttemptId()+".ndjson";
 if(std::filesystem::exists(std::filesystem::path(t496sink::TracePath()))){t496sink::Initialized().store(false);return false;}
 if(std::getenv("T693_BINARY_TRACE")){
  auto trace=std::make_unique<t693::Trace>();
  const std::string identity=t496sink::RunId()+" "+t496sink::Arm()+" "+t496sink::AttemptId()+" "+std::to_string(T561_GETPID());
  if(!trace->open(t496sink::TracePath()+".t693",identity))return false;
  t496sink::BinaryTrace()=std::move(trace);
  return t496diag::Begin(std::string(".codex/local-test/t496/")+t496sink::AttemptId());
 }
 if(t496sink::TraceHandleRef())return false;
 t496sink::TraceHandleRef()=std::fopen(t496sink::TracePath().c_str(),"wb");
 if(!t496sink::TraceHandleRef()){t496sink::Initialized().store(false);return false;}
 if(!t496diag::Begin(std::string(".codex/local-test/t496/")+t496sink::AttemptId()))return false;
 return true;
}

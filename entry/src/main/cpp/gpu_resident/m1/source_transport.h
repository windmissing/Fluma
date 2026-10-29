#pragma once
#include "source_owner.h"
#include "../source_transport.h"
namespace grs {namespace m1 {
// Source-core slice only: distinct from configureM1/full-quantum support.
// Same native GL owner; ordinary polling reads only a sealed 1024B tail.
class SourceCoreTransport {
public:
 SourceCoreTransport()=default;SourceCoreTransport(const SourceCoreTransport&)=delete;SourceCoreTransport& operator=(const SourceCoreTransport&)=delete;
 TransportCode Create(uint32_t K,uint32_t Kw,const Identity&);
 TransportCode Submit(const std::array<uint32_t,32>&,RootRequest);
 TransportCode Poll(AcceptedRoot&);ControlCode Stop();void ContextLost();TransportCode Release();
 ControlCode Acquire(uint32_t,U64,PublicationLease&,GLuint&);
 ControlCode ReleasePublication(uint32_t,U64,GLsync);
 TransportCode Freeze(bool);TransportCode CaptureAccepted(std::vector<uint32_t>&);
 const SourceRootOwner& Owner()const{return owner;}const SourceTransportStats& Stats()const{return stats;}const Words256& LastCompletion()const{return tail;}
 const TransportTraceRing& Trace()const{return trace;}
 size_t PublicationWords()const{return pubWords;}size_t HeaderOffsetWords()const{return headerAt;}
private:
 bool Current()const;bool Good();bool Set(const char*,uint32_t);bool Pair(const char*,U64);void Retire();
 SourceRootOwner owner;SourceTransportStats stats{};TransportTraceRing trace;Identity identity{};RootRequest pending{};Words256 tail{};
 GLuint program=0,banks[2]{},work=0,events[2]{},pubs[2]{},results[2]{};GLsync fence=nullptr,releases[2]{};U64 releaseGen[2]{};U64 lastFrame{};U64 arenaFrame[2]{};
 EGLContext context=EGL_NO_CONTEXT;EGLDisplay display=EGL_NO_DISPLAY;EGLSurface surface=EGL_NO_SURFACE;std::thread::id thread;
 size_t words=0,workWords=0,pubWords=0,headerAt=0;uint32_t K=0,Kw=0;bool ready=false,lost=false,frozen=false,fatal=false;
};
}}

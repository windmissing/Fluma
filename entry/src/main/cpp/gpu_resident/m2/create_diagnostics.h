#pragma once
#include "../control.h"
#include "layout_generated.h"
#include "../submit_observer.h"
#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <array>
#include <mutex>
#include <chrono>
#include <cstring>
#include <ostream>
#include <algorithm>
namespace grs::create468 {
inline uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
enum class Stage:uint32_t {CREATE_SHADER,SOURCE_UPLOAD,COMPILE,CREATE_PROGRAM,ATTACH,LINK,REFLECT,ALLOCATE,PREPARE};
inline const char* Name(Stage s){static const char* names[]={"CREATE_SHADER","SOURCE_UPLOAD","COMPILE","CREATE_PROGRAM","ATTACH","LINK","REFLECT","ALLOCATE","PREPARE"};return names[uint32_t(s)];}
struct Event {Stage stage=Stage::CREATE_SHADER;uint32_t object=0,handle=0,rawGl=0;uint64_t enter=0,exit=0,a=0,b=0,c=0;bool success=false;};
struct Log {uint32_t offset=0,reported=0,copied=0,queryError=0;bool queried=false,truncated=false,empty=true;};
struct Program {const char* name="";std::array<char,65> sha{};uint64_t bytes=0;std::array<unsigned char,4096> raw{};uint32_t used=0;Log compile{},link{};};
struct Limit {const char* api="";uint32_t token=0,index=0,error=0;int64_t value=0;};
struct Attempt {
 uint64_t serial=0,epoch=0,window=0,tid=0,context=0,begin=0,end=0;
 Identity identity{};uint32_t k=0,kw=0,features=7,wire=3,policy=1,code=0;
 std::array<Program,3> programs{};std::array<Event,96> events{};uint32_t count=0,dropped=0;
 std::array<Limit,32> limits{};uint32_t limitCount=0;GLenum rawContextError=0;
 std::array<std::array<char,512>,4> strings{};std::array<GLenum,4> stringErrors{};
 std::array<int64_t,8> probeValues{};uint32_t probeCode=0,probeRawGl=0;bool probeSeen=false;
 int contextVersion=0;uint32_t contextQueryError=0;
 bool calling=false,failed=false,sealed=false;Event current{},firstFailure{};
};
static_assert(sizeof(Program::raw)*3<=16384,"T462 total raw log bound");
class Store {
 std::mutex mutex;std::array<Attempt,3> attempts{};uint64_t serial=0;
public:
 template<class F> void Change(uint64_t id,F f){std::lock_guard<std::mutex> lock(mutex);auto& a=attempts[id%3];if(a.serial==id&&!a.sealed)f(a);}
 uint64_t Begin(uint64_t epoch,uint64_t window,uint64_t tid,uint64_t context,const Identity& identity,uint32_t k,uint32_t kw){
  std::lock_guard<std::mutex> lock(mutex);if(serial==UINT64_MAX)return 0;auto id=++serial;auto& a=attempts[id%3];a={};a.serial=id;a.epoch=epoch;a.window=window;a.tid=tid;a.context=context;a.identity=identity;a.k=k;a.kw=kw;a.begin=Now();
  a.programs[0].name="SOURCE_V3";a.programs[1].name="QUANTUM_INTERMEDIATE";a.programs[2].name="QUANTUM_FINAL_V3";return id;
 }
 bool Copy(uint64_t id,Attempt& out){std::lock_guard<std::mutex> lock(mutex);if(!id)id=serial;if(!id||attempts[id%3].serial!=id)return false;out=attempts[id%3];return true;}
};
inline Store& Global(){static Store store;return store;}
struct Recorder {
 Store& store;uint64_t serial=0;uint32_t object=0;GLenum pending=0;
 void Enter(Stage s,GLuint handle=0,uint64_t a=0,uint64_t b=0,uint64_t c=0){store.Change(serial,[&](Attempt& v){v.calling=true;v.current={s,object,handle,0,Now(),0,a,b,c,false};});}
 void Exit(bool success,GLenum error=0,uint64_t a=UINT64_MAX,uint64_t b=UINT64_MAX,uint64_t c=UINT64_MAX){
  if(error&&!pending)pending=error;
  store.Change(serial,[&](Attempt& v){auto e=v.current;e.exit=Now();e.success=success;e.rawGl=error;if(a!=UINT64_MAX)e.a=a;if(b!=UINT64_MAX)e.b=b;if(c!=UINT64_MAX)e.c=c;v.calling=false;if(v.count<v.events.size())v.events[v.count++]=e;else ++v.dropped;if((!success||error)&&!v.failed){v.failed=true;v.firstFailure=e;}});
 }
 GLenum Consume(GLenum error){auto original=pending?pending:error;pending=0;return original;}
 void Finish(uint32_t code){store.Change(serial,[&](Attempt& v){v.code=code;v.end=Now();v.sealed=true;v.calling=false;});}
 template<class Api> void ReadLog(Api& api,GLuint handle,bool program,bool success){
  GLint length=0;if(program)api.Programiv(handle,GL_INFO_LOG_LENGTH,&length);else api.Shaderiv(handle,GL_INFO_LOG_LENGTH,&length);
  GLenum query=api.Error();uint32_t used=0;store.Change(serial,[&](Attempt& v){used=v.programs[object].used;});
  const uint32_t cap=std::max(1u,std::min(4096u-used,success?1024u:4096u));
  std::array<char,4096> bytes{};GLsizei copied=0;
  if(program)api.ProgramLog(handle,GLsizei(cap),&copied,bytes.data());else api.ShaderLog(handle,GLsizei(cap),&copied,bytes.data());
  const GLenum logError=api.Error();if(!query)query=logError;
  uint32_t n=uint32_t(std::max(0,std::min(int(copied),int(cap-1))));
  store.Change(serial,[&](Attempt& v){auto& p=v.programs[object];n=std::min(n,4096u-p.used);Log entry{p.used,uint32_t(std::max(0,int(length))),n,query,true,uint32_t(std::max(0,int(length)))>n+1u,n==0};
   if(program)p.link=entry;else p.compile=entry;std::memcpy(p.raw.data()+p.used,bytes.data(),n);p.used+=n;});
 }
};
inline thread_local Recorder* active=nullptr;
inline GLenum Consume(GLenum e){return active?active->Consume(e):e;}
inline void Enter(Stage s,GLuint h=0,uint64_t a=0,uint64_t b=0,uint64_t c=0){if(active)active->Enter(s,h,a,b,c);}
inline void Exit(bool ok,GLenum error=0,uint64_t a=UINT64_MAX,uint64_t b=UINT64_MAX,uint64_t c=UINT64_MAX){if(active)active->Exit(ok,error,a,b,c);}
struct Scope {
 Recorder recorder;Recorder* previous;
 Scope(uint64_t epoch,uint64_t window,uint64_t tid,uint64_t context,const Identity& id,uint32_t k,uint32_t kw):recorder{Global()},previous(active){recorder.serial=Global().Begin(epoch,window,tid,context,id,k,kw);active=&recorder;}
 ~Scope(){active=previous;}
 void Finish(uint32_t code){recorder.Finish(code);}
};
struct GlApi {
 GLuint CreateShader(){return glCreateShader(GL_COMPUTE_SHADER);}void Source(GLuint s,const char* src){glShaderSource(s,1,&src,nullptr);}void Compile(GLuint s){glCompileShader(s);}
 void Shaderiv(GLuint s,GLenum p,GLint* v){glGetShaderiv(s,p,v);}void ShaderLog(GLuint s,GLsizei n,GLsizei* len,char* out){glGetShaderInfoLog(s,n,len,out);}
 GLuint CreateProgram(){return glCreateProgram();}void Attach(GLuint p,GLuint s){glAttachShader(p,s);}void Link(GLuint p){glLinkProgram(p);}
 void Programiv(GLuint p,GLenum k,GLint* v){glGetProgramiv(p,k,v);}void ProgramLog(GLuint p,GLsizei n,GLsizei* len,char* out){glGetProgramInfoLog(p,n,len,out);}
 void DeleteShader(GLuint s){glDeleteShader(s);}GLenum Error(){return glGetError();}
};
template<class Api> bool Build(Api& api,uint32_t object,const char* source,const char* sha,GLuint& program,GLenum& firstGlError,SubmitObserver observer={}){
 if(active){active->object=object;active->store.Change(active->serial,[&](Attempt& a){auto& p=a.programs[object];std::strncpy(p.sha.data(),sha,64);p.bytes=std::strlen(source);});}
 auto finish=[&](bool ok){auto e=api.Error();if(e&&!firstGlError)firstGlError=e;Exit(ok,e);};
 Enter(Stage::CREATE_SHADER);auto shader=api.CreateShader();finish(shader!=0);
 Enter(Stage::SOURCE_UPLOAD,shader);api.Source(shader,source);finish(true);
 Enter(Stage::COMPILE,shader);observer.Emit(SubmitStage::ShaderEnter);api.Compile(shader);GLint ok=0;api.Shaderiv(shader,GL_COMPILE_STATUS,&ok);finish(ok!=0);observer.Emit(SubmitStage::ShaderExit,0,uint32_t(ok));
 if(active)active->ReadLog(api,shader,false,ok!=0);
 if(!ok){api.DeleteShader(shader);return false;}
 Enter(Stage::CREATE_PROGRAM);program=api.CreateProgram();finish(program!=0);
 Enter(Stage::ATTACH,program,shader);api.Attach(program,shader);finish(true);
 Enter(Stage::LINK,program);observer.Emit(SubmitStage::ProgramEnter);api.Link(program);api.Programiv(program,GL_LINK_STATUS,&ok);finish(ok!=0);observer.Emit(SubmitStage::ProgramExit,0,uint32_t(ok));
 if(active)active->ReadLog(api,program,true,ok!=0);
 api.DeleteShader(shader);return ok!=0;
}
inline bool Build(uint32_t object,const char* src,const char* sha,GLuint& program,GLenum& error,SubmitObserver observer={}){GlApi api;return Build(api,object,src,sha,program,error,observer);}
inline GLenum Prepare(GLuint program,GLuint a,GLuint b,GLuint w,GLuint out,SubmitObserver observer){
 Enter(Stage::PREPARE,program);const auto e=Consume(PrepareProgram(program,a,b,w,out,observer));Exit(e==0,e);return e;
}
template<class Limits> inline void ProbeResult(uint32_t code,const Limits& l){
 if(active)active->store.Change(active->serial,[&](Attempt& a){a.probeSeen=true;a.probeCode=code;a.probeRawGl=l.glError;a.probeValues={l.bindings,l.computeBlocks,l.fragmentBlocks,l.invocations,l.sizeX,l.sharedBytes,l.alignment,l.blockBytes};});
}
inline void QueryContext(){
 if(!active)return;
 auto& r=*active;const auto prior=glGetError();if(prior&&!r.pending)r.pending=prior;
 GLint version=0;const auto result=eglQueryContext(eglGetCurrentDisplay(),eglGetCurrentContext(),EGL_CONTEXT_CLIENT_VERSION,&version);const auto error=result?0:eglGetError();
 r.store.Change(r.serial,[&](Attempt& a){a.contextVersion=version;a.contextQueryError=error;a.rawContextError=prior;});
 const GLenum names[]={GL_VENDOR,GL_RENDERER,GL_VERSION,GL_SHADING_LANGUAGE_VERSION};
 for(unsigned i=0;i<4;++i){const auto* s=glGetString(names[i]);const auto e=glGetError();r.store.Change(r.serial,[&](Attempt& a){a.stringErrors[i]=e;if(s)std::strncpy(a.strings[i].data(),reinterpret_cast<const char*>(s),511);});}
 auto limit=[&](const char* api,GLenum token,unsigned index,bool wide,bool indexed){GLint value=0;GLint64 large=0;if(wide)glGetInteger64v(token,&large);else if(indexed)glGetIntegeri_v(token,index,&value);else glGetIntegerv(token,&value);const auto e=glGetError();r.store.Change(r.serial,[&](Attempt& a){if(a.limitCount<a.limits.size())a.limits[a.limitCount++]={api,token,index,e,wide?int64_t(large):int64_t(value)};});};
 for(auto token:{GL_MAJOR_VERSION,GL_MINOR_VERSION,GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,GL_MAX_COMPUTE_SHARED_MEMORY_SIZE})limit("glGetIntegerv",token,0,false,false);
 limit("glGetInteger64v",GL_MAX_SHADER_STORAGE_BLOCK_SIZE,0,true,false);
 for(unsigned i=0;i<3;++i){limit("glGetIntegeri_v",GL_MAX_COMPUTE_WORK_GROUP_SIZE,i,false,true);limit("glGetIntegeri_v",GL_MAX_COMPUTE_WORK_GROUP_COUNT,i,false,true);}
}
inline void String(std::ostream& o,const char* s){o<<'"';for(const auto* p=reinterpret_cast<const unsigned char*>(s);*p;++p){if(*p=='"'||*p=='\\')o<<'\\'<<char(*p);else if(*p>=32&&*p<127)o<<char(*p);else{const char* h="0123456789abcdef";o<<"\\u00"<<h[*p>>4]<<h[*p&15];}}o<<'"';}
inline void Profiles(std::ostream& o,const Attempt& a){
 o<<"],\"probe\":{\"queried\":"<<(a.probeSeen?"true":"false")<<",\"code\":"<<a.probeCode<<",\"rawGl\":"<<a.probeRawGl<<",\"values\":[";for(unsigned i=0;i<8;++i){if(i)o<<',';o<<a.probeValues[i];}o<<"]},\"profileAdmission\":[";
 for(unsigned i=0;i<(sizeof(grs::m2::kProfiles)/sizeof(grs::m2::kProfiles[0]));++i){if(i)o<<',';const auto& p=grs::m2::kProfiles[i];const auto& v=a.probeValues;const bool valid=a.probeSeen&&a.probeRawGl==0;
 const bool supported=valid&&v[0]>=4&&v[1]>=4&&v[2]>=1&&v[3]>=64&&v[4]>=64&&v[5]>=2048&&v[6]>0&&v[6]%4==0&&v[7]>=int64_t(p.coldTransientBytes);
 o<<"{\"K\":"<<p.k<<",\"Kw\":"<<p.kw<<",\"requiredLargestSsbo\":"<<p.coldTransientBytes<<",\"residentPayloadBytes\":"<<p.payloadBytes<<",\"supportedNumerically\":"<<(supported?"true":"false")<<",\"reason\":\""<<(!valid?"QUERY_UNAVAILABLE":supported?"NUMERICAL_ONLY_NOT_COMPILE_PROOF":"LIMIT_REJECTED")<<"\"}";
 }
}
inline void Json(std::ostream& o,const Attempt& a){
 auto u=[&](const char* key,uint64_t v){o<<'"'<<key<<"\":\""<<v<<"\",";};
 o<<"{\"revision\":\"T462_M2_CREATE_DIAGNOSTIC_V1\",\"buildVariant\":\"T468_M2_V3_DIAGNOSTIC\",";
 u("bootSerial",a.serial);u("previousSerial",a.serial>1?a.serial-1:0);u("epoch",a.epoch);u("window",a.window);u("ownerTid",a.tid);u("eglContext",a.context);u("nativeBeginNs",a.begin);u("nativeEndNs",a.end);
 auto pair=[&](const char* key,U64 v){u(key,(uint64_t(v.hi)<<32)|v.lo);};
 pair("candidateRun",a.identity.run);pair("surface",a.identity.surface);pair("contextGeneration",a.identity.context);pair("geometry",a.identity.geometry);pair("config",a.identity.config);pair("owner",a.identity.owner);
 auto hash=[&](const char* key,const std::array<uint32_t,8>& words){o<<'"'<<key<<"\":\"";const char* h="0123456789abcdef";for(unsigned i=0;i<32;++i){auto b=(words[i/4]>>(8*(i%4)))&255;o<<h[b>>4]<<h[b&15];}o<<"\",";};
 hash("layoutSha",a.identity.layoutSha);hash("combinedShaderSha",a.identity.shaderSha);
 o<<"\"K\":"<<a.k<<",\"Kw\":"<<a.kw<<",\"feature\":7,\"wire\":3,\"policy\":1,\"code\":"<<a.code<<",\"sealed\":"<<(a.sealed?"true":"false")<<",\"inCall\":"<<(a.calling?"true":"false")<<",\"inCallDurationNs\":\""<<(a.calling&&Now()>=a.current.enter?Now()-a.current.enter:0)<<"\",\"incomplete\":"<<(a.dropped?"true":"false")<<",\"dropped\":"<<a.dropped<<",\"firstFailure\":";
 auto event=[&](const Event& e){o<<"{\"object\":"<<e.object<<",\"stage\":\""<<Name(e.stage)<<"\",\"enterNs\":\""<<e.enter<<"\",\"exitNs\":\""<<e.exit<<"\",\"handle\":"<<e.handle<<",\"success\":"<<(e.success?"true":"false")<<",\"rawGl\":"<<e.rawGl<<",\"a\":\""<<e.a<<"\",\"b\":\""<<e.b<<"\",\"c\":\""<<e.c<<"\"}";};
 if(a.failed)event(a.firstFailure);else o<<"null";o<<",\"current\":";event(a.current);
 o<<",\"contextClientVersion\":"<<a.contextVersion<<",\"contextQueryError\":"<<a.contextQueryError<<",\"contextPriorGlError\":"<<a.rawContextError<<",\"driver\":[";
 for(unsigned i=0;i<4;++i){if(i)o<<',';o<<"{\"value\":";String(o,a.strings[i].data());o<<",\"rawGl\":"<<a.stringErrors[i]<<'}';}o<<"],\"limits\":[";
 for(unsigned i=0;i<a.limitCount;++i){if(i)o<<',';auto& l=a.limits[i];o<<"{\"api\":\""<<l.api<<"\",\"token\":"<<l.token<<",\"index\":"<<l.index<<",\"value\":\""<<l.value<<"\",\"rawGl\":"<<l.error<<'}';}Profiles(o,a);o<<"],\"programs\":[";
 for(unsigned i=0;i<3;++i){if(i)o<<',';const auto& p=a.programs[i];o<<"{\"object\":"<<i<<",\"name\":\""<<p.name<<"\",\"sha256\":\""<<p.sha.data()<<"\",\"sourceBytes\":"<<p.bytes<<",\"rawBytes\":"<<p.used;
  auto log=[&](const char* key,const Log& l){o<<",\""<<key<<"\":{\"queried\":"<<(l.queried?"true":"false")<<",\"reportedLength\":"<<l.reported<<",\"copiedLength\":"<<l.copied<<",\"offset\":"<<l.offset<<",\"truncated\":"<<(l.truncated?"true":"false")<<",\"emptyLog\":"<<(l.empty?"true":"false")<<",\"queryError\":"<<l.queryError<<'}';};log("compileLog",p.compile);log("linkLog",p.link);o<<'}';
 }o<<"],\"events\":[";for(unsigned i=0;i<a.count;++i){if(i)o<<',';event(a.events[i]);}o<<"]}";
}
}

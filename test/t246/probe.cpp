// Independent window probe: no production renderer/M0 state, no CPU scene fallback.
#include "protocol.h"
#include "pair_sha256_v1.h"
#include <chrono>
#include "../platform/egl_context.h"
#include <GLES3/gl31.h>
#include <napi/native_api.h>
#include <mutex>
#include <vector>
#include <sstream>
#include <thread>
#include <cstring>
#include <algorithm>
#include "resources_generated.h"
namespace {
p013::EglContext egl;std::mutex mutex;std::thread::id owner;bool bound=false,quarantined=false;uint64_t generation=1,serial=0,allocatedEpoch=0;
struct Flight {GLsync fence=nullptr,release=nullptr;t246::Identity id;t246::Tail expected;t246::AsyncGate gate;uint32_t alignment=0,polls=0,pendingPolls=0;bool holdLease=false;};
Flight flight;std::vector<std::string> events;uint64_t dropped=0;
void event(const char* name,uint32_t raw=0){std::ostringstream o;o<<"{\"event\":\""<<name<<"\",\"owner\":"<<generation<<",\"context\":"<<egl.contextEpoch<<",\"surface\":"<<egl.surfaceEpoch<<",\"token\":"<<serial<<",\"raw\":"<<raw<<",\"contextCreated\":"<<egl.contextCreateCount<<",\"contextDestroyed\":"<<egl.contextDestroyCount<<",\"surfaceCreated\":"<<egl.surfaceCreateCount<<",\"surfaceDestroyed\":"<<egl.surfaceDestroyCount<<"}";if(events.size()==256){events.erase(events.begin());dropped++;}events.push_back(o.str());}
GLuint compute=0,draw=0,resourceProgram=0,vao=0,tail=0;std::vector<GLuint> buffers;GLuint textures[3]={},fbos[3]={};uint64_t live=0,peak=0;
const char* cs=R"(#version 310 es
layout(local_size_x=64) in;layout(std430,binding=0) buffer Tail{uint w[];};
uniform uint uRounds;void main(){uint i=gl_GlobalInvocationID.x;if(i>=14u&&i<254u){uint v=i;for(uint r=0u;r<uRounds;r++)v=v*1664525u+1013904223u;w[i]=uRounds==0u?i^0x246u:v;}}
)";
const char* vs=R"(#version 310 es
precision highp float;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)";
const char* fs=R"(#version 310 es
precision highp float;precision highp int;layout(std430,binding=0) readonly buffer Tail{uint w[];};layout(location=0)out vec4 color;
void main(){bool ok=w[0]==0x54323436u&&w[254]==0x444f4e45u&&w[200]==(200u^0x246u);vec2 p=gl_FragCoord.xy;
 vec3 bg=vec3(.08,.14,.23);vec3 bed=vec3(.8,.6,.25);vec3 sand=vec3(.95,.85,.4);bool grain=distance(mod(p,vec2(47.,61.)),vec2(20.))<3.;color=vec4(ok?(p.y<60.?bed:grain?sand:bg):vec3(1.,0.,1.),1.);}
)";
bool shader(GLenum kind,const char* src,GLuint& out,std::string& error){out=glCreateShader(kind);glShaderSource(out,1,&src,nullptr);glCompileShader(out);GLint ok=0;glGetShaderiv(out,GL_COMPILE_STATUS,&ok);if(!ok){GLint n=0;glGetShaderiv(out,GL_INFO_LOG_LENGTH,&n);std::string log(n?n:1,0);glGetShaderInfoLog(out,log.size(),nullptr,&log[0]);error=log;return false;}return true;}
bool program(bool computeOnly,GLuint& out,std::string& error,const char* source=nullptr){GLuint a=0,b=0;if(!shader(computeOnly?GL_COMPUTE_SHADER:GL_VERTEX_SHADER,computeOnly?(source?source:cs):vs,a,error))return false;
 if(!computeOnly&&!shader(GL_FRAGMENT_SHADER,fs,b,error)){glDeleteShader(a);return false;}out=glCreateProgram();glAttachShader(out,a);if(b)glAttachShader(out,b);glLinkProgram(out);glDeleteShader(a);if(b)glDeleteShader(b);GLint ok=0;glGetProgramiv(out,GL_LINK_STATUS,&ok);if(!ok){GLint n=0;glGetProgramiv(out,GL_INFO_LOG_LENGTH,&n);std::string log(n?n:1,0);glGetProgramInfoLog(out,log.size(),nullptr,&log[0]);error=log;}return ok;}
bool clear(uint32_t& raw){for(auto b:buffers)glDeleteBuffers(1,&b);glDeleteTextures(3,textures);glDeleteFramebuffers(3,fbos);if(compute)glDeleteProgram(compute);if(draw)glDeleteProgram(draw);if(resourceProgram)glDeleteProgram(resourceProgram);if(vao)glDeleteVertexArrays(1,&vao);
 raw=glGetError();if(raw!=GL_NO_ERROR){quarantined=true;return false;}
 buffers.clear();std::memset(textures,0,sizeof(textures));std::memset(fbos,0,sizeof(fbos));compute=draw=resourceProgram=vao=tail=0;live=0;allocatedEpoch=0;return true;}

void forgetLost(){flight={};buffers.clear();compute=draw=resourceProgram=vao=tail=0;std::memset(textures,0,sizeof(textures));std::memset(fbos,0,sizeof(fbos));live=allocatedEpoch=0;quarantined=false;}
void created(OH_NativeXComponent*c,void*w){std::lock_guard<std::mutex> lock(mutex);egl.OnWindowCreated(c,w);quarantined=false;generation++;event("WINDOW_CREATED");}
void changed(OH_NativeXComponent*c,void*w){std::lock_guard<std::mutex> lock(mutex);egl.OnWindowChanged(c,w);generation++;event("WINDOW_CHANGED");}
void destroyed(OH_NativeXComponent*c,void*w){std::lock_guard<std::mutex> lock(mutex);const auto before=egl.contextDestroyCount;egl.OnWindowDestroyed(c,w);generation++;event("WINDOW_DESTROYED");if(egl.contextDestroyCount>before){event("OLD_AUTHORITY_CONTEXT_RETIRED");forgetLost();}else quarantined=true;}
OH_NativeXComponent_Callback callbacks{created,changed,destroyed,nullptr};
std::string hashSource(const char* source){p011gpu::Sha256 sha;sha.update(source,std::strlen(source));auto bytes=sha.finish();std::string out;const char* h="0123456789abcdef";for(auto b:bytes){out+=h[b>>4];out+=h[b&15];}return out;}
napi_value jsonResult(napi_env e,const std::string& text){napi_value result;napi_create_string_utf8(e,text.c_str(),text.size(),&result);return result;}
napi_value run(napi_env e,napi_callback_info info){const auto start=std::chrono::steady_clock::now();std::lock_guard<std::mutex> lock(mutex);size_t argc=2;napi_value args[2];napi_get_cb_info(e,info,&argc,args,nullptr,nullptr);uint32_t profile=0,slow=0;if(argc>0)napi_get_value_uint32(e,args[0],&profile);if(argc>1)napi_get_value_uint32(e,args[1],&slow);slow=std::min(slow,64u);
 std::string stage="OWNER",detail="",ledger="";uint32_t raw=0;bool success=false;int status=0;GLenum wait=0;bool verified=false;uint32_t pixel[4]={};GLint alignment=0,fragmentBlocks=0,vertexBlocks=0;
 t246::Identity id;id.owner=generation;id.token=++serial;id.lease=generation;
 auto work=[&](){if(bound&&owner!=std::this_thread::get_id())return;owner=std::this_thread::get_id();bound=true;
 if(flight.fence||flight.release){stage="BUSY_LEASE_OR_INFLIGHT";return;}
 if(quarantined){stage="QUARANTINED_RECREATE_SURFACE_REQUIRED";return;}
 p013::SurfaceInfo si;stage="CURRENT";auto code=egl.EnsureCurrent({egl.surfaceEpoch,egl.width,egl.height},si);if(code==p013::Status::STALE_SURFACE)code=egl.EnsureCurrent({egl.surfaceEpoch,egl.width,egl.height},si);if(code!=p013::Status::OK){status=int(code);return;}
 id.context=egl.contextEpoch;id.surface=egl.surfaceEpoch;if(profile>=t246::profileCount){stage="PROFILE_INVALID";return;}
 GLint align=0,fragment=0,vertex=0;glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,&align);glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragment);glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,&vertex);alignment=align;align=std::max(256,align);fragmentBlocks=fragment;vertexBlocks=vertex;if(fragment<1){stage="FRAGMENT_CAPABILITY_REQUIRED";return;}
 if(allocatedEpoch!=egl.contextEpoch){buffers.clear();compute=draw=resourceProgram=vao=tail=0;std::memset(textures,0,sizeof(textures));std::memset(fbos,0,sizeof(fbos));live=0;allocatedEpoch=egl.contextEpoch;}
 // Fresh explicit allocation per run; no growth to evade quarantined resources.
 if(!clear(raw)){stage="RETIREMENT_UNCONFIRMED";return;}allocatedEpoch=egl.contextEpoch;stage="PROGRAM";const char* resourceShader=R"(#version 310 es
layout(local_size_x=64)in;layout(std430,binding=0)buffer B{uint v[];};void main(){for(uint i=gl_GlobalInvocationID.x;i<uint(v.length());i+=gl_NumWorkGroups.x*64u)v[i]=i^0x246u;})";
if(!program(true,compute,detail)||!program(false,draw,detail)||!program(true,resourceProgram,detail,resourceShader))return;glGenVertexArrays(1,&vao);
 std::ostringstream objects;stage="ALLOCATE_AND_RW";for(const auto& part:t246::profiles[profile]){
  GLuint b=0;glGenBuffers(1,&b);buffers.push_back(b);glBindBuffer(GL_SHADER_STORAGE_BUFFER,b);glBufferData(GL_SHADER_STORAGE_BUFFER,part.bytes,nullptr,GL_DYNAMIC_DRAW);raw=glGetError();if(raw)return;
  live+=part.bytes;peak=live>peak?live:peak;uint32_t word=0x246;glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,b);glUseProgram(resourceProgram);glDispatchCompute(1024,1,1);glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT);void* ptr=glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,4,GL_MAP_READ_BIT);raw=glGetError();if(!ptr||raw)return;uint32_t read=0;std::memcpy(&read,ptr,4);if(!glUnmapBuffer(GL_SHADER_STORAGE_BUFFER)||read!=word){stage="RESOURCE_RW";return;}
  ptr=glMapBufferRange(GL_SHADER_STORAGE_BUFFER,part.bytes-4,4,GL_MAP_READ_BIT);raw=glGetError();if(!ptr||raw)return;std::memcpy(&read,ptr,4);if(!glUnmapBuffer(GL_SHADER_STORAGE_BUFFER)||read!=(uint32_t(part.bytes/4-1)^0x246u)){stage="RESOURCE_TAIL_RW";return;}
  objects<<"{\"name\":\""<<part.name<<"\",\"bytes\":"<<part.bytes<<",\"offset\":0,\"rw\":true},";
 }
 ledger=objects.str();if(!ledger.empty())ledger.pop_back();
 glGenTextures(3,textures);glGenFramebuffers(3,fbos);for(int i=0;i<3;i++){glBindTexture(GL_TEXTURE_2D,textures[i]);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,si.width,si.height);glBindFramebuffer(GL_FRAMEBUFFER,fbos[i]);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[i],0);if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){stage="FULL_FRAME_TARGET";return;}live+=uint64_t(si.width)*si.height*4;peak=live>peak?live:peak;}
 GLuint b=0;glGenBuffers(1,&b);buffers.push_back(b);tail=b;glBindBuffer(GL_SHADER_STORAGE_BUFFER,tail);glBufferData(GL_SHADER_STORAGE_BUFFER,align+1024,nullptr,GL_DYNAMIC_DRAW);live+=align+1024;peak=live>peak?live:peak;
 auto expected=t246::makeTail(id);glBufferSubData(GL_SHADER_STORAGE_BUFFER,align,1024,expected.words.data());stage="RANGE";
 if(!t246::rangeValid(align,1024,align+1024,align)||t246::rangeValid(align+4,1024,align+1024,align)){stage="RANGE_CONTRACT";return;}
 glBindBufferRange(GL_SHADER_STORAGE_BUFFER,0,tail,align,1024);stage="COMPUTE";glUseProgram(compute);
 GLint rounds=glGetUniformLocation(compute,"uRounds");for(uint32_t i=0;i<slow;i++){glUniform1ui(rounds,4096u);glDispatchCompute(4,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);}glUniform1ui(rounds,0u);glDispatchCompute(4,1,1);raw=glGetError();if(raw)return;
 glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);stage="FRAGMENT_DRAW";glBindFramebuffer(GL_FRAMEBUFFER,fbos[0]);glViewport(0,0,si.width,si.height);glUseProgram(draw);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);raw=glGetError();if(raw)return;
 flight.fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);if(!flight.fence){stage="FENCE_CREATE";return;}flight.id=id;flight.expected=expected;for(uint32_t i=14;i<254;i++)flight.expected.words[i]=i^0x246u;flight.alignment=align;flight.gate.begin(id);flight.polls=flight.pendingPolls=0;glFlush();event("SUBMIT_REAL_FENCE");stage="PENDING";
 };work();if(egl.current&&!raw)raw=glGetError();uint32_t eglRaw=egl.error;auto unbind=egl.current?egl.Unbind():p013::Status::OK;if(unbind!=p013::Status::OK){success=false;stage="UNBIND";}
 std::string logHex;const char* digits="0123456789abcdef";for(unsigned char b:detail){logHex+=digits[b>>4];logHex+=digits[b&15];}
 std::ostringstream json;json<<"{\"revision\":\"T246_WINDOW_PROBE_V2\",\"state\":\""<<(success?"CURRENT":stage=="PENDING"?"PENDING":"FAULT")<<"\",\"stage\":\""<<stage<<"\",\"token\":"<<serial<<",\"owner\":"<<generation<<",\"context\":"<<id.context<<",\"surface\":"<<id.surface<<",\"rawGL\":"<<raw<<",\"rawEGL\":"<<eglRaw<<",\"status\":"<<status<<",\"wait\":"<<wait<<",\"tailVerified\":"<<(verified?"true":"false")<<",\"liveBytes\":"<<live<<",\"peakBytes\":"<<peak<<",\"manifestSha\":\""<<t246::manifestSha<<"\",\"computeSha\":\""<<hashSource(cs)<<"\",\"fragmentSha\":\""<<hashSource(fs)<<"\",\"vertexSha\":\""<<hashSource(vs)<<"\",\"frameCpuInclusiveMs\":"<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<",\"shaderLogHex\":\""<<logHex<<"\",\"alignment\":"<<alignment<<",\"fragmentBlocks\":"<<fragmentBlocks<<",\"vertexBlocks\":"<<vertexBlocks<<",\"resources\":["<<ledger<<"],\"pixel\":["<<pixel[0]<<","<<pixel[1]<<","<<pixel[2]<<","<<pixel[3]<<"],\"gpuTimer\":\"NOT_MEASURED\",\"programDriverBytes\":\"NOT_MEASURED\"}";
 napi_value result;napi_create_string_utf8(e,json.str().c_str(),NAPI_AUTO_LENGTH,&result);return result;
}
napi_value poll(napi_env e,napi_callback_info){std::lock_guard<std::mutex> lock(mutex);if(owner!=std::this_thread::get_id())return jsonResult(e,"{\"state\":\"FAULT\",\"stage\":\"OWNER\"}");
 if(!flight.fence&&!flight.release)return jsonResult(e,"{\"state\":\"IDLE\",\"productCommit\":false}");
 p013::SurfaceInfo si;auto status=egl.EnsureCurrent({egl.surfaceEpoch,egl.width,egl.height},si);if(status!=p013::Status::OK){event("POLL_CURRENT_FAILED",uint32_t(status));return jsonResult(e,"{\"state\":\"FAULT\",\"stage\":\"CURRENT\"}");}
 const bool releasing=flight.release!=nullptr;if(releasing&&flight.holdLease){event("LEASE_HELD_NO_REUSE");egl.Unbind();return jsonResult(e,"{\"state\":\"LEASE_HELD\",\"reused\":false}");}GLsync fence=releasing?flight.release:flight.fence;GLenum wait=glClientWaitSync(fence,0,0);flight.polls++;if(wait==GL_TIMEOUT_EXPIRED){flight.pendingPolls++;event(releasing?"RELEASE_PENDING":"GPU_PENDING",wait);egl.Unbind();return jsonResult(e,"{\"state\":\"PENDING\",\"productCommit\":false}");}
 std::string stage="FAULT";bool verified=false,success=false;uint32_t raw=0;const auto token=flight.id.token;const auto producerContext=flight.id.context,producerSurface=flight.id.surface;const auto polls=flight.polls,pendingPolls=flight.pendingPolls;
 if(wait==GL_WAIT_FAILED){quarantined=true;stage="FENCE_FAILED";event("WAIT_FAILED",glGetError());}
 else if(releasing){glDeleteSync(flight.release);flight.release=nullptr;flight.holdLease=false;flight.gate.retire();stage="LEASE_RETIRED";event("LEASE_RETIRED",wait);}
 else{glDeleteSync(flight.fence);flight.fence=nullptr;event("PRODUCER_SIGNALED",wait);
  auto current=flight.id;current.owner=generation;current.context=egl.contextEpoch;current.surface=egl.surfaceEpoch;
  if(!flight.gate.current(current)){stage=flight.gate.cancelled?"CANCELLED_LATE_RETIRED":"OLD_GENERATION_REJECTED";event(stage.c_str());flight.gate.retire();}
  else{t246::Tail actual;glBindBuffer(GL_SHADER_STORAGE_BUFFER,tail);void* ptr=glMapBufferRange(GL_SHADER_STORAGE_BUFFER,flight.alignment,1024,GL_MAP_READ_BIT);if(ptr){std::memcpy(actual.words.data(),ptr,1024);verified=glUnmapBuffer(GL_SHADER_STORAGE_BUFFER)&&actual.words==flight.expected.words;}raw=glGetError();
   if(!verified||raw)stage="PRIVATE_TAIL_REJECTED";
   else{glBindFramebuffer(GL_READ_FRAMEBUFFER,fbos[0]);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);glBlitFramebuffer(0,0,si.width,si.height,0,0,si.width,si.height,GL_COLOR_BUFFER_BIT,GL_NEAREST);raw=glGetError();status=raw?p013::Status::GL_RESOURCE_FAILED:egl.Swap(token);success=raw==0&&status==p013::Status::OK;stage=success?"CURRENT":"SWAP_FAILED";event(stage.c_str(),raw);
    // Real consumer retirement fence: new allocation stays blocked while leased.
    flight.release=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!flight.release){success=false;stage="RELEASE_FENCE_FAILED";quarantined=true;}else event("RENDER_RELEASE_FENCE");
   }
  }
 }
 if(egl.current&&egl.Unbind()!=p013::Status::OK){success=false;stage="UNBIND_FAILED";quarantined=true;}
 std::ostringstream out;out<<"{\"revision\":\"T246_ASYNC_PRIVATE_V2\",\"state\":\""<<(success?"CURRENT":stage=="LEASE_RETIRED"?"IDLE":"FAULT")<<"\",\"stage\":\""<<stage<<"\",\"productCommit\":false,\"privateTailVerified\":"<<(verified?"true":"false")<<",\"token\":"<<token<<",\"producerContext\":"<<producerContext<<",\"producerSurface\":"<<producerSurface<<",\"context\":"<<egl.contextEpoch<<",\"surface\":"<<egl.surfaceEpoch<<",\"wait\":"<<wait<<",\"polls\":"<<polls<<",\"pendingPolls\":"<<pendingPolls<<",\"rawGL\":"<<raw<<",\"rawEGL\":"<<egl.error<<",\"displayPresentVerified\":false}";return jsonResult(e,out.str());
}
napi_value cancel(napi_env e,napi_callback_info){std::lock_guard<std::mutex> lock(mutex);flight.gate.cancel();event("CANCEL_TOMBSTONE");return jsonResult(e,"{\"state\":\"CANCEL_REQUESTED\",\"reused\":false}");}
napi_value hold(napi_env e,napi_callback_info info){std::lock_guard<std::mutex> lock(mutex);size_t argc=1;napi_value arg;bool value=false;napi_get_cb_info(e,info,&argc,&arg,nullptr,nullptr);if(argc!=1||napi_get_value_bool(e,arg,&value)!=napi_ok)return jsonResult(e,"{\"stage\":\"INVALID_LEASE\"}");flight.holdLease=value;event(value?"HOLD_LEASE_REQUEST":"RELEASE_LEASE_REQUEST");return jsonResult(e,value?"{\"state\":\"HOLD_REQUESTED\"}":"{\"state\":\"RELEASE_REQUESTED\"}");}
napi_value active(napi_env e,napi_callback_info info){std::lock_guard<std::mutex> lock(mutex);size_t argc=1;napi_value arg;bool value=false;napi_get_cb_info(e,info,&argc,&arg,nullptr,nullptr);if(argc!=1||napi_get_value_bool(e,arg,&value)!=napi_ok)return jsonResult(e,"{\"stage\":\"INVALID_ACTIVE\"}");
 const auto destroyedBefore=egl.contextDestroyCount;auto code=egl.SetActive(value);generation++;event(value?"FOREGROUND":"BACKGROUND",uint32_t(code));if(egl.contextDestroyCount>destroyedBefore){event("CONTEXT_RETIRED_OLD_AUTHORITY_REJECTED");forgetLost();}return jsonResult(e,value?"{\"state\":\"FOREGROUND_RECREATE_REQUIRED\"}":"{\"state\":\"BACKGROUND_NO_AUTHORITY\"}");}
napi_value drain(napi_env e,napi_callback_info){std::lock_guard<std::mutex> lock(mutex);std::ostringstream out;out<<"{\"revision\":\"T246_LIFECYCLE_LOG_V2\",\"productCommit\":false,\"dropped\":"<<dropped<<",\"events\":[";for(size_t i=0;i<events.size();i++){if(i)out<<',';out<<events[i];}out<<"]}";events.clear();return jsonResult(e,out.str());}
napi_value init(napi_env e,napi_value exports){napi_property_descriptor d[]={{"run",nullptr,run,nullptr,nullptr,nullptr,napi_default,nullptr},{"poll",nullptr,poll,nullptr,nullptr,nullptr,napi_default,nullptr},{"cancel",nullptr,cancel,nullptr,nullptr,nullptr,napi_default,nullptr},{"setActive",nullptr,active,nullptr,nullptr,nullptr,napi_default,nullptr},{"drain",nullptr,drain,nullptr,nullptr,nullptr,napi_default,nullptr},{"setLeaseHeld",nullptr,hold,nullptr,nullptr,nullptr,napi_default,nullptr}};napi_define_properties(e,exports,6,d);bool has=false;napi_has_named_property(e,exports,OH_NATIVE_XCOMPONENT_OBJ,&has);if(has){napi_value v;OH_NativeXComponent*c=nullptr;napi_get_named_property(e,exports,OH_NATIVE_XCOMPONENT_OBJ,&v);napi_unwrap(e,v,reinterpret_cast<void**>(&c));if(c)OH_NativeXComponent_RegisterCallback(c,&callbacks);}return exports;}
napi_module mod={1,0,nullptr,init,"t246probe",nullptr,{0}};
}
extern "C" __attribute__((constructor)) void RegisterT246(){napi_module_register(&mod);}

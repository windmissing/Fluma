// T139 isolated GLES compute harness. No renderer or physics integration.
// Every sample owns its identity and failure; only valid samples enter stats.
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif

using Clock=std::chrono::steady_clock;
static PFNGLBUFFERSUBDATAPROC bufferSubData=nullptr;
static PFNGLGETINTEGER64VPROC getInteger64v=nullptr;
static PFNGLMEMORYBARRIERPROC memoryBarrier=nullptr;
static int liveBuffers=0,liveSyncs=0,livePrograms=0;
static bool loadFunctions(){
#ifdef _WIN32
  HMODULE egl=GetModuleHandleA("libEGL.dll");if(!egl)egl=LoadLibraryA("libEGL.dll");
  if(!egl)return false;
  using GetProc=void*(__stdcall*)(const char*);
  GetProc proc=(GetProc)GetProcAddress(egl,"eglGetProcAddress");if(!proc)return false;
#else
  auto proc=eglGetProcAddress;
#endif
  bufferSubData=(PFNGLBUFFERSUBDATAPROC)proc("glBufferSubData");
  getInteger64v=(PFNGLGETINTEGER64VPROC)proc("glGetInteger64v");
  memoryBarrier=(PFNGLMEMORYBARRIERPROC)proc("glMemoryBarrier");
  return bufferSubData&&getInteger64v&&memoryBarrier;
}
static double ms(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
static std::string hex(GLenum x){std::ostringstream s;s<<"0x"<<std::hex<<x;return s.str();}
static std::string errors(){std::string s;for(int i=0;i<16;i++){GLenum e=glGetError();if(e==GL_NO_ERROR)break;if(!s.empty())s+=",";s+=hex(e);}return s.empty()?"NONE":s;}
static std::string quote(const std::string& x){std::string s="\"";for(char c:x){if(c=='"'||c=='\\')s+='\\';s+=c;}return s+'"';}
static void write(const std::string& path,const std::string& data){std::ofstream f(path,std::ios::binary);f.write(data.data(),data.size());}
static const char* source=R"(#version 310 es
precision highp int;
layout(local_size_x=64) in;
layout(std430,binding=0) buffer InBuf { uint data[]; } inputBuffer;
layout(std430,binding=1) buffer OutBuf { uint data[]; } outputBuffer;
void main(){uint i=gl_GlobalInvocationID.x;
  if(i<uint(inputBuffer.data.length())) outputBuffer.data[i]=inputBuffer.data[i]*3u+1u;})";
struct Context {EGLDisplay display=EGL_NO_DISPLAY;EGLContext context=EGL_NO_CONTEXT;EGLSurface surface=EGL_NO_SURFACE;EGLConfig config=nullptr;};
static bool create(Context& c,double& createMs){auto a=Clock::now();
  c.display=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(c.display==EGL_NO_DISPLAY||!eglInitialize(c.display,nullptr,nullptr))return false;
  EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLint n=0;
  if(!eglChooseConfig(c.display,attrs,&c.config,1,&n)||n<1)return false;
  if(!eglBindAPI(EGL_OPENGL_ES_API))return false;
  EGLint contextAttrs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};c.context=eglCreateContext(c.display,c.config,EGL_NO_CONTEXT,contextAttrs);
  EGLint surfaceAttrs[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};c.surface=eglCreatePbufferSurface(c.display,c.config,surfaceAttrs);
  if(c.context==EGL_NO_CONTEXT||c.surface==EGL_NO_SURFACE||!eglMakeCurrent(c.display,c.surface,c.surface,c.context))return false;
  createMs=ms(a,Clock::now());return true;
}
static void destroy(Context& c){if(c.display!=EGL_NO_DISPLAY){eglMakeCurrent(c.display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
  if(c.surface!=EGL_NO_SURFACE)eglDestroySurface(c.display,c.surface);
  if(c.context!=EGL_NO_CONTEXT)eglDestroyContext(c.display,c.context);eglTerminate(c.display);}c=Context();}
static bool program(GLuint& shader,GLuint& linked,double& compileMs,std::string& why){auto a=Clock::now();
  shader=glCreateShader(GL_COMPUTE_SHADER);if(!shader){why="CREATE_SHADER";return false;}
  glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
  if(!ok){why="COMPILE";return false;}
  linked=glCreateProgram();glAttachShader(linked,shader);glLinkProgram(linked);glGetProgramiv(linked,GL_LINK_STATUS,&ok);
  if(!ok){why="LINK";return false;}compileMs=ms(a,Clock::now());why=errors();return why=="NONE";
}
struct Buffers {GLuint id[2]={0,0};int n=0;int epoch=0;bool live=false;};
static bool alloc(Buffers& b,int n,int epoch,const std::vector<uint32_t>& input,double& allocMs,std::string& err){auto a=Clock::now();
  glGenBuffers(2,b.id);if(b.id[0])liveBuffers++;if(b.id[1])liveBuffers++;
  if(!b.id[0]||!b.id[1]){err="GEN_BUFFER";return false;}
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,b.id[0]);glBufferData(GL_SHADER_STORAGE_BUFFER,n*4,input.data(),GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,b.id[0]);glBindBuffer(GL_SHADER_STORAGE_BUFFER,b.id[1]);
  glBufferData(GL_SHADER_STORAGE_BUFFER,n*4,nullptr,GL_STATIC_DRAW);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,b.id[1]);
  err=errors();allocMs=ms(a,Clock::now());b.n=n;b.epoch=epoch;b.live=err=="NONE";return b.live;
}
static void release(Buffers& b){if(b.id[0]||b.id[1]){glDeleteBuffers(2,b.id);if(b.id[0])liveBuffers--;if(b.id[1])liveBuffers--;}b=Buffers();}
static std::string waitName(GLenum x){switch(x){case GL_ALREADY_SIGNALED:return "ALREADY_SIGNALED";
  case GL_CONDITION_SATISFIED:return "CONDITION_SATISFIED";case GL_TIMEOUT_EXPIRED:return "TIMEOUT_EXPIRED";
  case GL_WAIT_FAILED:return "WAIT_FAILED";default:return hex(x);}}
struct Sample {int epoch=0,n=0,ordinal=0;std::string phase,reason="NONE",wait="NOT_RUN",inputHash;
  std::string dispatchError="NOT_RUN",barrierError="NOT_RUN",fenceError="NOT_RUN",waitError="NOT_RUN",mapError="NOT_RUN",unmapError="NOT_RUN",cleanupError="NOT_RUN";
  double uploadMs=0,dispatchMs=0,cpuFenceWaitMs=0,mapMs=0,attemptMs=0;int waitCalls=0,matches=0;bool valid=false;};
static std::string hashInput(const std::vector<uint32_t>& x){uint64_t h=1469598103934665603ull;
  for(uint32_t v:x)for(int k=0;k<4;k++){h^=(v>>(8*k))&255;h*=1099511628211ull;}
  std::ostringstream s;s<<std::hex<<std::setw(16)<<std::setfill('0')<<h;return s.str();}
static Sample run(GLuint prog,Buffers& b,int epoch,int ordinal,const std::string& phase,const std::vector<uint32_t>& input){
  Sample s;s.epoch=epoch;s.n=b.n;s.ordinal=ordinal;s.phase=phase;s.inputHash=hashInput(input);auto start=Clock::now();
  if(!b.live||b.epoch!=epoch||b.n!=int(input.size())){s.reason="STALE_CONTEXT_RESOURCE";return s;}
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,b.id[0]);auto t=Clock::now();
  bufferSubData(GL_SHADER_STORAGE_BUFFER,0,b.n*4,input.data());s.uploadMs=ms(t,Clock::now());
  std::string uploadError=errors();if(uploadError!="NONE"){s.reason="UPLOAD:"+uploadError;return s;}
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,b.id[0]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,b.id[1]);glUseProgram(prog);
  t=Clock::now();glDispatchCompute((b.n+63)/64,1,1);s.dispatchMs=ms(t,Clock::now());
  s.dispatchError=errors();if(s.dispatchError!="NONE"){s.reason="DISPATCH";return s;}
  // Shader writes become visible to CPU buffer map/update. Fence alone is not a memory barrier.
  memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);s.barrierError=errors();
  if(s.barrierError!="NONE"){s.reason="BARRIER";return s;}
  GLsync fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);s.fenceError=errors();
  if(fence)liveSyncs++;
  if(!fence||s.fenceError!="NONE"){s.reason="FENCE_CREATE";if(fence){glDeleteSync(fence);liveSyncs--;s.cleanupError=errors();}return s;}
  auto waitStart=Clock::now(),deadline=waitStart+std::chrono::seconds(1);glFlush();
  do {GLenum w=glClientWaitSync(fence,GL_SYNC_FLUSH_COMMANDS_BIT,10000000ull);s.wait=w==GL_TIMEOUT_EXPIRED?"TIMEOUT_EXPIRED":waitName(w);
    s.waitCalls++;s.waitError=errors();if(s.waitError!="NONE"||w==GL_WAIT_FAILED){s.reason="WAIT_FAILED";break;}
    if(w==GL_ALREADY_SIGNALED||w==GL_CONDITION_SATISFIED)break;
  }while(Clock::now()<deadline);
  s.cpuFenceWaitMs=ms(waitStart,Clock::now());glDeleteSync(fence);liveSyncs--;s.cleanupError=errors();
  if(s.cleanupError!="NONE")s.reason="SYNC_CLEANUP_FAILED";
  if(s.reason=="NONE"&&s.wait=="TIMEOUT_EXPIRED")s.reason="WAIT_TIMEOUT";
  if(s.reason!="NONE")return s;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,b.id[1]);t=Clock::now();
  const uint32_t* p=(const uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,b.n*4,GL_MAP_READ_BIT);
  s.mapError=errors();if(!p||s.mapError!="NONE"){s.reason="MAP_FAILED";if(p)glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);return s;}
  s.matches=1;for(int i=0;i<b.n;i++)if(p[i]!=input[i]*3u+1u){s.matches=0;break;}
  GLboolean unmap=glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);s.unmapError=errors();s.mapMs=ms(t,Clock::now());
  if(!unmap||s.unmapError!="NONE")s.reason="UNMAP_FAILED";
  else if(!s.matches)s.reason="DATA_MISMATCH";
  s.attemptMs=ms(start,Clock::now());s.valid=s.reason=="NONE";return s;
}
static std::string json(const Sample& s){std::ostringstream j;j<<std::fixed<<std::setprecision(6);
  j<<"{\"contextEpoch\":"<<s.epoch<<",\"N\":"<<s.n<<",\"ordinal\":"<<s.ordinal
   <<",\"phase\":"<<quote(s.phase)<<",\"inputHashFnv64\":"<<quote(s.inputHash)
   <<",\"valid\":"<<(s.valid?"true":"false")<<",\"reason\":"<<quote(s.reason)
   <<",\"waitReturn\":"<<quote(s.wait)<<",\"waitCalls\":"<<s.waitCalls
   <<",\"dispatchError\":"<<quote(s.dispatchError)<<",\"barrierError\":"<<quote(s.barrierError)
   <<",\"fenceError\":"<<quote(s.fenceError)<<",\"waitError\":"<<quote(s.waitError)
   <<",\"mapError\":"<<quote(s.mapError)<<",\"unmapError\":"<<quote(s.unmapError)
   <<",\"cleanupError\":"<<quote(s.cleanupError)
   <<",\"uploadCpuMs\":"<<s.uploadMs<<",\"dispatchCpuMs\":"<<s.dispatchMs
   <<",\"fenceWaitCpuMs\":"<<s.cpuFenceWaitMs<<",\"mapCpuMs\":"<<s.mapMs
   <<",\"attemptCpuMs\":"<<s.attemptMs<<",\"gpuRuntime\":null}";return j.str();}
int main(int argc,char**argv){std::string dir=argc>1?argv[1]:".codex/local-test/t139";
  std::string runId=argc>2?argv[2]:"T139_OFFLINE_HOST";Context c;double contextMs=0,compileMs=0;
  if(!create(c,contextMs)){std::fprintf(stderr,"EGL_CREATE_FAILED 0x%x\n",eglGetError());destroy(c);return 2;}
  GLint major=0,minor=0;glGetIntegerv(GL_MAJOR_VERSION,&major);glGetIntegerv(GL_MINOR_VERSION,&minor);
  if(major<3||(major==3&&minor<1)){std::fprintf(stderr,"GLES_31_REQUIRED\n");destroy(c);return 2;}
  if(!loadFunctions()){std::fprintf(stderr,"GLES_ENTRYPOINT_MISSING\n");destroy(c);return 2;}
  GLint64 ssboMax=0;getInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&ssboMax);
  GLint computeBlocks=0,globalBindings=0;glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&computeBlocks);
  glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&globalBindings);std::string limitError=errors();
  GLuint shader=0,prog=0;std::string progError;
  if(!program(shader,prog,compileMs,progError)){std::fprintf(stderr,"PROGRAM_FAILED %s\n",progError.c_str());destroy(c);return 3;}
  livePrograms++;
  std::vector<std::string> rows,allocations;std::string lifecycle="CONTEXT1_ACTIVE";int ordinal=0;
  std::string driver=(const char*)glGetString(GL_VENDOR),renderer=(const char*)glGetString(GL_RENDERER),version=(const char*)glGetString(GL_VERSION);
  for(int n:{192,512,4096}){std::vector<uint32_t> input(n);
    for(int i=0;i<n;i++)input[i]=uint32_t(i)*2654435761u;
    Buffers b;double allocMs=0;std::string allocError;
    if(!alloc(b,n,1,input,allocMs,allocError)){
      Sample failed;failed.epoch=1;failed.n=n;failed.ordinal=++ordinal;failed.phase="BUFFER_FIRST";
      failed.reason="BUFFER_ALLOC:"+allocError;rows.push_back(json(failed));
      std::fprintf(stderr,"BUFFER_FAILED %s\n",allocError.c_str());release(b);break;
    }
    {std::ostringstream a;a<<"{\"contextEpoch\":1,\"N\":"<<n<<",\"bufferFirstCpuMs\":"<<allocMs<<"}";allocations.push_back(a.str());}
    for(int rep=0;rep<5;rep++){auto s=run(prog,b,1,++ordinal,rep==0?"BUFFER_FIRST":"PERSISTENT_WARM",input);rows.push_back(json(s));
      if(!s.valid)std::fprintf(stderr,"SAMPLE_FAILED n=%d rep=%d %s\n",n,rep,s.reason.c_str());}
    release(b);lifecycle="BUFFER_RELEASED";
  }
  glDeleteProgram(prog);livePrograms--;glDeleteShader(shader);lifecycle="CONTEXT1_DESTROYED";destroy(c);
  // A new context cannot consume old GLuint names or samples. Every result is keyed by epoch.
  double recoveryMs=0;if(!create(c,recoveryMs)){std::fprintf(stderr,"CONTEXT_RECOVERY_FAILED\n");return 4;}
  if(!loadFunctions()){std::fprintf(stderr,"RECOVERY_ENTRYPOINT_MISSING\n");destroy(c);return 4;}
  GLuint shader2=0,prog2=0;double compile2Ms=0;std::string error2;
  if(!program(shader2,prog2,compile2Ms,error2)){std::fprintf(stderr,"RECOMPILE_FAILED %s\n",error2.c_str());destroy(c);return 4;}
  livePrograms++;
  std::vector<uint32_t> input(192);for(int i=0;i<192;i++)input[i]=uint32_t(i)*2654435761u;
  Buffers recovered;double alloc2=0;std::string allocError2;
  if(!alloc(recovered,192,2,input,alloc2,allocError2)){std::fprintf(stderr,"RECOVERY_BUFFER_FAILED\n");destroy(c);return 4;}
  {std::ostringstream a;a<<"{\"contextEpoch\":2,\"N\":192,\"bufferFirstCpuMs\":"<<alloc2<<"}";allocations.push_back(a.str());}
  rows.push_back(json(run(prog2,recovered,2,++ordinal,"CONTEXT_RECOVERY_FIRST",input)));
  release(recovered);glDeleteProgram(prog2);livePrograms--;glDeleteShader(shader2);destroy(c);lifecycle="CONTEXT2_DESTROYED";
  std::ostringstream out;out<<std::fixed<<std::setprecision(6);
  out<<"{\"revision\":\"T139_COMPUTE_HARNESS_V1\",\"runId\":"<<quote(runId)
     <<",\"buildId\":\"T139_SOURCE_V1\",\"driver\":"<<quote(driver)
     <<",\"renderer\":"<<quote(renderer)<<",\"glVersion\":"<<quote(version)
     <<",\"contextCreateCpuMs\":"<<contextMs
     <<",\"compileCpuMs\":"<<compileMs<<",\"recoveryContextCpuMs\":"<<recoveryMs
     <<",\"recoveryCompileCpuMs\":"<<compile2Ms<<",\"limits\":{\"ssboMaxBlockBytes64\":"<<ssboMax
     <<",\"computeStorageBlocks\":"<<computeBlocks<<",\"globalStorageBindings\":"<<globalBindings
     <<",\"glError\":"<<quote(limitError)<<"},\"lifecycle\":"<<quote(lifecycle)
     <<",\"gpuTimerStatus\":\"NOT_MEASURED\",\"liveObjectsAtEnd\":{\"buffers\":"<<liveBuffers
     <<",\"syncs\":"<<liveSyncs<<",\"programs\":"<<livePrograms
     <<"},\"barrierBits\":[\"SHADER_STORAGE\",\"BUFFER_UPDATE\"],\"bufferAllocations\":[";
  for(size_t i=0;i<allocations.size();i++){if(i)out<<",";out<<allocations[i];}out<<"],\"samples\":[";
  for(size_t i=0;i<rows.size();i++){if(i)out<<",";out<<rows[i];}out<<"]}\n";
  write(dir+"/t139.json",out.str());std::puts("T139 harness done");return 0;
}

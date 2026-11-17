// Execution facility only. No expected physics, acceptance oracle or mock GL.
#include "gpu_resident/g9spf/transport.h"
#include "gpu_resident/g9spf/cold_contract.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <iostream>
using namespace grs::g9spf;
static uint64_t now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static std::vector<uint32_t> read(const std::filesystem::path& path,size_t maximum){std::ifstream in(path,std::ios::binary|std::ios::ate);const auto bytes=in.tellg();if(!in||bytes<0||uint64_t(bytes)>maximum||bytes%4)throw std::runtime_error("INPUT_LENGTH");std::vector<uint32_t> out(size_t(bytes)/4);in.seekg(0);for(auto& w:out){unsigned char b[4];if(!in.read((char*)b,4))throw std::runtime_error("INPUT_READ");w=uint32_t(b[0])|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;}return out;}
static void write(const std::filesystem::path& path,const uint32_t* p,size_t n){std::ofstream out(path,std::ios::binary);for(size_t i=0;i<n;i++){const uint32_t v=p[i];const char b[]={char(v),char(v>>8),char(v>>16),char(v>>24)};out.write(b,4);}out.flush();if(!out)throw std::runtime_error("OUTPUT_WRITE");}
static bool authorize(void* user,const Permit& p,uint64_t ns){return p==*static_cast<Permit*>(user)&&ns<=p.deadline;}
static std::array<uint32_t,8> hashText(const char* text){std::array<uint32_t,8> out{};auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};for(size_t i=0;i<32;i++)out[i/4]|=uint32_t(digit(text[i*2])*16+digit(text[i*2+1]))<<(8*(i%4));return out;}
struct EGLScope {
 EGLDisplay display=EGL_NO_DISPLAY;EGLContext context=EGL_NO_CONTEXT;EGLSurface surface=EGL_NO_SURFACE;
 bool Create(){display=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(display==EGL_NO_DISPLAY||!eglInitialize(display,nullptr,nullptr)||!eglBindAPI(EGL_OPENGL_ES_API))return false;const EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};EGLConfig config;EGLint n=0;if(!eglChooseConfig(display,attr,&config,1,&n)||n!=1)return false;const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},sa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);surface=eglCreatePbufferSurface(display,config,sa);return context!=EGL_NO_CONTEXT&&surface!=EGL_NO_SURFACE&&eglMakeCurrent(display,surface,surface,context);}
 ~EGLScope(){if(display!=EGL_NO_DISPLAY){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);eglTerminate(display);}}
};
int main(int argc,char** argv){
 if(argc!=3){std::cerr<<"usage: g9-differential INPUT_DIRECTORY NEW_OUTPUT_DIRECTORY\n";return 2;}
 try{
  const std::filesystem::path input=argv[1],output=argv[2];if(std::filesystem::exists(output))throw std::runtime_error("FRESH_OUTPUT_REQUIRED");std::filesystem::create_directories(output);
  auto cfg=read(input/"config.u32",64);if(cfg.size()!=16)throw std::runtime_error("CONFIG_16_WORDS_REQUIRED");
  Config config;config.k=cfg[0];config.g=cfg[1];config.muStart=cfg[2];config.muStop=cfg[3];config.band=cfg[4];config.entrainRate=cfg[5];config.shear=cfg[6];config.settleTicks=cfg[7];config.revision=cfg[8];config.research=cfg[9]==1;config.synthetic=cfg[10]==1;config.fullTrialTrace=cfg[11]==1;for(size_t i=12;i<16;i++)if(cfg[i])throw std::runtime_error("CONFIG_RESERVED");config.parameterSha=ParameterHash(config);
  const auto* profile=SelectProfile(config.k);if(!profile)throw std::runtime_error("UNSUPPORTED_K");auto ident=read(input/"identity.u32",72);if(ident.size()!=18)throw std::runtime_error("IDENTITY_18_WORDS_REQUIRED");auto p=[&](size_t i){return Pair(ident.data(),i);};ControlIdentity identity{p(0),p(2),p(4),p(6),p(8),p(10),p(12),p(14),p(16)};
  SeedFileIdentity seed;seed.state=read(input/"seed.u32",profile->stateBytes);if(seed.state.size()!=profile->stateBytes/4)throw std::runtime_error("SEED_EXACT_SIZE_REQUIRED");seed.sha=HashWords(seed.state.data(),seed.state.size());
  EGLScope egl;if(!egl.Create())throw std::runtime_error("REAL_EGL_CONTEXT_UNAVAILABLE_NO_FALLBACK");
  std::ofstream receipt(output/"execution.jsonl");receipt<<"{\"revision\":\"T549_GPU_EXECUTION_V1\",\"evidenceOrigin\":\"ACTUAL_GL_EXECUTION\",\"physicsVerdict\":\"TESTER_OWNED\",\"renderer\":\""<<(const char*)glGetString(GL_RENDERER)<<"\",\"k\":"<<config.k<<",\"residentPayloadBytes\":"<<profile->payloadBytes<<",\"fullArenaBytes\":"<<(config.fullTrialTrace?6*profile->arenaBytes:0)<<"}\n";
  Transport transport;auto result=transport.Create(config,identity,seed);seed.state.clear();seed.state.shrink_to_fit();receipt<<"{\"stage\":\"CREATE\",\"result\":"<<uint32_t(result)<<"}\n";if(result!=Result::OK){transport.Release(true);return 3;}
  auto commands=read(input/"commands.u32",4096*192*4);if(commands.size()%192)throw std::runtime_error("COMMAND_ROWS_192_WORDS_REQUIRED");uint64_t commit=0,quanta=0;
  for(size_t row=0;row<commands.size()/192;row++){
   const auto* r=commands.data()+row*192;Input128 raw;std::copy(r,r+128,raw.begin());Permit permit;permit.identity=identity;permit.nonce=Pair(r,128);permit.plan=Pair(r,130);permit.frame=Pair(r,132);permit.control=Pair(r,134);permit.geometry=Pair(r,136);permit.kind=r[138];permit.ordinal=r[139];permit.baseBank=transport.CurrentBank();permit.candidateBank=1-permit.baseBank;permit.arena=r[140];permit.arenaGeneration=Pair(r,141);permit.baseCommit=commit;permit.quanta=quanta;permit.parameterSha=config.parameterSha;permit.shaderSha=transport.ShaderHash();std::copy(r+144,r+152,permit.layoutSha.begin());permit.inputSha=HashWords(raw.data(),raw.size());if(r[143])throw std::runtime_error("COMMAND_RESERVED_143");for(size_t i=152;i<192;i++)if(r[i])throw std::runtime_error("COMMAND_RESERVED");
   // Root clock starts here. Never extend the production absolute 250ms gate.
   write(output/("root-"+std::to_string(row)+"-request.u32"),r,192);permit.deadline=now()+250000000;result=transport.Begin(permit,raw,authorize,&permit);while(result==Result::OK||result==Result::IN_PROGRESS){result=transport.Pump();if(result!=Result::IN_PROGRESS)break;std::this_thread::yield();}
   const auto stem="root-"+std::to_string(row);const auto tail=transport.Tail();write(output/(stem+"-input.u32"),raw.data(),raw.size());write(output/(stem+"-completion.u32"),tail.data(),tail.size());const auto evidence=transport.Evidence();const auto accepted=CompletionGate::Accepted(*profile,permit,tail,evidence);const auto blocked=CompletionGate::Blocked(*profile,permit,tail,evidence);
   receipt<<"{\"root\":"<<row<<",\"transportResult\":"<<uint32_t(result)<<",\"typedAccepted\":"<<(accepted?"true":"false")<<",\"typedBlocked\":"<<(blocked?"true":"false")<<",\"dispatches\":"<<evidence.dispatches<<",\"barriers\":"<<evidence.barriers<<",\"fenceNs\":\""<<evidence.fenceNs<<"\",\"deadline\":\""<<permit.deadline<<"\",\"gl\":"<<evidence.gl<<",\"egl\":"<<evidence.egl<<"}\n";
   if(accepted){if(!transport.Commit(permit,tail))throw std::runtime_error("COMMIT_FAILED");commit=Pair(tail.data(),30);quanta=Pair(tail.data(),54);}else{while(!transport.RetireRejected(permit)){if(now()>permit.deadline+1000000000)throw std::runtime_error("RETIRE_TIMEOUT_CONTEXT_MUST_RETIRE");std::this_thread::yield();}}
   cold::Layout layout;if(!cold::Layout::Make(*profile,4,layout,config.fullTrialTrace))throw std::runtime_error("COLD_LAYOUT");std::unique_ptr<uint32_t[]> block(new uint32_t[layout.words]);std::array<uint32_t,256> quota{};auto q=read(input/"quota.u32",1024);if(q.size()!=256)throw std::runtime_error("QUOTA_RAW_REQUIRED");std::copy(q.begin(),q.end(),quota.begin());const bool captured=transport.CaptureSpan(block.get(),layout.words,config.fullTrialTrace,quota.data(),row+1,row+1,now());receipt<<"{\"root\":"<<row<<",\"coldCaptured\":"<<(captured?"true":"false")<<",\"coldGl\":"<<transport.ColdGl()<<",\"coldEgl\":"<<transport.ColdEgl()<<"}\n";if(captured)write(output/(stem+"-cold.u32"),block.get(),layout.words);
   receipt.flush();if(!receipt)throw std::runtime_error("RECEIPT_IO_FAILED");if(result!=Result::OK){transport.Release(true);return 6;}
  }
  transport.Release(true);return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 4;}
}

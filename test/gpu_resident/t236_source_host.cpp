// PC ANGLE pbuffer developer execution, not an OHOS/XComponent/device result.
#include "source_transport.h"
#include "scene.h"
#include "run.h"
#include "foundation.h"
#include "angles_generated.h"
#include "source_hash.h"
#include <fstream>
#include <string>
#include <iostream>
#include <cassert>
#include <chrono>
using namespace grs;
namespace {std::string kernel;const char* kernelHash=kHostSourceHash;}
namespace grs {
const char* SourceKernelSource(){return kernel.c_str();}const char* SourceKernelSha(){return kernelHash;}
FoundationInfo GetFoundationInfo(){return {"GRS_Q20_V1","GRS_FIXED120_TIME_V1","GRS_GPU_SCENE_Q20_V1",kLayoutManifestSha256,kAngleSha256,"HOST_NOT_USED",0,false,false};}
const uint32_t* ColdAngleWords(size_t& size){size=61444;return kAngleWords;}
}
void hashWords(const char* s,std::array<uint32_t,8>& out){for(size_t i=0;i<8;i++){uint32_t value=0;for(size_t j=0;j<4;j++){char hex[3]={s[i*8+j*2],s[i*8+j*2+1],0};value|=uint32_t(std::stoul(hex,nullptr,16))<<(8*j);}out[i]=value;}}
void raw(std::array<uint32_t,32>& a,uint32_t at,double v){uint64_t bits;std::memcpy(&bits,&v,8);a[at]=uint32_t(bits);a[at+1]=uint32_t(bits>>32);}
int main(int argc,char** argv){
 bool software=false,fault=false;uint32_t K=192;for(int i=1;i<argc;i++){if(std::string(argv[i])=="--swiftshader")software=true;if(std::string(argv[i])=="--fault-unlogged")fault=true;if(std::string(argv[i])=="--k512")K=512;}
 if(fault)kernelHash=kHostFaultHash;std::ifstream file(fault?".codex/local-test/t236-shaders/m0_unlogged_fault.comp":".codex/local-test/t236-shaders/m0_source.comp",std::ios::binary);kernel.assign(std::istreambuf_iterator<char>(file),{});assert(!kernel.empty());
 EGLDisplay display;
 if(software){
  // ANGLE official eglext_angle.h constants; explicit software oracle execution only.
  // https://raw.githubusercontent.com/google/angle/main/include/EGL/eglext_angle.h
  using GetPlatform=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);auto platform=reinterpret_cast<GetPlatform>(eglGetProcAddress("eglGetPlatformDisplayEXT"));assert(platform);EGLint platformArgs[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};display=platform(0x3202,nullptr,platformArgs);
 }else display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major=0,minor=0;assert(eglInitialize(display,&major,&minor));EGLConfig config=nullptr;EGLint count=0;
 EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(display,attrs,&config,1,&count)&&count>0);assert(eglBindAPI(EGL_OPENGL_ES_API));EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,software?1:0,EGL_NONE};EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);EGLint pa[]={EGL_WIDTH,256,EGL_HEIGHT,1024,EGL_NONE};EGLSurface surface=eglCreatePbufferSurface(display,config,pa);assert(context!=EGL_NO_CONTEXT&&surface!=EGL_NO_SURFACE&&eglMakeCurrent(display,surface,surface,context));
 const char* version=reinterpret_cast<const char*>(glGetString(GL_VERSION));const char* renderer=reinterpret_cast<const char*>(glGetString(GL_RENDERER));std::cout<<(software?"PC_SWIFTSHADER_EXPLICIT_SOFTWARE ":"PC_ANGLE ")<<version<<" "<<renderer<<std::endl;
 Identity id;id.run={1,0};id.owner={2,0};id.context={1,0};id.surface={1,0};id.geometry={1,0};id.config={1,0};hashWords(kLayoutManifestSha256,id.layoutSha);hashWords(kAngleSha256,id.angleSha);hashWords(kernelHash,id.shaderSha);
 SourceGpuTransport gpu;auto created=gpu.Create(K,id);std::cout<<"CREATE "<<int(created)<<std::endl;if(created==TransportCode::CAPABILITY_UNSUPPORTED){std::cout<<"{\"scope\":\"PC_ANGLE_CAPABILITY_GATE_ONLY\",\"gate\":\"PASS_EXPECTED_UNSUPPORTED\",\"sourceExecution\":\"NOT_RUN\",\"rawGlError\":"<<gpu.Stats().firstGlError<<"}"<<std::endl;gpu.Release();eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);return 0;}
 if(created!=TransportCode::OK)return 2;
 uint32_t frame=0,nonce=0;int passed=0;
 std::array<uint32_t,32> command{};command[0]=wire::magicWords::sourceCommand;command[1]=1;command[4]=1;command[6]=10;command[8]=1;command[10]=1;command[12]=1;command[14]=1;command[15]=1;command[22]=16;command[24]=1;command[26]=1;command[27]=1;command[28]=1;raw(command,16,64.5);raw(command,18,100.);raw(command,20,.25);
 auto submit=[&](std::array<uint32_t,32> cmd,bool stop=false){frame++;RootRequest r;r.nonce={++nonce,0};r.control={frame,0};r.frame={frame,0};r.publicationSlot=(frame-1)%2;r.eventArena=(frame-1)%2;r.eventCapacity=133*K+508;
  auto s=gpu.Submit(cmd,r);assert(s==TransportCode::OK);if(stop)assert(gpu.Stop()==ControlCode::QUARANTINED);AcceptedRoot out;auto start=std::chrono::steady_clock::now();
  TransportCode code;do {code=gpu.Poll(out);if(code!=TransportCode::BUSY)break;std::this_thread::yield();}while(std::chrono::steady_clock::now()-start<std::chrono::seconds(10));
  std::cout<<"ROOT "<<frame<<" code="<<int(code)<<" outcome="<<out.outcome<<" source="<<out.source.lo<<" physical="<<out.physics.lo<<" sourceSuccess="<<out.sourceSuccess<<std::endl;
  if(!stop)assert(code==TransportCode::OK);else assert(code==TransportCode::PROTOCOL_REJECTED);return out;
 };
 auto first=submit(command);assert(first.sourceSuccess&&first.source.lo==1&&first.physics.lo==1&&first.completion[61]==262144&&first.completion[66]==2);passed++;
 assert(gpu.FreezeDiagnostics(true)==TransportCode::OK);std::vector<uint32_t> state;assert(gpu.CaptureAcceptedState(state)==TransportCode::OK);StateSegments seg;assert(PlanStateSegments(K,256,SIZE_MAX,seg));assert(state[0]==131072&&state[1]==131072&&state[K*5]==1&&state[K*5+1]==2);assert(state[seg.column/4+8*128+64]==262144);std::ofstream snapshot("test/reports/t236/resume/source-state-"+std::string(fault?"fault-":"")+std::to_string(K)+".bin",std::ios::binary);snapshot.write(reinterpret_cast<const char*>(state.data()),state.size()*4);snapshot.close();assert(gpu.FreezeDiagnostics(false)==TransportCode::OK);passed++;
 if(fault){
  auto beforeHeader=gpu.Owner().Header();RootRequest root;root.frame={2,0};root.control={2,0};root.nonce={2,0};root.quantumOrdinal=1;root.publicationSlot=1;root.eventArena=1;root.eventCapacity=133*K+508;
  assert(gpu.SubmitQuantum(root,0)==TransportCode::OK);AcceptedRoot out;TransportCode code;auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);do{code=gpu.Poll(out);if(code!=TransportCode::BUSY)break;std::this_thread::yield();}while(std::chrono::steady_clock::now()<deadline);
  auto raw=gpu.LastCompletion();std::cout<<"FAULT code="<<int(code)<<" reason="<<raw[29]<<" stage="<<raw[30]<<" word="<<raw[101]<<" expected="<<raw[102]<<" actual="<<raw[103]<<std::endl;
  assert(code==TransportCode::PROTOCOL_REJECTED&&raw[29]==16&&raw[30]==7&&raw[101]==K&&raw[102]!=raw[103]&&gpu.Owner().Header()==beforeHeader&&gpu.Owner().Bank()==1&&!out.sourceSuccess);passed++;
  uint64_t dispatches=gpu.Stats().dispatches;root.nonce={3,0};assert(gpu.SubmitQuantum(root,0)==TransportCode::PROTOCOL_REJECTED&&gpu.Stats().dispatches==dispatches);assert(gpu.FreezeDiagnostics(true)==TransportCode::OK);std::vector<uint32_t> after;assert(gpu.CaptureAcceptedState(after)==TransportCode::OK&&after==state);passed++;
  std::cout<<"{\"scope\":\"TEST_SHADER_UNLOGGED_WRITE_REJECTION\",\"passed\":"<<passed<<",\"failed\":0,\"hardwareGPU\":false}"<<std::endl;
  assert(gpu.Release()==TransportCode::OK);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);return 0;
 }
 auto replay=submit(command);assert(!replay.sourceSuccess&&replay.outcome==3&&replay.completion[61]==262144&&replay.source.lo==1&&replay.physics.lo==1);passed++;
 auto conflict=command;raw(conflict,20,.5);auto rejected=submit(conflict);assert(!rejected.sourceSuccess&&rejected.outcome==2&&rejected.completion[64]==8&&rejected.source.lo==1&&rejected.physics.lo==1);passed++;
 auto unsupported=command;unsupported[6]=11;unsupported[2]=1;auto reject2=submit(unsupported);assert(reject2.outcome==2&&reject2.completion[64]==1&&gpu.Owner().Header()[33]==2);passed++;
 auto tiny=command;tiny[6]=12;raw(tiny,20,0.5/1048576.);auto reject3=submit(tiny);assert(reject3.outcome==2&&reject3.completion[64]==3&&gpu.Owner().Header()[33]==3);passed++;
 auto cancel=command;cancel[6]=13;auto stopped=submit(cancel,true);assert(!stopped.sourceSuccess&&gpu.Owner().Header()[14]==1&&gpu.Owner().Header()[33]==3&&gpu.Stats().mappedBytes==5120);passed++;
 auto again=command;again[6]=14;again[8]=2;auto resumed=submit(again);assert(resumed.sourceSuccess&&resumed.source.lo==2&&resumed.physics.lo==2&&gpu.Owner().Header()[33]==4);passed++;
 frame++;uint32_t frameSourceEnd=0;
 for(uint32_t q=1;q<=4;q++){
  RootRequest r;r.nonce={++nonce,0};r.control={frame,0};r.frame={frame,0};r.quantumOrdinal=q;r.publicationSlot=(nonce-1)%2;r.eventArena=(frame-1)%2;r.eventCapacity=133*K+508;
  assert(gpu.SubmitQuantum(r,0)==TransportCode::OK);AcceptedRoot out;TransportCode code;auto pollDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);do{code=gpu.Poll(out);if(code!=TransportCode::BUSY)break;std::this_thread::yield();}while(std::chrono::steady_clock::now()<pollDeadline);
  const auto& rawResult=gpu.LastCompletion();std::cout<<"QUANTUM "<<q<<" code="<<int(code)<<" reason="<<rawResult[29]<<" stage="<<rawResult[30]<<" ordinal="<<rawResult[31]<<" mask="<<rawResult[97]<<" audit="<<rawResult[99]<<std::endl;
  assert(code==TransportCode::OK&&!out.sourceSuccess&&out.source.lo==2&&out.physics.lo==2+q);assert(out.completion[52]==frameSourceEnd);frameSourceEnd=out.completion[53];passed++;
 }
 assert(gpu.FreezeDiagnostics(true)==TransportCode::OK);state.clear();assert(gpu.CaptureAcceptedState(state)==TransportCode::OK);assert(state[2*K]==104850464&&int32_t(state[4*K])==-342536);assert(gpu.FreezeDiagnostics(false)==TransportCode::OK);passed++;
 auto atBed=command;atBed[6]=15;atBed[8]=2;raw(atBed,18,0.);auto bornAtBed=submit(atBed);assert(bornAtBed.sourceSuccess&&bornAtBed.source.lo==3);passed++;
 {RootRequest r;r.nonce={++nonce,0};r.control={frame,0};r.frame={frame,0};r.quantumOrdinal=1;r.publicationSlot=(nonce-1)%2;r.eventArena=(frame-1)%2;r.eventCapacity=133*K+508;assert(gpu.SubmitQuantum(r,0)==TransportCode::OK);AcceptedRoot out;TransportCode code;auto pollDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);do{code=gpu.Poll(out);if(code!=TransportCode::BUSY)break;std::this_thread::yield();}while(std::chrono::steady_clock::now()<pollDeadline);auto rawResult=gpu.LastCompletion();std::cout<<"LAND code="<<int(code)<<" reason="<<rawResult[29]<<" stage="<<rawResult[30]<<" ordinal="<<rawResult[31]<<std::endl;assert(code==TransportCode::OK);assert(gpu.FreezeDiagnostics(true)==TransportCode::OK);state.clear();assert(gpu.CaptureAcceptedState(state)==TransportCode::OK);assert(state[4]==0&&state[K*10+4]==1&&state[5]==17477&&state[seg.column/4+2*128+64]==244667);assert(state[seg.header/4+60]==786432);assert(gpu.FreezeDiagnostics(false)==TransportCode::OK);passed++;}
 auto near=command;near[6]=16;near[8]=2;raw(near,16,(65.*1048576.-1.)/1048576.);raw(near,20,.125);auto bornNear=submit(near);assert(bornNear.sourceSuccess&&bornNear.source.lo==4);passed++;
 auto quantum=[&](uint32_t ordinal,int angle){RootRequest r;r.nonce={++nonce,0};r.control={frame,0};r.frame={frame,0};r.quantumOrdinal=ordinal;r.publicationSlot=(nonce-1)%2;r.eventArena=(frame-1)%2;r.eventCapacity=133*K+508;assert(gpu.SubmitQuantum(r,angle)==TransportCode::OK);AcceptedRoot out;TransportCode code;auto pollDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);do{code=gpu.Poll(out);if(code!=TransportCode::BUSY)break;std::this_thread::yield();}while(std::chrono::steady_clock::now()<pollDeadline);auto raw=gpu.LastCompletion();std::cout<<"MOTION angle="<<angle<<" code="<<int(code)<<" reason="<<raw[29]<<" stage="<<raw[30]<<" ordinal="<<raw[31]<<std::endl;assert(code==TransportCode::OK);return out;};
 auto crossed=quantum(1,3840);assert(crossed.completion[94]>0);assert(gpu.FreezeDiagnostics(true)==TransportCode::OK);state.clear();assert(gpu.CaptureAcceptedState(state)==TransportCode::OK);assert(state[5*K+4]==7&&state[K+4]/1048576==65&&state[10*K+4]==2);assert(state[seg.column/4+8*128+65]==131072&&state[seg.column/4+2*128+64]==262144);assert(gpu.FreezeDiagnostics(false)==TransportCode::OK);passed++;
 auto bedMoved=quantum(2,7680);assert(bedMoved.completion[95]>0&&int32_t(bedMoved.completion[107])<7680);assert(gpu.FreezeDiagnostics(true)==TransportCode::OK);state.clear();assert(gpu.CaptureAcceptedState(state)==TransportCode::OK);assert(state[seg.column/4+2*128+65]>0&&state[seg.column/4+2*128+64]<262144);uint64_t bedTotal=0,poolTotal=0;for(uint32_t col=0;col<128;col++)bedTotal+=state[seg.column/4+2*128+col];for(uint32_t slot=0;slot<K;slot++)poolTotal+=state[slot];assert(bedTotal+poolTotal==917504);assert(gpu.FreezeDiagnostics(false)==TransportCode::OK);passed++;
 struct SwapContext{EGLDisplay display;EGLSurface surface;bool fail=false;} swapContext{display,surface};
 auto swap=[](void* user){auto& x=*static_cast<SwapContext*>(user);return !x.fail&&eglSwapBuffers(x.display,x.surface)==EGL_TRUE;};
 ResidentScene scene;ResidentSceneFrame render;render.run=id.run;render.context=id.context;render.surface=id.surface;render.geometry=id.geometry;render.renderFrame=1;render.width=256;render.height=1024;render.boxX=16;render.boxY=16;render.boxWidth=224;render.boxHeight=992;
 uint32_t slot=(nonce-1)%2;U64 generation{gpu.Owner().Header()[72],gpu.Owner().Header()[73]};
 auto drawn=scene.Draw(gpu,slot,generation,render,swap,&swapContext);std::cout<<"SCENE stage="<<drawn.stage<<" gl="<<drawn.rawGl<<" swap="<<drawn.swapSucceeded<<std::endl;
 assert(drawn.current&&drawn.swapSucceeded&&!drawn.displayPresentVerified&&drawn.requestedToken==drawn.renderedToken&&!drawn.renderedToken.empty());passed++;
 std::string lastGood=scene.LastGoodToken();render.renderFrame++;swapContext.fail=true;
 // Test owner-loop polling retires the real render-release fence before a second draw.
 ResidentSceneReceipt failed;auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);do{failed=scene.Draw(gpu,slot,generation,render,swap,&swapContext);if(std::string(failed.stage)!="GRS_SCENE_LEASE")break;std::this_thread::yield();}while(std::chrono::steady_clock::now()<deadline);
 assert(!failed.current&&!failed.swapSucceeded&&std::string(failed.stage)=="GRS_SCENE_SWAP"&&scene.LastGoodToken()==lastGood);passed++;
 render.renderFrame++;render.geometry.lo++;auto wrong=scene.Draw(gpu,slot,generation,render,swap,&swapContext);assert(!wrong.current&&scene.LastGoodToken()==lastGood);passed++;
 scene.Reset(true);
 ResidentRun run(gpu,{nonce,0});assert(run.Advance(1,{++frame,0},{frame,0},0,K)==ControlCode::OK&&run.Tick().complete);passed++;
 auto queued=command;queued[6]=17;queued[8]=2;raw(queued,16,100.5);raw(queued,20,.125);assert(run.Offer(queued)==ControlCode::OK);queued[6]=18;assert(run.Offer(queued)==ControlCode::OK);
 uint64_t beforeMapped=gpu.Stats().mappedBytes;assert(run.Advance(0,{frame,0},{frame,0},0,K)==ControlCode::OK&&run.Queued()==2&&gpu.Stats().mappedBytes==beforeMapped);passed++;
 auto pumpRun=[&](){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(120);while(run.Tick().active&&std::chrono::steady_clock::now()<deadline){run.Pump();std::this_thread::yield();}std::cout<<"TICK active="<<run.Tick().active<<" complete="<<run.Tick().complete<<" fault="<<run.Tick().fault<<" submitted="<<run.Tick().submitted<<" accepted="<<run.Tick().accepted<<" code="<<int(run.Tick().code)<<std::endl;assert(run.Tick().complete&&!run.Tick().fault);};
 assert(run.Advance(1,{++frame,0},{frame,0},0,K)==ControlCode::OK);pumpRun();assert(run.Tick().time.quanta==0&&run.Tick().submitted==1&&run.Queued()==1);passed++;
 assert(run.Advance(33333333,{++frame,0},{frame,0},0,K)==ControlCode::OK);pumpRun();assert(run.Tick().time.quanta==4&&run.Tick().submitted==5&&run.Tick().successfulQuanta==4&&run.Queued()==0&&gpu.Stats().mappedBytes-beforeMapped==6144);passed++;
 for(int i=0;i<16;i++)assert(run.Offer(queued)==ControlCode::OK);assert(run.Offer(queued)==ControlCode::BACKPRESSURE&&run.Queued()==16&&run.Backpressured()==1);assert(run.Pause()==ControlCode::OK&&run.Queued()==0);passed++;
 std::cout<<"{\"scope\":\"PC_SWIFTSHADER_M0_ROOT_SLICE_DEV_ONLY\",\"passed\":"<<passed<<",\"failed\":0,\"mappedBytes\":"<<gpu.Stats().mappedBytes<<",\"coldMappedBytes\":"<<gpu.Stats().coldMappedBytes<<",\"dispatches\":"<<gpu.Stats().dispatches<<"}"<<std::endl;
 auto releaseDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);TransportCode released;do{released=gpu.Release();if(released!=TransportCode::BUSY)break;std::this_thread::yield();}while(std::chrono::steady_clock::now()<releaseDeadline);assert(released==TransportCode::OK);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);return 0;
}

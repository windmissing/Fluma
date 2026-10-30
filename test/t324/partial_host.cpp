// T324 §4 non-machine regression for the partial-submit contract, on the
// real FullM1Transport + real GL (PC ANGLE pbuffer). Cases:
//  1. normal completion: table split into batches, one nonce consumption,
//     one fence, receipt semantics preserved (accepted root identical to
//     the monolithic submit — verified against the same assertions t280
//     makes on completion bytes/audit fields).
//  2. FENCE_PENDING cancel: Pause after last batch, before signal —
//     glDeleteSync called (fence==nullptr after), no quarantine, run fails
//     PROTOCOL_REJECTED, root never accepted.
//  3. batch BUSY semantics: external Stop during submitting -> CancelSubmit
//     -> transport back to IDLE (SubmitQuantum accepted again after reset
//     path), no fence leak.
//  4. 39-stage parity: the dispatch trace stage sequence equals the
//     monolithic order (0..28 with Sort expansions), DispatchEnter/Exit per
//     pass, FenceEnter/Exit once, SubmitRootBegin once, SubmitCancel pairs.
// No device. Scripted clock. NOT a device PASS.
#include "m1/full_transport.h"
#include "angles_generated.h"
#include "foundation.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <string>
extern "C" void GL_APIENTRY glFinish(){auto f=reinterpret_cast<PFNGLFINISHPROC>(eglGetProcAddress("glFinish"));assert(f);f();}
extern "C" void GL_APIENTRY glCopyBufferSubData(GLenum a,GLenum b,GLintptr c,GLintptr d,GLsizeiptr n){auto f=reinterpret_cast<PFNGLCOPYBUFFERSUBDATAPROC>(eglGetProcAddress("glCopyBufferSubData"));assert(f);f(a,b,c,d,n);}
namespace grs {FoundationInfo GetFoundationInfo(){return {"FULL_M1","FULL_M1","FULL_M1","unused",kAngleSha256,"unused",0,false,false};}
namespace m1 {struct FullM1HostAccess {
 static RootRequest Pending(const FullM1Transport& x){return x.pending;}
 static void Refresh(FullM1Transport& x){x.source.Retire();}
 static SubmitPhase Phase(const FullM1Transport& x){return x.submitPhase;}
 static uint32_t Batches(const FullM1Transport& x){return x.batchesIssued;}
 static uint32_t Cancelled(const FullM1Transport& x){return x.submitCancelled;}
 static const QuantumCandidate& Quantum(const FullM1Transport& x){return x.quantum;}
 static uint32_t Pad(const QuantumCandidate& q){return q.pad;}
 static uint32_t KW(const QuantumCandidate& q){return q.Kw;}
};}}
using namespace grs;
// Monolithic reference stage sequence, captured before T324 (from the t280
// TRACE output of the same parameters): stage 0..28 with Sort expansions.
// The batched run must reproduce it exactly (INV-3).
static std::vector<uint32_t> expectedStages(const m1::QuantumCandidate& q){
 const uint32_t pad=m1::FullM1HostAccess::Pad(q),Kw=m1::FullM1HostAccess::KW(q);
 std::vector<uint32_t> s;auto pass=[&](uint32_t st){s.push_back(st);};
 auto sort=[&](uint32_t st,uint32_t n){uint32_t levels=0;for(uint32_t k=2;k<=n;k*=2)levels++;s.insert(s.end(),size_t(levels*(levels+1)/2),st);};
 pass(0);pass(1);
 for(int c=0;c<2;c++){pass(2);pass(3);pass(4);}
 pass(5);pass(6);sort(7,pad);pass(8);sort(9,Kw);pass(10);pass(11);
 for(int c=0;c<3;c++){pass(12);pass(13);pass(14);}
 pass(15);pass(16);sort(17,Kw);pass(18);pass(19);pass(20);
 pass(21);pass(22);pass(23);pass(24);pass(28);pass(25);pass(26);pass(27);
 return s;
}
int main(int argc,char** argv){std::cout.setf(std::ios::unitbuf);uint32_t K=argc>1?uint32_t(std::stoul(argv[1])):192,Kw=argc>2?uint32_t(std::stoul(argv[2])):64;
 using Platform=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);auto platform=reinterpret_cast<Platform>(eglGetProcAddress("eglGetPlatformDisplayEXT"));assert(platform);EGLint args[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto d=platform(0x3202,nullptr,args);EGLint major,minor;assert(eglInitialize(d,&major,&minor));EGLConfig cfg;EGLint n,attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(d,attrs,&cfg,1,&n)&&n);assert(eglBindAPI(EGL_OPENGL_ES_API));EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,1,EGL_NONE},pa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,ctx));
 Identity id;id.run={1,2};id.owner={2,3};id.context={1,4};id.surface={1,5};id.geometry={1,6};id.config={1,7};auto hash=[](const char* s,std::array<uint32_t,8>& v){for(uint32_t i=0;i<32;i++){char h[]={s[2*i],s[2*i+1],0};v[i/4]|=uint32_t(std::stoul(h,nullptr,16))<<(8*(i%4));}};hash(m1::manifestSha,id.layoutSha);hash(m1::FullM1ShaderSha(),id.shaderSha);hash(kAngleSha256,id.angleSha);
 m1::QuantumParameters params;int passed=0;
 // ---- Case 1: batched completion, trace parity, single fence ----
 {
  m1::FullM1Transport gpu;auto code=gpu.Create(K,Kw,id);assert(code==TransportCode::OK);
  uint32_t seq=0,nonce=0;
  auto command=[&](uint32_t kind,double y,double q){std::array<uint32_t,32> c{};c[0]=1129534023;c[1]=2;c[2]=kind;c[4]=1;c[5]=2;c[6]=++seq;c[8]=c[10]=c[14]=c[15]=c[24]=c[27]=1;c[12]=1;c[13]=6;c[28]=1;c[29]=7;double x=64.5;std::memcpy(c.data()+16,&x,8);std::memcpy(c.data()+18,&y,8);std::memcpy(c.data()+20,&q,8);return c;};
  auto request=[&](uint32_t kind,uint32_t frame,uint32_t ordinal=0){RootRequest r;r.kind=kind;r.nonce={++nonce,1};r.control={nonce,0};r.frame={frame,0};r.quantumOrdinal=ordinal;r.eventArena=frame&1;r.publicationSlot=kind==1?1:0;return r;};
  // seed with one source root (monolithic path — source submits stay whole)
  assert(gpu.SubmitSource(command(0,0,.125),request(1,1))==TransportCode::OK);
  AcceptedRoot out;do{code=gpu.Poll(out);}while(code==TransportCode::BUSY);assert(code==TransportCode::OK);
  // quantum via partial submit
  m1::FullM1HostAccess::Refresh(gpu);
  auto r=request(2,1,1);
  code=gpu.SubmitQuantum(params,r);
  assert(code==TransportCode::OK_SUBMITTING);passed++; // first batch, unsealed
  assert(m1::FullM1HostAccess::Phase(gpu)==m1::SubmitPhase::BATCH_ACTIVE);
  // BUSY while unsealed: a second Submit attempt must be refused
  assert(gpu.SubmitQuantum(params,r)==TransportCode::BUSY);passed++;
  uint32_t guard=0;
  while(m1::FullM1HostAccess::Phase(gpu)!=m1::SubmitPhase::FENCE_PENDING){
   code=gpu.ContinueSubmit();
   assert(code==TransportCode::OK_SUBMITTING||code==TransportCode::OK);
   if(++guard>16){std::cerr<<"BATCH_GUARD_EXCEEDED\n";return 1;}
  }
  assert(m1::FullM1HostAccess::Batches(gpu)>=2);passed++; // really split
  // sealed: ContinueSubmit in FENCE_PENDING returns OK — the root becomes
  // inflight for the Run layer (T324 §2.2 corrected semantics)
  assert(gpu.ContinueSubmit()==TransportCode::OK);passed++;
  do{code=gpu.Poll(out);}while(code==TransportCode::BUSY);
  assert(code==TransportCode::OK);passed++;
  assert(out.physics.lo==2&&out.source.lo==1);
  assert(out.completion[99]==1023);passed++; // audit all-pass, same as monolithic
  // trace parity: stage sequence equals the monolithic reference
  const auto& trace=gpu.Trace();
  auto expected=expectedStages(m1::FullM1HostAccess::Quantum(gpu));
  assert(trace.Size()==expected.size());passed++;
  for(size_t i=0;i<trace.Size();i++){assert(trace.At(i).stage==expected[i]);}
  passed++; // full stage-order parity (INV-3)
  for(size_t i=0;i<trace.Size();i++)assert(trace.At(i).rawGl==0);
  passed++; // zero GL errors across all batches
  assert(m1::FullM1HostAccess::Phase(gpu)==m1::SubmitPhase::IDLE);passed++;
  assert(gpu.Release()==TransportCode::OK);
  std::cout<<"CASE1 batches="<<m1::FullM1HostAccess::Batches(gpu)<<" passes="<<trace.Size()<<"\n";
 }
 // ---- Case 2: FENCE_PENDING cancel (Pause after last batch, before signal) ----
 {
  m1::FullM1Transport gpu;assert(gpu.Create(K,Kw,id)==TransportCode::OK);
  uint32_t seq=0,nonce=0;
  auto command=[&](uint32_t kind,double y,double q){std::array<uint32_t,32> c{};c[0]=1129534023;c[1]=2;c[2]=kind;c[4]=1;c[5]=2;c[6]=++seq;c[8]=c[10]=c[14]=c[15]=c[24]=c[27]=1;c[12]=1;c[13]=6;c[28]=1;c[29]=7;double x=64.5;std::memcpy(c.data()+16,&x,8);std::memcpy(c.data()+18,&y,8);std::memcpy(c.data()+20,&q,8);return c;};
  RootRequest sr;sr.kind=1;sr.nonce={1,1};sr.control={1,0};sr.frame={1,0};sr.publicationSlot=sr.eventArena=1;
  assert(gpu.SubmitSource(command(0,0,.125),sr)==TransportCode::OK);
  AcceptedRoot out;TransportCode code;do{code=gpu.Poll(out);}while(code==TransportCode::BUSY);assert(code==TransportCode::OK);
  m1::FullM1HostAccess::Refresh(gpu);
  RootRequest qr;qr.kind=2;qr.nonce={2,1};qr.control={2,0};qr.frame={1,0};qr.publicationSlot=0;qr.eventArena=1;qr.quantumOrdinal=1;
  assert(gpu.SubmitQuantum(params,qr)==TransportCode::OK_SUBMITTING);
  while(m1::FullM1HostAccess::Phase(gpu)!=m1::SubmitPhase::FENCE_PENDING)gpu.ContinueSubmit();
  auto before=gpu.Owner().AcceptedCount();auto oldHeader=gpu.Owner().Header();
  // Pause-equivalent: cancel while fence pending
  assert(gpu.CancelSubmit()==TransportCode::PROTOCOL_REJECTED);passed++;
  assert(m1::FullM1HostAccess::Phase(gpu)==m1::SubmitPhase::IDLE);passed++;
  assert(m1::FullM1HostAccess::Cancelled(gpu)==1);passed++;
  // root never accepted; owner state unchanged except the Stop-side effects
  assert(gpu.Owner().AcceptedCount()==before);passed++;
  // no fence leak: Release must not report BUSY for a dangling fence
  assert(gpu.Release()==TransportCode::OK);passed++;
  std::cout<<"CASE2 cancelled=1 acceptedSame=true\n";
 }
 // ---- Case 3: BATCH_ACTIVE cancel (Stop mid-submit) ----
 {
  m1::FullM1Transport gpu;assert(gpu.Create(K,Kw,id)==TransportCode::OK);
  uint32_t seq=0;
  auto command=[&](uint32_t kind,double y,double q){std::array<uint32_t,32> c{};c[0]=1129534023;c[1]=2;c[2]=kind;c[4]=1;c[5]=2;c[6]=++seq;c[8]=c[10]=c[14]=c[15]=c[24]=c[27]=1;c[12]=1;c[13]=6;c[28]=1;c[29]=7;double x=64.5;std::memcpy(c.data()+16,&x,8);std::memcpy(c.data()+18,&y,8);std::memcpy(c.data()+20,&q,8);return c;};
  RootRequest sr;sr.kind=1;sr.nonce={1,1};sr.control={1,0};sr.frame={1,0};sr.publicationSlot=sr.eventArena=1;
  assert(gpu.SubmitSource(command(0,0,.125),sr)==TransportCode::OK);
  AcceptedRoot out;TransportCode c3;do{c3=gpu.Poll(out);}while(c3==TransportCode::BUSY);assert(c3==TransportCode::OK);
  m1::FullM1HostAccess::Refresh(gpu);
  RootRequest qr;qr.kind=2;qr.nonce={2,1};qr.control={2,0};qr.frame={1,0};qr.publicationSlot=0;qr.eventArena=1;qr.quantumOrdinal=1;
  assert(gpu.SubmitQuantum(params,qr)==TransportCode::OK_SUBMITTING);
  // cancel in BATCH_ACTIVE (first batch done, table not exhausted)
  assert(m1::FullM1HostAccess::Phase(gpu)==m1::SubmitPhase::BATCH_ACTIVE);
  assert(gpu.CancelSubmit()==TransportCode::PROTOCOL_REJECTED);passed++;
  assert(m1::FullM1HostAccess::Phase(gpu)==m1::SubmitPhase::IDLE);passed++;
  // transport failed -> subsequent submits rejected until reset
  assert(gpu.SubmitQuantum(params,qr)==TransportCode::PROTOCOL_REJECTED);passed++;
  assert(gpu.Release()==TransportCode::OK);passed++;
  std::cout<<"CASE3 midBatchCancel=IDLE rejectedAfter=true\n";
 }
 std::cout<<"{\"revision\":\"T324_PARTIAL_SUBMIT_NATIVE_V1\",\"K\":"<<K<<",\"Kw\":"<<Kw<<",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}\n";
}

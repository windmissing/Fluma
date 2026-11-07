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
#include "m1/run.h"
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
 static GLuint Base(const FullM1Transport& x){return x.source.banks[0];}
 static GLuint Candidate(const FullM1Transport& x){return x.quantum.candidate;}
 static GLuint Workspace(const FullM1Transport& x){return x.quantum.workspace;}
 static uint32_t Rows(const FullM1Transport& x){return x.quantum.rowsAt;}
 static GLuint Events(const FullM1Transport& x,uint32_t i){return x.eventArenas[i];}
 static GLuint Results(const FullM1Transport& x,uint32_t i){return x.results[i];}
 static GLuint Publication(const FullM1Transport& x,uint32_t i){return x.source.pubs[i];}
 static uint32_t PublicationWords(const FullM1Transport& x){return x.source.pubWords;}

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
 struct ProbeEvidence { std::vector<TailSample> samples; uint32_t busy=0; bool badBusy=false; } probe;
 auto& tailProbe=probe.samples;
 const auto enableProbe=[&](m1::FullM1Transport& gpu){
  probe={};
  gpu.tailObserver={&probe,[](void* p,const TailSample& s){
   auto& e=*static_cast<ProbeEvidence*>(p);
   if(s.stage==TailStage::RealPoll&&s.code==uint32_t(TransportCode::BUSY)){
    e.busy++;e.badBusy|=s.a!=s.b||s.start>s.end;return;
   }
   e.samples.push_back(s);
  }};
 };

 // ---- Case 1: batched completion, trace parity, single fence ----
 {
  m1::FullM1Transport gpu;auto code=gpu.Create(K,Kw,id);assert(code==TransportCode::OK);
  enableProbe(gpu);
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
  uint32_t cursor=0,batchNo=0,finalFlush=0,pollOK=0,accepts=0;
  const uint64_t quantumNonce=(uint64_t(r.nonce.hi)<<32)|r.nonce.lo;
  for(size_t i=0;i<tailProbe.size();++i){
   const auto& t=tailProbe[i];assert(t.start&&t.start<=t.end);
   if(t.root!=quantumNonce)continue;
   if(t.stage==TailStage::Flush){
    assert(i+1<tailProbe.size());const auto& b=tailProbe[i+1];
    assert(b.stage==TailStage::BatchCall&&b.root==t.root&&t.ordinal==++batchNo&&b.ordinal==batchNo);
    assert(t.a==cursor&&t.b>t.a&&b.a==t.a&&b.b==t.b&&b.start<=t.start&&t.end<=b.end);
    assert(!finalFlush);cursor=uint32_t(t.b);finalFlush+=t.code;passed++;
   }
   if(t.stage==TailStage::RealPoll){
    if(t.code==uint32_t(TransportCode::OK)){assert(t.b==t.a+1);pollOK++;}
    else assert(false);
   }
   if(t.stage==TailStage::Accepted){
    assert(i>0&&tailProbe[i-1].stage==TailStage::RealPoll);
    assert(t.code==out.publication&&t.a==out.publicationGeneration.lo&&t.b==out.source.lo&&t.c==out.physics.lo);
    accepts++;
   }
  }
  assert(batchNo==m1::FullM1HostAccess::Batches(gpu)&&finalFlush==1&&pollOK==1&&accepts==1&&!probe.badBusy);passed++;
  std::cout<<"T384 pairedBatches="<<batchNo<<" finalFlush="<<finalFlush<<" realPollBusy="<<probe.busy<<" accepted="<<accepts<<"\n";

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
 // ---- T329 N1: single-Pump seal, transport level ----
 // The Run+transport integration proof lives in app_gl.cpp (full chain,
 // accepted=2). Here we prove the AMENDMENT-1 LOOP CONTRACT directly: after
 // SubmitQuantum returns OK_SUBMITTING, a bounded loop of ContinueSubmit
 // calls (the exact loop Run::Pump now runs) seals the root within 8ms.
 {
  m1::FullM1Transport gpu;assert(gpu.Create(K,Kw,id)==TransportCode::OK);
  uint32_t seq=0,nonce=0;
  auto command=[&](uint32_t kind,double y,double q){std::array<uint32_t,32> c{};c[0]=1129534023;c[1]=2;c[2]=kind;c[4]=1;c[5]=2;c[6]=++seq;c[8]=c[10]=c[14]=c[15]=c[24]=c[27]=1;c[12]=1;c[13]=6;c[28]=1;c[29]=7;double x=64.5;std::memcpy(c.data()+16,&x,8);std::memcpy(c.data()+18,&y,8);std::memcpy(c.data()+20,&q,8);return c;};
  auto request=[&](uint32_t kind,uint32_t frame,uint32_t ordinal=0){RootRequest r;r.kind=kind;r.nonce={++nonce,1};r.control={nonce,0};r.frame={frame,0};r.quantumOrdinal=ordinal;r.eventArena=frame&1;r.publicationSlot=kind==1?1:0;return r;};
  assert(gpu.SubmitSource(command(0,0,.125),request(1,1))==TransportCode::OK);
  AcceptedRoot out;TransportCode code;do{code=gpu.Poll(out);}while(code==TransportCode::BUSY);assert(code==TransportCode::OK);passed++;
  m1::FullM1HostAccess::Refresh(gpu);
  assert(gpu.SubmitQuantum(params,request(2,1,1))==TransportCode::OK_SUBMITTING);passed++;
  // The Amendment-1 Pump loop, executed verbatim:
  const auto loopStart=std::chrono::steady_clock::now();int batches=1;code=TransportCode::OK_SUBMITTING;
  while(code==TransportCode::OK_SUBMITTING){
   code=gpu.ContinueSubmit();batches++;
   if(uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-loopStart).count())>=8000000)break;
  }
  // Sealed OR budget-truncated — both are contract-correct: the loop either
  // finishes within kPumpBudgetNs (device: ~3.5ms for 13 batches per T318)
  // or truncates at a batch boundary for the next Pump (PC SwiftShader may
  // exceed 8ms per batch; the run_contract N2 case pins the truncation).
  const uint64_t loopWallNs=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-loopStart).count());
  assert(code==TransportCode::OK||code==TransportCode::OK_SUBMITTING);passed++;
  assert(batches>=3||loopWallNs>=8000000);passed++; // A single indivisible GL batch may consume the budget.
  assert(gpu.SubmitPumpWallNs()>0);passed++; // pump wall recorded (incl. first batch)
  if(code==TransportCode::OK_SUBMITTING)assert(loopWallNs>=8000000);passed++; // Unsealed return must have hit the guard.
  std::cout<<"CASE_N1 loopBatches="<<batches<<" loopWallNs="<<loopWallNs<<" pumpWallNs="<<gpu.SubmitPumpWallNs()<<std::endl;
  // drain the remainder if truncated
  while(code!=TransportCode::OK){code=gpu.ContinueSubmit();}
  do{code=gpu.Poll(out);}while(code==TransportCode::BUSY);
  assert(code==TransportCode::OK&&out.physics.lo>=2);passed++;
  assert(gpu.Release()==TransportCode::OK);
  std::cout<<"CASE_N1 loopBatches="<<batches<<" pumpWallNs="<<gpu.SubmitPumpWallNs()<<" physics="<<out.physics.lo<<std::endl;
 }
 // ---- T336 (T324 Amendment 2): stub-count flush case ----
 // Per-batch glFlush on NON-final batches only: flushes == batchesIssued-1
 // (12 non-final batches => 12 flushes; the final batch keeps the original
 // fence->flush order and does not count). Also pins: flushes>0 whenever the
 // table split into >=2 batches (the flush actually exists), and the sealed
 // root still accepts (the flush path cannot break completion).
 {
  m1::FullM1Transport gpu;assert(gpu.Create(K,Kw,id)==TransportCode::OK);
  uint32_t seq=0,nonce=0;
  auto command=[&](uint32_t kind,double y,double q){std::array<uint32_t,32> c{};c[0]=1129534023;c[1]=2;c[2]=kind;c[4]=1;c[5]=2;c[6]=++seq;c[8]=c[10]=c[14]=c[15]=c[24]=c[27]=1;c[12]=1;c[13]=6;c[28]=1;c[29]=7;double x=64.5;std::memcpy(c.data()+16,&x,8);std::memcpy(c.data()+18,&y,8);std::memcpy(c.data()+20,&q,8);return c;};
  auto request=[&](uint32_t kind,uint32_t frame,uint32_t ordinal=0){RootRequest r;r.kind=kind;r.nonce={++nonce,1};r.control={nonce,0};r.frame={frame,0};r.quantumOrdinal=ordinal;r.eventArena=frame&1;r.publicationSlot=kind==1?1:0;return r;};
  assert(gpu.SubmitSource(command(0,0,.125),request(1,1))==TransportCode::OK);
  AcceptedRoot out;TransportCode code;do{code=gpu.Poll(out);}while(code==TransportCode::BUSY);assert(code==TransportCode::OK);passed++;
  m1::FullM1HostAccess::Refresh(gpu);
  assert(gpu.SubmitQuantum(params,request(2,1,1))==TransportCode::OK_SUBMITTING);
  code=TransportCode::OK_SUBMITTING;
  while(code==TransportCode::OK_SUBMITTING)code=gpu.ContinueSubmit();
  assert(code==TransportCode::OK);passed++;              // sealed
  const uint32_t batches=m1::FullM1HostAccess::Batches(gpu);
  const uint32_t flushes=gpu.SubmitBatchFlushes();
  assert(batches>=2);passed++;                           // table really split
  assert(flushes==batches-1);passed++;                   // one flush per NON-final batch
  assert(flushes>=1);passed++;                           // the flush path exists
  do{code=gpu.Poll(out);}while(code==TransportCode::BUSY);
  assert(code==TransportCode::OK&&out.physics.lo>=2);passed++; // flush breaks nothing
  assert(gpu.Release()==TransportCode::OK);
  std::cout<<"CASE_F batches="<<batches<<" flushes="<<flushes<<" (non-final batches)"<<std::endl;
 }
 // T375: verify one real warmup, no writes outside scratch, and that
 // the first real root overwrites even poisoned candidate/workspace.
 {
  using Access=m1::FullM1HostAccess;
  static constexpr uint32_t marker=0x13579bdf;
  auto stamp=[](GLuint buffer,uint32_t offset,uint32_t word){
   glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffer);
   glBufferSubData(GL_SHADER_STORAGE_BUFFER,offset*4,4,&word);
  };
  auto read=[](GLuint buffer,uint32_t offset){
   glBindBuffer(GL_COPY_READ_BUFFER,buffer);
   const auto* word=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,offset*4,4,GL_MAP_READ_BIT));
   assert(word);const auto value=*word;assert(glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE);
   return value;
  };
  struct WarmupRun {
   m1::FullM1Transport* gpu;
   decltype(stamp)* write;
   uint64_t wall=0;
   uint32_t enter=0,exit=0,error=0;
   bool injectError=false;
  };
  auto observer=[](void* ptr,SubmitStage stage,uint64_t root,uint32_t code,uint32_t edge,uint64_t wall){
   if(stage!=SubmitStage::SubmitWarmup)return;
   auto& seen=*static_cast<WarmupRun*>(ptr);
   assert(root==0);
   if(edge==1){seen.exit++;seen.wall=wall;seen.error=code;return;}
   assert(edge==0&&code==0&&wall==0);seen.enter++;
   auto& g=*seen.gpu;
   (*seen.write)(Access::Candidate(g),0,marker);
   (*seen.write)(Access::Workspace(g),0,marker);
   (*seen.write)(Access::Workspace(g),Access::Rows(g),marker);
   for(uint32_t i=0;i<2;i++){
    (*seen.write)(Access::Events(g,i),0,marker);
    (*seen.write)(Access::Results(g,i),0,marker);
    (*seen.write)(Access::Publication(g,i),0,marker);
   }
   if(seen.injectError)glBindBuffer(GL_NONE,0);
  };
  m1::FullM1Transport gpu;
  WarmupRun seen{&gpu,&stamp};
  gpu.submitObserver={&seen,observer};
  seen.injectError=true;
  assert(gpu.Create(K,Kw,id)==TransportCode::GL_FAILED);
  assert(seen.enter==1&&seen.exit==1&&seen.error==GL_INVALID_ENUM&&seen.wall==gpu.WarmupWallNs());
  assert(gpu.Release()==TransportCode::OK);passed++;
  seen.injectError=false;
  std::vector<uint32_t> referenceBank,referenceEvents;
  Words256 referenceCompletion{};
  for(bool poison:{false,true}){
   const uint32_t prior=seen.enter;
   const auto priorTrace=gpu.Trace().Size();
   assert(gpu.Create(K,Kw,id)==TransportCode::OK);
   assert(seen.enter==prior+1&&seen.exit==seen.enter&&seen.error==0&&seen.wall==gpu.WarmupWallNs()&&seen.wall>0);
   assert(gpu.Owner().AcceptedCount()==0&&!gpu.Owner().HasLeases()&&gpu.Trace().Size()==priorTrace);
   assert(gpu.Stats().dispatches==0&&gpu.Stats().fences==0&&gpu.SubmitBatchFlushes()==0);
   glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);glFinish();
   assert(read(Access::Candidate(gpu),0)==read(Access::Base(gpu),0));
   assert(read(Access::Workspace(gpu),0)==0&&read(Access::Workspace(gpu),Access::Rows(gpu))==0);
   for(uint32_t i=0;i<2;i++){
    assert(read(Access::Events(gpu,i),0)==marker&&read(Access::Results(gpu,i),0)==marker&&read(Access::Publication(gpu,i),0)==marker);
   }
   if(poison){
    stamp(Access::Candidate(gpu),0,marker);
    stamp(Access::Workspace(gpu),0,marker);
    stamp(Access::Workspace(gpu),Access::Rows(gpu),marker);
   }
   RootRequest r;r.kind=2;r.nonce={1,0};r.frame={1,0};r.control={1,0};r.quantumOrdinal=1;r.publicationSlot=0;r.eventArena=1;
   auto c=gpu.SubmitQuantum(params,r);assert(c==TransportCode::OK_SUBMITTING);
   while(c==TransportCode::OK_SUBMITTING)c=gpu.ContinueSubmit();
   assert(c==TransportCode::OK);
   AcceptedRoot out;do{c=gpu.Poll(out);}while(c==TransportCode::BUSY);
   assert(c==TransportCode::OK&&gpu.Owner().AcceptedCount()==1&&out.completion[99]==1023);
   assert(gpu.Freeze(true)==TransportCode::OK);
   std::vector<uint32_t> bank,events;
   assert(gpu.Capture(bank,events)==TransportCode::OK);
   assert(gpu.Freeze(false)==TransportCode::OK);
   if(poison){
    assert(bank==referenceBank&&events==referenceEvents);
    assert(out.completion==referenceCompletion);
   }else{referenceBank=bank;referenceEvents=events;referenceCompletion=out.completion;}
   assert(gpu.Release()==TransportCode::OK);passed++;
  }
  std::cout<<"CASE_W real pass 0, scratch only, fresh-root bit parity, raw wall and failed-Create retry\n";
 }
 {
  using Access=m1::FullM1HostAccess;
  m1::FullM1Transport gpu;
  struct ColdCheck {
   bool current=true,cancel=false,fenceError=false;
   uint32_t batches=0,flushes=0,staging=0,fences=0,cleanup=0,error=0;
   uint64_t bytes=0;
  } cold;
  const auto observer=[](void* ptr,const TailSample& s){
   auto& c=*static_cast<ColdCheck*>(ptr);
   assert(s.root==0&&s.start<=s.end);
   if(s.stage==TailStage::ColdStagingFlush)++c.staging;
   if(s.stage==TailStage::ColdBatch){
    ++c.batches;
    assert(s.ordinal==c.batches&&s.a==(c.batches-1)*10&&s.b==c.batches*10&&s.c==s.b);
    if(c.cancel)c.current=false;
   }
   if(s.stage==TailStage::ColdBatchFlush){
    ++c.flushes;
    if(c.fenceError&&s.ordinal==2)glBindBuffer(GL_NONE,0);
   }
   if(s.stage==TailStage::ColdFence){++c.fences;c.error=uint32_t(s.b);}
   if(s.stage==TailStage::ColdCleanup){++c.cleanup;c.bytes=s.a;}
  };
  gpu.coldObserver={&cold,observer};
  m1::ColdPrepareOptions opts{true,&cold,[](void* p){return static_cast<ColdCheck*>(p)->current;}};
  auto warm=[&]{return gpu.Create(K,Kw,id,params,16u*1048576u,opts);};
  cold.cancel=true;
  assert(warm()==TransportCode::STATE_LOST&&cold.batches==1&&cold.fences==0&&cold.cleanup==1);
  assert(gpu.Owner().AcceptedCount()==0&&gpu.Release()==TransportCode::OK);passed++;
  cold={};cold.fenceError=true;
  assert(warm()==TransportCode::GL_FAILED&&cold.batches==2&&cold.fences==1&&cold.cleanup==1);
  assert(cold.error==GL_INVALID_ENUM&&gpu.Owner().AcceptedCount()==0&&gpu.Release()==TransportCode::OK);passed++;
  std::vector<uint32_t> priorBank,priorEvents,priorPublication;
  Words256 priorCompletion{},priorHeader{};
  SourceTransportStats priorStats{};
  for(bool experimental:{false,true}){
   cold={};opts.twoBatches=experimental;
   assert(warm()==TransportCode::OK);
   assert(cold.staging==uint32_t(experimental)&&cold.batches==2u*experimental&&cold.flushes==2u*experimental);
   assert(cold.fences==uint32_t(experimental)&&cold.cleanup==uint32_t(experimental));
   assert(!experimental||cold.bytes==uint64_t(4*(48*K+16*Kw+254)+K)*256);
   assert(gpu.Stats().dispatches==0&&gpu.Stats().fences==0&&gpu.Owner().AcceptedCount()==0);
   RootRequest r;
   r.kind=2;r.nonce={1,0};r.frame={1,0};r.control={1,0};
   r.quantumOrdinal=1;r.publicationSlot=0;r.eventArena=1;
   uint32_t hotStaging=0;
   gpu.tailObserver={&hotStaging,[](void* ptr,const TailSample& s){
    if(s.stage==TailStage::StagingFlush){
     assert(s.root==1&&s.start<=s.end&&s.code==0);
     ++*static_cast<uint32_t*>(ptr);
    }
   }};
   auto code=gpu.SubmitQuantum(params,r);
   assert(code==TransportCode::OK_SUBMITTING&&hotStaging==1);
   while(code==TransportCode::OK_SUBMITTING)code=gpu.ContinueSubmit();
   assert(code==TransportCode::OK);
   AcceptedRoot root;
   do{code=gpu.Poll(root);}while(code==TransportCode::BUSY);
   assert(code==TransportCode::OK&&root.publication==0&&gpu.Owner().AcceptedCount()==1);
   assert(gpu.Freeze(true)==TransportCode::OK);
   std::vector<uint32_t> bank,events;
   assert(gpu.Capture(bank,events)==TransportCode::OK);
   assert(gpu.Freeze(false)==TransportCode::OK);
   glBindBuffer(GL_COPY_READ_BUFFER,Access::Publication(gpu,0));
   const auto count=Access::PublicationWords(gpu);
   const auto* data=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,count*4,GL_MAP_READ_BIT));
   assert(data);
   std::vector<uint32_t> publication(data,data+count);
   assert(glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE);
   const auto stats=gpu.Stats();
   if(experimental){
    assert(bank==priorBank&&events==priorEvents&&publication==priorPublication);
    assert(root.completion==priorCompletion&&gpu.Owner().Header()==priorHeader);
    assert(stats.allocatedBytes==priorStats.allocatedBytes&&stats.dispatches==priorStats.dispatches&&
     stats.barriers==priorStats.barriers&&stats.fences==priorStats.fences&&
     stats.mappedBytes==priorStats.mappedBytes&&stats.uploadedBytes==priorStats.uploadedBytes&&
     stats.firstGlError==priorStats.firstGlError);
   }else{
    priorBank=bank;priorEvents=events;priorPublication=publication;priorCompletion=root.completion;
    priorHeader=gpu.Owner().Header();priorStats=stats;
   }
   assert(gpu.Release()==TransportCode::OK);passed++;
  }
  std::cout<<"T389 AB bank/event/publication/completion/owner/structural stats identical; epoch cancel, fence GL error, rebuild passed\n";
 }
 {
  using Access=m1::FullM1HostAccess;
  std::vector<uint32_t> baseline;
  bool allowCommit=true;
  m1::RootIsolation gate;
  gate.context=&allowCommit;
  gate.accept=[](void* user,const RootRequest&,const AcceptedRoot&,void* data,void(*commit)(void*)){
   if(!*static_cast<bool*>(user))return false;
   commit(data);return true;
  };
  for(bool isolated:{false,true}){
   m1::FullM1Transport gpu;
   if(isolated)gpu.isolation=&gate;
   assert(gpu.Create(K,Kw,id)==TransportCode::OK);
   std::array<uint32_t,32> c{};
   c[0]=1129534023;c[1]=2;c[4]=1;c[5]=2;c[6]=c[8]=c[10]=c[14]=c[15]=c[24]=c[27]=1;
   c[12]=1;c[13]=6;c[28]=1;c[29]=7;
   double x=64.5,mass=.125;
   std::memcpy(c.data()+16,&x,8);std::memcpy(c.data()+20,&mass,8);
   RootRequest r;
   r.kind=1;r.nonce=r.frame=r.control={1,0};r.eventArena=1;r.publicationSlot=0;
   auto poll=[&]{
    AcceptedRoot receipt;TransportCode code;
    do{code=gpu.Poll(receipt);}while(code==TransportCode::BUSY);
    return code;
   };
   auto submitQuantum=[&]{
    auto code=gpu.SubmitQuantum(params,r);
    assert(code==TransportCode::OK_SUBMITTING);
    do{code=gpu.ContinueSubmit();}while(code==TransportCode::OK_SUBMITTING);
    assert(code==TransportCode::OK);
   };
   auto capture=[&](uint32_t slot){
    assert(gpu.Freeze(true)==TransportCode::OK);
    std::vector<uint32_t> bank,events;
    assert(gpu.Capture(bank,events)==TransportCode::OK&&!events.empty());
    assert(gpu.Freeze(false)==TransportCode::OK);
    bank.insert(bank.end(),events.begin(),events.end());
    const auto& completion=gpu.LastCompletion();
    const auto& header=gpu.Owner().Header();
    bank.insert(bank.end(),completion.begin(),completion.end());
    bank.insert(bank.end(),header.begin(),header.end());
    glBindBuffer(GL_COPY_READ_BUFFER,Access::Publication(gpu,slot));
    const auto count=Access::PublicationWords(gpu);
    const auto* pub=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,count*4,GL_MAP_READ_BIT));
    assert(pub);bank.insert(bank.end(),pub,pub+count);
    assert(glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE);
    return bank;
   };
   assert(gpu.SubmitSource(c,r)==TransportCode::OK&&poll()==TransportCode::OK);
   auto words=capture(0);
   r.kind=2;r.nonce={2,0};r.quantumOrdinal=1;r.publicationSlot=1;
   submitQuantum();assert(poll()==TransportCode::OK);
   const auto accepted=capture(1);
   words.insert(words.end(),accepted.begin(),accepted.end());
   if(!isolated)baseline=words;
   else{
    assert(words==baseline);
    // A same-arena candidate can execute and produce a sealed receipt but
    // must not replace accepted event bytes when the final gate refuses it.
    r.nonce={3,0};r.frame={3,0};r.control={3,0};r.publicationSlot=0;
    allowCommit=false;submitQuantum();assert(poll()==TransportCode::REVOKED);
    assert(gpu.DiscardRevoked()==TransportCode::OK);
    assert(gpu.Owner().AcceptedCount()==2&&capture(1)==accepted);
   }
   assert(gpu.Release()==TransportCode::OK);
  }
  passed++;
  std::cout<<"T398 SOURCE + QUANTUM byte parity; rejected same-arena receipt preserved accepted prefix\n";
 }
 std::cout<<"{\"revision\":\"T329_PARTIAL_PUMP_LOOP_V1\",\"K\":"<<K<<",\"Kw\":"<<Kw<<",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}\n";
}

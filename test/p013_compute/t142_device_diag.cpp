// T142 standalone device diagnostic: run the pair chain, then dump the top table layer,
// candidate buffer and result words for tester-side oracle comparison.
// Reuses T141 executor/prepare/owner code verbatim; only adds dumps. No product code.
#include "pair_prepare_v1.h"
#include "pair_executor_v1.h"
#include "pair_owner_v1.h"
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>

using namespace p011gpu;
struct Pool {
 uint32_t nmax,cols=128;std::vector<double> mass,x,y,vx,vy,colH,colL;
 std::vector<uint8_t> phase,source,state;std::vector<uint32_t> id,sourceId;
 std::vector<int32_t> freeSlots;SnapshotView view;
 explicit Pool(uint32_t cap):nmax(cap),mass(cap,1),x(cap),y(cap),vx(cap),vy(cap),colH(cols),colL(cols),
  phase(cap),source(cap),state(cap),id(cap),sourceId(cap),freeSlots(cap){
  for(uint32_t i=0;i<cap;i++){state[i]=(i%11)!=0;x[i]=double((i*7)%16)+.5;
   y[i]=double((i*13)%32)+double(i%5)*.1;phase[i]=uint8_t(i%2);
   source[i]=uint8_t((i/3)%2);sourceId[i]=(i/7)%3;id[i]=i+1;freeSlots[i]=int32_t(i);}
  bind();}
 void bind(){uint32_t active=0;for(uint8_t s:state)active+=s!=0;
  uint32_t free=0;for(uint32_t i=0;i<nmax;i++)if(state[i]==0)freeSlots[free++]=int32_t(i);
  view={nmax,cols,active,nmax-active,double(nmax+1),mass.data(),x.data(),y.data(),vx.data(),vy.data(),
   phase.data(),source.data(),state.data(),id.data(),sourceId.data(),freeSlots.data(),colH.data(),colL.data()};
  view.byteLength={size_t(nmax)*8,size_t(nmax)*8,size_t(nmax)*8,size_t(nmax)*8,size_t(nmax)*8,
    size_t(nmax),size_t(nmax),size_t(nmax),size_t(nmax)*4,size_t(nmax)*4,size_t(nmax)*4,
    size_t(cols)*8,size_t(cols)*8};}
 bool loadFixture(const std::string& file){std::ifstream f(file,std::ios::binary);uint32_t count=0;
  f.read(reinterpret_cast<char*>(&count),4);if(!f||count!=nmax)return false;
  for(uint32_t i=0;i<nmax;i++){uint32_t pid=0,active=0,g[4]={};double fx=0,fy=0;
   f.read(reinterpret_cast<char*>(&pid),4);f.read(reinterpret_cast<char*>(&active),4);
   f.read(reinterpret_cast<char*>(&fx),8);f.read(reinterpret_cast<char*>(&fy),8);
   f.read(reinterpret_cast<char*>(g),16);if(!f)return false;
   id[i]=pid;state[i]=active?uint8_t(1+g[3]):0;mass[i]=active?1:0;
   x[i]=fx;y[i]=fy;phase[i]=uint8_t(g[0]);source[i]=uint8_t(g[1]);sourceId[i]=g[2];
  }bind();return f.peek()==std::char_traits<char>::eof();}
};
static RequestIdentity identity(uint32_t slot,uint64_t epoch,uint64_t serial){RequestIdentity x;
 x.runId="T142_DEVICE_DIAG";x.stateId="state-1";x.configId="config-1";
 x.buildManifestSha="T142_DEVICE_DIAG_SHA";x.ownerGeneration=1;x.contextEpoch=epoch;
 x.payloadSerial=serial;x.poolMutationSerial=1;x.attemptSerial=1;x.substepSerial=1;
 x.requestSerial=serial;x.candidateRole=5;x.candidateOrdinal=uint32_t(serial);
 x.slotIndex=slot;return x;}
int main(int argc,char**argv){if(argc<2)return 2;std::string outPath=argv[1];
 std::ostringstream rep;rep<<"{\"revision\":\"T142_DEVICE_DIAG_V1\",\"runs\":[";
 bool all=true;int runIdx=0;
 for(uint32_t cap:{192u,512u}){
  Pool pool(cap);
  if(argc>3&&!pool.loadFixture(argv[cap==192?2:3]))return 3;
  CpuSlot cpu(cap);PreparedView prepared;RequestIdentity id=identity(0,1,cap);
  auto prep=prepareExactRanges(pool.view,.5,id,cpu,prepared);
  ContextSlot gpu;RequestDiagnostics diag;
  auto init=gpu.initialize(cap,"test/p013_compute/glsl",diag);
  if(init!=ExecStatus::OK){rep<<(runIdx==0?"":",")<<"{\"Nmax\":"<<cap<<",\"init\":\""<<diag.stage<<"\"}";runIdx++;all=false;continue;}
  rep<<(runIdx==0?"":",")<<"{\"Nmax\":"<<cap<<",\"samples\":[";
  runIdx++;
  int sampleIdx=0;
  for(int rep2=0;rep2<5;rep2++){
  id=identity(0,gpu.contextEpoch(),cap*100+rep2+1); // fresh request identity per sample
  id.contextEpoch=gpu.contextEpoch();
  prep=prepareExactRanges(pool.view,.5,id,cpu,prepared);
  Result result;
  auto done=gpu.executeIntegerPair(prepared,0,{std::chrono::steady_clock::now()+std::chrono::seconds(2)},result,diag);
  rep<<(sampleIdx==0?"":",")<<"{\"exec\":\""<<(done==ExecStatus::OK?"OK":diag.stage)
   <<"\",\"dispatchCount\":"<<diag.dispatchCount
   <<",\"uploadCpuMs\":"<<diag.uploadCpuMs<<",\"dispatchCpuMs\":"<<diag.dispatchCpuMs
   <<",\"fenceWaitCpuMs\":"<<diag.fenceWaitCpuMs<<",\"mapCpuMs\":"<<diag.mapCpuMs
   <<",\"requestCpuMs\":"<<diag.requestCpuMs<<",\"waitCalls\":"<<diag.waitCalls
   <<",\"waitReturn\":\""<<diag.waitReturn<<"\"";
  sampleIdx++;
  if(done==ExecStatus::OK){
   const auto& w=result.w;
   rep<<",\"result\":[";
   for(int i=0;i<32;i++)rep<<(i?",":"")<<w[i];
   rep<<"],\"cpuLegacy\":";
   uint32_t a=0,b=0;bool found=legacyPair(pool.view,.5,a,b);
   rep<<"{\"found\":"<<(found?"true":"false");
   if(found)rep<<",\"slotA\":"<<a<<",\"slotB\":"<<b<<",\"idA\":"<<pool.view.id[a]<<",\"idB\":"<<pool.view.id[b];
   rep<<"},\"queryDump\":[";
   for(uint32_t i=0;i<prepared.n&&i<8;i++){const Query&q=prepared.queries[i];
    rep<<(i?",":"")<<"{\"slot\":"<<q.slot<<",\"id\":"<<q.id<<",\"rank\":"<<q.aRank
     <<",\"L\":"<<q.L<<",\"R\":"<<q.R<<",\"G0\":"<<q.G0<<",\"G1\":"<<q.G1<<",\"valid\":"<<q.valid<<"}";}
   rep<<"],\"leafTopSample\":[";
   // CPU-side top-layer expectation of first 8 leaves (rank order = Y-rank)
   // We can't read GPU buffers from here; instead dump CPU leaves for comparison of
   // candidate plausibility and the query windows.
   for(uint32_t i=0;i<prepared.n&&i<8;i++){const Leaf&l=prepared.leaves[i];
    rep<<(i?",":"")<<"{\"rank\":"<<l.rank<<",\"id\":"<<l.id<<",\"slot\":"<<l.slot<<",\"valid\":"<<l.valid<<"}";}
   rep<<"]";
  }
  if(done!=ExecStatus::OK)all=false;
  else{ // agreement check inline
   CurrentOwnerState owner;owner.identity=id;owner.current=&pool.view;
   owner.poolMutationSerial=1;owner.active=true;VerifiedPair verified;
   auto cs=verifyAndConsume(prepared,result,owner,verified);
   uint32_t a=0,b=0;bool found=legacyPair(pool.view,.5,a,b);
   bool agree=(cs==ConsumeStatus::OK&&found&&verified.slotA==a&&verified.slotB==b)||
     (cs==ConsumeStatus::NO_PAIR&&!found);
   rep<<",\"consume\":\""<<(cs==ConsumeStatus::OK?"OK":cs==ConsumeStatus::NO_PAIR?"NO_PAIR":
     cs==ConsumeStatus::STALE?"STALE":cs==ConsumeStatus::PROTOCOL_ERROR?"PROTOCOL_ERROR":
     cs==ConsumeStatus::ORACLE_MISMATCH?"ORACLE_MISMATCH":"INPUT_INVALID")<<"\",\"agree\":"<<(agree?"true":"false");
   rep<<",\"classification\":\""<<(done==ExecStatus::OK?(cs==ConsumeStatus::OK?"EXEC_CONSUME_OK":
     cs==ConsumeStatus::NO_PAIR?"EXEC_CONSUME_NO_PAIR":cs==ConsumeStatus::ORACLE_MISMATCH?"EXEC_ORACLE_MISMATCH":
     "EXEC_CONSUME_PROTOCOL_ERROR"):"EXEC_FAILED")<<"\",\"protocolPass\":"
     <<(cs==ConsumeStatus::OK||cs==ConsumeStatus::NO_PAIR?"true":"false")
    <<",\"legacyFound\":"<<(found?"true":"false")
    <<",\"legacySlotA\":"<<a<<",\"legacySlotB\":"<<b;
   all=all&&agree;
  }
  rep<<"}";
  }
  rep<<"]}";
 }
 rep<<"],\"allAgree\":"<<(all?"true":"false")<<"}\n";
 std::ofstream f(outPath,std::ios::binary);f<<rep.str();f.close();
 std::puts(all?"T142 diag AGREE":"T142 diag MISMATCH");return all?0:1;
}

#include "pair_prepare_v1.h"
#include "pair_executor_v1.h"
#include "pair_owner_v1.h"
#include <array>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

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
 x.runId="T141_HOST_PROTOTYPE";x.stateId="state-1";x.configId="config-1";
 x.buildManifestSha="T141_BUILD_MANIFEST_SHA_PENDING_ARCHIVE";x.ownerGeneration=1;x.contextEpoch=epoch;
 x.payloadSerial=serial;x.poolMutationSerial=1;x.attemptSerial=1;x.substepSerial=1;
 x.requestSerial=serial;x.candidateRole=5;x.candidateOrdinal=uint32_t(serial);
 x.slotIndex=slot;return x;}
static bool rangesMatch(const PreparedView& p){const SnapshotView& f=*p.snapshot;
 for(uint32_t i=0;i<p.n;i++){const Query& q=p.queries[i];
  for(uint32_t j=0;j<p.n;j++){const Leaf& b=p.leaves[j];volatile double d=f.y[q.slot]-f.y[b.slot];
   bool expected=sameGroup(f,q.slot,b.slot)&&std::fabs(d)<=p.radius;
   if(expected!=(j>=q.L&&j<q.R))return false;}}
 return true;
}
static Result simulatedResult(const PreparedView& p,bool found,uint32_t slotA,uint32_t slotB){Result r;
 auto& w=r.w;const auto& h=p.header->w;w[0]=kResultMagic;w[1]=1;w[2]=found?kStatusPair:kStatusNoPair;
 if(found){w[4]=p.snapshot->id[slotA];w[5]=p.snapshot->id[slotB];w[6]=slotA;w[7]=slotB;
  for(uint32_t i=0;i<p.n;i++){if(p.queries[i].slot==slotA)w[8]=i;if(p.leaves[i].slot==slotB)w[9]=i;}}
 uint32_t count=p.K,steps=0;if(p.n>=2){do{count=(count+63)/64;steps++;}while(count>1);w[10]=p.h+1+steps;}
 for(int i=0;i<4;i++)w[12+i]=h[28+i];for(int i=0;i<2;i++){
  w[16+i]=h[14+i];w[18+i]=h[16+i];w[20+i]=h[18+i];}
 for(int i=0;i<8;i++)w[22+i]=h[48+i];w[30]=kDone;return r;
}
int main(int argc,char**argv){const std::string output=argc>1?argv[1]:".codex/local-test/t141/prototype.json";
 std::ostringstream report;report<<"{\"revision\":\"T157_PAIR_DRIVER_V2\",\"runs\":[";
 bool all=true;for(uint32_t cap:{192u,512u}){
  Pool pool(cap);if(argc>3&&!pool.loadFixture(argv[cap==192?2:3]))return 3;
  CpuSlot cpu(cap);PreparedView prepared;RequestIdentity id=identity(0,1,cap);
  auto prep=prepareExactRanges(pool.view,.5,id,cpu,prepared);
  bool ranges=prep==PrepareStatus::OK&&rangesMatch(prepared);
  uint32_t oldA=0,oldB=0;bool oldFound=legacyPair(pool.view,.5,oldA,oldB);
  if(argc>3){bool frozenExpected=cap==192?!oldFound:(oldFound&&pool.view.id[oldA]==16&&pool.view.id[oldB]==342);
   all=all&&frozenExpected;}
  Result synthetic=simulatedResult(prepared,oldFound,oldA,oldB);
  CurrentOwnerState owner;owner.identity=id;owner.current=&pool.view;
  owner.poolMutationSerial=1;owner.active=true;VerifiedPair checked;
  auto accepted=verifyAndConsume(prepared,synthetic,owner,checked);
  bool ownerOK=oldFound?accepted==ConsumeStatus::OK&&checked.slotA==oldA&&checked.slotB==oldB:
    accepted==ConsumeStatus::NO_PAIR;
  Result tampered=synthetic;tampered.w[12]^=1;
  ownerOK=ownerOK&&verifyAndConsume(prepared,tampered,owner,checked)==ConsumeStatus::PROTOCOL_ERROR;
  owner.poolMutationSerial++;ownerOK=ownerOK&&verifyAndConsume(prepared,synthetic,owner,checked)==ConsumeStatus::STALE;
  owner.poolMutationSerial--;all=all&&ownerOK;
  ContextSlot gpu;RequestDiagnostics diagnostic;auto init=gpu.initialize(cap,"test/p013_compute/glsl",diagnostic);
  std::string execution="NOT_RUN",pair="NOT_RUN",exec=execStatusName(init),consume="NOT_RUN",oracle="NOT_EVALUATED";uint32_t dispatches=0;
  if(init==ExecStatus::OK){id.contextEpoch=gpu.contextEpoch();
   prep=prepareExactRanges(pool.view,.5,id,cpu,prepared);
   Result result;auto done=gpu.executeIntegerPair(prepared,0,{std::chrono::steady_clock::now()+std::chrono::seconds(1)},result,diagnostic);
   dispatches=diagnostic.dispatchCount;exec=execStatusName(done);
   execution=done==ExecStatus::OK?"OK":"GPU_FAILED";
   if(done==ExecStatus::OK){CurrentOwnerState owner;owner.identity=id;owner.current=&pool.view;
    owner.poolMutationSerial=1;owner.active=true;VerifiedPair verified;
    auto consumed=verifyAndConsume(prepared,result,owner,verified);consume=consumeStatusName(consumed);
    bool agree=(consumed==ConsumeStatus::OK&&oldFound&&verified.slotA==oldA&&verified.slotB==oldB)||
      (consumed==ConsumeStatus::NO_PAIR&&!oldFound);
    oracle=agree?"AGREE":consumed==ConsumeStatus::ORACLE_MISMATCH?"ORACLE_MISMATCH":"NOT_EVALUATED";pair=oracle;all=all&&agree;}
   else all=false;
  }else execution=diagnostic.stage.empty()?"ENV_UNSUPPORTED":diagnostic.stage;
  all=all&&ranges;
  if(cap!=192)report<<",";
  report<<"{\"Nmax\":"<<cap<<",\"K\":"<<cpu.K<<",\"h\":"<<cpu.h
   <<",\"rangeOracle\":"<<(ranges?"true":"false")<<",\"ownerProtocol\":"<<(ownerOK?"true":"false")
   <<",\"oldPairFound\":"<<(oldFound?"true":"false")
   <<",\"gpuExecution\":\""<<execution<<"\",\"pairOracle\":\""<<pair
   <<"\",\"execStatus\":\""<<exec<<"\",\"consumeStatus\":\""<<consume<<"\",\"oracleVerdict\":\""<<oracle<<"\",\"stage\":\""<<diagnostic.stage<<"\",\"rawGlError\":"<<diagnostic.rawGlError<<",\"rawWaitReturn\":"<<diagnostic.rawWaitReturn<<",\"waitReturn\":\""<<diagnostic.waitReturn<<"\",\"dispatchCount\":"<<dispatches<<",\"gpuRuntime\":null}";
 }
 report<<"],\"allOfflineChecks\":"<<(all?"true":"false")<<"}\n";
 std::ofstream f(output,std::ios::binary);f<<report.str();
 std::printf("T141 CPU ranges %s; GPU result in %s\n",all?"PASS":"FAIL",output.c_str());return all?0:1;
}

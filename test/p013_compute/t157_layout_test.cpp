// Independent T138 byte-layout expectations. Does not import the generator schema.
#include "pair_owner_v1.h"
#include "pair_executor_v1.h"
#include <cstdio>
using namespace p011gpu;
#define CHECK(x) do{if(!(x)){std::printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 double mass[4]={1,1,0,0},x[4]={.5,.5,0,0},y[4]={10,10.1,0,0},vx[4]={},vy[4]={},colL[1]={},col[1]={2};uint8_t phase[4]={},source[4]={},state[4]={1,1,0,0};uint32_t ids[4]={1,2,0,0},sid[4]={};int32_t free[4]={2,3,0,0};
 SnapshotView pool{4,1,2,2,3,mass,x,y,vx,vy,phase,source,state,ids,sid,free,col,colL};pool.byteLength={32,32,32,32,32,4,4,4,16,16,16,8,8};
 RequestIdentity identity;identity.runId="run";identity.stateId="state";identity.configId="config";identity.buildManifestSha=std::string(64,'b');identity.ownerGeneration=0x1111111122222222ULL;identity.contextEpoch=0x3333333344444444ULL;identity.payloadSerial=0x5555555566666666ULL;identity.poolMutationSerial=0x7777777788888888ULL;identity.attemptSerial=0x99999999aaaaaaaaULL;identity.substepSerial=0xbbbbbbbbccccccccULL;identity.requestSerial=0xddddddddeeeeeeeeULL;identity.candidateRole=5;identity.candidateOrdinal=7;
 CpuSlot cpu(4);PreparedView p;CHECK(prepareExactRanges(pool,.5,identity,cpu,p)==PrepareStatus::OK);
 CHECK(cpu.header.w[12]==0x22222222u&&cpu.header.w[13]==0x11111111u&&cpu.header.w[14]==0x44444444u&&cpu.header.w[15]==0x33333333u&&cpu.header.w[19]==0x77777777u&&cpu.header.w[21]==0x99999999u&&cpu.header.w[23]==0xbbbbbbbbu);
 // Synthetic unique header exercises every echo lane independently of prepare/hash generation.
 for(uint32_t i=0;i<64;i++)cpu.header.w[i]=0x12340000u+i*0x101u;
 auto expected=[&](bool pair,uint32_t dispatch){Result r;r.w[0]=0x47505231;r.w[1]=1;r.w[2]=pair?1:2;r.w[10]=dispatch;if(pair){r.w[4]=1;r.w[5]=2;r.w[6]=0;r.w[7]=1;r.w[8]=0;r.w[9]=1;}
  const int sourceWords[]={28,29,30,31,14,15,16,17,18,19,48,49,50,51,52,53,54,55};for(int i=0;i<18;i++)r.w[12+i]=0x12340000u+uint32_t(sourceWords[i])*0x101u;r.w[30]=0x444f4e45;return r;};
 CurrentOwnerState owner;owner.identity=identity;owner.current=&pool;owner.poolMutationSerial=identity.poolMutationSerial;owner.active=true;owner.diagnosticOracle=true;VerifiedPair pair;
 Result valid=expected(true,p.h+2);CHECK(verifyAndConsume(p,valid,owner,pair)==ConsumeStatus::OK);
 for(int word=0;word<32;word++){Result bad=valid;bad.w[word]^=1;CHECK(verifyAndConsume(p,bad,owner,pair)!=ConsumeStatus::OK);}
 owner.diagnosticOracle=false;valid=expected(false,p.h+2);CHECK(verifyAndConsume(p,valid,owner,pair)==ConsumeStatus::NO_PAIR);
 for(int word=0;word<32;word++){Result bad=valid;bad.w[word]^=1;CHECK(verifyAndConsume(p,bad,owner,pair)!=ConsumeStatus::NO_PAIR);}
 CHECK(cpuEmptyResult(cpu.header).w==expected(false,0).w);
 for(int mutation=0;mutation<9;mutation++){auto bad=owner;switch(mutation){case 0:bad.active=false;break;case 1:bad.identity.ownerGeneration++;break;case 2:bad.identity.contextEpoch++;break;case 3:bad.identity.configId+='x';break;case 4:bad.identity.runId+='x';break;case 5:bad.identity.attemptSerial++;break;case 6:bad.identity.candidateRole++;break;case 7:bad.identity.seed++;break;case 8:bad.poolMutationSerial++;break;}CHECK(verifyAndConsume(p,valid,bad,pair)==ConsumeStatus::STALE);}
 for(uint32_t n:{0u,1u}){pool.liveCount=n;pool.freeCount=4-n;pool.nextId=n+1;col[0]=n;
  for(uint32_t i=0;i<4;i++){state[i]=i<n?1:0;mass[i]=i<n?1:0;ids[i]=i<n?i+1:0;if(i>=n)free[i-n]=int32_t(i);}
  CHECK(prepareExactRanges(pool,.5,identity,cpu,p)==PrepareStatus::NO_PAIR);for(uint32_t i=0;i<64;i++)cpu.header.w[i]=0x12340000u+i*0x101u;
  auto empty=cpuEmptyResult(cpu.header);CHECK(empty.w==expected(false,0).w);owner.diagnosticOracle=true;CHECK(verifyAndConsume(p,empty,owner,pair)==ConsumeStatus::NO_PAIR);
 }
 RequestDiagnostics d;d.preserveGlError(0x505);d.preserveGlError(0);d.preserveGlError(0x502);CHECK(d.rawGlError==0x505);CHECK(std::string(consumeStatusName(ConsumeStatus::PROTOCOL_ERROR))=="PROTOCOL_ERROR");
 std::puts("PASS T157 independent 32-word PAIR/NO_PAIR/CPU_EMPTY golden, nonzero high words, 64 word flips, 9 owner mutations, first GL raw error");return 0;
}

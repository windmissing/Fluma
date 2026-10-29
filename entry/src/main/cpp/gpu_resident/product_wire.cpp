#include "product_wire.h"
namespace grs {
bool ValidateSandStateWord(uint32_t p){
 uint32_t state=p&15,wait=(p>>12)&15;if(p&0xffff0ff0u)return false;
 return state<=3&&(state<=1?wait==0:state==2?wait==1:wait>=2&&wait<=5);
}
bool ValidateEventWords(const uint32_t* e,size_t length,uint32_t& word){
 word=0;if(e==nullptr||length!=64)return false;
 if(e[0]!=wire::magicWords::event64)return false;word=1;if(e[1]!=wire::revisionWord)return false;
 word=6;uint32_t kind=e[6];if(kind<1||kind>7)return false;
 static constexpr uint64_t masks[7]={0x001f0003ffffffffULL,0x001ff000ffffffffULL,0x005fffffffffffffULL,0x005ff000ffffffffULL,0x001f0fffffffffffULL,0x00500fff0030ffffULL,0x001f0000ffffffffULL};
 uint64_t mask=masks[kind-1];for(uint32_t i=0;i<64;i++)if(((mask>>i)&1)==0&&e[i]){word=i;return false;}
 word=2;if((e[2]|e[3])==0)return false;word=7;if((e[7]|e[8])==0)return false;word=9;if((e[9]|e[10])==0)return false;word=4;if(e[4]!=(kind==1?1u:2u))return false;
 word=52;uint32_t f=e[52],oldState=f&15,newState=(f>>4)&15,oldWait=(f>>10)&15,newWait=(f>>14)&15,ns=(f>>18)&3;
 if((f&0xfff00300u)!=0||!ValidateSandStateWord(oldState|(oldWait<<12))||!ValidateSandStateWord(newState|(newWait<<12)))return false;
 if(ns!=(kind==1?3:kind==6?2:0))return false;
 word=5;if(e[5]!=(kind==1?1:kind==6?3:kind==7?4:2))return false;
 if(kind!=6){word=16;if(!e[17]||!e[18]||!e[19]||e[20]>=128||e[21]>=128)return false;}
 word=44;if((kind==2||kind==3||kind==4)&&(e[46]==0&&e[47]==0))return false;
 if(kind==2&&(e[44]!=1||e[45]!=0||e[46]!=1||e[47]!=0))return false;
 word=48;if(kind!=6&&((e[48]|e[49])==0||(e[50]|e[51])==0))return false;
 if(kind==1&&(e[48]!=e[12]||e[49]!=e[13]||e[24]||e[25]||e[28]||e[29]||e[30]||e[31]))return false;
 word=22;if(kind==1&&(e[22]!=0||e[23]==0||oldState!=0||newState!=1))return false;
 if(kind==2||kind==3||kind==4||kind==7){if(e[22]!=e[23])return false;}
 if(kind==2&&e[20]!=e[21])return false;
 if(kind==3&&(e[20]>e[21]?e[20]-e[21]:e[21]-e[20])!=1)return false;
 if(kind==5&&e[22]<=e[23])return false;
 word=0xffffffffu;return true;
}
DispatchPlan SourcePlan(uint32_t K){
 DispatchPlan p;if(K!=192&&K!=512)return p;uint32_t pad=K==192?256:512,h=K==192?8:9;
 auto add=[&](uint32_t s,uint32_t k=0,uint32_t j=0){p.passes[p.count]={s,p.count,k,j,0};p.count++;};
 add(Copy);add(PrepareSource);add(FreeFlags);for(uint32_t d=1;d<pad;d*=2)add(ScanUp,d);add(ScanRoot);for(uint32_t d=pad/2;d>0;d/=2)add(ScanDown,d);
 for(uint32_t s:{Birth,Fold,AuditFlags,AuditReduce,Provenance,ReplayCopy,Replay,ReplayAudit,StoreSource,Pack,PackAudit,Seal})add(s);
 return p;
}
DispatchPlan QuantumPlan(uint32_t K){
 DispatchPlan p;if(K!=192&&K!=512)return p;uint32_t pad=K==192?256:512;
 auto add=[&](uint32_t s,uint32_t k=0,uint32_t j=0,uint32_t color=0){p.passes[p.count]={s,p.count,k,j,color};p.count++;};
 add(Copy);add(PrepareQuantum);add(IndexInit);
 for(uint32_t k=2;k<=pad;k*=2)for(uint32_t j=k/2;j>0;j/=2)add(Sort,k,j);
 add(Proposal);add(Resolve);for(uint32_t color=0;color<3;color++){add(BedPropose,0,0,color);add(BedAllocate,0,0,color);add(BedApply,0,0,color);}
 for(uint32_t s:{FinalContactFold,AuditFlags,AuditReduce,ReplayCopy,Replay,ReplayAudit,Pack,PackAudit,Seal})add(s);
 return p;
}
}

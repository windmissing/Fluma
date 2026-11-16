uniform uint uV5Input,uV5Mode,uV5Rate,uV5Band,uV5Ramp,uV5Decay,uV5Speed,uV5Channel,uV5ActualAuxStart,uV5EventArena;
uniform uint uV5ParameterRev;
uniform uvec2 uV5InputEpoch,uV5PageSession;
uniform uvec4 uV5Sha0,uV5Sha1;
// The V4 scope scratch ends at word8192; these ranges are beyond its six
// control mirrors (ending6912), and are not another unaccounted state bank.
const uint V5_REQUESTS=7000u,V5_ENTRY_BED=7128u,V5_LIFT_START=7256u;
uint v5Scratch(uint at){return W[uM3Scratch+at];}
void v5ScratchPut(uint at,uint value){W[uM3Scratch+at]=value;}
bool v5EnvelopeValid(){
 if(W[uV5Input]!=893994572u||W[uV5Input+1u]!=5u||W[uV5Input+2u]!=uV5Mode||W[uV5Input+15u]!=uV5ParameterRev||uV5ParameterRev==0u||C[uHeader+162u]!=uV5Mode)return false;
 if(any(notEqual(uvec2(W[uV5Input+4u],W[uV5Input+5u]),uV5InputEpoch))||any(notEqual(uvec2(W[uV5Input+40u],W[uV5Input+41u]),uV5PageSession)))return false;
 for(uint i=0u;i<6u;++i)if(W[uV5Input+42u+i]!=B[uHeader+(i<2u?6u+i:i<4u?2u+i:4u+i)])return false;
 for(uint i=0u;i<8u;++i)if(W[uV5Input+48u+i]!=(i<4u?uV5Sha0[i]:uV5Sha1[i-4u]))return false;
 for(uint i=56u;i<64u;++i)if(W[uV5Input+i]!=0u)return false;
 if(uV5Mode==0u){for(uint i=6u;i<15u;++i)if(W[uV5Input+i]!=0u)return false;for(uint i=16u;i<40u;++i)if(W[uV5Input+i]!=0u)return false;return W[uV5Input+3u]==0u;}
 if(uV5Mode>2u||W[uV5Input+6u]==0u||W[uV5Input+7u]==0u||W[uV5Input+14u]!=uV5Mode||W[uV5Input+3u]!=(uV5Mode==1u?3u:2u))return false;
 for(uint i=16u;i<40u;i+=2u)if(((W[uV5Input+i+1u]>>20u)&2047u)==2047u)return false;
 return true;
}
bool v5InputEnergy(){
 if(!v5EnvelopeValid()){qfail(37u);return false;}
 uint E0=B[uHeader+163u],signal=0u,E1;
 if(uV5Mode!=0u){
  if(W[uV5Input]!=893994572u||W[uV5Input+1u]!=5u||W[uV5Input+2u]!=uV5Mode||W[uV5Input+6u]==0u||W[uV5Input+3u]>3u){qfail(37u);return false;}
  uvec2 raw=uvec2(W[uV5Input+18u],W[uV5Input+19u]);
  if(raw.y>0x3ff00000u||(raw.y==0x3ff00000u&&raw.x!=0u)){qfail(37u);return false;}
  uint q20;if(!grsFloorBinary64Q20(raw,1048576u,q20)){qfail(37u);return false;}signal=q20>>4u;
 }
 if(!v5Energy(E0,signal,uV5Mode,uV5Ramp,uV5Decay,E1)){qfail(37u);return false;}
 uint auxStart=uM3Role==4u&&B[uHeader+35u]==uFrame.x&&B[uHeader+36u]==uFrame.y?B[uHeader+179u]:0u;
 if(uM3Role==4u&&auxStart!=uV5ActualAuxStart){qfail(37u);return false;}
 v5BeginScope(uEventStart,auxStart);
 C[uHeader+180u]=uV5EventArena;
 C[uHeader+163u]=E1;C[uHeader+164u]=E0;C[uHeader+165u]=uV5Mode==0u?0u:W[uV5Input+6u];C[uHeader+166u]=uV5Mode==0u?0u:W[uV5Input+3u];
 C[uHeader+167u]=C[uHeader+168u]=C[uHeader+182u]=0u;
 for(uint c=0u;c<128u;++c){
  uint entry=B[uColumn+10u*128u+c],request=0u;
  if(uV5Mode!=0u&&!v5Request(E0,entry,uV5Band,uV5Rate,request)){qfail(37u);return false;}
  v5ScratchPut(V5_ENTRY_BED+c,entry);v5ScratchPut(V5_REQUESTS+c,request);
 }
 uint row[32];v5AuxRow(row,31u,0u);row[8]=E0;row[9]=E1;row[10]=signal;row[11]=uV5Mode;row[12]=uV5Ramp;row[13]=uV5Decay;row[14]=C[uHeader+165u];row[15]=C[uHeader+166u];
 return v5Credit(0u,1u)&&v5WriteAux(row)&&v5SealCompound();
}
uint v5LiftGuard(uint c,uint q){
 if(c>0u){int d=int(col(10u,c))-int(col(10u,c-1u))-int(W[5]);if(abs(d)>605394){qfail(14u);return 0u;}q=min(q,uint(d+605394));}
 if(c<127u){int d=int(col(10u,c+1u))-int(col(10u,c))-int(W[5]);if(abs(d)>605394){qfail(14u);return 0u;}q=min(q,uint(605394-d));}
 return q;
}
void v5Limit(uint bound,uint reason,inout uint q,inout uint first){if(bound<q){q=bound;if(first==0u)first=reason;}}
bool v5PhysicalLift(uint slot,uint c,uint q,uint oldBed,uint oldP,uint total,uint local,uint physical){
 uint row[64];for(uint i=0u;i<64u;++i)row[i]=0u;
 row[0]=1163088455u;row[1]=5u;row[2]=uNonce.x;row[3]=uNonce.y;row[4]=2u;row[5]=3u;row[6]=26u;row[7]=physical;
 row[9]=uFrame.x;row[10]=uFrame.y;row[11]=uQuantumOrdinal;row[12]=B[uHeader+14u];row[13]=B[uHeader+15u];
 uvec2 commit;if(!grsAdd64(uvec2(B[uHeader+16u],B[uHeader+17u]),uvec2(1u,0u),commit)){qfail(13u);return false;}row[14]=commit.x;row[15]=commit.y;
 row[16]=slot;row[17]=C[10u*uK+slot];row[18]=C[5u*uK+slot];row[19]=C[7u*uK+slot];row[20]=row[21]=c;row[23]=q;
 row[26]=C[uK+slot];row[27]=C[2u*uK+slot];row[30]=C[3u*uK+slot];row[31]=C[4u*uK+slot];row[32]=oldBed;row[33]=oldBed-q;row[34]=oldP;row[35]=oldP+q;
 row[36]=total;row[38]=total-q;row[40]=local;row[41]=local-q;row[48]=C[8u*uK+slot];row[49]=C[9u*uK+slot];uint ledger=uV5SourceLedger+(row[48]-1u)*16u;row[50]=W[ledger];row[51]=W[ledger+1u];
 row[52]=(1u<<4u)|(C[14u*uK+slot]<<9u)|(uM3Role<<22u);row[55]=q;row[56]=row[57]=65536u;row[59]=C[14u*uK+slot];row[60]=row[61]=eta(c);
 for(uint i=0u;i<64u;++i){uint mask=i<32u?4294967295u:4288611327u;if(((mask>>(i%32u))&1u)==0u)row[i]=0u;}
 uint at=uM3EventWord+(physical-1u)*64u;
 if(v5Replay){for(uint i=0u;i<64u;++i)if(E[at+i]!=row[i]){qfail(38u);return false;}}
 else for(uint i=0u;i<64u;++i)E[at+i]=row[i];
 return true;
}
bool v5LiftLoop(){
 uint channel=uV5Channel/120u;v5ScratchPut(V5_LIFT_START,W[1]);
 for(uint c=0u;c<128u;++c){
  uint request=v5Scratch(V5_REQUESTS+c),q=request,reason=0u,bed=col(10u,c),p=col(8u,c),depth=C[uV5Depth+c],total=C[uHeader+54u],local=col(15u,c);
  if(depth>64u){qfail(34u);return false;}
  v5Limit(bed,5u,q,reason);uint top=depth==0u?0u:C[v5Layer(c,depth-1u)+3u];v5Limit(top,6u,q,reason);v5Limit(122333u,7u,q,reason);v5Limit(total,1u,q,reason);v5Limit(local,2u,q,reason);v5Limit(channel,3u,q,reason);v5Limit(v5LiftGuard(c,q),4u,q,reason);
  uint slot=uK;if(q>0u){for(uint i=0u;i<uK;++i)if(C[i]==0u){slot=i;break;}if(slot==uK){q=0u;reason=8u;}else if(C[uHeader+126u]!=0u||C[uHeader+39u]==0u){q=0u;reason=9u;}else if(C[10u*uK+slot]==0xffffffffu){qfail(35u);return false;}}
  if(W[0]!=0u||!v5Credit(q==0u?0u:1u,q==0u?1u:2u))return false;
  uint row[32];v5AuxRow(row,30u,0u);row[8]=c;row[13]=request;row[16]=q;row[17]=total;row[18]=local;row[19]=channel;row[30]=reason;
  if(!v5WriteAux(row))return false;
  if(q>0u){
   uint rank=depth-1u;uvec3 origin=v5Origin(c,rank);uint id=C[uHeader+39u],generation=C[10u*uK+slot]+1u;
   int vx,vy;uint angle=uAngles+uint(int(W[4])+7680)*4u;if(!v5LiftVelocity(uV5Speed,int(W[angle+1u]),int(W[angle+2u]),vx,vy)){qfail(13u);return false;}
   if(!v5Pop(c,q)){qfail(34u);return false;}
   pc(2u,c,bed-q);pc(10u,c,bed-q);pc(8u,c,p+q);charge(q,c,c,false);channel-=q;
   for(uint f=0u;f<16u;++f)C[f*uK+slot]=0u;
   C[slot]=q;C[uK+slot]=c*1048576u+524288u;C[2u*uK+slot]=bed-q+1u;C[3u*uK+slot]=uint(vx);C[4u*uK+slot]=uint(vy);C[5u*uK+slot]=id;C[6u*uK+slot]=1u;C[7u*uK+slot]=origin.z;C[8u*uK+slot]=origin.x;C[9u*uK+slot]=origin.y;C[10u*uK+slot]=generation;
   C[14u*uK+slot]=col(6u,c)>0u&&C[2u*uK+slot]<eta(c)?1u:0u;C[15u*uK+slot]=W[6];
   C[uHeader+39u]=id+1u;if(id==0xffffffffu)C[uHeader+126u]=1u;C[uHeader+41u]++;C[uHeader+43u]--;C[uHeader+182u]++;
   uvec2 commit;if(!grsAdd64(uvec2(B[uHeader+16u],B[uHeader+17u]),uvec2(1u,0u),commit)||!v5BirthCertificate(slot,1u,commit,W[1]+1u,rank,top)){qfail(35u);return false;}
   if(!v5Plus(167u,q)||!v5Plus(169u,q)||!v5PhysicalLift(slot,c,q,bed,p,total,local,W[1]+1u))return false;
   ++W[1];v5AuxRow(row,29u,W[1]);row[8]=row[9]=c;row[10]=origin.x;row[11]=origin.y;row[12]=origin.z;row[13]=q;row[15]=q;row[16]=top;row[17]=top-q;row[20]=depth;row[21]=C[uV5Depth+c];row[24]=rank;row[25]=26u;row[26]=id;row[27]=generation;row[28]=slot;row[29]=(q==top?1u:0u)|(C[14u*uK+slot]==1u?8u:0u);
   if(!v5WriteAux(row))return false;derived(c);v5RefreshAcceptedMedium();
  }
  if(!v5SealCompound())return false;
 }
 C[uHeader+179u]=v5Cursor(3u);return true;
}

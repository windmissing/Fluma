// Source-root continuation. Current B is read-only; spare is inside the fixed WORK allocation.
uniform uint uSpare,uAudit,uAngles,uPubHeader,uPubSand,uPubWater,uPubColumn;
uniform uint uActualDispatches,uActualBarriers,uActualUploadedBytes,uCarry;
uint getState(bool spare,uint i){return spare?W[uSpare+i]:C[i];}
void putState(bool spare,uint i,uint value){if(spare)W[uSpare+i]=value;else C[i]=value;}
uvec2 header64(bool spare,uint i){return uvec2(getState(spare,uHeader+i),getState(spare,uHeader+i+1u));}
void put64(bool spare,uint i,uvec2 value){putState(spare,uHeader+i,value.x);putState(spare,uHeader+i+1u,value.y);}
void increment(bool spare,uint at){uvec2 value;if(!grsAdd64(header64(spare,at),uvec2(1u,0u),value)){fail(6u,13u,at);return;}put64(spare,at,value);}
void foldState(bool spare){
 for(uint col=0u;col<128u;col++)putState(spare,uColumn+8u*128u+col,0u);
 uint live=0u;uvec2 total=uvec2(0u);
 for(uint i=0u;i<uK;i++){uint m=getState(spare,i);if(m==0u)continue;uint x=getState(spare,uK+i);if(x>=134217728u){fail(6u,15u,i);return;}uint col=x>>20u,at=uColumn+8u*128u+col,old=getState(spare,at);if(m>268435456u||old>268435456u-m){fail(6u,15u,i);return;}putState(spare,at,old+m);live++;}
 for(uint col=0u;col<128u;col++){uint mh=getState(spare,uColumn+2u*128u+col),ph=getState(spare,uColumn+8u*128u+col);if(mh>268435456u||ph>268435456u-mh){fail(6u,15u,col);return;}putState(spare,uColumn+10u*128u+col,mh);putState(spare,uColumn+11u*128u+col,mh);putState(spare,uColumn+12u*128u+col,mh);putState(spare,uColumn+13u*128u+col,mh+ph);uvec2 next;if(!grsAdd64(total,uvec2(mh+ph,0u),next)){fail(6u,13u,col);return;}total=next;}
 put64(spare,56u,total);putState(spare,uHeader+41u,live);putState(spare,uHeader+43u,uK-live);
}
void metadata(bool spare){
 uint outcome=S(Outcome);if(outcome==1u){increment(spare,16u);if(uRootKind==1u){increment(spare,14u);uvec2 amount;if(!grsAdd64(header64(spare,60u),uvec2(S(Admitted),0u),amount)){fail(6u,13u,60u);return;}put64(spare,60u,amount);putState(spare,uHeader+39u,getState(spare,uHeader+39u)+S(BirthCount));putState(spare,uHeader+34u,getState(spare,uHeader+34u)+1u);}}
 if(S(Append)!=0u)putState(spare,uHeader+33u,getState(spare,uHeader+33u)+1u);
 put64(spare,74u,uvec2(getState(spare,uHeader+33u),0u));
 put64(spare,12u,uRootNonce);put64(spare,18u,uControlSerial);put64(spare,20u,uGeometryEpoch);put64(spare,49u,uFrameSerial);if(uRootKind==1u)put64(spare,82u,uvec2(D(24u),D(25u)));put64(spare,84u,uInputGeneration);if(uRootKind==1u)put64(spare,86u,uvec2(D(10u),D(11u)));increment(spare,72u);
 putState(spare,uHeader+76u,uBanks.y);putState(spare,uHeader+77u,uBanks.x);putState(spare,uHeader+78u,uBanks.z);putState(spare,uHeader+79u,uRootKind);putState(spare,uHeader+51u,uQuantumOrdinal);putState(spare,uHeader+80u,outcome);putState(spare,uHeader+81u,2u);putState(spare,uHeader+37u,uEventArena);putState(spare,uHeader+38u,uRootKind==1u?uEventStart+(outcome==1u?S(BirthCount):0u):S(EventCursor));put64(spare,35u,uFrameSerial);putState(spare,uHeader+47u,uCarry);putState(spare,uHeader+48u,0u);
}
bool equal64(uvec2 a,uvec2 b){return all(equal(a,b));}
bool sourceAudit(bool spare){
 uint auditedP[128];for(uint c=0u;c<128u;c++)auditedP[c]=0u;uint auditedLive=0u;uvec2 auditedTotal=uvec2(0u);
 for(uint i=0u;i<uK;i++){uint m=getState(spare,i);if(m==0u)continue;uint x=getState(spare,uK+i);if(x>=134217728u||m>268435456u)return false;uint c=x>>20u;if(auditedP[c]>268435456u-m)return false;auditedP[c]+=m;auditedLive++;}
 if(getState(spare,uHeader)!=GRS_wire_magicWords_stateHeader||getState(spare,uHeader+1u)!=1u||getState(spare,uHeader+2u)!=256u||getState(spare,uHeader+31u)!=1u||getState(spare,uHeader+32u)!=0u)return false;
 if(getState(spare,uHeader+41u)!=auditedLive||getState(spare,uHeader+43u)!=uK-auditedLive)return false;
 uint rootStatus=getState(spare,uHeader+81u),rootKind=getState(spare,uHeader+79u),rootOutcome=getState(spare,uHeader+80u);
 if(rootStatus!=0u&&rootStatus!=2u)return false;
 if(rootStatus==0u&&(any(notEqual(header64(spare,16u),uvec2(0u)))||any(notEqual(header64(spare,14u),uvec2(0u)))||auditedLive!=0u||getState(spare,uHeader+33u)!=0u))return false;
 if(rootStatus==2u&&(rootKind<1u||rootKind>2u||rootOutcome<1u||rootOutcome>3u||(rootKind==2u&&rootOutcome!=1u)))return false;
 uint initialBudget=getState(spare,uHeader+52u),remainingBudget=getState(spare,uHeader+54u),localUsedSum=0u;
 if(getState(spare,uHeader+53u)!=0u||getState(spare,uHeader+55u)!=0u||(initialBudget!=0u&&initialBudget!=2621440u)||remainingBudget>initialBudget)return false;
 for(uint i=64u;i<68u;i++)if(getState(spare,uHeader+i)!=0u)return false;
 uint prefix=getState(spare,uHeader+34u);if(prefix>4096u||prefix!=getState(spare,uHeader+14u)||getState(spare,uHeader+15u)!=0u)return false;
 // Candidate's new success row is staged only after replay; base prefix remains readable here.
 uint oldPrefix=B[uHeader+34u];if(oldPrefix>4096u)return false;
 if(oldPrefix==0u&&B[uHeader+39u]!=1u)return false;
 if(oldPrefix>0u&&W[uSourceEvents+(oldPrefix-1u)*16u+13u]!=B[uHeader+39u])return false;
 for(uint i=112u;i<256u;i++)if(getState(spare,uHeader+i)!=0u)return false;
 for(uint i=0u;i<12u*64u;i++)if(getState(spare,16u*uK+i)!=0u)return false;
 int angle=int(getState(spare,uHeader+45u));if(angle< -7680||angle>7680)return false;int tangent=int(W[uAngles+uint(angle+7680)*4u]);
 for(uint col=0u;col<128u;col++){
  for(uint field=0u;field<10u;field++)if(field!=2u&&field!=8u&&getState(spare,uColumn+field*128u+col)!=0u)return false;
  uint used=getState(spare,uColumn+14u*128u+col),rem=getState(spare,uColumn+15u*128u+col);if(used>244667u||rem>244667u||used+rem!=(initialBudget==0u?0u:244667u))return false;localUsedSum+=used;
  uint mh=getState(spare,uColumn+2u*128u+col),ph=getState(spare,uColumn+8u*128u+col);if(ph!=auditedP[col]||getState(spare,uColumn+11u*128u+col)!=mh||getState(spare,uColumn+12u*128u+col)!=mh)return false;uvec2 nextTotal;if(!grsAdd64(auditedTotal,uvec2(mh+ph,0u),nextTotal))return false;auditedTotal=nextTotal;if(mh>268435456u||ph>268435456u-mh||getState(spare,uColumn+10u*128u+col)!=mh||getState(spare,uColumn+13u*128u+col)!=mh+ph)return false;
  if(col<127u){int d=int(getState(spare,uColumn+2u*128u+col+1u))-int(mh)-tangent;if(abs(d)>605394)return false;}
 }
 for(uint i=0u;i<uK;i++){
  uint m=getState(spare,i);if(m==0u){for(uint f=1u;f<16u;f++)if(f!=10u&&getState(spare,f*uK+i)!=0u)return false;continue;}
  if(m>268435456u||getState(spare,5u*uK+i)==0u||getState(spare,7u*uK+i)==0u||getState(spare,10u*uK+i)==0u||getState(spare,uK+i)>=134217728u||getState(spare,2u*uK+i)>268435456u)return false;
  uint origin=getState(spare,8u*uK+i),originHi=getState(spare,9u*uK+i),id=getState(spare,5u*uK+i);if(origin==0u||originHi!=0u||origin>prefix)return false;
  if(uRootKind==1u&&S(Outcome)==1u&&origin>B[uHeader+34u]){if(getState(spare,7u*uK+i)!=D(14u)||id<B[uHeader+39u]||id-B[uHeader+39u]>=S(BirthCount))return false;}
  else {uint row=uSourceEvents+(origin-1u)*16u;if(getState(spare,7u*uK+i)!=W[row+9u]||id<W[row+7u]||id-W[row+7u]>=W[row+8u])return false;}
for(uint j=0u;j<i;j++)if(getState(spare,j)!=0u&&getState(spare,5u*uK+j)==id)return false;
  uint tag=getState(spare,6u*uK+i),state=tag&15u,wait=(tag>>12u)&15u;
  if((tag&0xffff0ff0u)!=0u||state<1u||state>3u||(state==1u&&wait!=0u)||(state==2u&&wait!=1u)||(state==3u&&(wait<2u||wait>5u))||getState(spare,11u*uK+i)!=wait)return false;
  uint x=getState(spare,uK+i);if(getState(spare,2u*uK+i)<getState(spare,uColumn+10u*128u+(x>>20u)))return false;
  for(uint f=3u;f<=4u;f++){uint v=getState(spare,f*uK+i);if(v>1073741824u&&v<3221225472u)return false;}
  for(uint f=12u;f<16u;f++)if(getState(spare,f*uK+i)!=0u)return false;
 }
 if(localUsedSum<initialBudget-remainingBudget||localUsedSum>2u*(initialBudget-remainingBudget)||!equal64(auditedTotal,header64(spare,56u)))return false;
 if(!equal64(header64(spare,56u),header64(spare,60u))||any(notEqual(header64(spare,58u),uvec2(0u)))||any(notEqual(header64(spare,62u),uvec2(0u)))||getState(spare,uHeader+42u)!=0u||getState(spare,uHeader+44u)!=64u)return false;
 return true;
}
void terminal(){
 if(S(Outcome)==3u)return;
 for(uint i=0u;i<32u;i++)W[uTerminal+i]=0u;
 uint admitted=S(Admitted),rejected=S(Requested)-admitted;
 W[uTerminal]=S(Outcome)==1u?(rejected==0u?1u:2u):3u;W[uTerminal+1u]=S(Reason);W[uTerminal+3u]=admitted;W[uTerminal+4u]=rejected;W[uTerminal+5u]=S(Column);W[uTerminal+6u]=S(BirthCount);W[uTerminal+7u]=S(BirthCount)==0u?0u:B[uHeader+39u];W[uTerminal+8u]=B[uHeader+39u];W[uTerminal+9u]=H(39u);
 for(uint i=0u;i<2u;i++){W[uTerminal+10u+i]=H(14u+i);W[uTerminal+12u+i]=H(16u+i);W[uTerminal+14u+i]=H(14u+i);W[uTerminal+16u+i]=B[uHeader+56u+i];W[uTerminal+18u+i]=H(56u+i);W[uTerminal+24u+i]=H(60u+i);W[uTerminal+28u+i]=H(74u+i);}
 if(S(Outcome)!=1u){W[uTerminal+14u]=0u;W[uTerminal+15u]=0u;}
}
void replayBirths(){
 if(S(Outcome)!=1u)return;
 uint col=S(Column),scanFrom=0u;
 for(uint rank=0u;rank<S(BirthCount);rank++){
  uint at=(uEventStart+rank)*64u,slot=E[at+16u];if(slot>=uK){fail(7u,16u,rank+1u);return;}
  for(uint freeSlot=scanFrom;freeSlot<slot;freeSlot++)if(B[freeSlot]==0u){fail(7u,16u,rank+1u);return;}scanFrom=slot+1u;
  for(uint w=0u;w<64u;w++)if(((w<32u?grsMask1.x:grsMask1.y)>>(w%32u)&1u)==0u&&E[at+w]!=0u){fail(7u,16u,rank+1u);return;}
  uint mass=rank+1u==S(BirthCount)?S(Admitted)-rank*uPackage:uPackage;
  if(E[at]!=GRS_wire_magicWords_event64||E[at+1u]!=1u||E[at+2u]!=uRootNonce.x||E[at+3u]!=uRootNonce.y||E[at+4u]!=1u||E[at+5u]!=1u||E[at+6u]!=1u||E[at+7u]!=uEventStart+rank+1u||E[at+8u]!=0u||E[at+9u]!=uFrameSerial.x||E[at+10u]!=uFrameSerial.y||E[at+11u]!=0u||E[at+14u]!=H(16u)||E[at+15u]!=H(17u)||E[at+24u]!=0u||E[at+25u]!=0u||E[at+28u]!=0u||E[at+29u]!=0u||E[at+30u]!=0u||E[at+31u]!=0u||E[at+17u]!=W[uSpare+10u*uK+slot]+1u||E[at+18u]!=B[uHeader+39u]+rank||E[at+19u]!=D(14u)||E[at+20u]!=col||E[at+21u]!=col||E[at+22u]!=0u||E[at+23u]!=mass||E[at+26u]!=S(X)||E[at+27u]!=S(Y)||W[uSpare+slot]!=0u||E[at+50u]!=D(6u)||E[at+51u]!=D(7u)||E[at+52u]!=((3u<<18u)|(1u<<4u))){fail(7u,16u,rank+1u);return;}
  uint phAt=uSpare+uColumn+8u*128u+col,ph=W[phAt];if(E[at+32u]!=ph||E[at+33u]!=ph+mass){fail(7u,16u,rank+1u);return;}W[phAt]=ph+mass;
  for(uint f=0u;f<16u;f++)if(f!=10u)W[uSpare+f*uK+slot]=0u;
  W[uSpare+slot]=mass;W[uSpare+uK+slot]=S(X);W[uSpare+2u*uK+slot]=S(Y);W[uSpare+5u*uK+slot]=E[at+18u];W[uSpare+6u*uK+slot]=1u;W[uSpare+7u*uK+slot]=D(14u);W[uSpare+8u*uK+slot]=E[at+48u];W[uSpare+9u*uK+slot]=E[at+49u];W[uSpare+10u*uK+slot]=E[at+17u];
  uvec2 origin;if(!grsAdd64(uvec2(B[uHeader+14u],B[uHeader+15u]),uvec2(1u,0u),origin)||!equal64(origin,uvec2(E[at+48u],E[at+49u]))||!equal64(origin,uvec2(E[at+12u],E[at+13u]))){fail(7u,16u,rank+1u);return;}
 }
}
void storeSource(){
 if(S(Append)!=0u){uint at=uDedup+S(TableIndex)*64u;for(uint i=0u;i<32u;i++)W[at+i]=D(i);for(uint i=0u;i<32u;i++)W[at+32u+i]=W[uTerminal+i];W[at+34u]=0u;}
 if(S(Outcome)==1u&&uRootKind==1u){uint at=uSourceEvents+B[uHeader+34u]*16u;for(uint i=0u;i<16u;i++)W[at+i]=0u;W[at]=D(6u);W[at+1u]=D(7u);W[at+2u]=H(14u);W[at+3u]=H(15u);W[at+5u]=S(Column);W[at+6u]=S(Admitted);W[at+7u]=B[uHeader+39u];W[at+8u]=S(BirthCount);W[at+9u]=D(14u);W[at+10u]=S(X);W[at+11u]=S(Y);W[at+12u]=B[uHeader+39u];W[at+13u]=H(39u);}
}
void pack(){
 uint size=uPubHeader+256u;for(uint i=0u;i<size;i++)E[i]=0u;
 for(uint i=0u;i<uK;i++){uint m=C[i];if(m==0u)continue;
  E[uPubSand+i]=C[5u*uK+i];E[uPubSand+uK+i]=C[10u*uK+i];E[uPubSand+2u*uK+i]=C[7u*uK+i];E[uPubSand+3u*uK+i]=m;E[uPubSand+4u*uK+i]=C[uK+i];E[uPubSand+5u*uK+i]=C[2u*uK+i];E[uPubSand+6u*uK+i]=C[3u*uK+i];E[uPubSand+7u*uK+i]=C[4u*uK+i];E[uPubSand+8u*uK+i]=grsColumnPreservingFloatBits(C[uK+i]);E[uPubSand+9u*uK+i]=grsQ20FloatBits(C[2u*uK+i]);E[uPubSand+10u*uK+i]=0x3fc00000u;E[uPubSand+11u*uK+i]=C[uK+i]>>20u;E[uPubSand+12u*uK+i]=C[6u*uK+i]&15u;E[uPubSand+14u*uK+i]=C[8u*uK+i];E[uPubSand+15u*uK+i]=C[9u*uK+i];
 }
 for(uint c=0u;c<128u;c++){uint s=C[uColumn+10u*128u+c],en=C[uColumn+13u*128u+c];E[uPubColumn+c]=s;E[uPubColumn+128u+c]=en;E[uPubColumn+256u+c]=s;E[uPubColumn+384u+c]=grsQ20FloatBits(s);E[uPubColumn+512u+c]=grsQ20FloatBits(en);E[uPubColumn+640u+c]=grsQ20FloatBits(s);E[uPubColumn+896u+c]=c;}
 uint ph=uPubHeader;E[ph]=GRS_wire_magicWords_publicationHeader;E[ph+1u]=1u;E[ph+2u]=256u;
 for(uint i=0u;i<8u;i++)E[ph+4u+i]=H(4u+i);
 for(uint i=0u;i<20u;i++)E[ph+12u+i]=H(14u+i);
 E[ph+31u]=uBanks.y;E[ph+32u]=H(72u);E[ph+33u]=H(73u);E[ph+34u]=H(41u);E[ph+35u]=0u;E[ph+36u]=128u;E[ph+37u]=uFrameSerial.x;E[ph+38u]=uFrameSerial.y;E[ph+39u]=H(72u);E[ph+40u]=H(73u);
 for(uint i=0u;i<24u;i++)E[ph+41u+i]=H(88u+i);
 E[ph+3u]=GRS_wire_sealWord;
}
void seal(){
 if(S(Fatal)==0u&&(W[uStatus+32u]!=uActualDispatches||W[uStatus+33u]!=127u||W[uStatus+34u]!=1023u))fail(9u,15u,0u);
 if(S(Fatal)==0u)W[uStatus+33u]|=128u;
 for(uint i=0u;i<256u;i++)E[i]=0u;E[0]=GRS_wire_magicWords_completion256;E[1]=1u;E[2]=256u;E[4]=uRootNonce.x;E[5]=uRootNonce.y;
 for(uint i=0u;i<8u;i++)E[6u+i]=H(4u+i);
 for(uint i=0u;i<4u;i++)E[14u+i]=B[uHeader+14u+i];for(uint i=0u;i<4u;i++)E[18u+i]=H(14u+i);
 E[22]=uControlSerial.x;E[23]=uControlSerial.y;E[24]=uFrameSerial.x;E[25]=uFrameSerial.y;E[26]=uQuantumOrdinal;E[27]=uRootKind;E[28]=S(Fatal)!=0u?4u:S(Outcome);E[29]=S(Reason);E[30]=S(FirstStage);E[31]=S(FirstOrdinal);E[32]=uBanks.x;E[33]=uBanks.y;E[34]=uBanks.z;E[35]=uPublicationSlot;E[36]=H(72u);E[37]=H(73u);
 for(uint i=0u;i<13u;i++)E[38u+i]=H(20u+i);
 E[51]=uEventArena;E[52]=uEventStart;E[53]=H(38u);E[54]=uEventCapacity;E[55]=B[uHeader+34u];E[56]=H(34u);E[57]=B[uHeader+33u];E[58]=H(33u);E[59]=D(14u);E[60]=D(15u);
 E[61]=W[uTerminal+3u];E[62]=W[uTerminal+4u];E[63]=W[uTerminal];E[64]=W[uTerminal+1u];E[65]=W[uTerminal+2u];E[66]=W[uTerminal+6u];E[67]=W[uTerminal+7u];E[68]=W[uTerminal+9u];
 for(uint i=0u;i<2u;i++){E[69u+i]=B[uHeader+56u+i];E[71u+i]=H(56u+i);E[77u+i]=H(60u+i);E[83u+i]=H(54u+i);}
 E[85]=W[uStatus+20u];E[86]=W[uStatus+21u];E[87]=W[uStatus+22u];E[88]=H(41u);E[89]=H(43u);E[91]=64u;E[92]=H(39u);E[93]=H(40u);if(uRootKind==1u)E[96]=S(Outcome)==1u?S(BirthCount):0u;else{for(uint i=59u;i<=68u;i++)E[i]=0u;E[94]=W[uStatus+18u];E[95]=W[uStatus+19u];}E[81]=H(52u)-H(54u);E[97]=W[uStatus+33u];E[99]=W[uStatus+34u];E[101]=S(Fatal)==0u?0xffffffffu:S(35u);E[102]=S(36u);E[103]=S(37u);E[106]=S(Outcome)==2u||S(Outcome)==3u?1u:0u;E[107]=H(45u);E[108]=uCarry;E[110]=W[uStatus+32u];E[111]=uActualBarriers;E[112]=uActualUploadedBytes;for(uint i=0u;i<24u;i++)E[114u+i]=H(88u+i);
 E[3]=GRS_wire_sealWord;
}
void rootStage(){
 if(uPass==17u){seal();return;}if(S(Fatal)!=0u)return;
 if(uPass==7u){foldState(false);metadata(false);terminal();if(S(Fatal)==0u)W[uStatus+33u]|=8u;}
 if(uPass==8u){if(!sourceAudit(false))fail(6u,15u,0u);else W[uStatus+34u]|=uRootKind==1u?79u:111u;}
 if(uPass==9u){uint cap=0u;int minGuard=2147483647,maxGuard=-2147483647,tanValue=int(W[uAngles+uint(int(H(45u))+7680)*4u]);for(uint col=0u;col<128u;col++){cap=max(cap,C[uColumn+13u*128u+col]);if(col<127u){int g=int(C[uColumn+10u*128u+col+1u])-int(C[uColumn+10u*128u+col])-tanValue;minGuard=min(minGuard,g);maxGuard=max(maxGuard,g);}}W[uStatus+20u]=uint(minGuard);W[uStatus+21u]=uint(maxGuard);W[uStatus+22u]=cap;if(H(74u)!=H(33u)||H(75u)!=0u)fail(6u,15u,74u);if(S(Fatal)==0u)W[uStatus+33u]|=16u;}
 if(uPass==13u){if(S(Outcome)==3u&&!payloadEqual(uDedup+S(TableIndex)*64u))fail(6u,15u,0u);if(S(Fatal)==0u)W[uStatus+34u]|=32u;}
 if(uPass==10u)for(uint i=0u;i<uStateWords;i++)W[uSpare+i]=B[i];
 if(uPass==11u){if(uRootKind==1u){replayBirths();if(S(Fatal)==0u)foldState(true);}else replayQuantum();if(S(Fatal)==0u)metadata(true);}
 if(uPass==12u){if(!sourceAudit(true)){fail(7u,16u,0u);return;}for(uint i=0u;i<uStateWords;i++)if(C[i]!=W[uSpare+i]){mismatch(7u,i,i,C[i],W[uSpare+i]);return;}W[uStatus+33u]|=32u;W[uStatus+34u]|=400u;}
 if(uPass==14u)storeSource();
 if(uPass==15u)pack();
 if(uPass==16u){
  for(uint i=0u;i<uK;i++){
   bool live=C[i]!=0u;uint expected[16];for(int f=0;f<16;f++)expected[f]=0u;
   if(live){expected[0]=C[5u*uK+i];expected[1]=C[10u*uK+i];expected[2]=C[7u*uK+i];expected[3]=C[i];expected[4]=C[uK+i];expected[5]=C[2u*uK+i];expected[6]=C[3u*uK+i];expected[7]=C[4u*uK+i];expected[8]=grsColumnPreservingFloatBits(C[uK+i]);expected[9]=grsQ20FloatBits(C[2u*uK+i]);expected[10]=0x3fc00000u;expected[11]=C[uK+i]>>20u;expected[12]=C[6u*uK+i]&15u;expected[14]=C[8u*uK+i];expected[15]=C[9u*uK+i];}
   for(uint f=0u;f<16u;f++)if(E[uPubSand+f*uK+i]!=expected[f]){fail(8u,15u,i);return;}
  }
  for(uint i=0u;i<16u*64u;i++)if(E[uPubWater+i]!=0u){fail(8u,15u,i);return;}
  for(uint col=0u;col<128u;col++){uint s=C[uColumn+10u*128u+col],en=C[uColumn+13u*128u+col];uint values[8]=uint[8](s,en,s,grsQ20FloatBits(s),grsQ20FloatBits(en),grsQ20FloatBits(s),0u,col);for(uint f=0u;f<8u;f++)if(E[uPubColumn+f*128u+col]!=values[f]){fail(8u,15u,col);return;}}
  for(uint i=0u;i<256u;i++){uint expected=0u;
   if(i==0u)expected=GRS_wire_magicWords_publicationHeader;else if(i==1u)expected=1u;else if(i==2u)expected=256u;else if(i==3u)expected=GRS_wire_sealWord;
   else if(i>=4u&&i<=11u)expected=H(i);else if(i>=12u&&i<=30u)expected=H(i+2u);else if(i==31u)expected=uBanks.y;else if(i==32u||i==33u)expected=H(i+40u);else if(i==34u)expected=H(41u);else if(i==36u)expected=128u;else if(i==37u)expected=uFrameSerial.x;else if(i==38u)expected=uFrameSerial.y;else if(i==39u||i==40u)expected=H(i+33u);else if(i>=41u&&i<=64u)expected=H(i+47u);
   if(E[uPubHeader+i]!=expected){fail(8u,15u,i);return;}
  }
  W[uStatus+33u]|=64u;W[uStatus+34u]|=512u;
 }
}

// Deterministic M0 quantum. Parallel proposal/sort/edge work; one ID-order resolver.
uniform uint uMotion,uIndexA,uIndexB,uEdges,uSortK,uSortJ,uColor,uGravity,uDamping,uSoft,uFlux;
uniform int uRequestedAngle;
bool qReplay=false;uint qCursor=0u,qMotionRows=0u,qBedRows=0u;
uint qs(uint at){return getState(qReplay,at);}void qw(uint at,uint v){putState(qReplay,at,v);}
uint col(uint field,uint c){return qs(uColumn+field*128u+c);}void putcol(uint field,uint c,uint v){qw(uColumn+field*128u+c,v);}
uint ph(uint at){return qs(uHeader+at);}void puth(uint at,uint v){qw(uHeader+at,v);}
void qfatal(uint reason,uint ordinal){fail(qReplay?7u:4u,reason,ordinal);}
bool slotLess(uint a,uint b){bool av=a<uK&&B[a]!=0u,bv=b<uK&&B[b]!=0u;if(av!=bv)return av;uint ai=av?B[5u*uK+a]:0u,bi=bv?B[5u*uK+b]:0u;if(ai!=bi)return ai<bi;return a<b;}
int chooseAngle(){
 int low=-605394,high=605394;for(uint c=0u;c<127u;c++){int d=int(col(10u,c+1u))-int(col(10u,c));low=max(low,d-605394);high=min(high,d+605394);}
 if(low>high){qfatal(14u,0u);return 0;}
 int lo=0,hi=15361;while(lo<hi){int mid=(lo+hi)/2;if(int(W[uAngles+uint(mid)*4u])<low)lo=mid+1;else hi=mid;}int first=lo;
 lo=0;hi=15361;while(lo<hi){int mid=(lo+hi)/2;if(int(W[uAngles+uint(mid)*4u])<=high)lo=mid+1;else hi=mid;}int last=lo-1;
 if(first>last||first>=15361||last<0){qfatal(14u,0u);return 0;}return clamp(uRequestedAngle+7680,first,last)-7680;
}
void prepareQuantum(bool spare){
 qReplay=spare;if(!spare){for(uint i=0u;i<32u;i++)W[uStatus+i]=0u;setS(Outcome,1u);setS(EventCursor,uEventStart);}
 if(!sourceAudit(spare)||uRequestedAngle< -7680||uRequestedAngle>7680||uGravity>1073741824u||uDamping>1048576u||uSoft>=605394u){qfatal(20u,0u);return;}
 int accepted=chooseAngle();if(S(Fatal)!=0u)return;
 if(!spare){W[uStatus+16u]=uint(accepted);W[uStatus+17u]=W[uAngles+uint(accepted+7680)*4u];}else if(W[uStatus+16u]!=uint(accepted)){qfatal(16u,0u);return;}
 puth(45u,uint(accepted));puth(46u,uint(uRequestedAngle));puth(52u,2621440u);puth(53u,0u);puth(54u,2621440u);puth(55u,0u);
 for(uint c=0u;c<128u;c++){putcol(14u,c,0u);putcol(15u,c,244667u);}
}
void proposal(uint slot){
 W[uMotion+31u*uK+slot]=0u;W[uMotion+30u*uK+slot]=0u;if(qs(slot)==0u)return;
 int vx=int(qs(3u*uK+slot)),vy=int(qs(4u*uK+slot)),dvx,dvy,gx,gy,sinx=int(W[uAngles+uint(int(W[uStatus+16u])+7680)*4u+1u]),cosy=int(W[uAngles+uint(int(W[uStatus+16u])+7680)*4u+2u]);
 if(!grsMulDivRNE(vx,int(uDamping),1048576u,dvx)||!grsMulDivRNE(vy,int(uDamping),1048576u,dvy)||!grsMulDivRNE(int(uGravity),sinx,1048576u,gx)||!grsMulDivRNE(int(uGravity),cosy,1048576u,gy)||!grsMulDivRNE(gx,1,120u,gx)||!grsMulDivRNE(gy,1,120u,gy)){W[uMotion+30u*uK+slot]=13u;return;}
 int nx=dvx+gx,ny=dvy-gy,dx,dy;
 if(abs(nx)>1073741824||abs(ny)>1073741824||!grsMulDivRNE(nx,1,120u,dx)||!grsMulDivRNE(ny,1,120u,dy)){W[uMotion+30u*uK+slot]=13u;return;}
 W[uMotion+slot]=uint(nx);W[uMotion+uK+slot]=uint(ny);W[uMotion+2u*uK+slot]=uint(int(qs(uK+slot))+dx);W[uMotion+3u*uK+slot]=uint(int(qs(2u*uK+slot))+dy);
}
uint clipGuard(uint request,uint donor,uint receiver){
 uint q=request;int tangent=int(W[uStatus+17u]);uint start=min(donor,receiver),end=max(donor,receiver);
 for(int edge=int(start)-1;edge<=int(end);edge++){if(edge<0||edge>=127)continue;int c0=edge,c1=edge+1,k=(uint(c1)==receiver?1:0)-(uint(c1)==donor?1:0)-(uint(c0)==receiver?1:0)+(uint(c0)==donor?1:0),d=int(col(10u,uint(c1)))-int(col(10u,uint(c0)))-tangent;
  if(abs(d)>605394){qfatal(14u,uint(edge));return 0u;}if(k>0)q=min(q,uint(605394-d)/uint(k));else if(k<0)q=min(q,uint(605394+d)/uint(-k));
 }return q;
}
uint clipLand(uint request,uint c){uint q=request;int tangent=int(W[uStatus+17u]);if(c>0u){int d=int(col(10u,c))-int(col(10u,c-1u))-tangent;if(abs(d)>605394){qfatal(14u,c);return 0u;}q=min(q,uint(605394-d));}if(c<127u){int d=int(col(10u,c+1u))-int(col(10u,c))-tangent;if(abs(d)>605394){qfatal(14u,c);return 0u;}q=min(q,uint(605394+d));}return q;}
void charge(uint amount,uint a,uint b,bool pair){
 if(amount>ph(54u)||amount>col(15u,a)||(pair&&amount>col(15u,b))){qfatal(15u,0u);return;}puth(54u,ph(54u)-amount);putcol(15u,a,col(15u,a)-amount);putcol(14u,a,col(14u,a)+amount);if(pair){putcol(15u,b,col(15u,b)-amount);putcol(14u,b,col(14u,b)+amount);}
}
void beforeSlot(uint slot,out uint data[16]){for(uint f=0u;f<16u;f++)data[f]=qs(f*uK+slot);}
void logSand(uint kind,uint slot,uint from,uint to,uint old[16],uint sa0,uint sa1,uint sb0,uint sb1,uint totalBefore,uint laBefore,uint lbBefore,uint num,uint den,uint edge){
 uint row=qReplay?qCursor:S(EventCursor);if(row>=uEventCapacity){qfatal(12u,row+1u);return;}
 if(kind!=7u){uint count=W[uMotion+31u*uK+slot];if(count>=32u){qfatal(12u,row+1u);return;}W[uMotion+31u*uK+slot]=count+1u;}
 uint e[64];for(int i=0;i<64;i++)e[i]=0u;e[0]=GRS_wire_magicWords_event64;e[1]=1u;e[2]=uRootNonce.x;e[3]=uRootNonce.y;e[4]=2u;e[5]=kind==7u?4u:2u;e[6]=kind;e[7]=row+1u;e[9]=uFrameSerial.x;e[10]=uFrameSerial.y;e[11]=uQuantumOrdinal;e[12]=B[uHeader+14u];e[13]=B[uHeader+15u];uvec2 commit;if(!grsAdd64(uvec2(B[uHeader+16u],B[uHeader+17u]),uvec2(1u,0u),commit)){qfatal(13u,row+1u);return;}e[14]=commit.x;e[15]=commit.y;
 e[16]=slot;e[17]=old[10];e[18]=old[5];e[19]=old[7];e[20]=from;e[21]=to;e[22]=old[0];e[23]=qs(slot);e[24]=old[1];e[25]=old[2];e[26]=qs(uK+slot);e[27]=qs(2u*uK+slot);e[28]=old[3];e[29]=old[4];e[30]=qs(3u*uK+slot);e[31]=qs(4u*uK+slot);
 e[48]=old[8];e[49]=old[9];if(old[8]==0u||old[9]!=0u||old[8]>B[uHeader+34u]){qfatal(15u,row+1u);return;}uint origin=uSourceEvents+(old[8]-1u)*16u;e[50]=W[origin];e[51]=W[origin+1u];uint nextTag=qs(6u*uK+slot);e[52]=(old[6]&15u)|((nextTag&15u)<<4u)|(((old[6]>>12u)&15u)<<10u)|(((nextTag>>12u)&15u)<<14u);
 if(kind==3u||kind==5u){e[32]=sa0;e[33]=sa1;e[34]=sb0;e[35]=sb1;e[36]=totalBefore;e[38]=ph(54u);e[40]=laBefore;e[41]=col(15u,from);if(kind==3u){e[42]=lbBefore;e[43]=col(15u,to);}}
 if(kind==2u||kind==3u||kind==4u){e[44]=num;e[46]=den;}
 if(kind==3u||kind==4u)e[54]=edge;
 uvec2 mask=kind==2u?grsMask2:kind==3u?grsMask3:kind==4u?grsMask4:kind==5u?grsMask5:grsMask7;
 for(uint i=0u;i<64u;i++)if(((i<32u?mask.x:mask.y)>>(i%32u)&1u)==0u)e[i]=0u;
 if(qReplay){for(uint i=0u;i<64u;i++)if(E[row*64u+i]!=e[i]){mismatch(7u,row+1u,i,e[i],E[row*64u+i]);return;}qCursor++;if(kind!=7u)qMotionRows++;}
 else {for(uint i=0u;i<64u;i++)E[row*64u+i]=e[i];setS(EventCursor,row+1u);if(kind!=7u)W[uStatus+18u]++;}
}
void contact(uint slot,uint c,uint x,uint y,int vx,int vy,uint state,uint wait,uint num,uint den,uint edge){uint old[16];beforeSlot(slot,old);qw(uK+slot,x);qw(2u*uK+slot,y);qw(3u*uK+slot,uint(vx));qw(4u*uK+slot,uint(vy));qw(6u*uK+slot,state|(wait<<12u));qw(11u*uK+slot,wait);if(old[1]!=x||old[2]!=y||old[3]!=uint(vx)||old[4]!=uint(vy)||old[6]!=(state|(wait<<12u)))logSand(4u,slot,c,c,old,0u,0u,0u,0u,0u,0u,0u,num,den,edge);}
void land(uint slot,uint c){
 uint mass=qs(slot),q=min(mass,min(ph(54u),col(15u,c)));q=clipLand(q,c);if(q==0u||S(Fatal)!=0u)return;uint old[16];beforeSlot(slot,old);uint mh=col(2u,c),p=col(8u,c),tb=ph(54u),lb=col(15u,c);
 if(mh>268435456u-q||p<q){qfatal(15u,slot+1u);return;}charge(q,c,c,false);putcol(2u,c,mh+q);putcol(10u,c,mh+q);putcol(8u,c,p-q);
 if(q==mass){for(uint f=0u;f<16u;f++)if(f!=10u)qw(f*uK+slot,0u);}else{qw(slot,mass-q);qw(2u*uK+slot,mh+q);qw(4u*uK+slot,0u);qw(6u*uK+slot,2u|(1u<<12u));qw(11u*uK+slot,1u);}
 logSand(5u,slot,c,c,old,mh,mh+q,p,p-q,tb,lb,0u,0u,0u,0u);
}
bool ratioLess(uint an,uint ad,uint bn,uint bd){uvec2 a=grsMul32(an,bd),b=grsMul32(bn,ad);return grsCompare64(a,b)<0;}
void candidateRatio(int num,uint den,uint type,inout uint n,inout uint d,inout uint hit){if(num<0||den==0u||uint(num)>den)return;uint nn=uint(num);if(ratioLess(nn,den,n,d)||(grsCompare64(grsMul32(nn,d),grsMul32(n,den))==0&&type<hit)){n=nn;d=den;hit=type;}}
void resolve(uint slot){
 if(qs(slot)==0u)return;uint c=qs(uK+slot)>>20u;int x0=int(B[uK+slot]),y0=int(B[2u*uK+slot]),endx=int(W[uMotion+2u*uK+slot]),endy=int(W[uMotion+3u*uK+slot]),dx=endx-x0,dy=endy-y0,vx=int(W[uMotion+slot]),vy=int(W[uMotion+uK+slot]);
 if(qs(2u*uK+slot)<col(10u,c)||qs(2u*uK+slot)==col(10u,c)&&dy<=0){contact(slot,c,qs(uK+slot),col(10u,c),int(qs(3u*uK+slot)),0,2u,1u,0u,1u,c);if(S(Fatal)==0u)land(slot,c);return;}
 if((qs(6u*uK+slot)&15u)!=1u)contact(slot,c,qs(uK+slot),qs(2u*uK+slot),int(qs(3u*uK+slot)),int(qs(4u*uK+slot)),1u,0u,0u,1u,c);
 uint lastn=0u,lastd=1u;
 for(uint turn=0u;turn<32u&&S(Fatal)==0u;turn++){
  uint n=1u,d=1u,hit=4u;c=qs(uK+slot)>>20u;
  if(dy<0)candidateRatio(y0-int(col(10u,c)),uint(-dy),0u,n,d,hit);
  if(dx<0)candidateRatio(x0,uint(-dx),1u,n,d,hit);else if(dx>0)candidateRatio(134217727-x0,uint(dx),1u,n,d,hit);
  if(dy>0)candidateRatio(268435456-y0,uint(dy),2u,n,d,hit);
  if(dx>0)candidateRatio(int((c+1u)*1048576u)-x0,uint(dx),3u,n,d,hit);else if(dx<0)candidateRatio(x0-int(c*1048576u),uint(-dx),3u,n,d,hit);
  if(ratioLess(n,d,lastn,lastd)){qfatal(17u,slot+1u);return;}
  int ix,iy,ivx,ivy;
  if(!grsMulDivRNE(dx,int(n),d,ix)||!grsMulDivRNE(dy,int(n),d,iy)||!grsMulDivRNE(vx-int(B[3u*uK+slot]),int(n),d,ivx)||!grsMulDivRNE(vy-int(B[4u*uK+slot]),int(n),d,ivy)){qfatal(13u,slot+1u);return;}
  ix+=x0;iy+=y0;ivx+=int(B[3u*uK+slot]);ivy+=int(B[4u*uK+slot]);
  if(hit==0u){contact(slot,c,uint(ix),col(10u,c),ivx,0,2u,1u,n,d,c);if(S(Fatal)==0u)land(slot,c);return;}
  if(hit==1u||hit==2u){uint x=uint(clamp(ix,0,134217727)),y=uint(clamp(iy,0,268435456));contact(slot,c,x,y,hit==1u?0:ivx,hit==2u?0:ivy,3u,5u,n,d,c);return;}
  if(hit==3u){
   int target=int(c)+(dx>0?1:-1);if(target<0||target>=128){qfatal(17u,slot+1u);return;}uint to=uint(target),mass=qs(slot),wall=dx>0?(c+1u)*1048576u-1u:c*1048576u,reason=0u;
   if(iy<int(col(10u,to)))reason=3u;else if(mass>268435456u-col(10u,to)-col(8u,to))reason=2u;else if(mass>ph(54u)||mass>col(15u,c)||mass>col(15u,to))reason=4u;
   if(reason!=0u){contact(slot,c,wall,uint(iy),0,ivy,3u,reason,n,d,min(c,to));return;}
   uint old[16];beforeSlot(slot,old);uint fromP=col(8u,c),toP=col(8u,to),tb=ph(54u),la=col(15u,c),lb=col(15u,to);charge(mass,c,to,true);putcol(8u,c,fromP-mass);putcol(8u,to,toP+mass);qw(uK+slot,dx>0?(c+1u)*1048576u:c*1048576u-1u);qw(2u*uK+slot,uint(iy));qw(3u*uK+slot,uint(vx));qw(4u*uK+slot,uint(vy));logSand(3u,slot,c,to,old,fromP,fromP-mass,toP,toP+mass,tb,la,lb,n,d,min(c,to));lastn=n;lastd=d;if(n==d)return;continue;
  }
  if(endx<0||endx>=134217728||endy<int(col(10u,c))||endy>268435456){qfatal(17u,slot+1u);return;}
  uint old[16];beforeSlot(slot,old);qw(uK+slot,uint(endx));qw(2u*uK+slot,uint(endy));qw(3u*uK+slot,uint(vx));qw(4u*uK+slot,uint(vy));if(old[1]!=uint(endx)||old[2]!=uint(endy)||old[3]!=uint(vx)||old[4]!=uint(vy))logSand(2u,slot,c,c,old,0u,0u,0u,0u,0u,0u,0u,1u,1u,0u);return;
 }if(S(Fatal)==0u)qfatal(12u,slot+1u);
}

void bedPropose(uint edge){
 if(edge>=127u||edge%3u!=uColor)return;uint at=uEdges+16u*edge;for(uint i=0u;i<16u;i++)W[at+i]=0u;
 int delta=int(col(10u,edge+1u))-int(col(10u,edge))-int(W[uStatus+17u]);if(abs(delta)<=int(uSoft))return;uint donor=delta>0?edge+1u:edge,receiver=delta>0?edge:edge+1u;
 uint free=268435456u-col(10u,receiver)-col(8u,receiver),request=uint(abs(delta)-int(uSoft))/2u;request=min(request,col(2u,donor));request=min(request,uFlux/120u);request=min(request,free);request=min(request,col(15u,donor));request=min(request,col(15u,receiver));
 W[at]=donor;W[at+1u]=receiver;W[at+2u]=request;W[at+4u]=col(2u,donor);W[at+5u]=col(2u,receiver);
}
void bedAllocate(){
 for(uint edge=uColor;edge<127u;edge+=3u){uint at=uEdges+16u*edge,request=W[at+2u];if(request==0u)continue;uint donor=W[at],receiver=W[at+1u],grant=clipGuard(min(request,ph(54u)),donor,receiver);if(S(Fatal)!=0u)return;if(grant==0u)continue;
  uint row=qReplay?qCursor:S(EventCursor);if(row>=uEventCapacity){qfatal(12u,row+1u);return;}
  W[at+3u]=grant;W[at+6u]=col(15u,donor);W[at+7u]=col(15u,receiver);W[at+8u]=ph(54u);W[at+10u]=row;charge(grant,donor,receiver,true);W[at+9u]=ph(54u);
  if(qReplay)qCursor++;else{setS(EventCursor,row+1u);W[uStatus+19u]++;}
 }
}
void bedApply(uint edge){
 if(edge>=127u||(!qReplay&&edge%3u!=uColor))return;uint at=uEdges+16u*edge,q=W[at+3u];if(q==0u)return;uint donor=W[at],receiver=W[at+1u],oldDonor=W[at+4u],oldReceiver=W[at+5u];putcol(2u,donor,oldDonor-q);putcol(10u,donor,oldDonor-q);putcol(2u,receiver,oldReceiver+q);putcol(10u,receiver,oldReceiver+q);
 uint e[64];for(int i=0;i<64;i++)e[i]=0u;uint row=W[at+10u];e[0]=GRS_wire_magicWords_event64;e[1]=1u;e[2]=uRootNonce.x;e[3]=uRootNonce.y;e[4]=2u;e[5]=3u;e[6]=6u;e[7]=row+1u;e[9]=uFrameSerial.x;e[10]=uFrameSerial.y;e[11]=uQuantumOrdinal;e[12]=B[uHeader+14u];e[13]=B[uHeader+15u];uvec2 commit;if(!grsAdd64(uvec2(B[uHeader+16u],B[uHeader+17u]),uvec2(1u,0u),commit)){qfatal(13u,row+1u);return;}e[14]=commit.x;e[15]=commit.y;e[20]=donor;e[21]=receiver;e[32]=oldDonor;e[33]=oldDonor-q;e[34]=oldReceiver;e[35]=oldReceiver+q;e[36]=W[at+8u];e[38]=W[at+9u];e[40]=W[at+6u];e[41]=col(15u,donor);e[42]=W[at+7u];e[43]=col(15u,receiver);e[52]=2u<<18u;e[54]=edge;
 if(qReplay){for(uint i=0u;i<64u;i++)if(E[row*64u+i]!=e[i]){mismatch(7u,row+1u,i,e[i],E[row*64u+i]);return;}qBedRows++;}else for(uint i=0u;i<64u;i++)E[row*64u+i]=e[i];
}
void finalContactFold(){
 for(uint order=0u;order<uPad;order++){uint slot=W[uIndexA+order];if(slot>=uK||qs(slot)==0u)continue;uint c=qs(uK+slot)>>20u,y=qs(2u*uK+slot),s=col(10u,c),tag=qs(6u*uK+slot),old[16];beforeSlot(slot,old);
  if(y<s){qw(2u*uK+slot,s);qw(4u*uK+slot,0u);qw(6u*uK+slot,2u|(1u<<12u));qw(11u*uK+slot,1u);}
  else if((tag&15u)==2u&&y>s){qw(6u*uK+slot,1u);qw(11u*uK+slot,0u);}
  if(old[2]!=qs(2u*uK+slot)||old[6]!=qs(6u*uK+slot))logSand(7u,slot,c,c,old,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u);
  if(S(Fatal)!=0u)return;
 }
 foldState(qReplay);
}
bool sortedValid(){
 for(uint i=0u;i<uPad;i++)W[uScan+i]=0u;
 for(uint i=0u;i<uPad;i++){uint slot=W[uIndexA+i];if(slot>=uPad||W[uScan+slot]!=0u)return false;W[uScan+slot]=1u;if(i>0u&&slotLess(slot,W[uIndexA+i-1u]))return false;}return true;
}
void resolveAll(){
 if(!sortedValid()){qfatal(15u,0u);return;}
 for(uint i=0u;i<uPad;i++){uint slot=W[uIndexA+i];if(slot<uK&&B[slot]!=0u&&W[uMotion+30u*uK+slot]!=0u){qfatal(W[uMotion+30u*uK+slot],i+1u);return;}}
 for(uint i=0u;i<uPad&&S(Fatal)==0u;i++){uint slot=W[uIndexA+i];if(slot<uK)resolve(slot);}
}
// A second, write-set-only replay prevents an unlogged writer from passing merely
// because deterministic integration repeats the same hidden mutation.
bool eventWord(uint row,uint word,uint expected){uint actual=E[row*64u+word];if(actual==expected)return true;mismatch(7u,row+1u,word,expected,actual);return false;}
void patchQuantumEvents(){
 for(uint i=0u;i<uStateWords;i++)W[uSpare+i]=B[i];qReplay=true;prepareQuantum(true);if(S(Fatal)!=0u)return;
 uint end=S(EventCursor);if(end<uEventStart||end>uEventCapacity){qfatal(16u,0u);return;}
 for(uint row=uEventStart;row<end;row++){
  uint at=row*64u,kind=E[at+6u];if(kind<2u||kind>7u){mismatch(7u,row+1u,6u,2u,kind);return;}
  uvec2 mask=kind==2u?grsMask2:kind==3u?grsMask3:kind==4u?grsMask4:kind==5u?grsMask5:kind==6u?grsMask6:grsMask7;
  for(uint word=0u;word<64u;word++)if(((word<32u?mask.x:mask.y)>>(word%32u)&1u)==0u&&!eventWord(row,word,0u))return;
  uint identity[16];for(uint i=0u;i<16u;i++)identity[i]=0u;identity[0]=GRS_wire_magicWords_event64;identity[1]=1u;identity[2]=uRootNonce.x;identity[3]=uRootNonce.y;identity[4]=2u;identity[5]=kind==6u?3u:kind==7u?4u:2u;identity[6]=kind;identity[7]=row+1u;identity[9]=uFrameSerial.x;identity[10]=uFrameSerial.y;identity[11]=uQuantumOrdinal;identity[12]=B[uHeader+14u];identity[13]=B[uHeader+15u];uvec2 next;if(!grsAdd64(uvec2(B[uHeader+16u],B[uHeader+17u]),uvec2(1u,0u),next)){qfatal(13u,row+1u);return;}identity[14]=next.x;identity[15]=next.y;for(uint i=0u;i<16u;i++)if(!eventWord(row,i,identity[i]))return;
  uint a=E[at+20u],b=E[at+21u],slot=E[at+16u],flags=E[at+52u],q=0u;
  if(a>=128u||b>=128u){qfatal(16u,row+1u);return;}
  if(kind==6u){
   if(!eventWord(row,52u,2u<<18u)||!eventWord(row,54u,min(a,b))||!eventWord(row,32u,col(2u,a))||!eventWord(row,34u,col(2u,b)))return;
   if((a>b?a-b:b-a)!=1u||E[at+33u]>=E[at+32u]){qfatal(16u,row+1u);return;}q=E[at+32u]-E[at+33u];
   if(col(2u,b)>268435456u-q||!eventWord(row,35u,col(2u,b)+q)||clipGuard(q,a,b)!=q){qfatal(16u,row+1u);return;}
  }else{
   if(slot>=uK||qs(slot)==0u){qfatal(16u,row+1u);return;}
   const uint fields[9]=uint[9](10u,5u,7u,0u,1u,2u,3u,4u,8u);const uint words[9]=uint[9](17u,18u,19u,22u,24u,25u,28u,29u,48u);
   for(uint i=0u;i<9u;i++)if(!eventWord(row,words[i],qs(fields[i]*uK+slot)))return;
   if(!eventWord(row,49u,qs(9u*uK+slot))||a!=(qs(uK+slot)>>20u)){qfatal(16u,row+1u);return;}
   uint origin=qs(8u*uK+slot);if(origin==0u||origin>B[uHeader+34u]){qfatal(16u,row+1u);return;}uint source=uSourceEvents+(origin-1u)*16u;if(!eventWord(row,50u,W[source])||!eventWord(row,51u,W[source+1u]))return;
   uint tag=qs(6u*uK+slot),newState=(flags>>4u)&15u,newWait=(flags>>14u)&15u;
   if((flags&0xfffc0300u)!=0u||(flags&15u)!=(tag&15u)||((flags>>10u)&15u)!=((tag>>12u)&15u)||newState>3u||(newState<=1u?newWait!=0u:newState==2u?newWait!=1u:newWait<2u||newWait>5u)){qfatal(16u,row+1u);return;}
   if(kind==3u){
    q=qs(slot);if((a>b?a-b:b-a)!=1u||!eventWord(row,23u,q)||col(8u,a)<q||col(8u,b)>268435456u-col(10u,b)||q>268435456u-col(10u,b)-col(8u,b)||!eventWord(row,32u,col(8u,a))||!eventWord(row,33u,col(8u,a)-q)||!eventWord(row,34u,col(8u,b))||!eventWord(row,35u,col(8u,b)+q)){if(S(Fatal)==0u)qfatal(16u,row+1u);return;}
   }else if(kind==5u){
    if(a!=b||E[at+23u]>=qs(slot)){qfatal(16u,row+1u);return;}q=qs(slot)-E[at+23u];if(col(8u,a)<q||col(2u,a)>268435456u-q||clipLand(q,a)!=q||!eventWord(row,32u,col(2u,a))||!eventWord(row,33u,col(2u,a)+q)||!eventWord(row,34u,col(8u,a))||!eventWord(row,35u,col(8u,a)-q)){if(S(Fatal)==0u)qfatal(16u,row+1u);return;}
   }else if(a!=b||!eventWord(row,23u,qs(slot))){qfatal(16u,row+1u);return;}
   if(kind!=5u&&(E[at+26u]>>20u)!=b){qfatal(16u,row+1u);return;}
   if(kind==2u&&(newState!=(tag&15u)||newWait!=((tag>>12u)&15u))){qfatal(16u,row+1u);return;}
  }
  if(kind==3u||kind==5u||kind==6u){
   bool pair=kind!=5u;if(!eventWord(row,36u,ph(54u))||!eventWord(row,40u,col(15u,a))||(pair&&!eventWord(row,42u,col(15u,b))))return;
   charge(q,a,b,pair);if(S(Fatal)!=0u)return;if(!eventWord(row,38u,ph(54u))||!eventWord(row,41u,col(15u,a))||(pair&&!eventWord(row,43u,col(15u,b))))return;
   if(kind==3u){putcol(8u,a,E[at+33u]);putcol(8u,b,E[at+35u]);}
   else if(kind==5u){putcol(2u,a,E[at+33u]);putcol(10u,a,E[at+33u]);putcol(8u,a,E[at+35u]);}
   else{putcol(2u,a,E[at+33u]);putcol(10u,a,E[at+33u]);putcol(2u,b,E[at+35u]);putcol(10u,b,E[at+35u]);}
  }
  if(kind!=6u){uint newMass=E[at+23u];if(newMass==0u){for(uint f=0u;f<16u;f++)if(f!=10u)qw(f*uK+slot,0u);for(uint word=26u;word<=27u;word++)if(!eventWord(row,word,0u))return;if(!eventWord(row,30u,0u)||!eventWord(row,31u,0u)||((flags>>4u)&15u)!=0u||((flags>>14u)&15u)!=0u){qfatal(16u,row+1u);return;}}
   else{qw(slot,newMass);qw(uK+slot,E[at+26u]);qw(2u*uK+slot,E[at+27u]);qw(3u*uK+slot,E[at+30u]);qw(4u*uK+slot,E[at+31u]);qw(6u*uK+slot,((flags>>4u)&15u)|(((flags>>14u)&15u)<<12u));qw(11u*uK+slot,(flags>>14u)&15u);}
  }
 }
 foldState(true);qReplay=false;
}
void replayQuantum(){
 qReplay=true;qCursor=uEventStart;qMotionRows=qBedRows=0u;prepareQuantum(true);if(S(Fatal)!=0u)return;
 for(uint i=0u;i<uK;i++)proposal(i);resolveAll();if(S(Fatal)!=0u)return;
 for(uint color=0u;color<3u;color++){
  // No cross-workgroup synchronization in replay: a single invocation replays the color order.
  for(uint edge=color;edge<127u;edge+=3u){uint at=uEdges+16u*edge;for(uint i=0u;i<16u;i++)W[at+i]=0u;int delta=int(col(10u,edge+1u))-int(col(10u,edge))-int(W[uStatus+17u]);if(abs(delta)<=int(uSoft))continue;uint donor=delta>0?edge+1u:edge,receiver=delta>0?edge:edge+1u;uint free=268435456u-col(10u,receiver)-col(8u,receiver);W[at]=donor;W[at+1u]=receiver;uint request=uint(abs(delta)-int(uSoft))/2u;request=min(request,col(2u,donor));request=min(request,uFlux/120u);request=min(request,free);request=min(request,col(15u,donor));W[at+2u]=min(request,col(15u,receiver));W[at+4u]=col(2u,donor);W[at+5u]=col(2u,receiver);}
  // Allocate the same edge prefix, then compare/apply each disjoint pair.
  for(uint edge=color;edge<127u;edge+=3u){uint at=uEdges+16u*edge,request=W[at+2u];if(request==0u)continue;uint donor=W[at],receiver=W[at+1u],grant=clipGuard(min(request,ph(54u)),donor,receiver);if(grant==0u)continue;W[at+3u]=grant;W[at+6u]=col(15u,donor);W[at+7u]=col(15u,receiver);W[at+8u]=ph(54u);W[at+10u]=qCursor++;charge(grant,donor,receiver,true);W[at+9u]=ph(54u);}
  for(uint edge=color;edge<127u;edge+=3u){bedApply(edge);if(S(Fatal)!=0u)return;}
 }
 finalContactFold();if(qCursor!=C[uHeader+38u]||qMotionRows!=W[uStatus+18u]||qBedRows!=W[uStatus+19u])qfatal(16u,qCursor+1u);
 if(S(Fatal)==0u)patchQuantumEvents();qReplay=false;
}
void quantumPass(){
 uint i=gl_GlobalInvocationID.x;qReplay=false;
 if(uPass==18u){if(i==0u){prepareQuantum(false);if(S(Fatal)==0u)W[uStatus+33u]|=2u;}return;}
 if(S(Fatal)!=0u)return;
 if(uPass==19u){if(i<uPad)W[uIndexA+i]=i;return;}
 if(uPass==20u){uint other=i^uSortJ;if(i<uPad&&other>i){uint a=W[uIndexA+i],b=W[uIndexA+other];bool asc=(i&uSortK)==0u,less=slotLess(a,b);W[uIndexB+i]=asc?(less?a:b):(less?b:a);W[uIndexB+other]=asc?(less?b:a):(less?a:b);}return;}
 if(uPass==21u){if(i<uK)proposal(i);return;}
 if(uPass==22u){if(i==0u){resolveAll();if(S(Fatal)==0u)W[uStatus+33u]|=4u;}return;}
 if(uPass==23u){bedPropose(i);return;}if(uPass==24u){if(i==0u)bedAllocate();return;}if(uPass==25u){bedApply(i);return;}
 if(uPass==26u&&i==0u){finalContactFold();metadata(false);if(S(Fatal)==0u)W[uStatus+33u]|=8u;}
}

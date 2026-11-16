bool g9TouchWeights(){
 uvec4 decodedX,decodedRadius;uint x,radius;
 if(!g9DecodeBinary64(g9InputPair(46u),20u,true,decodedX)||!g9DecodeBinary64(g9InputPair(48u),20u,true,decodedRadius)||!g9Uint(decodedX,x)||!g9Uint(decodedRadius,radius)||x>=134217728u||radius==0u||radius>134217728u){g9Fault(22u,0u,46u);return false;}
 uint total=0u;for(uint c=0u;c<128u;++c){uint distance=uint(abs(int(c*1048576u+524288u)-int(x))),weight=distance<radius?radius-distance:0u;W[uHamilton+c*2048u+1600u]=weight;total+=weight;}
 // N*r can exceed U32. Sum in limbs and use the same denominator on all
 // components; equal remainders resolve by ascending column.
 uvec4 sum=uvec4(0u);for(uint c=0u;c<128u;++c)if(!g9UAdd(sum,g9U32(W[uHamilton+c*2048u+1600u]),sum)){g9Fault(29u,c,0u);return false;}
 if(g9Zero(sum)){g9Set(56u,2u);return true;}
 for(uint component=0u;component<2u;++component){uvec4 impulse=component==0u?g9S64(g9InputPair(56u)):g9U64(g9InputPair(58u)),magnitude=g9Magnitude(impulse),admitted=uvec4(0u);uvec4 limit=uvec4(0u,1u<<26u,0u,0u);
  if(g9UCmp(magnitude,limit)>0){g9Fault(22u,0u,56u+2u*component);return false;}
  for(uint c=0u;c<128u;++c){uint at=uHamilton+c*2048u+1604u+component*12u;uvec4 product,q,r;
   if(!g9UMul(magnitude,g9U32(W[uHamilton+c*2048u+1600u]),product)||!g9UDiv(product,sum,q,r)||!g9UAdd(admitted,q,admitted)){g9Fault(29u,c,1u);return false;}
   g9WriteWide(at,q);g9WriteWide(at+4u,r);W[at+8u]=0u;
  }
  uvec4 remaining;if(!g9USub(magnitude,admitted,remaining)||any(notEqual(remaining.yzw,uvec3(0u)))||remaining.x>=128u){g9Fault(29u,0u,2u);return false;}
  for(uint n=0u;n<remaining.x;++n){uint best=128u;uvec4 greatest=uvec4(0u);for(uint c=0u;c<128u;++c){uint at=uHamilton+c*2048u+1604u+component*12u;uvec4 rem=g9ReadWide(at+4u);if(W[at+8u]==0u&&(best==128u||g9UCmp(rem,greatest)>0)){best=c;greatest=rem;}}
   if(best==128u){g9Fault(29u,0u,n);return false;}uint at=uHamilton+best*2048u+1604u+component*12u;uvec4 plus;if(!g9UAdd(g9ReadWide(at),g9U32(1u),plus)){g9Fault(29u,best,n);return false;}g9WriteWide(at,plus);W[at+8u]=1u;
  }
  if(component==0u&&g9Negative(impulse))for(uint c=0u;c<128u;++c){uint at=uHamilton+c*2048u+1604u;uvec4 signedValue;if(!g9FromMagnitude(g9ReadWide(at),true,signedValue)){g9Fault(29u,c,3u);return false;}g9WriteWide(at,signedValue);}
 }return true;
}
// Snapshot one column and its full bed/flow maps in its fixed Hamilton range.
// A rejected local touch restores all recruited sources, p and rest state.
void g9SaveTouchColumn(uint c){uint base=uHamilton+c*2048u;for(uint i=0u;i<32u;++i)W[base+1664u+i]=g9Column(c,i);for(uint i=0u;i<256u;++i){W[base+512u+i]=C[uBed+c*256u+i];W[base+768u+i]=C[uFlow+c*256u+i];}}
void g9RestoreTouchColumn(uint c){uint base=uHamilton+c*2048u;for(uint i=0u;i<32u;++i)g9PutColumn(c,i,W[base+1664u+i]);for(uint i=0u;i<256u;++i){C[uBed+c*256u+i]=W[base+512u+i];C[uFlow+c*256u+i]=W[base+768u+i];}}
bool g9Launch(uint c,uvec4 jy,out uvec4 actualJy,out uvec4 work){
 actualJy=work=uvec4(0u);uint flow=g9Column(c,1u),count=g9MapCount(true,c);if(flow==0u||count==0u||g9Zero(jy))return true;
 uint originAt=g9MapAddress(true,c,0u),q=min(C[originAt+3u],min(W[uConfig+2u],244666u)),slot=uK;
 if(q==0u)return true;for(uint i=0u;i<uK;++i)if(g9Packet(i,0u)==0u){slot=i;break;}
 if(slot==uK||C[18u]==0u)return true;if(g9Packet(slot,7u)==0xffffffffu){g9Fault(29u,slot,7u);return false;}
 uvec4 vyWide,vxWide;if(!g9SDiv(jy,g9U32(q),0u,vyWide)||!g9SDiv(g9Momentum(c),g9U32(flow),0u,vxWide)){g9Fault(29u,c,0u);return false;}int vx,vy;
 if(!g9I32(vxWide,vx)||!g9I32(vyWide,vy)||vy<=0||vy>1073741824||abs(vx)>1073741824)return true;
 uvec4 px,newP,oldP=g9Momentum(c);if(!g9SMul(g9U32(q),g9S32(vx),px)||!g9SMul(g9U32(q),g9S32(vy),actualJy)||!g9SSub(oldP,px,newP)){g9Fault(29u,c,1u);return false;}
 G9EnergyInterval before,after,delta;if(!g9ColumnEnergy(c,before)){g9Fault(29u,c,2u);return false;}
 uint x=c*1048576u+524288u,y2=2u*(g9Column(c,0u)+flow-q)+q;uvec4 packetEnergy;
 if(!g9PacketEnergy(q,x,y2,vx,vy,packetEnergy)){g9Fault(29u,c,3u);return false;}
 // Preflight numerical split energy using temporary column scalars, without
 // altering origins, ID or slot before the work cap has accepted this mapping.
 g9PutColumn(c,1u,flow-q);if(!g9PutMomentum(c,newP))return false;
 if(!g9ColumnEnergy(c,after)||!g9IntervalAdd(after,g9Exact(packetEnergy),after)||!g9IntervalSubtract(after,before,delta)){g9Fault(29u,c,4u);return false;}
 g9PutColumn(c,1u,flow);if(!g9PutMomentum(c,oldP))return false;
 if(!g9ChargeTouch(delta)){g9Set(57u,1u);actualJy=uvec4(0u);return true;}work=delta.high;
 uint event,path;if(!g9Reserve(1u,0u,event,path))return false;uvec3 origin=g9Key(true,c,0u);uint parent=C[originAt+3u],generation=g9Packet(slot,7u)+1u,id=C[18u];
 if(!g9MapTake(true,c,0u,q)){g9Fault(24u,c,0u);return false;}g9PutColumn(c,1u,flow-q);g9PutColumn(c,8u,g9Column(c,8u)+q);g9PutColumn(c,5u,0u);g9PutColumn(c,13u,g9Column(c,13u)|4u);if(!g9PutMomentum(c,newP))return false;g9Derived(c);
 g9RetirePacket(slot);g9PutPacket(slot,0u,q);g9PutPacket(slot,1u,x);g9PutPacket(slot,2u,y2);g9PutPacket(slot,3u,uint(vx));g9PutPacket(slot,4u,uint(vy));g9PutPacket(slot,5u,c);g9PutPacket(slot,6u,id);g9PutPacket(slot,7u,generation);g9PutPacket(slot,8u,origin.x);g9PutPacket(slot,9u,origin.y);g9PutPacket(slot,10u,origin.z);g9PutPacket(slot,11u,0u);
 if(!g9PacketCertificate(slot,1u,origin.x,origin.z,q,c,0u,parent,event+1u)){g9Fault(29u,slot,0u);return false;}C[18u]=id+1u;
 uint row[64];g9Event(row,G9_EVENT_LAUNCH,slot,c,c);row[12]=id;row[13]=generation;row[14]|=4u;row[15]=q;row[17]=origin.x;row[18]=origin.y;row[19]=origin.z;row[20]=x;row[21]=y2;row[22]=uint(vx);row[23]=uint(vy);row[24]=flow;row[25]=flow-q;row[26]=oldP.x;row[27]=oldP.y;row[28]=newP.x;row[29]=newP.y;row[30]=px.x;row[31]=px.y;row[32]=actualJy.x;row[33]=actualJy.y;for(uint i=0u;i<4u;++i)row[34u+i]=work[i];return g9WriteEvent(event,row)&&g9Seal();
}
bool g9Touch(){
 if(g9Input(40u)!=2u||g9Ctl(18u)!=0u)return true;
 if(!g9TouchWeights())return false;if(g9Ctl(56u)==2u)return true;
 uvec4 cap=uvec4(g9Input(60u),g9Input(61u),g9Input(62u),g9Input(63u));if(g9UCmp(cap,uvec4(0u,0u,0u,1u<<24u))>0){g9Fault(22u,0u,60u);return false;}g9WriteWide(uControls+32u,uvec4(0u));g9WriteWide(uControls+36u,cap);
 for(uint c=0u;c<128u;++c){uint base=uHamilton+c*2048u;uvec4 jx=g9ReadWide(base+1604u),jy=g9ReadWide(base+1616u);if(g9Zero(jx)&&g9Zero(jy))continue;
  uint eventBefore=g9Ctl(6u),pathBefore=g9Ctl(8u);uvec4 usedBefore=g9ReadWide(uControls+32u);g9SaveTouchColumn(c);g9Set(57u,0u);
  uint band=min(g9Column(c,0u),W[uConfig+2u]);uvec4 threshold,product;if(!g9UMul(g9U32(W[uConfig]),g9U32(C[23u]),product)||!g9UMul(product,g9U32(band),product)||!g9SDiv(product,g9U32(65536u*120u),3u,threshold)){g9Fault(29u,c,5u);return false;}
  bool activate=!g9Zero(jy)||g9UCmp(g9Magnitude(jx),threshold)>0;
  if(activate){uvec4 q,r;if(!g9UMul(g9U32(band),g9U32(W[uConfig+3u]),product)||!g9UDiv(product,g9U32(65536u*120u),q,r)){g9Fault(29u,c,6u);return false;}if(q.x>0u&&!g9Activate(c,min(q.x,g9Column(c,0u)),2u))return false;}
  uint flow=g9Column(c,1u);uvec4 oldP=g9Momentum(c),newP,admittedX=uvec4(0u),admittedY=uvec4(0u),touchWork=uvec4(0u);
  if(flow>0u&&!g9Zero(jx)){
   G9EnergyInterval before,after,delta;if(!g9ColumnEnergy(c,before)||!g9SAdd(oldP,jx,newP)){g9Fault(29u,c,7u);return false;}
   uvec4 bound;if(!g9UMul(g9U32(flow),g9U32(1073741824u),bound)){g9Fault(29u,c,8u);return false;}
   if(g9UCmp(g9Magnitude(newP),bound)<=0){if(!g9PutMomentum(c,newP)||!g9ColumnEnergy(c,after)||!g9IntervalSubtract(after,before,delta)){g9Fault(29u,c,9u);return false;}
    if(g9ChargeTouch(delta)){admittedX=jx;touchWork=delta.high;}else{g9Set(57u,1u);if(!g9PutMomentum(c,oldP))return false;}
   }
  }
  uvec4 launchWork=uvec4(0u);if(g9Ctl(57u)==0u&&!g9Launch(c,jy,admittedY,launchWork))return false;
  if(!g9SAdd(touchWork,launchWork,touchWork)){g9Fault(29u,c,10u);return false;}
  if(g9Ctl(57u)!=0u||g9Zero(admittedX)&&g9Zero(admittedY)){
   // A cap failure rolls back this column's entire touch, including an
   // earlier accepted tangent impulse and recruitment, without refund credit
   // from a later negative-work operation.
   g9RestoreTouchColumn(c);g9Set(6u,eventBefore);g9Set(7u,eventBefore);g9Set(8u,pathBefore);g9Set(9u,pathBefore);g9WriteWide(uControls+32u,usedBefore);
   uint denied[64];g9Event(denied,G9_EVENT_TOUCH_WORK,c,c,c);denied[14]|=8u;denied[15]=g9Ctl(57u)!=0u?18u:3u;denied[16]=denied[17]=g9Column(c,1u);denied[18]=jx.x;denied[19]=jx.y;denied[22]=jy.x;denied[23]=jy.y;uvec4 unchanged=g9Momentum(c);denied[26]=denied[28]=unchanged.x;denied[27]=denied[29]=unchanged.y;
   uvec4 remaining;if(!g9USub(cap,usedBefore,remaining)){g9Fault(29u,c,11u);return false;}for(uint i=0u;i<4u;++i)denied[34u+i]=remaining[i];if(!g9OneEvent(denied))return false;continue;
  }
  g9PutColumn(c,5u,0u);g9PutColumn(c,13u,g9Column(c,13u)|8u);
  W[uColumnStage+c*64u+21u]=g9Column(c,13u);
  if(!g9Accumulate(58u,admittedX)||!g9Accumulate(62u,admittedY)||!g9Accumulate(86u,touchWork))return false;
  uint row[64];g9Event(row,G9_EVENT_TOUCH_WORK,c,c,c);row[14]|=4u;row[16]=flow;row[17]=g9Column(c,1u);row[18]=jx.x;row[19]=jx.y;row[20]=admittedX.x;row[21]=admittedX.y;row[22]=jy.x;row[23]=jy.y;row[24]=admittedY.x;row[25]=admittedY.y;row[26]=oldP.x;row[27]=oldP.y;uvec4 nowP=g9Momentum(c);row[28]=nowP.x;row[29]=nowP.y;for(uint i=0u;i<4u;++i)row[30u+i]=touchWork[i];uvec4 left;if(!g9USub(cap,g9ReadWide(uControls+32u),left)){g9Fault(29u,c,11u);return false;}for(uint i=0u;i<4u;++i)row[34u+i]=left[i];if(!g9OneEvent(row))return false;
 }return true;
}

uvec4 g9VelocityDelta(int a,int b){uvec4 value;if(!g9SSub(g9S32(a),g9S32(b),value)){g9Fault(29u,0u,0u);return uvec4(0u);}return value;}
// Full rational constant-acceleration trajectories. Each query is evaluated
// from the immutable fragment entry; binary search never accumulates rounding.
bool g9Trajectory(int start,int velocity,int acceleration,uint n,uint d,uint scale,out int value){
 uvec4 denominator,term0,term1,term2,numerator;
 if(d==0u||!g9UMul(g9U32(d),g9U32(d),denominator)||!g9SMul(g9S32(start),denominator,term0)||!g9SShift(term0,1u,term0)||
    !g9SMul(g9S32(velocity),g9U32(n),term1)||!g9SMul(term1,g9U32(d),term1)||!g9SMul(term1,g9U32(2u*scale),term1)||
    !g9SMul(g9S32(acceleration),g9U32(n),term2)||!g9SMul(term2,g9U32(n),term2)||!g9SMul(term2,g9U32(scale),term2)||
    !g9SAdd(term0,term1,numerator)||!g9SAdd(numerator,term2,numerator)||!g9UShift(denominator,1u,denominator)||!g9SDiv(numerator,denominator,1u,numerator)||!g9I32(numerator,value))return false;
 return true;
}
bool g9VelocityAt(int velocity,int acceleration,uint tick,out int result){
 uvec4 change;if(!g9RneProduct(g9S32(acceleration),g9U32(tick),65536u*120u*uJ,change)||!g9SAdd(g9S32(velocity),change,change)||!g9I32(change,result))return false;return abs(result)<=1073741824;
}
bool g9PositionAt(int start,int velocity,int acceleration,uint tick,uint scale,out int value){return g9Trajectory(start,velocity,acceleration,tick,65536u*120u*uJ,scale,value);}
bool g9Turn(int velocity,int acceleration,out uint numerator,out uint denominator){
 numerator=denominator=0u;if(velocity==0||acceleration==0||(velocity<0)==(acceleration<0))return false;
 numerator=uint(abs(velocity));denominator=uint(abs(acceleration));
 uvec4 left,right;if(!g9UMul(g9U32(numerator),g9U32(120u*uJ),left)||!g9UMul(g9U32(denominator),g9U32(1u),right))return false;
 return g9UCmp(left,right)<0;
}
// Compare at the rational TURN before mapping its diagnostic time to T16.
// This endpoint preserves a within-tick excursion instead of guessing the
// extremum from an already rounded tick or a sign test at two integer ticks.
bool g9Reach(int start,int velocity,int acceleration,uint scale,int plane,bool greater,uint begin,uint end,out uint hit){
 hit=0xffffffffu;int a,b;if(!g9PositionAt(start,velocity,acceleration,begin,scale,a)||!g9PositionAt(start,velocity,acceleration,end,scale,b))return false;
 uint tn,td;bool turning=g9Turn(velocity,acceleration,tn,td);uint turnTick=0u;
 if(turning){uvec4 scaled,q,r;if(!g9UMul(g9U32(tn),g9U32(65536u*120u*uJ),scaled)||!g9UDiv(scaled,g9U32(td),q,r))return false;turnTick=q.x+(g9Zero(r)?0u:1u);}
 for(uint segment=0u;segment<(turning?2u:1u);++segment){
  uint lo=segment==0u?begin:max(begin,turnTick),hi=segment==0u&&turning?min(end,turnTick):end;
  if(lo>hi)continue;int endValue;
  if(segment==0u&&turning&&turnTick<=end&&turnTick>=begin){if(!g9Trajectory(start,velocity,acceleration,tn,td,scale,endValue))return false;}
  else if(!g9PositionAt(start,velocity,acceleration,hi,scale,endValue))return false;
  int beginValue;if(!g9PositionAt(start,velocity,acceleration,lo,scale,beginValue))return false;
  bool initial=greater?beginValue>=plane:beginValue<=plane;
  if(initial&&beginValue==plane){
   uvec4 derivative,term;
   if(!g9SMul(g9S32(velocity),g9U32(65536u*120u*uJ),derivative)||!g9SMul(g9S32(acceleration),g9U32(lo),term)||!g9SAdd(derivative,term,derivative))return false;
   bool leaving=greater?g9SCmp(derivative,uvec4(0u))<0:g9SCmp(derivative,uvec4(0u))>0;
   if(leaving){if(segment==0u&&turning)continue;initial=false;if(lo==65536u)continue;++lo;}
  }
  if(initial){hit=min(hit,lo);continue;}
  if(!(greater?endValue>=plane:endValue<=plane))continue;
  for(uint iteration=0u;iteration<16u;++iteration){if(lo>=hi)break;uint mid=lo+(hi-lo)/2u;int value;
   bool clampTurn=segment==0u&&turning&&mid>=turnTick;
   if(clampTurn){if(!g9Trajectory(start,velocity,acceleration,tn,td,scale,value))return false;}else if(!g9PositionAt(start,velocity,acceleration,mid,scale,value))return false;
   if(greater?value>=plane:value<=plane)hi=mid;else lo=mid+1u;
  }hit=min(hit,lo);
 }return true;
}
bool g9PathRow(uint at,uint parent,uint slot,uint kind,uint t0,uint t1,int x0,int x1,int y0,int y1,int vx0,int vy0,int vx1,int vy1){
 if(at>=g9Ctl(8u)){g9Fault(28u,slot,at);return false;}uint row[16]=uint[16](parent,uMicro,slot,kind,t0,t1,uint(x0),uint(x1),uint(y0),uint(y1),uint(vx0),uint(vy0),uint(vx1),uint(vy1),0u,G9_SEAL);
 for(uint i=0u;i<15u;++i)E[uPathWord+at*16u+i]=row[i];memoryBarrierBuffer();E[uPathWord+at*16u+15u]=G9_SEAL;return true;
}
// TURN rows carry rational-extremum coordinates. T16 is diagnostic only;
// collision queries below still evaluate the immutable analytic trajectory.
bool g9EmitTurns(uint slot,uint parent,uint first,inout uint count,int x0,int y0,int vx0,int vy0,int gx,int ay,uint begin,uint end,inout uint cursor,inout int px,inout int py,inout int pvx,inout int pvy){
 uint nx,dx,ny,dy;bool tx=g9Turn(vx0,gx,nx,dx),ty=g9Turn(vy0,ay,ny,dy);
 for(uint order=0u;order<2u;++order){
  bool useX=tx;
  if(tx&&ty){uvec4 a,b;if(!g9UMul(g9U32(nx),g9U32(dy),a)||!g9UMul(g9U32(ny),g9U32(dx),b))return false;useX=g9UCmp(a,b)<=0;}
  if(!tx&&!ty)break;uint n=useX?nx:ny,d=useX?dx:dy;if(useX)tx=false;else ty=false;
  uvec4 ticks,left,right,q,r;
  if(!g9UMul(g9U32(n),g9U32(65536u*120u*uJ),ticks)||!g9UMul(g9U32(d),g9U32(begin),left)||!g9UMul(g9U32(d),g9U32(end),right))return false;
  if(g9UCmp(ticks,left)<=0||g9UCmp(ticks,right)>=0)continue;
  if(!g9UDiv(ticks,g9U32(d),q,r))return false;uint tick=q.x+(g9Zero(r)?0u:1u);
  int x,y,vx,vy;uvec4 v,change;
  if(!g9Trajectory(x0,vx0,gx,n,d,1u,x)||!g9Trajectory(y0,vy0,ay,n,d,2u,y)||!g9RneProduct(g9S32(gx),g9U32(n),d,change)||!g9SAdd(g9S32(vx0),change,v)||!g9I32(v,vx)||!g9RneProduct(g9S32(ay),g9U32(n),d,change)||!g9SAdd(g9S32(vy0),change,v)||!g9I32(v,vy))return false;
  if(count>=8u||!g9PathRow(first+count,parent,slot,useX?1u:2u,cursor,tick,px,x,py,y,pvx,pvy,vx,vy))return false;
  ++count;cursor=tick;px=x;py=y;pvx=vx;pvy=vy;
 }return true;
}
bool g9FlightPacket(uint slot){
 int gx=int(C[22u]),ay=-int(C[23u]);
 uint mass=g9Packet(slot,0u),birth=g9Packet(slot,11u),scratch=uContacts+slot*16u;
  W[scratch+9u]=0u;W[scratch+10u]=0u;W[scratch+11u]=0u;
  uvec4 proposed;bool sameRoot=g9UAdd(g9U64(uBaseCommit),g9U32(1u),proposed)&&g9UCmp(g9U64(uvec2(g9Packet(slot,23u),g9Packet(slot,24u))),proposed)==0;
  if(mass==0u||birth==uMicro&&sameRoot)return true;
  int x0=int(g9Packet(slot,1u)),y0=int(g9Packet(slot,2u)),vx0=int(g9Packet(slot,3u)),vy0=int(g9Packet(slot,4u));uint c=g9Packet(slot,5u),begin=0u,pathCount=0u;
  if(c>=128u||y0<int(2u*W[uColumnStage+c*64u+20u]+mass)){g9Reject(9u,slot,0u);return false;}
  uint eventStart,pathStart;if(!g9Reserve(1u,8u,eventStart,pathStart))return false;
  uint stop=65536u,contact=0u,column=c;int finalX,finalY,finalVx,finalVy;
  for(uint segment=0u;segment<4u;++segment){
   uint hit=65536u,kind=8u,candidate;int bedPlane=int(2u*W[uColumnStage+column*64u+20u]+mass);
   if(!g9Reach(y0,vy0,ay,2u,bedPlane,false,begin,65536u,candidate)){g9Fault(29u,slot,1u);return false;}if(candidate<=hit){hit=candidate;kind=4u;}
   if(!g9Reach(y0,vy0,ay,2u,int(536870912u-mass),true,begin,65536u,candidate)){g9Fault(29u,slot,2u);return false;}if(candidate<hit||candidate==hit&&kind!=4u){hit=candidate;kind=6u;}
   bool leftWall=column==0u,rightWall=column==127u;int left=int(column*1048576u),right=int((column+1u)*1048576u);
   if(!g9Reach(x0,vx0,gx,1u,leftWall?0:left-1,false,begin,65536u,candidate)){g9Fault(29u,slot,3u);return false;}uint leftKind=leftWall?7u:3u;if(candidate<hit||candidate==hit&&kind==8u){hit=candidate;kind=leftKind;}
   bool toLeft=kind==leftKind&&hit==candidate;
   if(!g9Reach(x0,vx0,gx,1u,rightWall?134217727:right,true,begin,65536u,candidate)){g9Fault(29u,slot,4u);return false;}if(candidate<hit||candidate==hit&&kind==8u){hit=candidate;kind=rightWall?7u:3u;toLeft=false;}
   if(hit>65536u)hit=65536u;
   int x,y,vx,vy;if(!g9PositionAt(x0,vx0,gx,hit,1u,x)||!g9PositionAt(y0,vy0,ay,hit,2u,y)||!g9VelocityAt(vx0,gx,hit,vx)||!g9VelocityAt(vy0,ay,hit,vy)){g9Fault(29u,slot,5u);return false;}
   if(kind==4u){y=bedPlane;vy=0;contact=4u;stop=hit;W[scratch+9u]=4u;W[scratch+10u]=hit;}
   else if(kind==6u){y=int(536870912u-mass);vy=0;stop=hit;}
   else if(kind==7u){x=toLeft?0:134217727;vx=0;stop=hit;}
   else if(kind==3u){
    uint next=toLeft?column-1u:column+1u;x=toLeft?left-1:right;
    if(y<int(2u*W[uColumnStage+next*64u+20u]+mass)){x=toLeft?left:right-1;vx=0;kind=5u;stop=hit;}
    else column=next;
   }
   int oldX,oldY,oldVx,oldVy;if(!g9PositionAt(x0,vx0,gx,begin,1u,oldX)||!g9PositionAt(y0,vy0,ay,begin,2u,oldY)||!g9VelocityAt(vx0,gx,begin,oldVx)||!g9VelocityAt(vy0,ay,begin,oldVy)){g9Fault(29u,slot,6u);return false;}
   uint pathBegin=begin;
   if(!g9EmitTurns(slot,eventStart+1u,pathStart,pathCount,x0,y0,vx0,vy0,gx,ay,begin,hit,pathBegin,oldX,oldY,oldVx,oldVy)){g9Fault(28u,slot,pathCount);return false;}
   if(pathCount>=8u||!g9PathRow(pathStart+pathCount,eventStart+1u,slot,kind,pathBegin,hit,oldX,x,oldY,y,oldVx,oldVy,vx,vy)){g9Fault(28u,slot,pathCount);return false;}++pathCount;
   finalX=x;finalY=y;finalVx=vx;finalVy=vy;
   if(kind!=3u||hit==65536u)break;
   if(segment>0u){g9Reject(5u,slot,1u);return false;}begin=hit;
  }
  int freeVx,freeVy;if(!g9VelocityAt(vx0,gx,stop,freeVx)||!g9VelocityAt(vy0,ay,stop,freeVy)){g9Fault(29u,slot,7u);return false;}
  uvec4 gravityX,gravityY,supportX,supportY;
  if(!g9SMul(g9U32(mass),g9VelocityDelta(freeVx,vx0),gravityX)||!g9SMul(g9U32(mass),g9VelocityDelta(freeVy,vy0),gravityY)||!g9SMul(g9U32(mass),g9VelocityDelta(finalVx,freeVx),supportX)||!g9SMul(g9U32(mass),g9VelocityDelta(finalVy,freeVy),supportY)||
     !g9Accumulate(50u,gravityX)||!g9Accumulate(54u,gravityY)||!g9Accumulate(74u,supportX)||!g9Accumulate(78u,supportY)){g9Fault(29u,slot,8u);return false;}
  // Projection onto a contact/wall is a local conversion, checked before any
  // later friction can disguise an increase. Free-flight rounding is also
  // covered by the closed micro audit.
  if(contact!=0u||stop<65536u){int freeX,freeY;uvec4 oldEnergy,newEnergy,delta;
   if(!g9PositionAt(x0,vx0,gx,stop,1u,freeX)||!g9PositionAt(y0,vy0,ay,stop,2u,freeY)||!g9PacketEnergy(mass,uint(freeX),uint(freeY),freeVx,freeVy,oldEnergy)||!g9PacketEnergy(mass,uint(finalX),uint(finalY),finalVx,finalVy,newEnergy)||!g9SSub(newEnergy,oldEnergy,delta)){g9Fault(29u,slot,9u);return false;}
   if(!g9NonIncrease(g9Exact(delta),slot))return false;
  }
  g9PutPacket(slot,1u,uint(finalX));g9PutPacket(slot,2u,uint(finalY));g9PutPacket(slot,3u,uint(finalVx));g9PutPacket(slot,4u,uint(finalVy));g9PutPacket(slot,5u,column);g9PutPacket(slot,13u,pathStart);g9PutPacket(slot,14u,pathCount);
  // Return unused path reservation before jointly sealing the compound.
  g9Set(8u,pathStart+pathCount);
  uint row[64];g9Event(row,G9_EVENT_FLIGHT,slot,c,column);row[12]=g9Packet(slot,6u);row[13]=g9Packet(slot,7u);row[15]=mass;row[16]=g9Packet(slot,8u);row[17]=g9Packet(slot,9u);row[18]=g9Packet(slot,10u);row[19]=uint(x0);row[20]=uint(y0);row[21]=uint(vx0);row[22]=uint(vy0);row[23]=uint(finalX);row[24]=uint(finalY);row[25]=uint(finalVx);row[26]=uint(finalVy);row[27]=pathStart;row[28]=pathCount;row[29]=c;row[30]=column;row[31]=contact;row[32]=stop;row[33]=65536u-stop;row[34]=W[uColumnStage+column*64u+20u];row[35]=supportX.x;row[36]=supportX.y;row[37]=supportY.x;row[38]=supportY.y;
  if(!g9WriteEvent(eventStart,row)||!g9Seal())return false;

 return true;
}
// Each lane owns slot = batch*64+lane. Global paired cursors and environment
// ledgers serialize in lane order, with every lane crossing identical barriers.
// This conservative deterministic first implementation makes no speedup claim.
bool g9Flight(){
 uint lane=gl_LocalInvocationIndex;
 for(uint c=lane;c<128u;c+=64u)W[uColumnStage+c*64u+20u]=g9Material(c);
 memoryBarrierBuffer();barrier();
 for(uint batch=0u;batch<uK/64u;++batch)for(uint owner=0u;owner<64u;++owner){
  if(lane==owner&&g9Ctl(0u)==0u&&g9Ctl(14u)==0u)g9FlightPacket(batch*64u+lane);
  memoryBarrierBuffer();barrier();
 }
 return g9Ctl(0u)==0u&&g9Ctl(14u)==0u;
}
bool g9Fold(){
 for(uint c=0u;c<128u;++c)g9PutColumn(c,8u,0u);
 for(uint slot=0u;slot<uK;++slot){uint mass=g9Packet(slot,0u);if(mass==0u)continue;uint c=g9Packet(slot,1u)>>20u;
  if(c>=128u){g9Fault(24u,slot,1u);return false;}uint sum;if(!g9AddMass(g9Column(c,8u),mass,sum)){g9Reject(8u,slot,1u);return false;}g9PutColumn(c,8u,sum);g9PutPacket(slot,5u,c);
 }
 for(uint c=0u;c<128u;++c)if(g9Material(c)>268435456u-g9Column(c,8u)){g9Reject(8u,c,1u);return false;}
 return true;
}

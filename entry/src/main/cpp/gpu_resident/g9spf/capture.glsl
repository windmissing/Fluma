bool g9CaptureLess(uint a,uint b){
 uint ca=g9Packet(a,5u),cb=g9Packet(b,5u);if(ca!=cb)return ca<cb;
 int ba=int(g9Packet(a,2u))-int(g9Packet(a,0u)),bb=int(g9Packet(b,2u))-int(g9Packet(b,0u));if(ba!=bb)return ba<bb;return g9Packet(a,6u)<g9Packet(b,6u);
}
bool g9Capture(){
 // Deterministic order is independent of slot allocation and invocation order.
 for(uint slot=0u;slot<uK;++slot)W[uContacts+slot*16u+15u]=0u;
 for(uint ordinal=0u;ordinal<uK;++ordinal){
  uint best=uK;for(uint slot=0u;slot<uK;++slot)if(g9Packet(slot,0u)!=0u&&W[uContacts+slot*16u+9u]==4u&&W[uContacts+slot*16u+15u]==0u&&(best==uK||g9CaptureLess(slot,best)))best=slot;
  if(best==uK)break;uint slot=best,c=g9Packet(slot,5u),mass=g9Packet(slot,0u),y2=g9Packet(slot,2u),surface=2u*g9Material(c)+mass;W[uContacts+slot*16u+15u]=1u;
  if(y2>surface)continue; // SUPPORT_MOVED_DEFER: retained P, no teleport.
  if(y2<surface){g9Reject(9u,slot,0u);return false;}
  uvec3 key=uvec3(g9Packet(slot,8u),g9Packet(slot,9u),g9Packet(slot,10u));
  if(!g9MapCanAdd(true,c,key)||mass>268435456u-g9Column(c,0u)-g9Column(c,1u)){g9Reject(8u,c,0u);return false;}
  uint oldF=g9Column(c,1u),count=g9MapCount(true,c),id=g9Packet(slot,6u),generation=g9Packet(slot,7u),x=g9Packet(slot,1u);int vx=int(g9Packet(slot,3u)),vy=int(g9Packet(slot,4u));
  uvec4 oldP=g9Momentum(c),packetPx,packetPy,newP,packetEnergy;
  G9EnergyInterval beforeColumn,afterColumn,before,after,delta;
  if(!g9SMul(g9U32(mass),g9S32(vx),packetPx)||!g9SMul(g9U32(mass),g9S32(vy),packetPy)||!g9SAdd(oldP,packetPx,newP)||!g9ColumnEnergy(c,beforeColumn)||!g9PacketEnergy(mass,x,y2,vx,vy,packetEnergy)||!g9IntervalAdd(beforeColumn,g9Exact(packetEnergy),before)){g9Fault(29u,slot,0u);return false;}
  uint event,path;if(!g9Reserve(1u,0u,event,path))return false;
  if(!g9MapAdd(true,c,key,mass)){g9Fault(24u,c,0u);return false;}
  g9PutColumn(c,1u,oldF+mass);g9PutColumn(c,8u,g9Column(c,8u)-mass);g9PutColumn(c,5u,0u);g9PutColumn(c,13u,g9Column(c,13u)|2u);if(!g9PutMomentum(c,newP))return false;g9Derived(c);
  if(!g9ColumnEnergy(c,afterColumn)||!g9IntervalSubtract(afterColumn,before,delta)){g9Fault(29u,c,1u);return false;}
  if(!g9NonIncrease(delta,c))return false;
  // The FLIGHT stage has already accounted for normal impact; normally vy=0.
  // Any remaining actual normal impulse is accounted once, with its sign.
  uvec4 support;if(!g9SNeg(packetPy,support)||!g9Accumulate(78u,support)){g9Fault(29u,c,2u);return false;}
  uint row[64];g9Event(row,G9_EVENT_CAPTURE,slot,c,c);row[12]=id;row[13]=generation;row[14]|=4u;row[15]=mass;row[16]=key.x;row[17]=key.y;row[18]=key.z;row[19]=y2;row[20]=oldF;row[21]=oldF+mass;row[22]=oldP.x;row[23]=oldP.y;row[24]=newP.x;row[25]=newP.y;row[26]=packetPx.x;row[27]=packetPx.y;row[28]=packetPy.x;row[29]=packetPy.y;row[30]=count;row[31]=g9MapCount(true,c);row[32]=generation;row[34]=x;row[35]=c*1048576u+524288u;
  uvec4 projection,dx;if(!g9SSub(g9U32(row[35]),g9U32(x),dx)||!g9SMul(g9S32(int(C[22u])),dx,projection)||!g9SMul(projection,g9U32(mass),projection)||!g9SShift(projection,1u,projection)||!g9SNeg(projection,projection)){g9Fault(29u,c,3u);return false;}for(uint i=0u;i<4u;++i)row[36u+i]=projection[i];
  g9RetirePacket(slot);if(!g9WriteEvent(event,row)||!g9Seal())return false;
 }
 return true;
}

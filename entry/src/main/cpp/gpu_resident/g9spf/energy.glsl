bool g9IntervalAdd(G9EnergyInterval a,G9EnergyInterval b,out G9EnergyInterval result){return g9SAdd(a.low,b.low,result.low)&&g9SAdd(a.high,b.high,result.high);}
bool g9IntervalSubtract(G9EnergyInterval after,G9EnergyInterval before,out G9EnergyInterval result){return g9SSub(after.low,before.high,result.low)&&g9SSub(after.high,before.low,result.high);}
G9EnergyInterval g9Exact(uvec4 value){G9EnergyInterval result;result.low=result.high=value;return result;}
bool g9ColumnPotential(uint c,uint bed,uint flow,out uvec4 potential){
 uvec4 x=g9U32(c*1048576u+524288u),gx=g9S32(int(C[22u])),gn=g9U32(C[23u]);
 uvec4 total=g9U32(bed+flow),horizontal,vertical,inside;
 if(!g9SMul(gx,x,horizontal)||!g9SShift(horizontal,1u,horizontal)||!g9SMul(gn,total,vertical)||!g9SSub(vertical,horizontal,inside)||!g9SMul(total,inside,potential)||!g9SShift(potential,24u,potential))return false;
 return true; // U_bed+U_flow = (S+F)*(gn*(S+F)-2gx*x).
}
bool g9PacketEnergy(uint mass,uint x,uint y2,int vx,int vy,out uvec4 result){
 uvec4 xx,yy,kinetic,potential,horizontal;
 if(!g9SMul(g9S32(vx),g9S32(vx),xx)||!g9SMul(g9S32(vy),g9S32(vy),yy)||!g9SAdd(xx,yy,kinetic)||
    !g9SMul(g9U32(C[23u]),g9U32(y2),potential)||!g9SMul(g9S32(int(C[22u])),g9U32(x),horizontal)||!g9SShift(horizontal,1u,horizontal)||
    !g9SSub(potential,horizontal,potential)||!g9SAdd(kinetic,potential,result)||!g9SMul(result,g9U32(mass),result)||!g9SShift(result,24u,result))return false;
 return true;
}
bool g9ColumnEnergy(uint c,out G9EnergyInterval result){
 uvec4 potential;G9EnergyInterval kinetic;if(!g9ColumnPotential(c,g9Column(c,0u),g9Column(c,1u),potential)||!g9FlowKinetic(uvec2(g9Column(c,2u),g9Column(c,3u)),g9Column(c,1u),kinetic))return false;
 return g9IntervalAdd(kinetic,g9Exact(potential),result);
}
bool g9NonIncrease(G9EnergyInterval delta,uint c){
 if(g9SCmp(delta.high,uvec4(0u))<=0)return true;
 g9Reject(g9SCmp(delta.low,uvec4(0u))>0?16u:17u,c,0u);return false;
}
// Two 128-bit counters preserve positive work and cap separately. Negative
// touch work and recruitment loss never replenish the cap.
bool g9ChargeTouch(G9EnergyInterval delta){
 if(g9SCmp(delta.high,uvec4(0u))<=0)return true;
 uvec4 used=g9ReadWide(uControls+32u),cap=g9ReadWide(uControls+36u),next;
 if(!g9UAdd(used,delta.high,next)){g9Fault(29u,0u,32u);return false;}
 if(g9UCmp(next,cap)>0)return false;g9WriteWide(uControls+32u,next);return true;
}
bool g9SnapshotMicro(){
 for(uint c=0u;c<128u;++c){
  uint at=uWideLedger+c*18u;W[at]=g9Column(c,0u);W[at+1u]=g9Column(c,1u);W[at+2u]=g9Column(c,2u);W[at+3u]=g9Column(c,3u);
  G9EnergyInterval before;if(!g9ColumnEnergy(c,before)){g9Fault(29u,c,0u);return false;}g9WriteWide(at+4u,before.low);g9WriteWide(at+8u,before.high);
 }
 for(uint slot=0u;slot<uK;++slot){uint at=uContacts+slot*16u,mass=g9Packet(slot,0u);
  for(uint f=0u;f<5u;++f)W[at+f]=g9Packet(slot,f);
  uvec4 energy=uvec4(0u);if(mass!=0u&&!g9PacketEnergy(mass,g9Packet(slot,1u),g9Packet(slot,2u),int(g9Packet(slot,3u)),int(g9Packet(slot,4u)),energy)){g9Fault(29u,slot,0u);return false;}g9WriteWide(at+5u,energy);
 }
 return true;
}
bool g9AuditMicroEnergy(){
 G9EnergyInterval delta=g9Exact(uvec4(0u));
 for(uint c=0u;c<128u;++c){uint at=uWideLedger+c*18u;
  bool same=W[at]==g9Column(c,0u)&&W[at+1u]==g9Column(c,1u)&&W[at+2u]==g9Column(c,2u)&&W[at+3u]==g9Column(c,3u);
  if(same)continue;G9EnergyInterval before,after,change;before.low=g9ReadWide(at+4u);before.high=g9ReadWide(at+8u);
  // The rational kinetic term cancels exactly when its operands are unchanged,
  // even if S changed. Do not manufacture a one-unit interval for a no-op.
  if(W[at+1u]==g9Column(c,1u)&&W[at+2u]==g9Column(c,2u)&&W[at+3u]==g9Column(c,3u)){
   uvec4 oldPotential,newPotential,exact;
   if(!g9ColumnPotential(c,W[at],W[at+1u],oldPotential)||!g9ColumnPotential(c,g9Column(c,0u),g9Column(c,1u),newPotential)||!g9SSub(newPotential,oldPotential,exact)){g9Fault(29u,c,1u);return false;}change=g9Exact(exact);
  }else if(!g9ColumnEnergy(c,after)||!g9IntervalSubtract(after,before,change)){g9Fault(29u,c,2u);return false;}
  if(!g9IntervalAdd(delta,change,delta)){g9Fault(29u,c,3u);return false;}
 }
 for(uint slot=0u;slot<uK;++slot){uint at=uContacts+slot*16u;bool same=true;for(uint f=0u;f<5u;++f)if(W[at+f]!=g9Packet(slot,f))same=false;
  if(same)continue;uvec4 after=uvec4(0u),change;uint mass=g9Packet(slot,0u);
  if(mass!=0u&&!g9PacketEnergy(mass,g9Packet(slot,1u),g9Packet(slot,2u),int(g9Packet(slot,3u)),int(g9Packet(slot,4u)),after)){g9Fault(29u,slot,4u);return false;}
  if(!g9SSub(after,g9ReadWide(at+5u),change)||!g9IntervalAdd(delta,g9Exact(change),delta)){g9Fault(29u,slot,5u);return false;}
 }
 if(!g9NonIncrease(delta,0u))return false;
 uvec4 dissipated;if(!g9SNeg(delta.high,dissipated)||!g9Accumulate(94u,dissipated)){g9Fault(29u,0u,94u);return false;}
 g9WriteWide(uControls+40u,delta.high);return true;
}

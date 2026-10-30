void resolvePacket(uint ns,uint s){
 uint c=C[at(ns,1u,s)]>>20u,tau=0u,lastPlane=0xffffffffu;uint old[16];before(ns,s,old);uint y=old[2],m=classify(ns,c,y,int(old[4]));
 if((tag(ns,s)&15u)!=1u&&((tag(ns,s)&15u)!=2u||y>(ns==1u?W[uFrozen+c]:col(10u,c)))){state(ns,s,1u,0u,m);emit(12u,ns,s,c,c,old,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u);}
 else if(m!=medium(ns,s)){state(ns,s,1u,0u,m);emit(6u,ns,s,c,c,old,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,1024u+c,0u);}
 uint angle=uAngles+uint(int(W[4])+7680)*4u;int ax=wrne(wp(int(W[angle+1u]),uGravity),Q),ay=-wrne(wp(int(W[angle+2u]),uGravity),Q),tx=wrne(wp(int(W[angle+1u]),uSettle),Q),ty=-wrne(wp(int(W[angle+2u]),uSettle),Q);
 for(uint iteration=0u;iteration<49u&&tau<TICKS;iteration++){
  if(W[0]!=0u)return;before(ns,s,old);int x=int(old[1]);y=old[2];int vx=int(old[3]),vy=int(old[4]);m=medium(ns,s);bool wet=m==1u;uint k=ns==1u?uKFlight:wet?uKWet:uKAir,r=TICKS-tau;
  uint turnX=turnTime(vx,r,k,ax,tx,wet),turnY=turnTime(vy,r,k,ay,ty,wet),seg=min(turnX,turnY);int ex=velocity(vx,seg,k,ax,tx,wet),ey=velocity(vy,seg,k,ay,ty,wet),dx=displacement(vx,ex,seg),dy=displacement(vy,ey,seg);
  uint num=1u,den=1u,hit=seg<r?5u:6u,turnMask=(turnX==seg?1u:0u)|(turnY==seg?2u:0u);int boundary=dx<0?int(c*Q):int((c+1u)*Q);
  if(y<=col(10u,c))plane(0,1u,0u,num,den,hit);else if(dy<0)plane(int(y-col(10u,c)),uint(-dy),0u,num,den,hit);
  if(dy>0)plane(int(CQ-y),uint(dy),1u,num,den,hit);
  if(dx!=0){bool wall=(dx<0&&c==0u)||(dx>0&&c==127u);plane(dx<0?x-boundary:boundary-x,uint(abs(dx)),wall?1u:2u,num,den,hit);}
  if(ns==1u){if(y<=W[uFrozen+c])plane(0,1u,3u,num,den,hit);else if(dy<0)plane(int(y-W[uFrozen+c]),uint(-dy),3u,num,den,hit);}
  else if(W[uFrozen+128u+c]!=0u){if(!wet&&dy<0&&y>=W[uFrozen+c])plane(int(y-W[uFrozen+c]),uint(-dy),3u,num,den,hit);if(wet&&dy>0&&y<=W[uFrozen+c])plane(int(W[uFrozen+c]-y),uint(dy),3u,num,den,hit);}
  uint ticks=tickCeil(seg,num,den),nextTau=tau+ticks,planeTag=(hit+1u)*256u+(hit==2u||hit==1u?uint(boundary)/Q:c);
  if(ticks==0u&&lastPlane==planeTag){qfail(17u);return;}lastPlane=ticks==0u?planeTag:0xffffffffu;
  int nx=point(x,dx,num,den),ny=point(int(y),dy,num,den),nvx=velocity(vx,ticks,k,ax,tx,wet),nvy=velocity(vy,ticks,k,ay,ty,wet);uint kind=ns==0u?3u:4u,to=c,tf=C[uHeader+54u],lf=col(15u,c),lt=lf;uint a0=0u,a1=0u,b0=0u,b1=0u;
  bool wait=false,landNow=false;
  if(hit==0u){ny=int(col(10u,c));nvy=0;wait=true;landNow=ns==0u;state(ns,s,2u,1u,0u);kind=7u;}
  else if(hit==1u){bool ceiling=dy>0&&ratioEqual(CQ-y,uint(dy),num,den);if(ceiling){planeTag=512u+129u;ny=int(CQ);nvy=0;}else{nx=dx<0?0:int(128u*Q-1u);nvx=0;}wait=true;kind=7u;state(ns,s,3u,5u,m);}
  else if(hit==2u){to=dx<0?c-1u:c+1u;lt=col(15u,to);uint mass=old[0];bool high=ny<=int(col(10u,to)),capacity=mass>CQ-occupied(to),budget=ns==0u&&(mass>tf||mass>lf||mass>lt);
   if(high||capacity||budget){if(high)planeTag=256u+to;to=c;nx=dx<0?boundary:boundary-1;nvx=0;wait=true;kind=7u;state(ns,s,3u,high?3u:capacity?2u:4u,m);}
   else{uint f=ns==0u?8u:7u;a0=col(f,c);b0=col(f,to);if(a0<mass){qfail(15u);return;}pc(f,c,a0-mass);pc(f,to,b0+mass);a1=a0-mass;b1=b0+mass;if(ns==0u)charge(mass,c,to,true);derived(c);derived(to);nx=dx<0?boundary-1:boundary;kind=5u;state(ns,s,1u,0u,classify(ns,to,uint(ny),nvy));}
  }else if(hit==3u){ny=ns==1u?min(int(y),int(W[uFrozen+c])):int(W[uFrozen+c]);if(ns==1u){wait=true;nvy=0;kind=7u;state(ns,s,2u,6u,0u);}else{if(wet)ny++;state(ns,s,1u,0u,wet?0u:1u);kind=6u;}}
  else if(hit==5u){kind=14u;state(ns,s,1u,0u,m);}
  else state(ns,s,1u,0u,m);
  if(nx<0||nx>=int(128u*Q)||ny<0||ny>int(CQ)){qfail(15u);return;}C[at(ns,1u,s)]=uint(nx);C[at(ns,2u,s)]=uint(ny);C[at(ns,3u,s)]=uint(nvx);C[at(ns,4u,s)]=uint(nvy);
  if(ns==0u)C[at(0u,14u,s)]=classify(0u,to,uint(ny),nvy);
  emit(kind,ns,s,c,to,old,a0,a1,b0,b1,tf,lf,lt,num,den,tau,wait?TICKS:nextTau,seg,planeTag,hit==5u?turnMask:0u);c=to;tau=wait?TICKS:nextTau;if(landNow)land(s,c);if(C[at(ns,0u,s)]==0u)return;
 }
 if(tau<TICKS)qfail(12u);
}
void resolvePool(uint ns){uint count=ns==0u?uPad:uKw,offset=ns==0u?uSandIndex:uWaterIndex;for(uint i=0u;i<count;i++){uint s=W[offset+i];if(s>=slots(ns)||C[at(ns,0u,s)]==0u)continue;resolvePacket(ns,s);if(W[0]!=0u)return;}}

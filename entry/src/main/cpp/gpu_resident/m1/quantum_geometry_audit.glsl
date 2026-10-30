// Verify the logged segment's selection, not just its arithmetic endpoint.
bool replayGeometry(uint row,uint ns,uint from,uint old[16]){
 uint r=TICKS-ev(row,56u),m=ev(row,58u),k=ns==1u?uKFlight:m==1u?uKWet:uKAir,a=uAngles+uint(int(W[4])+7680)*4u;bool wet=m==1u;int ax=wrne(wp(int(W[a+1u]),uGravity),Q),ay=-wrne(wp(int(W[a+2u]),uGravity),Q),tx=wrne(wp(int(W[a+1u]),uSettle),Q),ty=-wrne(wp(int(W[a+2u]),uSettle),Q),vx=int(old[3]),vy=int(old[4]);uint turnX=turnTime(vx,r,k,ax,tx,wet),turnY=turnTime(vy,r,k,ay,ty,wet),seg=min(turnX,turnY);if(!requireQ(ev(row,63u)==seg,16u))return false;
 int dx=displacement(vx,velocity(vx,seg,k,ax,tx,wet),seg),dy=displacement(vy,velocity(vy,seg,k,ay,ty,wet),seg),x=int(old[1]);uint y=old[2],num=1u,den=1u,hit=seg<r?5u:6u;int boundary=dx<0?int(from*Q):int((from+1u)*Q);
 if(y<=rc(10u,from))plane(0,1u,0u,num,den,hit);else if(dy<0)plane(int(y-rc(10u,from)),uint(-dy),0u,num,den,hit);if(dy>0)plane(int(CQ-y),uint(dy),1u,num,den,hit);
 if(dx!=0){bool wall=(dx<0&&from==0u)||(dx>0&&from==127u);plane(dx<0?x-boundary:boundary-x,uint(abs(dx)),wall?1u:2u,num,den,hit);}
 uint etaR=W[uReplayFrozen+from];if(ns==1u){if(y<=etaR)plane(0,1u,3u,num,den,hit);else if(dy<0)plane(int(y-etaR),uint(-dy),3u,num,den,hit);}else if(W[uReplayFrozen+128u+from]!=0u){if(!wet&&dy<0&&y>=etaR)plane(int(y-etaR),uint(-dy),3u,num,den,hit);if(wet&&dy>0&&y<=etaR)plane(int(etaR-y),uint(dy),3u,num,den,hit);}
 if(!requireQ(ev(row,44u)==num&&ev(row,46u)==den,16u))return false;uint planeTag=(hit+1u)*256u+(hit==2u||hit==1u?uint(boundary)/Q:from),kind=ns==0u?3u:4u,to=from,state=1u,wait=0u,turnMask=0u;int nx=point(x,dx,num,den),ny=point(int(y),dy,num,den);
 if(hit==0u){kind=7u;state=2u;wait=1u;ny=int(rc(10u,from));}
 else if(hit==1u){bool ceiling=dy>0&&ratioEqual(CQ-y,uint(dy),num,den);kind=7u;state=3u;wait=5u;if(ceiling){planeTag=641u;ny=int(CQ);}else nx=dx<0?0:int(128u*Q-1u);}
 else if(hit==2u){to=dx<0?from-1u:from+1u;bool high=ny<=int(rc(10u,to)),capacity=old[0]>CQ-rc(13u,to),budget=ns==0u&&(old[0]>rr(uHeader+54u)||old[0]>rc(15u,from)||old[0]>rc(15u,to));if(high||capacity||budget){if(high)planeTag=256u+to;to=from;nx=dx<0?boundary:boundary-1;kind=7u;state=3u;wait=high?3u:capacity?2u:4u;}else{kind=5u;nx=dx<0?boundary-1:boundary;}}
 else if(hit==3u){ny=ns==1u?min(int(y),int(etaR)):int(etaR);if(ns==1u){kind=7u;state=2u;wait=6u;}else{kind=6u;if(wet)ny++;}}
 else if(hit==5u){kind=14u;turnMask=(turnX==seg?1u:0u)|(turnY==seg?2u:0u);}
 uint flags=ev(row,52u);return requireQ(ev(row,6u)==kind&&ev(row,21u)==to&&ev(row,26u)==uint(nx)&&ev(row,27u)==uint(ny)&&ev(row,62u)==planeTag&&((flags>>4u)&15u)==state&&((flags>>14u)&15u)==wait&&((flags>>20u)&3u)==turnMask,16u);
}

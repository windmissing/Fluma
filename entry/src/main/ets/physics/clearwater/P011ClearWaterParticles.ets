// T209/T196: one ascending-ID trajectory pass, bounded chord events, no merge.
import { ClearWaterState } from './P011ClearWaterState';
import { ClearWaterRunConfig,ClearWaterStatus as S } from './P011ClearWaterTypes';
import { ClearWaterBConfig,BReason,BKind,bFlags } from './P011ClearWaterBTypes';
import { ClearWaterExactMass } from './P011ClearWaterExactMass';
import { ClearWaterBGuard } from './P011ClearWaterGuard';
import { ClearWaterBArena } from './P011ClearWaterEvents';
export class ClearWaterBParticles {
  reason:BReason=BReason.NONE;private vx:number=0;private vy:number=0;
  private velocity(h:number,wet:number,vx:number,vy:number,config:ClearWaterRunConfig,b:ClearWaterBConfig,g:ClearWaterBGuard):boolean {
    if(wet===0){const decay=Math.exp(-b.dragAir*h);this.vx=vx*decay+b.gModel*g.scalars[9]*h;this.vy=vy-b.gModel*g.scalars[10]*h;}
    else {const tx=config.settlingSpeed*g.scalars[9],ty=-config.settlingSpeed*g.scalars[10],decay=Math.exp(-config.dragWet*h);this.vx=tx+(vx-tx)*decay;this.vy=ty+(vy-ty)*decay;}
    return Number.isFinite(this.vx)&&Number.isFinite(this.vy);
  }
  private medium(s:ClearWaterState,c:number,y:number,eta:Float64Array,vy:number):number {return s.W[c]>0&&y>s.S[c]&&(y<eta[c]||y===eta[c]&&vy<=0)?1:0;}
  private record(s:ClearWaterState,i:number,kind:number,nx:number,ny:number,nvx:number,nvy:number,ns:number,nw:number,om:number,nm:number,
    r0:number,r1:number,h:number,lambda:number,plane:number,coordinate:number,to:number,reason:number,a:ClearWaterBArena,b:ClearWaterBConfig,counters:Float64Array):S {
    const p=s.pool,c=Math.floor(p.pX[i]);
    if(!a.reserveInto(kind,i,p.pId[i],s.originSourceCommit![i],c,to,1,b.e))return S.EVENT_FULL;
    a.particleBefore(s,i);a.set(15,nx);a.set(16,ny);a.set(17,nvx);a.set(18,nvy);a.set(19,r0);a.set(20,r1);a.set(21,h);a.set(22,lambda);a.set(23,plane);a.set(24,coordinate);a.set(25,om);a.set(26,nm);
    a.set(39,bFlags(p.pState[i],ns,s.waitReason![i],nw,om,nm,reason));
    if(!a.commitRowInto())return this.fail(a.reason);
    p.pX[i]=nx;p.pY[i]=ny;p.pVx[i]=nvx;p.pVy[i]=nvy;p.pState[i]=ns;s.waitReason![i]=nw;counters[17+kind]++;
    return S.OK;
  }
  unlink(s:ClearWaterState,i:number,counters:Float64Array):void {
    const p=s.pool,left=p.prev[i],right=p.next[i];if(left===-1)p.head=right;else p.next[left]=right;if(right===-1)p.tail=left;else p.prev[right]=left;
    p.prev[i]=p.next[i]=-1;p.pMass[i]=0;p.pState[i]=0;p.freeSlots[p.freeCount++]=i;p.liveCount--;s.originSourceCommit![i]=0;s.waitReason![i]=0;counters[33]++;
  }
  private land(s:ClearWaterState,i:number,g:ClearWaterBGuard,m:ClearWaterExactMass,a:ClearWaterBArena,b:ClearWaterBConfig,counters:Float64Array):S {
    const p=s.pool,c=Math.floor(p.pX[i]),mass=p.pMass[i],q=g.admitPatchInto(s,mass,c,-1,1,0);if(q<=0){counters[37]++;return S.OK;}
    const newM=q===mass?0:mass-q,newMh=s.mh[c]+q,newS=(((s.bh[c]+0)+newMh)+0);
    if(!Number.isFinite(newMh)||newM<0||Math.abs(((newS-s.S[c])+(newM-mass)))>1e-6)return this.fail(BReason.LOCAL_NUMERIC_INVALID);
    if(newM===mass||newMh===s.mh[c]){counters[35]++;return S.OK;}
    if(!g.actual(s,c,-1,newS,0)){counters[35]++;return S.OK;}
    if(!m.fitsPatchInto(c,newS,s.W[c],mass,newM)){if(m.reason!==BReason.NONE)return this.fail(m.reason);counters[36]++;return S.OK;}
    if(!a.reserveInto(BKind.LAND,i,p.pId[i],s.originSourceCommit![i],c,c,2,b.e))return S.EVENT_FULL;
    a.particleBefore(s,i);a.set(7,mass);a.set(8,q);a.set(10,newM);a.set(15,p.pX[i]);a.set(16,newM===0?p.pY[i]:newS);a.set(17,p.pVx[i]);a.set(18,newM===0?p.pVy[i]:0);
    a.set(23,s.bh[c]);a.set(24,s.bh[c]);a.set(25,s.mh[c]);a.set(26,newMh);g.writeBudget(a,q,c,-1);a.set(39,bFlags(p.pState[i],newM===0?0:2,s.waitReason![i],newM===0?0:1,0,0,0));
    if(!a.commitRowInto())return this.fail(a.reason);
    if(!m.applyMassInto(c,mass,newM))return this.fail(m.reason);
    s.mh[c]=newMh;s.S[c]=newS;g.debit(q,c,-1,mass);counters[22]++;
    if(newM===0)this.unlink(s,i,counters);else{p.pMass[i]=newM;p.pY[i]=newS;p.pVy[i]=0;p.pState[i]=2;s.waitReason![i]=1;}return S.OK;
  }
  advanceInto(s:ClearWaterState,config:ClearWaterRunConfig,b:ClearWaterBConfig,dt:number,eta:Float64Array,g:ClearWaterBGuard,m:ClearWaterExactMass,a:ClearWaterBArena,counters:Float64Array):S {
    let i=s.pool.head;
    while(i!==-1){const next=s.pool.next[i];counters[13]++;const result=this.one(s,i,config,b,dt,eta,g,m,a,counters);if(result!==S.OK)return result;i=next;}return S.OK;
  }
  private one(s:ClearWaterState,i:number,config:ClearWaterRunConfig,b:ClearWaterBConfig,dt:number,eta:Float64Array,g:ClearWaterBGuard,m:ClearWaterExactMass,a:ClearWaterBArena,counters:Float64Array):S {
    const p=s.pool;let c=Math.floor(p.pX[i]),remaining=dt,wet=this.medium(s,c,p.pY[i],eta,p.pVy[i]),lastZeroPlane=0,lastZeroCoordinate=NaN;
    if(p.pY[i]<=s.S[c]){
      if(p.pState[i]!==2||s.waitReason![i]!==1||p.pY[i]!==s.S[c]||p.pVy[i]!==0){const result=this.record(s,i,BKind.CONTACT,p.pX[i],s.S[c],p.pVx[i],0,2,1,wet,0,remaining,remaining,0,0,1,s.S[c],c,0,a,b,counters);if(result!==S.OK)return result;}
      if(a.summary[i*16+14]>=b.e-1)return this.fail(BReason.EVENT_LIMIT,S.EVENT_FULL);return this.land(s,i,g,m,a,b,counters);
    }
    if(p.pState[i]===2&&s.waitReason![i]===1){const result=this.record(s,i,BKind.CONTACT,p.pX[i],p.pY[i],p.pVx[i],p.pVy[i],1,0,wet,wet,remaining,remaining,0,0,0,0,c,0,a,b,counters);if(result!==S.OK)return result;}
    while(remaining>0){
      if(a.summary[i*16+14]>=b.e-1)return this.fail(BReason.EVENT_LIMIT,S.EVENT_FULL);
      c=Math.floor(p.pX[i]);const x=p.pX[i],y=p.pY[i],vx=p.pVx[i],vy=p.pVy[i];
      if(!this.velocity(remaining,wet,vx,vy,config,b,g))return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      const targetX=x+this.vx*remaining,targetY=y+this.vy*remaining,dx=targetX-x,dy=targetY-y;
      if(!Number.isFinite(targetX)||!Number.isFinite(targetY)||!Number.isFinite(dx)||!Number.isFinite(dy))return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      let lambda=1,plane=0,coordinate=0,rank=99;
      if(dy<0&&targetY<=s.S[c]){const v=(s.S[c]-y)/dy;if(v>=0&&v<=lambda){lambda=v;plane=1;coordinate=s.S[c];rank=1;}}
      if(dy>0&&targetY>=256){const v=(256-y)/dy;if(v>=0&&(v<lambda||v===lambda&&2<rank)){lambda=v;plane=2;coordinate=256;rank=2;}}
      if(dx!==0){const edge=dx>0?c+1:c,v=(edge-x)/dx,type=edge===0||edge===128?2:3,priority=type===2?2:3;
        if(v>=0&&v<=1&&(v<lambda||v===lambda&&priority<rank)){lambda=v;plane=type;coordinate=edge;rank=priority;}}
      if(s.W[c]>0&&dy!==0){const v=(eta[c]-y)/dy;
        // Only an outward crossing of the current medium is admissible at t=0.
        const direction=wet===1?dy>0:dy<0;
        if(direction&&v>=0&&v<=1&&(v<lambda||v===lambda&&4<rank)){lambda=v;plane=4;coordinate=eta[c];rank=4;}}
      const h=remaining*lambda,newRemaining=remaining-h;
      if(h===0&&plane===lastZeroPlane&&coordinate===lastZeroCoordinate)return this.fail(BReason.STEP_TIME_NO_PROGRESS);
      if(!Number.isFinite(h)||!Number.isFinite(newRemaining)||h<0||newRemaining<0||!this.velocity(h,wet,vx,vy,config,b,g))return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      let nx=x+dx*lambda,ny=y+dy*lambda,nvx=this.vx,nvy=this.vy,ns=1,nw=0,nm=wet,kind=BKind.MOVE,to=c,reason=0;
      if(plane===1){ny=s.S[c];nvy=0;ns=2;nw=1;kind=BKind.CONTACT;}
      else if(plane===2){if(coordinate===256){ny=256;nvy=0;}else{nx=coordinate===128?m.nextDown(128):0;nvx=0;}ns=2;nw=2;kind=BKind.CONTACT;reason=BReason.CAPACITY_NO_GRANT;}
      else if(plane===4){ny=coordinate;nm=wet===0?1:0;kind=BKind.MEDIUM;}
      else if(plane===3){
        to=dx>0?c+1:c-1;const mass=p.pMass[i];let block=0;
        if(ny<=s.S[to])block=4;
        else if(g.budget(mass,c,to)<mass)block=3;
        else if(!m.fitsPatchInto(c,s.S[c],s.W[c],mass,0)||!m.fitsPatchInto(to,s.S[to],s.W[to],0,mass)){if(m.reason!==BReason.NONE)return this.fail(m.reason);block=2;}
        if(block!==0){nx=dx>0?m.nextDown(c+1):c;nvx=0;ns=2;nw=block;to=c;kind=BKind.CONTACT;reason=block===4?BReason.HIGH_BED_WAIT:block===3?(mass>28*dt?BReason.BUDGET_QUANTUM_BLOCKED:BReason.BUDGET_NO_GRANT):BReason.CAPACITY_NO_GRANT;counters[block===4?38:block===3?37:36]++;}
        else {nx=dx>0?c+1:m.nextDown(c);kind=BKind.CROSS;nm=this.medium(s,to,ny,eta,nvy);}
      }
      const topology=kind===BKind.MEDIUM||kind===BKind.CROSS||kind===BKind.CONTACT;
      if(newRemaining===remaining&&(!topology||kind===BKind.MEDIUM&&nm===wet))return this.fail(BReason.STEP_TIME_NO_PROGRESS);
      if(nx<0||nx>=128||ny<0||ny>256)return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      if(kind===BKind.CROSS){
        if(!a.reserveInto(kind,i,p.pId[i],s.originSourceCommit![i],c,to,1,b.e))return S.EVENT_FULL;
        a.particleBefore(s,i);a.set(7,p.pMass[i]);a.set(8,p.pMass[i]);a.set(15,nx);a.set(16,ny);a.set(17,nvx);a.set(18,nvy);a.set(19,remaining);a.set(20,newRemaining);a.set(21,h);a.set(22,lambda);a.set(23,plane);a.set(24,coordinate);a.set(25,wet);a.set(26,nm);a.set(39,bFlags(p.pState[i],ns,s.waitReason![i],nw,wet,nm,reason));g.writeBudget(a,p.pMass[i],c,to);
        if(!a.commitRowInto())return this.fail(a.reason);
        if(!m.applyMassInto(c,p.pMass[i],0)||!m.applyMassInto(to,0,p.pMass[i]))return this.fail(m.reason);g.debit(p.pMass[i],c,to,p.pMass[i]);
        p.pX[i]=nx;p.pY[i]=ny;p.pVx[i]=nvx;p.pVy[i]=nvy;p.pState[i]=ns;s.waitReason![i]=nw;counters[20]++;
      }else{const result=this.record(s,i,kind,nx,ny,nvx,nvy,ns,nw,wet,nm,remaining,newRemaining,h,lambda,plane,coordinate,to,reason,a,b,counters);if(result!==S.OK)return result;}
      const plan=i*16;a.summary[plan+11]+=h;a.summary[plan+12]=newRemaining;
      remaining=newRemaining;wet=nm;
      lastZeroPlane=h===0?plane:0;lastZeroCoordinate=coordinate;
      if(kind===BKind.CONTACT){if(nw===1){if(a.summary[plan+14]>=b.e-1)return this.fail(BReason.EVENT_LIMIT,S.EVENT_FULL);return this.land(s,i,g,m,a,b,counters);}return S.OK;}
    }return S.OK;
  }
  reconcileGeometryAndFoldInto(s:ClearWaterState,b:ClearWaterBConfig,a:ClearWaterBArena,counters:Float64Array,eta:Float64Array):S {
    const p=s.pool;s.P.fill(0);p.colParticleMassH.fill(0);p.colParticleMassL.fill(0);let i=p.head;
    while(i!==-1){counters[14]++;const c=Math.floor(p.pX[i]),buried=p.pY[i]<s.S[c],unsupported=p.pState[i]===2&&s.waitReason![i]===1&&p.pY[i]>s.S[c];
      if(buried||unsupported){const wet=this.medium(s,c,p.pY[i],eta,p.pVy[i]),result=this.record(s,i,BKind.CONTACT,p.pX[i],buried?s.S[c]:p.pY[i],p.pVx[i],buried?0:p.pVy[i],buried?2:1,buried?1:0,wet,wet,0,0,0,0,buried?1:0,buried?s.S[c]:0,c,0,a,b,counters);if(result!==S.OK)return result;}
      s.P[c]+=p.pMass[i];p.colParticleMassH[c]=s.P[c];const o=i*16;a.summary[o]=p.pId[i];a.summary[o+1]=i;a.summary[o+2]=s.originSourceCommit![i];a.summary[o+3]=p.pState[i];a.summary[o+4]=s.waitReason![i];a.summary[o+5]=c;a.summary[o+6]=p.pMass[i];a.summary[o+7]=p.pX[i];a.summary[o+8]=p.pY[i];a.summary[o+9]=p.pVx[i];a.summary[o+10]=p.pVy[i];i=p.next[i];
    }
    for(let c=0;c<128;c++){s.S[c]=(((s.bh[c]+0)+s.mh[c])+0);s.eta[c]=s.S[c]+s.W[c];s.R[c]=s.eta[c]+s.P[c];}counters[34]++;return S.OK;
  }
  private fail(reason:BReason,status:S=S.NUMERIC_UNREPRESENTABLE):S {this.reason=reason;return status;}
}

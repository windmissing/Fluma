import { ClearWaterState } from './P011ClearWaterState';
import { ClearWaterRunConfig,ClearWaterStatus as S,CW_HARD_MAX } from './P011ClearWaterTypes';
import { ClearWaterBConfig,BReason,BKind } from './P011ClearWaterBTypes';
import { ClearWaterExactMass } from './P011ClearWaterExactMass';
import { ClearWaterBGuard } from './P011ClearWaterGuard';
import { ClearWaterBArena } from './P011ClearWaterEvents';
export class ClearWaterBHydraulics {
  reason:BReason=BReason.NONE;
  advanceWaterInto(s:ClearWaterState,config:ClearWaterRunConfig,dt:number,serial:number,g:ClearWaterBGuard,m:ClearWaterExactMass,a:ClearWaterBArena,counters:Float64Array):S {
    for(let pass=0;pass<2;pass++)for(let edge=(serial+pass)%2;edge<127;edge+=2){
      counters[16]++;const u=g.scalars[8],h0=s.S[edge]+s.W[edge]-(edge+.5)*u,h1=s.S[edge+1]+s.W[edge+1]-(edge+1.5)*u;
      if(!Number.isFinite(h0)||!Number.isFinite(h1))return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      if(h0===h1)continue;const d=h0>h1?edge:edge+1,r=d===edge?edge+1:edge,hd=d===edge?h0:h1,hr=d===edge?h1:h0;
      const delta=hd-hr,flow=config.kappaW*dt*delta,zd=s.S[d]-(d+.5)*u,zr=s.S[r]-(r+.5)*u,over=hd-Math.max(zd,zr),free=256-s.S[r]-s.P[r]-s.W[r];
      if(!Number.isFinite(delta)||!Number.isFinite(flow)||!Number.isFinite(over)||!Number.isFinite(free))return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      const q=Math.min(flow,delta/2,s.W[d],Math.max(0,over),Math.max(0,free));if(q<=0)continue;
      const wd=s.W[d]-q,wr=s.W[r]+q;if(wd===s.W[d]||wr===s.W[r]){counters[35]++;continue;}
      if(!Number.isFinite(wd)||!Number.isFinite(wr)||wd<0||Math.abs(((wd-s.W[d])+(wr-s.W[r])))>1e-6)return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      if(!m.fitsPatchInto(d,s.S[d],wd,0,0)||!m.fitsPatchInto(r,s.S[r],wr,0,0)){if(m.reason!==BReason.NONE)return this.fail(m.reason);counters[36]++;continue;}
      if(!a.reserveInto(BKind.WATER,-1,0,0,d,r,0,0))return S.EVENT_FULL;
      a.set(7,q);a.set(8,q);a.set(27,s.W[d]);a.set(28,wd);a.set(29,s.W[r]);a.set(30,wr);
      if(!a.commitRowInto())return this.fail(a.reason);s.W[d]=wd;s.W[r]=wr;counters[24]++;
    }return S.OK;
  }
  advanceBedInto(s:ClearWaterState,config:ClearWaterBConfig,dt:number,serial:number,g:ClearWaterBGuard,m:ClearWaterExactMass,a:ClearWaterBArena,counters:Float64Array):S {
    for(let pass=0;pass<2;pass++)for(let edge=(serial+pass)%2;edge<127;edge+=2){
      counters[17]++;const residual=(s.S[edge+1]-s.S[edge])-g.scalars[8],rate=config.maxAvalancheFlux*dt;
      if(!Number.isFinite(residual)||!Number.isFinite(rate))return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      if(Math.abs(residual)<=CW_HARD_MAX)continue;
      const d=residual>0?edge+1:edge,r=d===edge?edge+1:edge,request=Math.min((Math.abs(residual)-CW_HARD_MAX)/2,s.mh[d]+s.bh[d],rate);
      if(request<=0)continue;const q=g.admitPatchInto(s,request,d,r,-1,1);if(q<=0){counters[37]++;continue;}
      const fm=Math.min(q,s.mh[d]),fb=q-fm,md=s.mh[d]-fm,bd=s.bh[d]-fb,h=fm+fb,mr=s.mh[r]+h,sd=(((bd+0)+md)+0),sr=(((s.bh[r]+0)+mr)+0);
      if(fb>s.bh[d]||!Number.isFinite(mr)||md<0||bd<0||Math.abs(((sd-s.S[d])+(sr-s.S[r])))>1e-6)return this.fail(BReason.LOCAL_NUMERIC_INVALID);
      if(sd===s.S[d]||sr===s.S[r]){counters[35]++;continue;}
      if(!g.actual(s,d,r,sd,sr)){counters[35]++;continue;}
      if(!m.fitsPatchInto(d,sd,s.W[d],0,0)||!m.fitsPatchInto(r,sr,s.W[r],0,0)){if(m.reason!==BReason.NONE)return this.fail(m.reason);counters[36]++;continue;}
      if(!a.reserveInto(BKind.BED,-1,0,0,d,r,0,0))return S.EVENT_FULL;
      a.set(7,request);a.set(8,q);a.set(9,fm);a.set(10,fb);a.set(19,s.bh[d]);a.set(20,bd);a.set(21,s.mh[d]);a.set(22,md);a.set(23,s.bh[r]);a.set(24,s.bh[r]);a.set(25,s.mh[r]);a.set(26,mr);g.writeBudget(a,q,d,r);
      if(!a.commitRowInto())return this.fail(a.reason);s.bh[d]=bd;s.mh[d]=md;s.mh[r]=mr;s.S[d]=sd;s.S[r]=sr;g.debit(q,d,r,request);counters[23]++;
    }return S.OK;
  }
  private fail(reason:BReason):S {this.reason=reason;return S.NUMERIC_UNREPRESENTABLE;}
}

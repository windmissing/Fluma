import { p011GuardAdmitNumericInto,P011GuardNumericOutcome } from '../P011MaterialGuard';
import { ClearWaterState } from './P011ClearWaterState';
import { CW_HARD_MAX } from './P011ClearWaterTypes';
import { BReason } from './P011ClearWaterBTypes';
import { ClearWaterBArena } from './P011ClearWaterEvents';
export class ClearWaterBGuard {
  readonly workLocalRemaining:Float64Array=new Float64Array(128);readonly workLocalUsed:Float64Array=new Float64Array(128);
  readonly replayLocalRemaining:Float64Array=new Float64Array(128);readonly replayLocalUsed:Float64Array=new Float64Array(128);
  readonly entryResidual:Float64Array=new Float64Array(128);readonly entryGuardBound:Float64Array=new Float64Array(128);
  readonly scalars:Float64Array=new Float64Array(32);
  readonly residual:Float64Array=new Float64Array(this.scalars.buffer as ArrayBuffer,128,3);
  readonly influence:Float64Array=new Float64Array(this.scalars.buffer as ArrayBuffer,152,3);
  readonly edgeId:Float64Array=new Float64Array(this.scalars.buffer as ArrayBuffer,176,3);
  readonly bounds:Float64Array=new Float64Array(this.scalars.buffer as ArrayBuffer,200,3);
  readonly outcome:P011GuardNumericOutcome=new P011GuardNumericOutcome();count:number=0;reason:BReason=BReason.NONE;
  beginStepInto(s:ClearWaterState,dt:number,angle:number):boolean {
    const a=this.scalars;a.fill(0);a[0]=dt;a[1]=a[2]=a[4]=300*dt;a[6]=angle;a[7]=angle*Math.PI/180;a[8]=Math.tan(a[7]);a[9]=Math.sin(a[7]);a[10]=Math.cos(a[7]);
    this.workLocalRemaining.fill(28*dt);this.workLocalUsed.fill(0);this.replayLocalRemaining.fill(28*dt);this.replayLocalUsed.fill(0);
    for(let c=0;c<127;c++){const d=(s.S[c+1]-s.S[c])-a[8];if(!Number.isFinite(d))return false;}return true;
  }
  budget(request:number,d:number,r:number):number {return Math.min(request,this.scalars[2],this.workLocalRemaining[d],r<0?request:this.workLocalRemaining[r]);}
  admitPatchInto(s:ClearWaterState,requested:number,d:number,r:number,dd:number,dr:number,replay:boolean=false):number {
    this.reason=BReason.NONE;this.count=0;
    const lo=Math.max(0,Math.min(d,r<0?d:r)-1),hi=Math.min(126,Math.max(d,r<0?d:r));
    for(let edge=lo;edge<=hi;edge++){
      const left=edge===d?dd:edge===r?dr:0,right=edge+1===d?dd:edge+1===r?dr:0;if(left===0&&right===0)continue;
      const n=this.count++;this.edgeId[n]=edge;this.residual[n]=(s.S[edge+1]-s.S[edge])-this.scalars[8];this.influence[n]=right-left;this.bounds[n]=Math.max(CW_HARD_MAX,Math.abs(this.residual[n]));
    }
    const q=replay?Math.min(requested,this.scalars[4],this.replayLocalRemaining[d],r<0?requested:this.replayLocalRemaining[r]):this.budget(requested,d,r);p011GuardAdmitNumericInto(q,this.residual,this.influence,this.count,CW_HARD_MAX,this.outcome);
    this.scalars[13]=requested;this.scalars[14]=this.outcome.accepted;return this.outcome.accepted;
  }
  actual(s:ClearWaterState,d:number,r:number,newD:number,newR:number):boolean {
    for(let j=0;j<this.count;j++){const e=this.edgeId[j],left=e===d?newD:e===r?newR:s.S[e],right=e+1===d?newD:e+1===r?newR:s.S[e+1],value=(right-left)-this.scalars[8];if(!Number.isFinite(value)||Math.abs(value)>this.bounds[j])return false;}return true;
  }
  writeBudget(arena:ClearWaterBArena,q:number,d:number,r:number):void {
    const reserved=this.budget(arena.get(7),d,r),refund=reserved-q;
    arena.set(31,this.scalars[2]);arena.set(32,(this.scalars[2]-reserved)+refund);arena.set(33,this.workLocalRemaining[d]);arena.set(34,(this.workLocalRemaining[d]-reserved)+refund);
    arena.set(35,r<0?0:this.workLocalRemaining[r]);arena.set(36,r<0?0:(this.workLocalRemaining[r]-reserved)+refund);
  }
  debit(q:number,d:number,r:number,requested:number):void {
    const reserved=this.budget(requested,d,r),refund=reserved-q;
    this.scalars[2]=(this.scalars[2]-reserved)+refund;this.scalars[3]=(this.scalars[3]+reserved)-refund;
    this.workLocalRemaining[d]=(this.workLocalRemaining[d]-reserved)+refund;this.workLocalUsed[d]=(this.workLocalUsed[d]+reserved)-refund;
    if(r>=0&&r!==d){this.workLocalRemaining[r]=(this.workLocalRemaining[r]-reserved)+refund;this.workLocalUsed[r]=(this.workLocalUsed[r]+reserved)-refund;}
  }
  replayBudgetInto(a:Float64Array,b:number,d:number,r:number):boolean {
    const q=a[b+8],remaining=this.scalars[4],reserved=Math.min(a[b+7],remaining,this.replayLocalRemaining[d],r<0?a[b+7]:this.replayLocalRemaining[r]),refund=reserved-q;
    if(q<=0||q>reserved||a[b+31]!==remaining||a[b+32]!==(remaining-reserved)+refund||a[b+33]!==this.replayLocalRemaining[d]||a[b+34]!==(this.replayLocalRemaining[d]-reserved)+refund||a[b+35]!==(r<0?0:this.replayLocalRemaining[r])||a[b+36]!==(r<0?0:(this.replayLocalRemaining[r]-reserved)+refund))return false;
    this.scalars[4]=(remaining-reserved)+refund;this.scalars[5]=(this.scalars[5]+reserved)-refund;this.replayLocalRemaining[d]=(this.replayLocalRemaining[d]-reserved)+refund;this.replayLocalUsed[d]=(this.replayLocalUsed[d]+reserved)-refund;
    if(r>=0&&r!==d){this.replayLocalRemaining[r]=(this.replayLocalRemaining[r]-reserved)+refund;this.replayLocalUsed[r]=(this.replayLocalUsed[r]+reserved)-refund;}return true;
  }
  finalInto(s:ClearWaterState,entry:ClearWaterState):boolean {
    let max=0,legal=true;for(let c=0;c<127;c++){const value=Math.abs((s.S[c+1]-s.S[c])-this.scalars[8]),bound=Math.max(CW_HARD_MAX,Math.abs((entry.S[c+1]-entry.S[c])-this.scalars[8]));if(!Number.isFinite(value)||value>bound)return false;max=Math.max(max,value);if(value>CW_HARD_MAX)legal=false;}
    this.scalars[11]=max;this.scalars[12]=legal?1:0;return true;
  }
  sealEntry(entry:ClearWaterState):void {for(let c=0;c<127;c++){const d=(entry.S[c+1]-entry.S[c])-this.scalars[8];this.entryResidual[c]=d;this.entryGuardBound[c]=Math.max(CW_HARD_MAX,Math.abs(d));}}
  auditBudget(replay:boolean):boolean {
    const rem=replay?this.replayLocalRemaining:this.workLocalRemaining,used=replay?this.replayLocalUsed:this.workLocalUsed,t=this.scalars[replay?4:2],u=this.scalars[replay?5:3],limit=this.scalars[1],local=28*this.scalars[0];
    if(!Number.isFinite(t)||!Number.isFinite(u)||t< -1e-9||u< -1e-9||u>limit+1e-9||Math.abs((t+u)-limit)>1e-9)return false;
    for(let c=0;c<128;c++)if(!Number.isFinite(rem[c])||!Number.isFinite(used[c])||rem[c]< -1e-9||used[c]< -1e-9||used[c]>local+1e-9||Math.abs((rem[c]+used[c])-local)>1e-9)return false;return true;
  }
}

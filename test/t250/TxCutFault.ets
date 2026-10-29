import { TxFaultPlan } from './TxFaultPlan';
import { P011ParticleHotPlanBuffer } from '../P011ParticleHot';
export class TxCutFault {
  readonly rows:string[]=[];
  plan:TxFaultPlan|null=null;
  capture:()=>string=():string=>'NOT_INSTALLED';
  private seen=0;
  hit=false;
  private savedOwner=0;
  enter(root:number,p:P011ParticleHotPlanBuffer,row:number):boolean {
    const f=this.plan;if(f===null||root!==f.rootId||this.hit)return false;
    this.seen++;if(this.seen!==f.ordinal)return false;
    this.savedOwner=p.ownerId;
    this.rows.push(JSON.stringify({edge:'BEFORE',plan:f,requestRow:row,ownerId:p.ownerId,raw:this.capture()}));
    // Corrupt the real input, not the returned code; original requestIdentityHot
    // decides the first error and the original failApply/rollback path runs.
    p.ownerId=-1;this.hit=true;return true;
  }
  leave(p:P011ParticleHotPlanBuffer,code:number):void {
    p.ownerId=this.savedOwner;
    this.rows.push(JSON.stringify({edge:'AFTER_ORIGINAL_CHECK',firstCode:code,ownerId:p.ownerId,raw:this.capture()}));
  }
}

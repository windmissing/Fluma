import { TxFaultPlan } from './TxFaultPlan';
import { P011ParticleHotPlanBuffer } from '../P011ParticleHot';
import { P011GestureSandRequest } from '../WaterSandField';
interface TxSourceReadset {heavy0:number;cols:number;sourceId:number;sequence:number;}
export class TxCutFault {
  readonly rows:string[]=[];
  plan:TxFaultPlan|null=null;
  capture:()=>string=():string=>'NOT_INSTALLED';
  private seen=0;
  hit=false;
  private savedOwner=0;
  private savedMass=0;
  enterField(root:number,water:Float64Array):boolean {const f=this.plan;if(f===null||f.cutId!==18||f.edge!=='BEFORE'||root!==f.rootId||this.hit)return false;if(++this.seen!==f.ordinal)return false;this.savedMass=water[0];this.rows.push(JSON.stringify({edge:'BEFORE',plan:f,raw:this.capture(),water0:water[0]}));water[0]=-1;this.hit=true;return true;}
  leaveField(water:Float64Array,code:number):void {water[0]=this.savedMass;this.rows.push(JSON.stringify({edge:'AFTER_ORIGINAL_CHECK',firstCode:code,raw:this.capture()}));}
  enterSource(root:number,r:P011GestureSandRequest):boolean {
    const f=this.plan;if(f===null||f.cutId!==1||f.edge!=='BEFORE'||root!==f.rootId||this.hit)return false;
    if(++this.seen!==f.ordinal)return false;
    this.savedMass=r.perColumnHeavy[0];const readset:TxSourceReadset={heavy0:this.savedMass,cols:r.perColumnHeavy.length,sourceId:r.sourceId,sequence:r.sequence};this.rows.push(JSON.stringify({edge:'BEFORE',plan:f,readset:readset,raw:this.capture()}));
    r.perColumnHeavy[0]=-1;this.hit=true;return true;
  }
  leaveSource(r:P011GestureSandRequest,code:number):void {r.perColumnHeavy[0]=this.savedMass;this.rows.push(JSON.stringify({edge:'AFTER_ORIGINAL_CHECK',firstCode:code,returnContract:'ORIGINAL_NULL_PRECHECK_NOT_A_BUSINESS_ERROR',raw:this.capture()}));}
  enter(root:number,p:P011ParticleHotPlanBuffer,row:number):boolean {
    const f=this.plan;if(f===null||f.cutId!==6||root!==f.rootId||this.hit)return false;
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

import { TxWholePairAdapter } from '../tx/TxWholePairAdapter';
import { DerivedStatus } from './P011DerivedTypesV2';
import { DerivedCoordinatorV2 } from './P011DerivedTupleBarrierV2';

export const CROSS_VERTICAL_V1:string='P011_CPU_DRY_CROSS_VERTICAL_V1';
export const CROSS_CHECKPOINT_V1:string='P011_DERIVED_CHECKPOINT_CROSS_VERTICAL_V1';
export function crossSameF64(a:number,b:number):boolean{return a===b&&(a!==0||1/a===1/b);}
export const CROSS_CAPACITY:number=1;
export const CROSS_BUDGET:number=2;
export const CROSS_LAND_REMAINDER:number=3;

// Entries retain the row-time image even when dry lift reuses the Hot banks.
export class DryCrossByteBackup {
 private readonly sources:Uint8Array[]=[];
 private readonly before:Uint8Array[]=[];
 bytes=0;
 constructor(buffers:ArrayBuffer[]){
  for(const buffer of buffers){
   this.sources.push(new Uint8Array(buffer));this.before.push(new Uint8Array(buffer.byteLength));
   this.bytes+=buffer.byteLength;
  }
 }
 save():void {for(let i=0;i<this.sources.length;i++)this.before[i].set(this.sources[i]);}
 restore():void {for(let i=0;i<this.sources.length;i++)this.sources[i].set(this.before[i]);}
}

export class DryCrossRows {
 readonly keys:Uint32Array;
 readonly values:Float64Array;
 count=0;
 readonly capacity:number;
 constructor(capacity:number){
  this.capacity=capacity;
  this.keys=new Uint32Array(capacity*8);this.values=new Float64Array(capacity*20);
 }
 copyFrom(src:DryCrossRows):void {
  this.count=src.count;
  for(let i=0;i<src.count*8;i++)this.keys[i]=src.keys[i];
  for(let i=0;i<src.count*20;i++)this.values[i]=src.values[i];
 }
}

export class P011DryCrossVerticalV1 {
 readonly staged:DryCrossRows;
 private readonly published:DryCrossRows;
 private readonly seen:Uint32Array;
 private readonly expected:Float64Array;
 private readonly expectedState:Uint8Array;
 private readonly stagedSlotGeneration:Uint32Array;
 private readonly publishedFinal:Float64Array;
 private readonly publishedFinalState:Uint8Array;
 private stagedOwner=0;private stagedRun='';private stagedEpoch=0;private stagedSubstep=0;
 private stagedAttempt='';private stagedHotEpoch=0;private stagedHotAttempt=0;
 private publishedOwner=0;private publishedHotEpoch=0;private publishedHotAttempt=0;
 private failed=false;private failureRun='';private failureEpoch=0;private failureSubstep=0;
 private failureAttempt='';private failureCut='';private failureHot=0;private failureHotStage=0;
 private failureField=0;private failureCommit=0;private failureRolledBack=false;
 private generation=0;
 private resolved=false;
 private publishedCommit=0;
 private publishedRun='';
 private publishedEpoch=0;
 private publishedSubstep=0;
 private publishedAttempt:string|null=null;
 constructor(n:number){
  this.staged=new DryCrossRows(n);this.published=new DryCrossRows(n);
  this.seen=new Uint32Array(n);this.expected=new Float64Array(n*4);this.expectedState=new Uint8Array(n);
  this.stagedSlotGeneration=new Uint32Array(n);this.publishedFinal=new Float64Array(n*4);this.publishedFinalState=new Uint8Array(n);
 }
 stage(rows:DryCrossRows,owner:DerivedCoordinatorV2,run:string,epoch:number,substep:number,
  attempt:string,hotEpoch:number,hotAttempt:number):void {
  this.staged.copyFrom(rows);this.resolved=false;
  this.stagedOwner=owner.ownerId;this.stagedRun=run;this.stagedEpoch=epoch;this.stagedSubstep=substep;
  this.stagedAttempt=attempt;this.stagedHotEpoch=hotEpoch;this.stagedHotAttempt=hotAttempt;
  for(let i=0;i<rows.count;i++)this.stagedSlotGeneration[i]=owner.canonical.generations[rows.keys[i*8]];
 }
 matches(owner:number,run:string,epoch:number,substep:number,attempt:string|null):boolean{
  return this.stagedOwner===owner&&this.stagedRun===run&&this.stagedEpoch===epoch&&
   this.stagedSubstep===substep&&attempt!==null&&this.stagedAttempt===attempt&&this.stagedHotEpoch>0&&this.stagedHotAttempt>0;
 }
 discard():void {this.staged.count=0;this.stagedSubstep=0;this.resolved=false;}
 beginAttempt():void {this.failed=false;this.failureRolledBack=false;this.discard();}
 recordFailure(run:string,epoch:number,substep:number,attempt:string,cut:string,hot:number,stage:number,field:number,commit:number):void {
  if(this.failed)return;
  this.failed=true;this.failureRun=run;this.failureEpoch=epoch;this.failureSubstep=substep;this.failureAttempt=attempt;
  this.failureCut=cut;this.failureHot=hot;this.failureHotStage=stage;this.failureField=field;this.failureCommit=commit;
 }
 finishRollback():void {if(this.failed)this.failureRolledBack=true;}
 exportFailureCold():string {
  return JSON.stringify({revision:'P011_CPU_DRY_CROSS_VERTICAL_FAILURE_V1',failed:this.failed,
   rolledBack:this.failureRolledBack,run:this.failureRun,epoch:this.failureEpoch,substep:this.failureSubstep,
   attempt:this.failureAttempt,cut:this.failureCut,originalHotCode:this.failureHot,originalHotStage:this.failureHotStage,
   originalFieldCode:this.failureField,activeCommit:this.failureCommit});
 }
 ready():boolean {return this.resolved;}
 commit(run:string,epoch:number,substep:number,attempt:string|null,commit:number):void {
  this.published.copyFrom(this.staged);this.publishedCommit=commit;
  this.publishedOwner=this.stagedOwner;this.publishedHotEpoch=this.stagedHotEpoch;this.publishedHotAttempt=this.stagedHotAttempt;
  for(let i=0;i<this.staged.count;i++){
   this.publishedFinalState[i]=this.expectedState[i];
   for(let j=0;j<4;j++)this.publishedFinal[i*4+j]=this.expected[i*4+j];
  }
  this.publishedRun=run;this.publishedEpoch=epoch;this.publishedSubstep=substep;this.publishedAttempt=attempt;
  this.discard();
 }
 exportCold():string {
  const rows=this.published;
  const bits=new DataView(new ArrayBuffer(8));const words:string[]=[];
  for(let i=0;i<rows.count*4;i++){
   bits.setFloat64(0,this.publishedFinal[i],false);
   words.push(bits.getUint32(0,false).toString(16).padStart(8,'0')+bits.getUint32(4,false).toString(16).padStart(8,'0'));
  }
  return JSON.stringify({revision:'P011_CPU_DRY_CROSS_VERTICAL_RECEIPT_V1',run:this.publishedRun,
   owner:this.publishedOwner,hotEpoch:this.publishedHotEpoch,hotAttempt:this.publishedHotAttempt,role:1,
   stateEpoch:this.publishedEpoch,substep:this.publishedSubstep,attempt:this.publishedAttempt,commit:this.publishedCommit,
   keys:['slot','id','sourceId','column','targetPlusOne','ordinal','packed','eventIndex'],
   fields:['mass','x0','y0','vx0','vy0','yFree','vyFree','predictedY','rowX','rowY','rowVx','rowVy','Srow','Sfinal',
    'ticket','reserved','refunded','grant','remainder','requested'],
   finalFields:['x','y','vx','vy'],finalF64Hex:words,
   finalState:Array.from(this.publishedFinalState.subarray(0,rows.count)),
   integers:Array.from(rows.keys.subarray(0,rows.count*8)),values:Array.from(rows.values.subarray(0,rows.count*20))});
 }
 // First independently validate the entire batch and calculate the expected
 // image. No tail writer runs while any row identity or final bed is unknown.
 resolve(owner:DerivedCoordinatorV2,eps:number,tx:TxWholePairAdapter|null):number {
  const p=owner.canonical.pool,f=owner.domain.field,rows=this.staged;
  const k=rows.keys,v=rows.values;
  this.generation++;
  if(this.generation>0xffffffff){this.seen.fill(0);this.generation=1;}
  for(let i=0;i<rows.count;i++){
   const a=i*8,b=i*20,e=i*4,slot=k[a],column=k[a+3],flags=k[a+6];
   if(slot>=p.nmax||column>=p.cols||this.seen[slot]===this.generation||
      (this.stagedSubstep>0&&owner.canonical.generations[slot]!==this.stagedSlotGeneration[i])||
      p.pId[slot]!==k[a+1]||p.pSourceId[slot]!==k[a+2]||p.pPhase[slot]!==((flags>>8)&1)||
      p.pSource[slot]!==((flags>>9)&1)||p.pState[slot]!==((flags>>4)&3)||
      Math.floor(p.pX[slot])!==column||!crossSameF64(p.pMass[slot],v[b])||
      !crossSameF64(p.pX[slot],v[b+8])||!crossSameF64(p.pY[slot],v[b+9])||
      !crossSameF64(p.pVx[slot],v[b+10])||!crossSameF64(p.pVy[slot],v[b+11]))
    return owner.fail(DerivedStatus.CERT_STALE,'CROSS_VERTICAL_TAIL_IDENTITY');
   this.seen[slot]=this.generation;
   const bed=((f.bh[column]+f.bl[column])+f.mh[column])+f.ml[column];
   if(!Number.isFinite(bed)||bed<0||bed>f.capacity||!crossSameF64(f.surface[column],bed))
    return owner.fail(DerivedStatus.INPUT_INVALID,'CROSS_VERTICAL_TAIL_BED');
   v[b+13]=bed;
   const contact=p.pState[slot]===2||Math.min(v[b+2],v[b+5])<=bed;
   this.expectedState[i]=contact?2:1;
   this.expected[e]=v[b+8];
   this.expected[e+1]=contact?Math.min(f.capacity,bed+eps):v[b+9];
   this.expected[e+2]=v[b+10];
   this.expected[e+3]=contact?0:v[b+11];
  }
  for(let i=0;i<rows.count;i++){
   const a=i*8,b=i*20,e=i*4,slot=k[a];
   const hit=p.pState[slot]===2||Math.min(v[b+2],v[b+5])<=f.surface[k[a+3]];
   if(hit){
    const mutation=owner.begin(p,slot,-1,'CROSS_VERTICAL_TAIL');
    if(mutation===null)return owner.status;
    tx?.before(p,slot);
    if(p.pState[slot]!==2)k[a+6]|=1024;
    p.pState[slot]=2;p.pY[slot]=Math.min(f.capacity,f.surface[k[a+3]]+eps);p.pVy[slot]=0;
    const status=owner.finish(p,mutation);
    if(status!==0)return status;
    tx?.after(p,slot);
   }
   if(p.pState[slot]!==this.expectedState[i]||!crossSameF64(p.pX[slot],this.expected[e])||
      !crossSameF64(p.pY[slot],this.expected[e+1])||!crossSameF64(p.pVx[slot],this.expected[e+2])||
      !crossSameF64(p.pVy[slot],this.expected[e+3])||!crossSameF64(p.pMass[slot],v[b]))
    return owner.fail(DerivedStatus.CERT_STALE,'CROSS_VERTICAL_TAIL_COMPARE');
  }
  this.resolved=true;
  return 0;
 }
 verifyClosed(owner:DerivedCoordinatorV2):boolean{
  const p=owner.canonical.pool,rows=this.staged,k=rows.keys,v=rows.values;
  for(let i=0;i<rows.count;i++){
   const a=i*8,b=i*20,e=i*4,slot=k[a],column=k[a+3],flags=k[a+6];
   if(p.pId[slot]!==k[a+1]||p.pSourceId[slot]!==k[a+2]||p.pPhase[slot]!==((flags>>8)&1)||
      p.pSource[slot]!==((flags>>9)&1)||owner.canonical.generations[slot]!==this.stagedSlotGeneration[i]||
      p.pState[slot]!==this.expectedState[i]||!crossSameF64(p.pMass[slot],v[b])||
      !crossSameF64(p.pX[slot],this.expected[e])||!crossSameF64(p.pY[slot],this.expected[e+1])||
      !crossSameF64(p.pVx[slot],this.expected[e+2])||!crossSameF64(p.pVy[slot],this.expected[e+3])||
      !crossSameF64(owner.domain.field.surface[column],v[b+13]))return false;
  }
  return this.resolved;
 }
}

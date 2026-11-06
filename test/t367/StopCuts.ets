import { P011ParticleSnapshot } from '../P011ParticleField';
import { P011ParticleHotPlanBuffer, P011ParticleHotResultBuffer, P011ParticleHotEventBuffer } from '../P011ParticleHot';
export class StopCuts {
 enabled:boolean=false;readonly ids=new Uint32Array(3);readonly rows=new Float64Array(256*72);
 readonly bits=new DataView(new ArrayBuffer(8));count=0;head=0;overflow=0;serial=0;frame=0;
 run:string='';token:string='';actual:string='';epoch=0;commit=0;source=0;held=false;active=false;
 admissionCount=0;frozen=0;lastError=0;derivedStatus=0;previousNs=0;currentNs=0;dt=0;elapsed=0;rollbacks=0;attempt=0;substep=0;tilt=0;motions=0;requests=0;applyCount=0;writes=0;reason=0;stepOk=0;stepped=0;dormant=0;q=0;subDt=0;
 private readonly runs:string[]=new Array<string>(256).fill('');private readonly actuals:string[]=new Array<string>(256).fill('');
 private readonly tokens:string[]=new Array<string>(256).fill('');
 private record(stage:number,id:number,slot:number,y:number,vy:number,present:boolean):void {
  const a=this.head*72,v=this.rows;v.fill(0,a,a+72);v[a]=++this.serial;v[a+1]=this.frame;v[a+2]=stage;v[a+3]=id;v[a+4]=slot;v[a+5]=present?1:0;
  this.bits.setFloat64(0,y,true);v[a+6]=this.bits.getUint32(0,true);v[a+7]=this.bits.getUint32(4,true);
  this.bits.setFloat64(0,vy,true);v[a+8]=this.bits.getUint32(0,true);v[a+9]=this.bits.getUint32(4,true);
  v[a+10]=this.epoch;v[a+11]=this.commit;v[a+12]=this.source;v[a+13]=this.held?1:0;v[a+14]=this.active?1:0;v[a+15]=this.previousNs;v[a+16]=this.currentNs;v[a+17]=this.dt;v[a+18]=this.elapsed;v[a+19]=this.rollbacks;v[a+20]=this.attempt;v[a+21]=this.substep;v[a+22]=this.tilt;v[a+23]=this.motions;v[a+24]=this.requests;v[a+25]=this.applyCount;v[a+26]=this.writes;v[a+27]=this.reason;v[a+28]=this.stepOk;v[a+29]=this.stepped;v[a+30]=this.q;v[a+31]=this.subDt;v[a+32]=this.dormant;v[a+33]=this.admissionCount;v[a+34]=this.frozen;v[a+35]=this.lastError;v[a+36]=this.derivedStatus;
  v[a+37]=this.role;v[a+38]=this.transaction;v[a+39]=this.cohortEpoch;v[a+40]=this.admissionDelta;v[a+41]=this.finalTilt;v[a+42]=this.acceptedTotal;
  v.set(this.detail,a+43);
  this.actuals[this.head]='';this.runs[this.head]=this.run;this.tokens[this.head]=this.token;this.head=(this.head+1)%256;if(this.count<256)this.count++;else this.overflow++;
 }
 private readonly beforeBed=new Float64Array(128);
 private readonly stopAnchor=new Float64Array(3*72);
 private readonly firstAnchor=new Float64Array(3*72);
 private readonly lastAnchor=new Float64Array(3*72);
 private readonly detail=new Float64Array(19);
 private readonly committedRequest=new Float64Array(3*19);
 private readonly committedColumn=new Int32Array(3);
 private readonly terminalRequest=new Float64Array(3*72);
 cohortRun:string='';cohortEpoch=0;transaction=0;acceptedTotal=0;lastFrameAccepted=0;admissionDelta=0;
 stopReason:string='';stopTime=0;stopElapsed=0;firstCommit=0;finalTilt=0;role=0;
 resetRun():void {
  this.ids.fill(0);this.stopAnchor.fill(0);this.firstAnchor.fill(0);this.lastAnchor.fill(0);this.terminalRequest.fill(0);
  this.cohortRun='';this.cohortEpoch=0;this.transaction=0;this.stopReason='';this.stopTime=0;
  this.firstCommit=0;this.acceptedTotal=0;this.lastFrameAccepted=0;this.admissionCount=0;this.elapsed=0;
  this.count=0;this.head=0;this.overflow=0;
 }
 admission(first:number,run:string,epoch:number,tx:number,source:number,amount:number,p:P011ParticleSnapshot,bed:Float64Array):void {
  if(!this.enabled)return;
  this.cohortRun=run;this.cohortEpoch=epoch;this.transaction=tx;this.source=source;
  this.admissionCount++;this.acceptedTotal+=amount;this.ids.fill(0);
  this.stopReason='';this.stopTime=0;this.firstCommit=0;
  this.stopAnchor.fill(0);this.firstAnchor.fill(0);this.lastAnchor.fill(0);this.terminalRequest.fill(0);
  let used=0;
  for(let i=0;i<p.nmax&&used<3;i++){
   const column=Math.floor(p.pX[i]);
   if(p.pState[i]===1&&p.pId[i]>=first&&p.pY[i]>bed[column]+1e-9)this.ids[used++]=p.pId[i];
  }
 }
 stop(reason:string,time:number):void {
  if(!this.enabled||this.stopReason.length!==0)return;
  this.stopReason=reason;this.stopTime=time;this.stopElapsed=this.elapsed;
 }
 committed(commit:number,tilt:number,dt:number,p:P011ParticleSnapshot,bed:Float64Array,mh:Float64Array,ml:Float64Array):void {
  if(!this.enabled)return;
  this.elapsed+=dt;this.commit=commit;this.finalTilt=tilt;
  if(this.stopReason.length===0)return;
  this.capture(11,p,bed,mh,ml);
  if(this.firstCommit===0){this.firstCommit=commit;this.firstAnchor.set(this.lastAnchor);}
 }
 capture(stage:number,p:P011ParticleSnapshot,bed:Float64Array|null=null,mh:Float64Array|null=null,ml:Float64Array|null=null):void {
  if(!this.enabled)return;
  if(stage===0){
   this.committedRequest.fill(0);this.committedColumn.fill(-1);
   if(mh!==null&&ml!==null)for(let i=0;i<128;i++)this.beforeBed[i]=mh[i]+ml[i];
  }
  for(let k=0;k<3;k++)if(this.ids[k]!==0){
   let slot=-1;
   for(let i=0;i<p.nmax;i++)if(p.pState[i]!==0&&p.pId[i]===this.ids[k]){slot=i;break;}
   this.detail.fill(0);this.detail[0]=slot<0?1:0;
   if(slot>=0){
    const col=Math.floor(p.pX[slot]);this.detail[1]=p.pState[slot];this.detail[2]=p.pMass[slot];
    if(bed!==null){this.detail[3]=bed[col];this.detail[4]=p.pY[slot]-bed[col];this.detail[18]=1;}
    if(mh!==null&&ml!==null)this.detail[13]=mh[col]+ml[col]-this.beforeBed[col];
   }
   if(stage===11&&this.committedRequest[k*19+5]>0){
    for(let j=5;j<=17;j++)if(j!==13)this.detail[j]=this.committedRequest[k*19+j];
    const column=this.committedColumn[k];
    if(column>=0&&mh!==null&&ml!==null)this.detail[13]=mh[column]+ml[column]-this.beforeBed[column];
   }
   const at=this.head*72;
   this.record(stage,this.ids[k],slot,slot<0?0:p.pY[slot],slot<0?0:p.pVy[slot],slot>=0);
   if(stage===9&&this.stopAnchor[k*72]===0)for(let j=0;j<72;j++)this.stopAnchor[k*72+j]=this.rows[at+j];
   if(stage===11){
    for(let j=0;j<72;j++)this.lastAnchor[k*72+j]=this.rows[at+j];
    if(this.detail[11]!==0)for(let j=0;j<72;j++)this.terminalRequest[k*72+j]=this.rows[at+j];
   }
  }
 }
 predicted(p:P011ParticleHotPlanBuffer,r:P011ParticleHotResultBuffer,e:P011ParticleHotEventBuffer):void {
  if(!this.enabled)return;
  this.attempt=p.attemptToken;this.substep=p.substepId;this.tilt=p.effectiveTiltRad;this.motions=p.motionCount;this.requests=p.requestCount;
  for(let k=0;k<3;k++)if(this.ids[k]!==0){
   const id=this.ids[k];let motion=-1,request=-1;
   for(let i=0;i<p.motionCount;i++)if(p.motionParticleId[i]===id){motion=i;break;}
   for(let i=0;i<p.requestCount;i++)if(p.particleId[i]===id){request=i;break;}
   this.detail.fill(0);
   if(request>=0){
    const i=request;
    this.detail[5]=p.kind[i];this.detail[6]=p.requested[i];this.detail[7]=r.granted[i];this.detail[8]=r.remainder[i];
    this.detail[9]=r.granted[i]!==0?0:r.capacityClipped[i]===p.requested[i]?1:r.budgetClipped[i]===p.requested[i]?2:r.guardRejected[i]>0?3:r.poolClipped[i]>0?4:r.capacityClipped[i]>0?1:5;
    this.detail[10]=r.disposition[i];this.detail[11]=r.released[i];
    this.detail[12]=r.eventCount[i]>0?e.kind[r.eventBegin[i]]:0;
    this.detail[14]=r.budgetClipped[i];this.detail[15]=r.guardRejected[i];this.detail[16]=r.capacityClipped[i];this.detail[17]=r.poolClipped[i];
    if(this.role===1){this.committedRequest.set(this.detail,k*19);this.committedColumn[k]=p.column[i];}
    this.record(1,id,-1,p.predictedY[i],p.predictedVy[i],true);
   }else{
    this.detail[0]=motion<0?2:0;
    this.record(1,id,-1,motion<0?0:p.motionY[motion],motion<0?0:p.motionVy[motion],motion>=0);
   }
  }
 }
 attach(requested:string,actual:string):void{if(!this.enabled)return;this.token=requested;this.actual=actual;for(let i=0;i<this.count;i++){const at=(this.head-this.count+256+i)%256;if(this.rows[at*72+1]===this.frame){this.tokens[at]=requested;this.actuals[at]=actual;}}}
 finishStep(ok:boolean,stepped:boolean,dormant:boolean,q:number,subDt:number):void{if(!this.enabled)return;this.stepOk=ok?1:0;this.stepped=stepped?1:0;this.dormant=dormant?1:0;this.q=q;this.subDt=subDt;this.detail.fill(0);this.record(8,0,-1,0,0,false);}
 exportCold():string {const rows:number[][]=[],runs:string[]=[],tokens:string[]=[],actuals:string[]=[];for(let i=0;i<this.count;i++){const at=(this.head-this.count+256+i)%256;rows.push(Array.from(this.rows.subarray(at*72,at*72+72)));runs.push(this.runs[at]);tokens.push(this.tokens[at]);actuals.push(this.actuals[at]);}return JSON.stringify({revision:'T367_STOP_COHORT_V1',rowWidth:72,cohortRun:this.cohortRun,cohortEpoch:this.cohortEpoch,admissionTransactionId:this.transaction,ids:Array.from(this.ids),stopReason:this.stopReason,stopTimeMs:this.stopTime,stopElapsed:this.stopElapsed,firstCommit:this.firstCommit,stopAnchor:Array.from(this.stopAnchor),firstAnchor:Array.from(this.firstAnchor),lastAnchor:Array.from(this.lastAnchor),terminalRequest:Array.from(this.terminalRequest),stopClock:'CALLBACK_DATE_MS',sampleStatus:['OBSERVED','ABSENT_ID','NO_MOTION_ROW'],roles:['BOUNDARY','AUTHORITATIVE','PREVIEW'],zeroGrant:['NOT_ZERO','CAPACITY','BUDGET','GUARD','POOL','UNCLASSIFIED'],capacity:256,overflow:this.overflow,rows:rows,runs:runs,tokens:tokens,actualTokens:actuals,actualToken:this.actual,stages:['canonicalBefore','predicted','afterApply','prepublication','bank','slot','retired','rollback','stepReceipt','stop','reserved','committedSubstep'],fields:['serial','frame','stage','id','physicalSlot','present','yLo','yHi','vyLo','vyHi','epoch','commit','sourceEpoch','held','active','previousNs','currentNs','clippedDt','acceptedElapsed','rollbacks','attempt','substep','acceptedTiltRad','motionCount','requestCount','applyCount','writeCount','rollbackReason','stepOk','stepped','Q','substepDt','dormant','admissionCount','frozen','lastError','derivedStatus','role','admissionTransactionId','cohortEpoch','frameAcceptedMass','finalAcceptedDeg','cumulativeAcceptedMass','sampleStatus','particleState','mass','bedHeight','netHeight','requestKind','requested','granted','remainder','zeroGrant','disposition','released','eventKind','bedMassDelta','budgetClipped','guardRejected','capacityClipped','poolClipped','bedPresent']});}
}

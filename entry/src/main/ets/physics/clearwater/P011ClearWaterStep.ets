import { ClearWaterState } from './P011ClearWaterState';
import { ClearWaterStatus as S,ClearWaterRunConfig,ClearWaterStepResult,ClearWaterAllocation,cwSameFiniteBits } from './P011ClearWaterTypes';
import { ClearWaterBConfig,BReason,BKind,bField,bAllocation,ClearWaterBFrameCounters } from './P011ClearWaterBTypes';
import { ClearWaterExactMass } from './P011ClearWaterExactMass';
import { ClearWaterBGuard } from './P011ClearWaterGuard';
import { ClearWaterBArena,ClearWaterBOrigins } from './P011ClearWaterEvents';
import { ClearWaterBHydraulics } from './P011ClearWaterHydraulics';
import { ClearWaterBParticles } from './P011ClearWaterParticles';
type BTypedArray=Float64Array|Uint16Array|Uint8Array|Uint32Array|Int32Array;
class BStorageBinding {
 readonly read:()=>BTypedArray;readonly buffer:ArrayBuffer;readonly bytes:number;readonly offset:number;
 constructor(read:()=>BTypedArray){this.read=read;const a=read();this.buffer=a.buffer as ArrayBuffer;this.bytes=a.byteLength;this.offset=a.byteOffset;}
 valid():boolean {const a=this.read();return a.buffer===this.buffer&&a.byteLength===this.bytes&&a.byteOffset===this.offset;}
}
export class ClearWaterBStep {
  private readonly bindings:BStorageBinding[]=[];
  readonly ownedBuffers:ArrayBuffer[]=[];

  readonly replay:ClearWaterState;readonly counters:Float64Array=new Float64Array(48);
  readonly mass:ClearWaterExactMass;readonly guard:ClearWaterBGuard=new ClearWaterBGuard();
  readonly hydraulics:ClearWaterBHydraulics=new ClearWaterBHydraulics();readonly particles:ClearWaterBParticles=new ClearWaterBParticles();
  readonly etaMedium:Float64Array=new Float64Array(128);readonly wetMask:Uint8Array=new Uint8Array(128);
  readonly arena0:ClearWaterBArena;readonly arena1:ClearWaterBArena;readonly origins:ClearWaterBOrigins=new ClearWaterBOrigins();
  readonly config:ClearWaterBConfig;private sealedIndex:number=-1;serial:number=0;stage:number=0;reason:BReason=BReason.NONE;
  frameId:number=0;fullScope:boolean=false;private readonly scalarBindings:ArrayBuffer[];
  frameEnqueueMs:number=NaN;frameSourceMs:number=NaN;frameExportMs:number=NaN;private replayOrdinal:number=0;
  setClock(clock:(()=>number)|null):void {this.clock=clock;this.lastClock=NaN;}
  clearScopeTimes():void {this.frameEnqueueMs=this.frameSourceMs=this.frameExportMs=this.clock===null?NaN:0;}
  scopeTime(kind:number,start:number,end:number):void {const value=Number.isFinite(start)&&Number.isFinite(end)?end-start:NaN;
    if(kind===0)this.frameEnqueueMs=Number.isFinite(this.frameEnqueueMs)&&Number.isFinite(value)?this.frameEnqueueMs+value:value;
    else if(kind===1)this.frameSourceMs=Number.isFinite(this.frameSourceMs)&&Number.isFinite(value)?this.frameSourceMs+value:value;
    else this.frameExportMs=Number.isFinite(this.frameExportMs)&&Number.isFinite(value)?this.frameExportMs+value:value;
  }
  clock:(()=>number)|null=null;private lastClock:number=NaN;private segmentClock:number=NaN;private wholeClock:number=NaN;private clockStatus:number=0;
  readClock():number {
    if(this.clock===null)return NaN;
    try{const value=this.clock();if(!Number.isFinite(value)||Number.isFinite(this.lastClock)&&value<this.lastClock){this.clockStatus=3;return NaN;}this.lastClock=value;if(this.clockStatus===0)this.clockStatus=1;return value;}
    catch(error){this.clockStatus=2;return NaN;}
  }
  private tick(slot:number):void {const value=this.readClock();if(Number.isFinite(value)&&Number.isFinite(this.segmentClock))this.candidate.header[slot]=value-this.segmentClock;this.segmentClock=value;}
  constructor(config:ClearWaterBConfig,k:number){
    this.config=JSON.parse(JSON.stringify(config)) as ClearWaterBConfig;this.replay=new ClearWaterState(k);this.replay.enableB();this.replay.bindBStorage();
    this.mass=new ClearWaterExactMass(this.counters);this.arena0=new ClearWaterBArena(config.eventCapacity,config.lineageCapacity,k);this.arena1=new ClearWaterBArena(config.eventCapacity,config.lineageCapacity,k);
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.etaMedium));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.wetMask));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.counters));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.origins.bytes));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.mass.pExact70));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.mass.capacityBound70));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.mass.scratch0));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.mass.scratch1));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.mass.bits));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.workLocalRemaining));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.workLocalUsed));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.replayLocalRemaining));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.replayLocalUsed));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.entryResidual));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.entryGuardBound));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.scalars));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.residual));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.influence));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.edgeId));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.guard.bounds));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.arena0.events));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.arena0.lineage));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.arena0.header));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.arena0.summary));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.arena1.events));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.arena1.lineage));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.arena1.header));
    this.bindings.push(new BStorageBinding(():BTypedArray=>this.arena1.summary));
    for(const binding of this.bindings)this.ownedBuffers.push(binding.buffer);
    this.scalarBindings=[this.arena0.events.buffer as ArrayBuffer,this.arena1.events.buffer as ArrayBuffer,this.arena0.lineage.buffer as ArrayBuffer,this.arena1.lineage.buffer as ArrayBuffer,
      this.mass.pExact70.buffer as ArrayBuffer,this.mass.capacityBound70.buffer as ArrayBuffer,this.guard.scalars.buffer as ArrayBuffer,this.counters.buffer as ArrayBuffer];
  }
  get candidate():ClearWaterBArena {return this.sealedIndex===0?this.arena1:this.arena0;}
  get sealed():ClearWaterBArena|null {return this.sealedIndex<0?null:this.sealedIndex===0?this.arena0:this.arena1;}
  storageValid():boolean {
    for(let j=0;j<this.bindings.length;j++)if(!this.bindings[j].valid())return false;
    return this.replay.bStorageValid()&&this.arena0.events.buffer===this.scalarBindings[0]&&this.arena1.events.buffer===this.scalarBindings[1]&&this.arena0.events.length===40*this.config.eventCapacity&&this.arena1.events.length===40*this.config.eventCapacity&&
      this.arena0.lineage.buffer===this.scalarBindings[2]&&this.arena1.lineage.buffer===this.scalarBindings[3]&&this.arena0.lineage.length===3*this.config.lineageCapacity&&this.arena1.lineage.length===3*this.config.lineageCapacity&&
      this.mass.pExact70.buffer===this.scalarBindings[4]&&this.mass.pExact70.length===8960&&this.mass.capacityBound70.buffer===this.scalarBindings[5]&&this.mass.capacityBound70.length===8960&&this.guard.scalars.buffer===this.scalarBindings[6]&&this.guard.scalars.length===32&&this.counters.buffer===this.scalarBindings[7]&&this.counters.length===48;
  }
  reset():void {this.serial=0;this.sealedIndex=-1;this.origins.reset();this.replay.reset(null);this.arena0.reset();this.arena1.reset();this.counters.fill(0);this.guard.scalars.fill(0);this.guard.entryResidual.fill(0);this.guard.entryGuardBound.fill(0);this.guard.workLocalRemaining.fill(0);this.guard.workLocalUsed.fill(0);this.guard.replayLocalRemaining.fill(0);this.guard.replayLocalUsed.fill(0);this.mass.pExact70.fill(0);this.mass.capacityBound70.fill(0);this.etaMedium.fill(0);this.wetMask.fill(0);this.fullScope=false;this.frameId=0;this.lastClock=this.segmentClock=this.wholeClock=NaN;this.clockStatus=0;this.clearScopeTimes();}
  private fail(status:S,reason:BReason,out:ClearWaterStepResult):S {
    this.reason=reason===BReason.NONE?BReason.WORK_AUDIT_FAILED:reason;out.status=status;out.stepReason=this.reason;out.stage=this.stage;out.ordinal=this.stage<=2?1:this.stage===7?this.replayOrdinal:this.candidate.count+1;out.eventCount=this.stage<=2?0:this.candidate.count;out.lineageCount=this.stage<=2?0:this.candidate.lineageCount;this.counters[8]++;
    if(this.counters[46]===0){this.counters[46]=this.stage;this.counters[47]=out.ordinal;}return status;
  }
  private fold(s:ClearWaterState):void {
    s.P.fill(0);s.pool.colParticleMassH.fill(0);s.pool.colParticleMassL.fill(0);let i=s.pool.head;
    while(i!==-1){const c=Math.floor(s.pool.pX[i]);s.P[c]+=s.pool.pMass[i];s.pool.colParticleMassH[c]=s.P[c];this.counters[15]++;i=s.pool.next[i];}
    for(let c=0;c<128;c++){s.S[c]=(((s.bh[c]+0)+s.mh[c])+0);s.eta[c]=s.S[c]+s.W[c];s.R[c]=s.eta[c]+s.P[c];}this.counters[34]++;
  }
  private bAudit(s:ClearWaterState):boolean {
    for(let i=0;i<s.pool.nmax;i++){
      if(s.pool.pState[i]===0){if(s.originSourceCommit![i]!==0||s.waitReason![i]!==0)return false;continue;}
      const c=Math.floor(s.pool.pX[i]);if(!Number.isSafeInteger(s.originSourceCommit![i])||s.originSourceCommit![i]<=0||s.originSourceCommit![i]>s.sourceCommit||s.waitReason![i]>4||s.pool.pState[i]===1&&s.waitReason![i]!==0||s.pool.pState[i]===2&&s.waitReason![i]===0||s.pool.pY[i]<s.S[c])return false;
    }return true;
  }
  // Pure evidence replay: no trajectory integration, no candidate search.
  replayInto(before:ClearWaterState,a:ClearWaterBArena,marks:Uint8Array):boolean {
    const s=this.replay;s.copyFrom(before);this.counters[30]++;this.counters[31]+=this.stateBytes(s);
    if(!this.mass.buildFromStateInto(s,false))return false;
    let expectedLine=0;
    for(let n=0;n<a.count;n++){
      this.replayOrdinal=n+1;this.counters[32]++;const v=a.events,b=n*40,k=v[b],i=v[b+2],d=v[b+5],r=v[b+6],flags=v[b+39];
      if(!a.validateRow(b)||v[b+37]!==expectedLine||!Number.isInteger(d)||d<0||d>=128||!Number.isInteger(r)||r<0||r>=128)return false;expectedLine+=v[b+38];
      if(k<=5){const p=s.pool;if(p.pState[i]===0||p.pId[i]!==v[b+3]||s.originSourceCommit![i]!==v[b+4]||Math.floor(p.pX[i])!==d||p.pState[i]!==bField(flags,0,2)||s.waitReason![i]!==bField(flags,4,3)||
          !cwSameFiniteBits(p.pMass[i],v[b+9])||!cwSameFiniteBits(p.pX[i],v[b+11])||!cwSameFiniteBits(p.pY[i],v[b+12])||!cwSameFiniteBits(p.pVx[i],v[b+13])||!cwSameFiniteBits(p.pVy[i],v[b+14]))return false;}
      if(k===7){if(d===r||Math.abs(d-r)!==1||!cwSameFiniteBits(s.W[d],v[b+27])||!cwSameFiniteBits(s.W[r],v[b+29])||v[b+8]<=0||v[b+28]!==v[b+27]-v[b+8]||v[b+30]!==v[b+29]+v[b+8]||Math.abs((v[b+28]-s.W[d])+(v[b+30]-s.W[r]))>1e-6||!this.mass.fitsPatchInto(d,s.S[d],v[b+28],0,0)||!this.mass.fitsPatchInto(r,s.S[r],v[b+30],0,0))return false;s.W[d]=v[b+28];s.W[r]=v[b+30];}
      else if(k===6){
        if(d===r||Math.abs(d-r)!==1||!cwSameFiniteBits(s.bh[d],v[b+19])||!cwSameFiniteBits(s.mh[d],v[b+21])||!cwSameFiniteBits(s.bh[r],v[b+23])||!cwSameFiniteBits(s.mh[r],v[b+25])||v[b+9]!==Math.min(v[b+8],s.mh[d])||v[b+10]!==v[b+8]-v[b+9]||v[b+20]!==s.bh[d]-v[b+10]||v[b+22]!==s.mh[d]-v[b+9]||v[b+24]!==s.bh[r]||v[b+26]!==s.mh[r]+(v[b+9]+v[b+10]))return false;
        const sd=(((v[b+20]+0)+v[b+22])+0),sr=(((v[b+24]+0)+v[b+26])+0);
        this.guard.admitPatchInto(s,v[b+7],d,r,-1,1,true);if(this.guard.outcome.accepted<v[b+8]||!this.guard.actual(s,d,r,sd,sr)||!this.mass.fitsPatchInto(d,sd,s.W[d],0,0)||!this.mass.fitsPatchInto(r,sr,s.W[r],0,0)||Math.abs((sd-s.S[d])+(sr-s.S[r]))>1e-6||!this.guard.replayBudgetInto(v,b,d,r))return false;
        s.bh[d]=v[b+20];s.mh[d]=v[b+22];s.bh[r]=v[b+24];s.mh[r]=v[b+26];s.S[d]=sd;s.S[r]=sr;
      }else if(k===5){
        const mass=s.pool.pMass[i],q=v[b+8],nm=q===mass?0:mass-q,newMh=s.mh[d]+q,ss=(((s.bh[d]+0)+newMh)+0);
        if(d!==r||q<=0||q>mass||!cwSameFiniteBits(v[b+10],nm)||!cwSameFiniteBits(v[b+23],s.bh[d])||!cwSameFiniteBits(v[b+24],s.bh[d])||!cwSameFiniteBits(v[b+25],s.mh[d])||!cwSameFiniteBits(v[b+26],newMh)||Math.abs((ss-s.S[d])+(nm-mass))>1e-6)return false;
        this.guard.admitPatchInto(s,v[b+7],d,-1,1,0,true);if(this.guard.outcome.accepted<q||!this.guard.actual(s,d,-1,ss,0)||!this.mass.fitsPatchInto(d,ss,s.W[d],mass,nm)||!this.guard.replayBudgetInto(v,b,d,-1)||!this.mass.applyMassInto(d,mass,nm))return false;
        s.mh[d]=newMh;s.S[d]=ss;s.pool.pMass[i]=nm;
      }else if(k===3){const mass=s.pool.pMass[i];if(Math.abs(d-r)!==1||v[b+8]!==mass||!this.mass.fitsPatchInto(d,s.S[d],s.W[d],mass,0)||!this.mass.fitsPatchInto(r,s.S[r],s.W[r],0,mass)||!this.guard.replayBudgetInto(v,b,d,r)||!this.mass.applyMassInto(d,mass,0)||!this.mass.applyMassInto(r,0,mass))return false;}
      if(k<=5){const p=s.pool;
        if(k!==5&&v[b+10]!==v[b+9])return false;
        if(k<=4&&(v[b+19]<0||v[b+20]<0||v[b+21]<0||v[b+22]<0||v[b+22]>1||v[b+19]-v[b+21]!==v[b+20]))return false;
        p.pX[i]=v[b+15];p.pY[i]=v[b+16];p.pVx[i]=v[b+17];p.pVy[i]=v[b+18];p.pState[i]=bField(flags,2,2);s.waitReason![i]=bField(flags,7,3);
        if(k===5&&v[b+10]===0)this.particles.unlink(s,i,this.counters);
        else if(Math.floor(p.pX[i])!==r||p.pY[i]<s.S[r])return false;
      }
    }
    if(expectedLine!==a.lineageCount)return false;
    this.fold(s);const auditStart=this.readClock();this.counters[12]++;const valid=s.audit(marks,256)===S.OK&&this.bAudit(s)&&this.guard.finalInto(s,before)&&this.guard.auditBudget(true);
    const auditEnd=this.readClock();if(Number.isFinite(auditStart)&&Number.isFinite(auditEnd))a.header[49]=auditEnd-auditStart;return valid;
  }
  runInto(canonical:ClearWaterState,before:ClearWaterState,work:ClearWaterState,config:ClearWaterRunConfig,dt:number,angle:number,attempt:number,runGeneration:number,input:number,geometry:number,clockId:number,owner:number,marks:Uint8Array,out:ClearWaterStepResult):S {
    const a=this.candidate;this.stage=1;this.reason=BReason.NONE;this.counters[6]++;this.counters[9]++;
    this.clockStatus=0;this.wholeClock=this.readClock();this.segmentClock=this.wholeClock;
    if(!this.storageValid()||!canonical.bStorageValid()||!before.bStorageValid()||!work.bStorageValid())return this.fail(S.INVALID_VALUE,BReason.STORAGE_OR_ALIAS,out);
    if(!Number.isSafeInteger(this.serial+1)||!Number.isSafeInteger(canonical.physicsCommit+1))return this.fail(S.TOKEN_EXHAUSTED,BReason.TOKEN_EXHAUSTED,out);
    this.counters[10]++;if(canonical.audit(marks,256)!==S.OK)return this.fail(S.AUDIT_FAILED,BReason.WORK_AUDIT_FAILED,out);
    const entryEnd=this.readClock(),priorTrajectory=this.counters[13],priorFold=this.counters[14],priorBytes=this.counters[31];
    before.copyFrom(canonical);work.copyFrom(before);this.counters[30]+=2;this.counters[31]+=this.stateBytes(work)*2;a.reset();
    if(Number.isFinite(entryEnd)&&Number.isFinite(this.wholeClock))a.header[40]=entryEnd-this.wholeClock;this.segmentClock=entryEnd;this.tick(41);
    this.stage=2;if(!this.guard.beginStepInto(before,dt,angle)||!this.mass.buildFromStateInto(before,true))return this.fail(S.NUMERIC_UNREPRESENTABLE,this.mass.reason===0?BReason.LOCAL_NUMERIC_INVALID:this.mass.reason,out);
    this.tick(42);
    this.stage=3;let status=this.hydraulics.advanceWaterInto(work,config,dt,this.serial,this.guard,this.mass,a,this.counters);
    if(status!==S.OK)return this.fail(status,status===S.EVENT_FULL?a.reason:this.hydraulics.reason,out);
    this.tick(43);
    for(let c=0;c<128;c++){this.etaMedium[c]=work.S[c]+work.W[c];this.wetMask[c]=work.W[c]>0?1:0;}
    this.stage=4;status=this.particles.advanceInto(work,config,this.config,dt,this.etaMedium,this.guard,this.mass,a,this.counters);
    if(status!==S.OK)return this.fail(status,status===S.EVENT_FULL?(a.reason!==0?a.reason:this.particles.reason):this.particles.reason,out);
    this.tick(44);
    this.stage=5;status=this.hydraulics.advanceBedInto(work,this.config,dt,this.serial,this.guard,this.mass,a,this.counters);
    if(status!==S.OK)return this.fail(status,status===S.EVENT_FULL?a.reason:this.hydraulics.reason,out);
    this.tick(45);
    this.stage=6;status=this.particles.reconcileGeometryAndFoldInto(work,this.config,a,this.counters,this.etaMedium);if(status!==S.OK)return this.fail(status,status===S.EVENT_FULL?a.reason:this.particles.reason,out);
    this.tick(46);
    this.counters[11]++;if(work.audit(marks,256)!==S.OK||!this.bAudit(work)||Math.abs(work.waterTotal()-before.waterTotal())>1e-6||Math.abs(work.sandTotal()-before.sandTotal())>1e-6)return this.fail(S.AUDIT_FAILED,BReason.WORK_AUDIT_FAILED,out);
    if(!this.guard.finalInto(work,before)||!this.guard.auditBudget(false))return this.fail(S.AUDIT_FAILED,BReason.FINAL_GUARD_FAILED,out);
    this.tick(47);
    this.stage=7;if(!this.replayInto(before,a,marks))return this.fail(S.AUDIT_FAILED,BReason.REPLAY_OLD_BITS_MISMATCH,out);
    this.tick(48);
    if(!this.equalState(work,this.replay)||!this.equalArray(this.guard.workLocalRemaining,this.guard.replayLocalRemaining)||!this.equalArray(this.guard.workLocalUsed,this.guard.replayLocalUsed)||this.guard.scalars[2]!==this.guard.scalars[4]||this.guard.scalars[3]!==this.guard.scalars[5])return this.fail(S.AUDIT_FAILED,BReason.REPLAY_RESULT_MISMATCH,out);
    this.stage=8;if(!this.storageValid()||!canonical.bStorageValid()||canonical.sourceCommit!==before.sourceCommit||canonical.physicsCommit!==before.physicsCommit)return this.fail(S.AUDIT_FAILED,BReason.COMMIT_STORAGE_INVALID,out);
    const serial=this.serial+1,h=a.header;h[0]=serial;h[1]=a.count;h[2]=a.lineageCount;h[3]=before.physicsCommit+1;h[4]=before.sourceCommit;h[5]=runGeneration;h[6]=this.config.configRev;h[7]=input;h[8]=geometry;h[9]=serial;h[10]=attempt;h[11]=angle;h[12]=dt;h[13]=clockId;h[14]=owner;h[15]=this.config.e;h[16]=this.config.eventCapacity;h[17]=this.config.lineageCapacity;h[18]=128;h[19]=work.pool.nmax;h[28]=this.guard.scalars[11];h[29]=this.guard.scalars[12];h[39]=1;
    work.physicsCommit++;const bh=work.bHeader!;bh[0]=serial;bh[1]=angle;bh[2]=dt;bh[3]=work.physicsCommit;bh[4]=work.sourceCommit;bh[5]=serial;bh[6]=a.count;bh[7]=a.lineageCount;bh[8]=runGeneration;bh[9]=this.config.configRev;bh[10]=input;bh[11]=geometry;bh[12]=h[28];bh[13]=h[29];bh[14]=attempt;
    // No external callbacks from the protected copy until sealed publication.
    // All buffers have been preflighted; copyFrom uses only fixed .set calls.
    canonical.copyFrom(work);this.counters[30]++;this.counters[31]+=this.stateBytes(work);this.guard.sealEntry(before);this.serial=serial;this.sealedIndex=a===this.arena0?0:1;this.counters[7]++;this.counters[25]+=a.lineageCount;
    // Normalize non-authoritative patch scratch on a sealed boundary. Cold
    // snapshots can reconstruct the sealed budget from event debits even after
    // a later failed candidate overwrote the working scalars/local arrays.
    for(let j=13;j<32;j++)this.guard.scalars[j]=0;
    h[20]=127;h[21]=127;h[22]=before.pool.liveCount;h[23]=this.counters[13]-priorTrajectory;h[24]=this.counters[14]-priorFold;h[25]=a.count;h[26]=3;h[27]=this.counters[31]-priorBytes;h[30]=this.counters[35];h[31]=this.counters[36];h[32]=this.counters[37];h[33]=this.counters[38];h[34]=this.counters[1];h[35]=this.counters[2];h[36]=this.counters[5];h[37]=this.counters[6];h[38]=1;
    const commitEnd=this.readClock();if(Number.isFinite(commitEnd)&&Number.isFinite(this.segmentClock))h[50]=commitEnd-this.segmentClock;if(Number.isFinite(commitEnd)&&Number.isFinite(this.wholeClock))h[51]=commitEnd-this.wholeClock;h[52]=this.frameSourceMs;h[53]=this.frameEnqueueMs;h[54]=this.frameExportMs;h[55]=this.clockStatus;
    if(this.clockStatus>1)for(let j=40;j<=54;j++)h[j]=NaN;
    out.status=S.OK;out.stepped=true;out.failedAttempt=null;out.lastAcceptedCommit=canonical.physicsCommit;out.stepReason=0;out.stage=8;out.lastBStepSerial=serial;out.arenaSerial=serial;out.eventCount=a.count;out.lineageCount=a.lineageCount;out.maxResidual=h[28];out.allEdgesWithinH=h[29]===1;return S.OK;
  }
  private equalArray(a:Float64Array,b:Float64Array):boolean {if(a.length!==b.length)return false;for(let i=0;i<a.length;i++)if(!cwSameFiniteBits(a[i],b[i]))return false;return true;}
  private equalState(a:ClearWaterState,b:ClearWaterState):boolean {
    if(!this.equalArray(a.W,b.W)||!this.equalArray(a.bh,b.bh)||!this.equalArray(a.mh,b.mh)||!this.equalArray(a.S,b.S)||!this.equalArray(a.P,b.P)||!this.equalArray(a.eta,b.eta)||!this.equalArray(a.R,b.R)||!this.equalArray(a.originSourceCommit!,b.originSourceCommit!)||!this.equalArray(a.bHeader!,b.bHeader!))return false;
    const x=a.pool,y=b.pool;for(let i=0;i<x.nmax;i++)if(!cwSameFiniteBits(x.pMass[i],y.pMass[i])||!cwSameFiniteBits(x.pX[i],y.pX[i])||!cwSameFiniteBits(x.pY[i],y.pY[i])||!cwSameFiniteBits(x.pVx[i],y.pVx[i])||!cwSameFiniteBits(x.pVy[i],y.pVy[i])||x.pState[i]!==y.pState[i]||x.pId[i]!==y.pId[i]||x.pSourceId[i]!==y.pSourceId[i]||x.pPhase[i]!==y.pPhase[i]||x.pSource[i]!==y.pSource[i]||x.freeSlots[i]!==y.freeSlots[i]||x.prev[i]!==y.prev[i]||x.next[i]!==y.next[i]||a.waitReason![i]!==b.waitReason![i])return false;
    return x.head===y.head&&x.tail===y.tail&&x.nextId===y.nextId&&x.freeCount===y.freeCount&&x.liveCount===y.liveCount&&a.sourceCommit===b.sourceCommit&&a.physicsCommit===b.physicsCommit&&a.initialWater===b.initialWater&&a.initialSand===b.initialSand&&a.I_water===b.I_water&&a.I_sand===b.I_sand&&a.sandSuccessCount===b.sandSuccessCount&&a.waterSuccessCount===b.waterSuccessCount&&this.equalArray(x.colParticleMassH,y.colParticleMassH)&&this.equalArray(x.colParticleMassL,y.colParticleMassL);
  }
  stateBytes(s:ClearWaterState):number {return 63*s.pool.nmax+72*128+9*s.pool.nmax+128;}
  copyCounters(out:ClearWaterBFrameCounters):void {out.values.set(this.counters);out.frameId=this.frameId;out.scope=this.fullScope?'COMPLETE_EXPLICIT_FRAME':'PARTIAL_SCOPE';out.K=this.replay.pool.nmax;out.e=this.config.e;out.F=this.counters[6];out.Q=this.counters[2];out.B=this.counters[5];out.L=this.counters[18]+this.counters[19]+this.counters[20]+this.counters[21]+this.counters[22];}
  allocations():ClearWaterAllocation[] {
    const out=this.replay.allocations('replay'),g=this.guard,m=this.mass;
    out.push(bAllocation('etaMedium','Float64Array',128,1024),bAllocation('wetMask','Uint8Array',128,128),bAllocation('pExact70','Uint16Array',8960,17920),bAllocation('capacityBound70','Uint16Array',8960,17920),bAllocation('massScratch0','Uint16Array',70,140),bAllocation('massScratch1','Uint16Array',70,140),bAllocation('massBitScratch','Uint8Array',8,8));
    const arrays:Float64Array[]=[g.workLocalRemaining,g.workLocalUsed,g.replayLocalRemaining,g.replayLocalUsed,g.entryResidual,g.entryGuardBound,g.scalars,this.arena0.events,this.arena1.events,this.arena0.lineage,this.arena1.lineage,this.arena0.header,this.arena1.header,this.arena0.summary,this.arena1.summary,this.counters];
    const names:string[]=['workLocalRemaining','workLocalUsed','replayLocalRemaining','replayLocalUsed','entryResidual','entryGuardBound','budgetScalars','arena0.event40','arena1.event40','arena0.lineage3','arena1.lineage3','arena0.header','arena1.header','plan0','plan1','counters'];
    for(let i=0;i<arrays.length;i++)out.push(bAllocation(names[i],'Float64Array',arrays[i].length,arrays[i].byteLength));out.push(bAllocation('sourceIntervals','Uint8Array',65536,65536));return out;
  }
}

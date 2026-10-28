import { ClearWaterRunConfig, ClearWaterInitialState, ClearWaterStatus as S, ClearWaterSourceRequest, ClearWaterSourceReceipt,
  ClearWaterStepResult, ClearWaterAllocation, CW_MEDIUM, CW_ROUTE, cwConfigValid, cwSafePositive } from './P011ClearWaterTypes';
import { ClearWaterState } from './P011ClearWaterState';
import { ClearWaterSource } from './P011ClearWaterSource';
import { ClearWaterBStep } from './P011ClearWaterStep';
import { ClearWaterBConfig,ClearWaterBAttachResult,ClearWaterBEvidenceBuffers,ClearWaterBFrameCounters,BReason,cwBConfigValid } from './P011ClearWaterBTypes';
let clearWaterOwnerSerial: number = 0;
export interface ClearWaterBoundaryIdentity { runId: string; runGeneration: number; configRev: number; inputGeneration: number; geometryEpoch: number; }
export interface ClearWaterClockWindow { id: number; previous: number; current: number; unit: string; inputGeneration: number; geometryEpoch: number; }
export class ClearWaterBatchResult {
  processed: number=0;accepted: number=0;rejected: number=0;status: S=S.OK;lastSourceCommit: number=0;lastPhysicsCommit: number=0;
}
export interface ClearWaterSnapshot {
  revision: string; mediumMode: string; routeId: string; runId: string; runGeneration: number;
  config: ClearWaterRunConfig; inputGeneration: number; geometryEpoch: number; state: ClearWaterState;
  queue: Float64Array; dedup: Float64Array; events: Float64Array; pendingCount: number; dedupCount: number; eventCount: number;
  queueHead: number; enqueueOrdinal: number; clockWindowId: number; clockRaw: number; clockUnit: string; failedAttempt: number;
  implementation: string;
}
export interface ClearWaterAllocationLedger {
  revision: string; arrays: ClearWaterAllocation[]; payloadBytes: number; coldSnapshotExtraBytes: number;
  queueRows: number; dedupRows: number; eventRows: number; excluded: string[]; notImplemented: string[];
}
export interface ClearWaterSnapshotB {
  revision:string;A:ClearWaterSnapshot;BConfig:ClearWaterBConfig;originSourceCommit:Float64Array;waitReason:Uint8Array;bHeader:Float64Array;
  arenaHeader:Float64Array;events:Float64Array;lineage:Float64Array;localRemaining:Float64Array;localUsed:Float64Array;entryResidual:Float64Array;entryGuardBound:Float64Array;budgetScalars:Float64Array;
  implementation:string;payloadBytes:number;
}
export class P011ClearWaterEngine {
  private b:ClearWaterBStep|null=null;private frameOpen:boolean=false;private bOwnedBuffers:ArrayBuffer[]=[];
  readonly mediumMode: string=CW_MEDIUM;readonly routeId: string=CW_ROUTE;
  private readonly config: ClearWaterRunConfig;
  private readonly canonical: ClearWaterState;private readonly sourceBefore: ClearWaterState;private readonly stepBefore: ClearWaterState;private readonly work: ClearWaterState;
  private readonly sources: ClearWaterSource;private readonly marks: Uint8Array;
  private readonly lastReceipt: ClearWaterSourceReceipt=new ClearWaterSourceReceipt();
  private run: string='';private generation: number=0;private inputGeneration: number=1;private geometry: number=1;
  private readonly owner: number;private busy: boolean=false;private failedAttempt: number=0;
  private clockId: number=0;private clockRaw: number=0;private clockUnit: string='';
  get runId(): string {return this.run;}get runGeneration(): number {return this.generation;}
  get currentInputGeneration(): number {return this.inputGeneration;}get geometryEpoch(): number {return this.geometry;}
  get pendingSourceCount(): number {return this.sources.pendingCount;}
  constructor(config: ClearWaterRunConfig, initial: ClearWaterInitialState | null = null){
    if(!cwConfigValid(config)||!cwSafePositive(clearWaterOwnerSerial+1))throw new Error('CW_INVALID_CONFIG');
    this.config=JSON.parse(JSON.stringify(config)) as ClearWaterRunConfig;this.owner=++clearWaterOwnerSerial;
    this.canonical=new ClearWaterState(config.particleCapacity);this.sourceBefore=new ClearWaterState(config.particleCapacity);
    this.stepBefore=new ClearWaterState(config.particleCapacity);this.work=new ClearWaterState(config.particleCapacity);
    this.sources=new ClearWaterSource(this.config);this.marks=new Uint8Array(config.particleCapacity);
    const status=this.resetClearWaterInto(initial);if(status!==S.OK)throw new Error('CW_INVALID_INITIAL_'+status);
  }
  resetClearWaterInto(initial: ClearWaterInitialState | null = null): S {
    if(this.busy)return S.REENTRANT;
    if(this.b!==null&&(!this.b.storageValid()||!this.canonical.bStorageValid()||!this.sourceBefore.bStorageValid()||!this.stepBefore.bStorageValid()||!this.work.bStorageValid()))return S.INVALID_VALUE;
    if(!cwSafePositive(this.generation+1))return S.TOKEN_EXHAUSTED;
    if(initial!==null&&(initial.W.length!==128||initial.bh.length!==128||initial.mh.length!==128))return S.INVALID_VALUE;
    this.busy=true;try{
      this.work.reset(initial);
      if(this.work.audit(this.marks,this.config.capacity)!==S.OK)return S.AUDIT_FAILED;
      this.canonical.copyFrom(this.work);this.sourceBefore.copyFrom(this.work);this.stepBefore.copyFrom(this.work);
      this.sources.reset();if(this.b!==null)this.b.reset();this.frameOpen=false;this.generation++;this.inputGeneration=this.geometry=1;this.clockId=0;this.clockRaw=0;this.clockUnit='';this.failedAttempt=0;
      this.run='cw-run-'+Date.now()+'-'+this.owner+'-'+this.generation;return S.OK;
    }finally{this.busy=false;}
  }
  bindSourceClockWindow(window: ClearWaterClockWindow): S {
    if(this.busy)return S.REENTRANT;
    if(window.inputGeneration!==this.inputGeneration)return S.STALE_INPUT_GENERATION;
    if(window.geometryEpoch!==this.geometry)return S.STALE_GEOMETRY;
    if(!cwSafePositive(window.id)||window.id<=this.clockId||!Number.isFinite(window.previous)||!Number.isFinite(window.current)||window.previous<0||window.current<=window.previous||
      (window.unit!=='NS'&&window.unit!=='MS'))return S.INVALID_TIME_WINDOW;
    const seconds=(window.current-window.previous)/(window.unit==='NS'?1e9:1000);
    if(!Number.isFinite(seconds)||seconds<=0||seconds>1/30)return S.INVALID_TIME_WINDOW;
    if(this.clockId!==0&&(window.unit!==this.clockUnit||window.previous!==this.clockRaw))return S.INVALID_TIME_WINDOW;
    this.clockId=window.id;this.clockRaw=window.current;this.clockUnit=window.unit;return S.OK;
  }
  enqueueSourceInto(request: ClearWaterSourceRequest, out: ClearWaterSourceReceipt): S {
    if(this.busy){out.status=S.REENTRANT;return S.REENTRANT;}
    if(this.b!==null&&(!this.b.storageValid()||!this.canonical.bStorageValid()||!this.sourceBefore.bStorageValid()||!this.work.bStorageValid()))return out.status=S.INVALID_VALUE;
    if(this.b!==null){this.b.counters[0]++;if(!this.frameOpen)this.b.fullScope=false;}
    this.busy=true;const started=this.b===null?NaN:this.b.readClock();try{return this.sources.enqueue(request,this.run,this.generation,this.inputGeneration,this.geometry,this.clockId,this.clockRaw,this.clockUnit,this.canonical,out);}finally{if(this.b!==null)this.b.scopeTime(0,started,this.b.readClock());this.busy=false;}
  }
  commitQueuedSourcesInto(boundary: ClearWaterBoundaryIdentity, out: ClearWaterBatchResult): S {
    out.processed=out.accepted=out.rejected=0;out.status=S.OK;out.lastSourceCommit=this.canonical.sourceCommit;out.lastPhysicsCommit=this.canonical.physicsCommit;
    if(this.busy)return out.status=S.REENTRANT;
    if(boundary.runId!==this.run||boundary.runGeneration!==this.generation)return out.status=S.STALE_RUN;
    if(boundary.configRev!==this.config.configRev)return out.status=S.INVALID_IDENTITY;
    if(boundary.inputGeneration!==this.inputGeneration)return out.status=S.STALE_INPUT_GENERATION;
    if(boundary.geometryEpoch!==this.geometry)return out.status=S.STALE_GEOMETRY;
    if(this.b!==null&&(!this.b.storageValid()||!this.canonical.bStorageValid()||!this.sourceBefore.bStorageValid()||!this.work.bStorageValid()))return out.status=S.INVALID_VALUE;
    this.busy=true;const started=this.b===null?NaN:this.b.readClock();try{
      const count=this.sources.pendingCount;if(this.b!==null){this.b.counters[1]++;if(!this.frameOpen)this.b.fullScope=false;}
      for(let i=0;i<count;i++){
        const bornBefore=this.canonical.pool.liveCount;
        const status=this.sources.commitNext(this.run,this.inputGeneration,this.geometry,this.canonical,this.sourceBefore,this.work,this.marks,this.lastReceipt);
        if(this.b!==null){this.b.counters[2]++;this.b.counters[status===S.OK||status===S.PARTIAL_CAPACITY?3:4]++;this.b.counters[5]+=this.canonical.pool.liveCount-bornBefore;}
        out.processed++;if(status===S.OK||status===S.PARTIAL_CAPACITY)out.accepted++;else{out.rejected++;out.status=status;}
      }
      out.lastSourceCommit=this.canonical.sourceCommit;out.lastPhysicsCommit=this.canonical.physicsCommit;return out.status;
    }finally{if(this.b!==null)this.b.scopeTime(1,started,this.b.readClock());this.busy=false;}
  }
  stopSourceGeneration(reason: string, nextInputGeneration: number, geometryEpoch: number = this.geometry): S {
    if(this.busy)return S.REENTRANT;
    if(!cwSafePositive(nextInputGeneration)||nextInputGeneration<=this.inputGeneration)return S.INVALID_IDENTITY;
    if(!cwSafePositive(geometryEpoch)||geometryEpoch<this.geometry)return S.STALE_GEOMETRY;
    this.sources.clearPending();this.inputGeneration=nextInputGeneration;this.geometry=geometryEpoch;this.clockId=0;this.clockRaw=0;this.clockUnit='';return S.OK;
  }
  advanceSubstepInto(dt: number, acceptedAngle: number, out: ClearWaterStepResult): S {
    out.stepRevision='CW_B_STEP_V1';out.stepReason=0;out.stage=0;out.ordinal=0;out.sourceCommitAtEntry=this.canonical.sourceCommit;out.acceptedAngleDeg=acceptedAngle;out.lastBStepSerial=this.b===null?0:this.b.serial;out.arenaSerial=this.b===null||this.b.sealed===null?0:this.b.sealed!.header[0];out.eventCount=0;out.lineageCount=0;
    out.stepped=false;out.lastAcceptedCommit=this.canonical.physicsCommit;out.failedAttempt=null;out.implementation='NOT_IMPLEMENTED_B_COUPLING';
    if(this.busy)return out.status=S.REENTRANT;
    if(!Number.isFinite(dt)||dt<0||dt>1/60||!Number.isFinite(acceptedAngle)||Math.abs(acceptedAngle)>30)return out.status=S.INVALID_VALUE;
    if(this.b!==null)out.implementation='CW_B_STEP_V1';
    if(dt===0)return out.status=S.OK;
    if(this.b!==null&&!this.frameOpen)this.b.fullScope=false;
    if(this.b!==null&&this.frameOpen&&this.b.counters[6]>=2)return out.status=S.INVALID_VALUE;
    if(!cwSafePositive(this.failedAttempt+1))return out.status=S.TOKEN_EXHAUSTED;
    if(this.b!==null){
      out.failedAttempt=++this.failedAttempt;out.implementation='CW_B_STEP_V1';this.busy=true;
      try{return this.b.runInto(this.canonical,this.stepBefore,this.work,this.config,dt,acceptedAngle,this.failedAttempt,this.generation,this.inputGeneration,this.geometry,this.clockId,this.owner,this.marks,out);}
      catch(error){out.status=S.AUDIT_FAILED;out.stepReason=BReason.COMMIT_STORAGE_INVALID;out.stage=this.b.stage;out.ordinal=this.b.candidate.count+1;return out.status;}
      finally{this.busy=false;}
    }
    this.stepBefore.copyFrom(this.canonical);out.failedAttempt=++this.failedAttempt;
    // No partially implemented hydraulic/particle writer can run in phase A.
    // The source commit is outside this step rollback boundary and survives.
    return out.status=S.UNSUPPORTED_FEATURE;
  }
  audit(): S {return this.canonical.audit(this.marks,this.config.capacity);}
  snapshot():ClearWaterSnapshot {
    if(this.b===null)return this.copyASnapshot();if(this.busy)throw new Error('CW_B_SNAPSHOT_BUSY');
    this.busy=true;const started=this.b.readClock();try{const result=this.copyASnapshot();this.b.counters[45]+=63*this.config.particleCapacity+80*128+this.sources.queue.byteLength+this.sources.dedup.byteLength+this.sources.events.byteLength;return result;}
    finally{this.b.scopeTime(2,started,this.b.readClock());this.busy=false;}
  }
  private copyASnapshot(): ClearWaterSnapshot {
    const state=new ClearWaterState(this.config.particleCapacity);state.copyFrom(this.canonical);
    return {revision:'CLEAR_WATER_SNAPSHOT_V1',mediumMode:CW_MEDIUM,routeId:CW_ROUTE,runId:this.run,runGeneration:this.generation,
      config:JSON.parse(JSON.stringify(this.config)) as ClearWaterRunConfig,inputGeneration:this.inputGeneration,geometryEpoch:this.geometry,state:state,
      queue:this.sources.queue.slice(),dedup:this.sources.dedup.slice(),events:this.sources.events.slice(),pendingCount:this.sources.pendingCount,dedupCount:this.sources.dedupCount,eventCount:this.sources.eventCount,
      queueHead:this.sources.queueHead,enqueueOrdinal:this.sources.enqueueOrdinal,clockWindowId:this.clockId,clockRaw:this.clockRaw,clockUnit:this.clockUnit,failedAttempt:this.failedAttempt,
      implementation:this.b===null?'A_SOURCE_INVENTORY_ONLY;B_COUPLING_AND_C_RENDER_NOT_IMPLEMENTED':'A_VIEW_ONLY_B_METADATA_OMITTED;NOT_A_B_RESTORE_PACKAGE;C_RENDER_NOT_IMPLEMENTED'};
  }
  allocationLedger(): ClearWaterAllocationLedger {
    const arrays=this.canonical.allocations('canonical').concat(this.sourceBefore.allocations('sourceBefore'),this.stepBefore.allocations('stepBefore'),this.work.allocations('work'),this.sources.allocations());
    if(this.b!==null)arrays.push(...this.b.allocations());
    arrays.push({name:'engine.auditMarks',type:'Uint8Array',length:this.marks.length,byteLength:this.marks.byteLength,lifetime:'COLD_RUN_FIXED'});
    let total=0;for(const row of arrays)total+=row.byteLength;
    const snapshotExtra=63*this.config.particleCapacity+80*128+this.sources.queue.byteLength+this.sources.dedup.byteLength+this.sources.events.byteLength;
    return {revision:'CW_ALLOCATION_LEDGER_A_V1',arrays:arrays,payloadBytes:total,coldSnapshotExtraBytes:snapshotExtra,
      queueRows:this.config.queueCapacity,dedupRows:this.config.dedupCapacity,eventRows:this.config.eventCapacity,
      excluded:['JS_OBJECT_STRING_VIEW_HEADER_AND_ALIGNMENT_ENGINE_DEPENDENT','COLD_RETURN_OBJECTS_AND_CALLER_RETAINED_SNAPSHOTS','NO_NATIVE_GPU_DRIVER_RESOURCES_ALLOCATED_BY_A'],
      notImplemented:this.b===null?['B_HYDRAULICS_SEGMENTED_PARTICLES_GUARD_EVENTS_LINEAGE','C_PUBLICATION_RENDER_INPUT_PAGE','SNAPSHOT_RESTORE_NO_PARTIAL_RESTORE_API']:['C_PUBLICATION_RENDER_INPUT_PAGE','SNAPSHOT_RESTORE_NO_PARTIAL_RESTORE_API']};
  }
  sourceLayoutManifest(): string {return this.sources.layoutManifest();}
  attachBInto(config:ClearWaterBConfig,out:ClearWaterBAttachResult):S {
    out.status=S.OK;out.reason=BReason.NONE;out.payloadBytes=0;
    if(this.busy)return out.status=S.REENTRANT;
    if(this.b!==null||!cwBConfigValid(config,this.config.particleCapacity)){out.reason=BReason.INVALID_CONFIG;return out.status=S.INVALID_CONFIG;}
    this.busy=true;
    try{
      if(this.canonical.audit(this.marks,256)!==S.OK){out.reason=BReason.WORK_AUDIT_FAILED;return out.status=S.AUDIT_FAILED;}
      const runtime=new ClearWaterBStep(config,this.config.particleCapacity),origins=runtime.origins;
      for(let n=0;n<this.sources.eventCount;n++){
        const e=n*24,v=this.sources.events;if(v[e+1]===1)continue;
        if(v[e+1]!==0||v[e+7]<=0||!Number.isInteger(v[e+7])||!origins.append(v[e+6],v[e+6]+v[e+7]-1,v[e])){out.reason=BReason.ORIGIN_UNRESOLVED;return out.status=S.INVALID_IDENTITY;}
      }
      // Resolve against controlled source event24 before installing any B data.
      let slot=this.canonical.pool.head;while(slot!==-1){if(origins.find(this.canonical.pool.pId[slot])===0){out.reason=BReason.ORIGIN_UNRESOLVED;return out.status=S.INVALID_IDENTITY;}slot=this.canonical.pool.next[slot];}
      this.canonical.enableB();this.sourceBefore.enableB();this.stepBefore.enableB();this.work.enableB();
      slot=this.canonical.pool.head;while(slot!==-1){this.canonical.originSourceCommit![slot]=origins.find(this.canonical.pool.pId[slot]);this.canonical.waitReason![slot]=this.canonical.pool.pState[slot]===2?1:0;slot=this.canonical.pool.next[slot];}
      this.sourceBefore.copyFrom(this.canonical);this.stepBefore.copyFrom(this.canonical);this.work.copyFrom(this.canonical);runtime.replay.copyFrom(this.canonical);
      this.canonical.bindBStorage();this.sourceBefore.bindBStorage();this.stepBefore.bindBStorage();this.work.bindBStorage();
      this.bOwnedBuffers=this.canonical.bStorageBuffers().concat(this.sourceBefore.bStorageBuffers(),this.stepBefore.bStorageBuffers(),this.work.bStorageBuffers(),runtime.replay.bStorageBuffers(),runtime.ownedBuffers,
        [this.sources.queue.buffer as ArrayBuffer,this.sources.dedup.buffer as ArrayBuffer,this.sources.events.buffer as ArrayBuffer,this.marks.buffer as ArrayBuffer]);
      this.sources.bOrigins=origins;this.sources.bCounters=runtime.counters;this.b=runtime;out.payloadBytes=this.allocationLedger().payloadBytes;return S.OK;
    }catch(error){
      // No authority stocks/queue/IDs were modified by cold installation.
      this.canonical.originSourceCommit=this.sourceBefore.originSourceCommit=this.stepBefore.originSourceCommit=this.work.originSourceCommit=null;
      this.canonical.waitReason=this.sourceBefore.waitReason=this.stepBefore.waitReason=this.work.waitReason=null;
      this.canonical.bHeader=this.sourceBefore.bHeader=this.stepBefore.bHeader=this.work.bHeader=null;
      this.sources.bOrigins=null;this.sources.bCounters=null;this.b=null;out.reason=BReason.STORAGE_OR_ALIAS;return out.status=S.INVALID_CONFIG;
    }finally{this.busy=false;}
  }
  setBClockInto(clock:(()=>number)|null):S {if(this.busy)return S.REENTRANT;if(this.b===null)return S.UNSUPPORTED_FEATURE;this.b.setClock(clock);return S.OK;}
  beginBFrameInto(frameId:number):S {
    if(this.busy)return S.REENTRANT;if(this.b===null)return S.UNSUPPORTED_FEATURE;
    if(this.frameOpen||!Number.isSafeInteger(frameId)||frameId<=this.b.frameId)return S.INVALID_IDENTITY;
    this.b.counters.fill(0);this.b.clearScopeTimes();this.b.frameId=frameId;this.b.fullScope=false;this.frameOpen=true;return S.OK;
  }
  endBFrameInto(frameId:number):S {
    if(this.busy)return S.REENTRANT;if(this.b===null)return S.UNSUPPORTED_FEATURE;if(!this.frameOpen||frameId!==this.b.frameId)return S.INVALID_IDENTITY;
    this.frameOpen=false;this.b.fullScope=true;return S.OK;
  }
  copyBFrameCountersInto(out:ClearWaterBFrameCounters):S {
    if(this.busy)return S.REENTRANT;if(this.b===null)return S.UNSUPPORTED_FEATURE;
    if(out.values.length!==48)return S.INVALID_VALUE;for(let j=0;j<this.bOwnedBuffers.length;j++)if(out.values.buffer===this.bOwnedBuffers[j])return S.INVALID_VALUE;this.b.copyCounters(out);return S.OK;
  }
  copyLastBEventsInto(out:ClearWaterBEvidenceBuffers):S {
    if(this.busy)return S.REENTRANT;if(this.b===null||this.b.sealed===null)return S.UNSUPPORTED_FEATURE;
    const a=this.b.sealed;
    if(out.expectedOwnerId!==this.owner||out.expectedRunGeneration!==this.generation||out.expectedArenaSerial!==a.header[0])return S.INVALID_IDENTITY;
    if(out.header.length!==64||out.events.length!==a.events.length||out.lineage.length!==a.lineage.length||out.summary.length!==a.summary.length)return S.INVALID_VALUE;
    // Destinations must be independent of every owner buffer and each other.
    if(this.bOutputAliases(out))return S.INVALID_VALUE;
    out.header.set(a.header);out.events.set(a.events);out.lineage.set(a.lineage);out.summary.set(a.summary);out.currentPhysicsCommit=this.canonical.physicsCommit;out.currentSourceCommit=this.canonical.sourceCommit;this.b.counters[45]+=a.header.byteLength+a.events.byteLength+a.lineage.byteLength+a.summary.byteLength;return S.OK;
  }
  private bOutputAliases(out:ClearWaterBEvidenceBuffers):boolean {
    const h=out.header.buffer,e=out.events.buffer,l=out.lineage.buffer,p=out.summary.buffer;
    if(h===e||h===l||h===p||e===l||e===p||l===p)return true;
    for(let j=0;j<this.bOwnedBuffers.length;j++){const buffer=this.bOwnedBuffers[j];if(buffer===h||buffer===e||buffer===l||buffer===p)return true;}return false;
  }
  snapshotB():ClearWaterSnapshotB {
    if(this.busy||this.b===null)throw new Error('CW_B_SNAPSHOT_UNAVAILABLE');this.busy=true;const started=this.b.readClock();try{const a=this.b.sealed;
    const header=a===null?new Float64Array(64):a.header.slice(),events=a===null?new Float64Array(this.b.config.eventCapacity*40):a.events.slice(),lineage=a===null?new Float64Array(this.b.config.lineageCapacity*3):a.lineage.slice();
    const remaining=new Float64Array(128).fill(a===null?0:28*header[12]),used=new Float64Array(128),scalars=new Float64Array(32);
    if(a!==null){scalars[0]=header[12];scalars[1]=scalars[2]=300*header[12];scalars[6]=header[11];scalars[7]=header[11]*Math.PI/180;scalars[8]=Math.tan(scalars[7]);scalars[9]=Math.sin(scalars[7]);scalars[10]=Math.cos(scalars[7]);
      for(let n=0;n<a.count;n++){const o=n*40,k=events[o];if(k===3||k===5||k===6){const d=events[o+5],r=events[o+6],q=events[o+8],reserved=Math.min(events[o+7],events[o+31],events[o+33],k===5?events[o+7]:events[o+35]),refund=reserved-q;remaining[d]=events[o+34];used[d]=(used[d]+reserved)-refund;if(k!==5){remaining[r]=events[o+36];used[r]=(used[r]+reserved)-refund;}scalars[2]=events[o+32];scalars[3]=(scalars[3]+reserved)-refund;}}scalars[4]=scalars[2];scalars[5]=scalars[3];scalars[11]=header[28];scalars[12]=header[29];}
    const A=this.copyASnapshot(),k=this.config.particleCapacity,payload=this.allocationLedger().coldSnapshotExtraBytes+9*k+128+512+events.byteLength+lineage.byteLength+4*1024+256;
    this.b.counters[45]+=payload;
    return {revision:'CLEAR_WATER_SNAPSHOT_B_V1',A:A,BConfig:JSON.parse(JSON.stringify(this.b.config)) as ClearWaterBConfig,originSourceCommit:this.canonical.originSourceCommit!.slice(),waitReason:this.canonical.waitReason!.slice(),bHeader:this.canonical.bHeader!.slice(),
      arenaHeader:header,events:events,lineage:lineage,localRemaining:remaining,localUsed:used,entryResidual:this.b.guard.entryResidual.slice(),entryGuardBound:this.b.guard.entryGuardBound.slice(),budgetScalars:scalars,implementation:'CW_B_STEP_V1;C_AND_RESTORE_NOT_IMPLEMENTED',payloadBytes:payload};
    }finally{this.b.scopeTime(2,started,this.b.readClock());this.busy=false;}
  }
  restoreBInto(snapshot:ClearWaterSnapshotB):S {return S.UNSUPPORTED_FEATURE;}
}

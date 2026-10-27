import { ClearWaterRunConfig, ClearWaterInitialState, ClearWaterStatus as S, ClearWaterSourceRequest, ClearWaterSourceReceipt,
  ClearWaterStepResult, ClearWaterAllocation, CW_MEDIUM, CW_ROUTE, cwConfigValid, cwSafePositive } from './P011ClearWaterTypes';
import { ClearWaterState } from './P011ClearWaterState';
import { ClearWaterSource } from './P011ClearWaterSource';
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
export class P011ClearWaterEngine {
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
    if(!cwSafePositive(this.generation+1))return S.TOKEN_EXHAUSTED;
    if(initial!==null&&(initial.W.length!==128||initial.bh.length!==128||initial.mh.length!==128))return S.INVALID_VALUE;
    this.busy=true;try{
      this.work.reset(initial);
      if(this.work.audit(this.marks,this.config.capacity)!==S.OK)return S.AUDIT_FAILED;
      this.canonical.copyFrom(this.work);this.sourceBefore.copyFrom(this.work);this.stepBefore.copyFrom(this.work);
      this.sources.reset();this.generation++;this.inputGeneration=this.geometry=1;this.clockId=0;this.clockRaw=0;this.clockUnit='';this.failedAttempt=0;
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
    this.busy=true;try{return this.sources.enqueue(request,this.run,this.generation,this.inputGeneration,this.geometry,this.clockId,this.clockRaw,this.clockUnit,this.canonical,out);}finally{this.busy=false;}
  }
  commitQueuedSourcesInto(boundary: ClearWaterBoundaryIdentity, out: ClearWaterBatchResult): S {
    out.processed=out.accepted=out.rejected=0;out.status=S.OK;out.lastSourceCommit=this.canonical.sourceCommit;out.lastPhysicsCommit=this.canonical.physicsCommit;
    if(this.busy)return out.status=S.REENTRANT;
    if(boundary.runId!==this.run||boundary.runGeneration!==this.generation)return out.status=S.STALE_RUN;
    if(boundary.configRev!==this.config.configRev)return out.status=S.INVALID_IDENTITY;
    if(boundary.inputGeneration!==this.inputGeneration)return out.status=S.STALE_INPUT_GENERATION;
    if(boundary.geometryEpoch!==this.geometry)return out.status=S.STALE_GEOMETRY;
    this.busy=true;try{
      const count=this.sources.pendingCount;
      for(let i=0;i<count;i++){
        const status=this.sources.commitNext(this.run,this.inputGeneration,this.geometry,this.canonical,this.sourceBefore,this.work,this.marks,this.lastReceipt);
        out.processed++;if(status===S.OK||status===S.PARTIAL_CAPACITY)out.accepted++;else{out.rejected++;out.status=status;}
      }
      out.lastSourceCommit=this.canonical.sourceCommit;out.lastPhysicsCommit=this.canonical.physicsCommit;return out.status;
    }finally{this.busy=false;}
  }
  stopSourceGeneration(reason: string, nextInputGeneration: number, geometryEpoch: number = this.geometry): S {
    if(this.busy)return S.REENTRANT;
    if(!cwSafePositive(nextInputGeneration)||nextInputGeneration<=this.inputGeneration)return S.INVALID_IDENTITY;
    if(!cwSafePositive(geometryEpoch)||geometryEpoch<this.geometry)return S.STALE_GEOMETRY;
    this.sources.clearPending();this.inputGeneration=nextInputGeneration;this.geometry=geometryEpoch;this.clockId=0;this.clockRaw=0;this.clockUnit='';return S.OK;
  }
  advanceSubstepInto(dt: number, acceptedAngle: number, out: ClearWaterStepResult): S {
    out.stepped=false;out.lastAcceptedCommit=this.canonical.physicsCommit;out.failedAttempt=null;out.implementation='NOT_IMPLEMENTED_B_COUPLING';
    if(this.busy)return out.status=S.REENTRANT;
    if(!Number.isFinite(dt)||dt<0||dt>1/60||!Number.isFinite(acceptedAngle)||Math.abs(acceptedAngle)>30)return out.status=S.INVALID_VALUE;
    if(dt===0)return out.status=S.OK;
    if(!cwSafePositive(this.failedAttempt+1))return out.status=S.TOKEN_EXHAUSTED;
    this.stepBefore.copyFrom(this.canonical);out.failedAttempt=++this.failedAttempt;
    // No partially implemented hydraulic/particle writer can run in phase A.
    // The source commit is outside this step rollback boundary and survives.
    return out.status=S.UNSUPPORTED_FEATURE;
  }
  audit(): S {return this.canonical.audit(this.marks,this.config.capacity);}
  snapshot(): ClearWaterSnapshot {
    const state=new ClearWaterState(this.config.particleCapacity);state.copyFrom(this.canonical);
    return {revision:'CLEAR_WATER_SNAPSHOT_V1',mediumMode:CW_MEDIUM,routeId:CW_ROUTE,runId:this.run,runGeneration:this.generation,
      config:JSON.parse(JSON.stringify(this.config)) as ClearWaterRunConfig,inputGeneration:this.inputGeneration,geometryEpoch:this.geometry,state:state,
      queue:this.sources.queue.slice(),dedup:this.sources.dedup.slice(),events:this.sources.events.slice(),pendingCount:this.sources.pendingCount,dedupCount:this.sources.dedupCount,eventCount:this.sources.eventCount,
      queueHead:this.sources.queueHead,enqueueOrdinal:this.sources.enqueueOrdinal,clockWindowId:this.clockId,clockRaw:this.clockRaw,clockUnit:this.clockUnit,failedAttempt:this.failedAttempt,
      implementation:'A_SOURCE_INVENTORY_ONLY;B_COUPLING_AND_C_RENDER_NOT_IMPLEMENTED'};
  }
  allocationLedger(): ClearWaterAllocationLedger {
    const arrays=this.canonical.allocations('canonical').concat(this.sourceBefore.allocations('sourceBefore'),this.stepBefore.allocations('stepBefore'),this.work.allocations('work'),this.sources.allocations());
    arrays.push({name:'engine.auditMarks',type:'Uint8Array',length:this.marks.length,byteLength:this.marks.byteLength,lifetime:'COLD_RUN_FIXED'});
    let total=0;for(const row of arrays)total+=row.byteLength;
    const snapshotExtra=this.canonical.allocations('snapshot').reduce((n: number,r: ClearWaterAllocation):number=>n+r.byteLength,0)+this.sources.queue.byteLength+this.sources.dedup.byteLength+this.sources.events.byteLength;
    return {revision:'CW_ALLOCATION_LEDGER_A_V1',arrays:arrays,payloadBytes:total,coldSnapshotExtraBytes:snapshotExtra,
      queueRows:this.config.queueCapacity,dedupRows:this.config.dedupCapacity,eventRows:this.config.eventCapacity,
      excluded:['JS_OBJECT_STRING_VIEW_HEADER_AND_ALIGNMENT_ENGINE_DEPENDENT','COLD_RETURN_OBJECTS_AND_CALLER_RETAINED_SNAPSHOTS','NO_NATIVE_GPU_DRIVER_RESOURCES_ALLOCATED_BY_A'],
      notImplemented:['B_HYDRAULICS_SEGMENTED_PARTICLES_GUARD_EVENTS_LINEAGE','C_PUBLICATION_RENDER_INPUT_PAGE','SNAPSHOT_RESTORE_NO_PARTIAL_RESTORE_API']};
  }
  sourceLayoutManifest(): string {return this.sources.layoutManifest();}
}

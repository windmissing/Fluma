import { DerivedFieldV2 } from './derived/P011DryDomainV2';
import { P011ParticleM2Projection } from './P011ParticleM2Projection';
// P-011 Route C D2: bind the M1 fixed-buffer executor to WaterSandField's
// canonical arrays. The binding is cold-created when the route is armed;
// substep calls reuse its workspace, sink and all typed-array storage.
import { P011ParticleParams, P011ParticleField, P011ParticleSnapshot } from './P011ParticleField';
import { P011ParticleHotConfig, P011ParticleHotWorkspace, P011ParticleHotFieldBuffer,
  configureHot, warmUpHot, beginStepHot, collectParticlePlanInto,
  compareParticlePlansHot, copyRequestRowHot, applyParticlePlanInto, finishStepHot, rollbackStepHot } from './P011ParticleHot';
import { P011ParticleM2CommitSink, P011ParticleM2Host } from './P011ParticleM2Sink';
import { P011ParticlePrototypeEvent } from './P011ParticlePrototype';
import { P011ParticleHotEvidence, exportParticleEvidence,
  registerParticleHotRun } from './P011ParticleHotEvidence';

export type P011ParticleRouteCode = 'OK' | 'INVALID_PARAMS' | 'ROUTE_CONFLICT' | 'BUSY_PARTICLES';
export interface P011ParticleRouteResult { ok: boolean; code: P011ParticleRouteCode; active: boolean; }
export const P011_PARTICLE_M2_BUSY = 1;
export const P011_PARTICLE_M2_INVALID_ROUTE = 2;
export const P011_PARTICLE_M2_STEP_FAILED = 3;
export const P011_PARTICLE_M2_ROUTE_CONFLICT = 4;
export class P011CollectFirstFailure {
  phase: string = 'NOT_RUN'; m2Code: number = 0; hotCode: number = 0; rollbackCode: number = 0;
  rollbackExecuted: boolean = false;
  predicate: string = ''; failureIndex: number = -1;
  entryStepOpen: number = 0; entryActive: number = 0; entryEpoch: number = 0;
  entryBudgetEpoch: number = 0; entryBudgetOpen: number = 0; entryPageBacklog: number = 0;
  exitStepOpen: number = 0; exitActive: number = 0; exitEpoch: number = 0;
  exitBudgetEpoch: number = 0; exitBudgetOpen: number = 0; exitPageBacklog: number = 0;
  attemptToken: number = 0; runToken: number = 0; substepId: number = 0; seed: number = 0;
  dt: number = 0; tiltRad: number = 0; paramsRevision: number = 1; inputRevision: number = 1;
}

export function defaultP011ParticleParams(): P011ParticleParams {
  return { massPackage: 0.1, nmax: 512, terminalSpeedHeavy: 64, terminalSpeedLight: 38,
    dragHeavy: 3, dragLight: 2, liftSpeedHeavy: 8, liftSpeedLight: 8, liftSpread: 2,
    mergeColRadius: 1, mergeHeightRadius: 2, contactEps: 1e-6, channelRateCap: 0 };
}

/** Stable field-array view. Primitive ledger values are synchronized at the
 * boundary; all material arrays alias WaterSandField's canonical storage. */
export class P011ParticleM2FieldBuffer implements P011ParticleHotFieldBuffer,DerivedFieldV2 {
  readonly cols: number; readonly capacity: number;
  readonly waterTop: Float64Array; readonly bh: Float64Array; readonly bl: Float64Array;
  readonly mh: Float64Array; readonly ml: Float64Array; readonly suspH: Float64Array;
  readonly suspL: Float64Array; readonly surface: Float64Array;
  mediumMode: string = 'WET_COLUMN_V1';
  sourceInjectedH = 0; sourceInjectedL = 0; sourceEpoch = 0;
  constructor(cols: number, capacity: number, waterTop: Float64Array,
    bh: Float64Array, bl: Float64Array, mh: Float64Array, ml: Float64Array,
    suspH: Float64Array, suspL: Float64Array, surface: Float64Array) {
    this.cols = cols; this.capacity = capacity;
    this.waterTop = waterTop; this.bh = bh; this.bl = bl; this.mh = mh; this.ml = ml;
    this.suspH = suspH; this.suspL = suspL; this.surface = surface;
  }
  syncSources(heavy: number, light: number, epoch: number): void {
    this.sourceInjectedH = heavy; this.sourceInjectedL = light; this.sourceEpoch = epoch;
  }
}

/** Production adapter for the D2 field boundary. M1 owns the pool and hot
 * execution; this class owns the live-field view, per-step dual collect and
 * synchronization with the field's already allocated motion budgets. */
export class P011ParticleM2FieldBinding {
  readonly work: P011ParticleHotWorkspace;
  readonly projection: P011ParticleM2Projection;
  readonly field: P011ParticleM2FieldBuffer;
  readonly sink: P011ParticleM2CommitSink;
  private runToken = 1;
  private attemptToken = 0;
  private stepOpen = 0;
  private birthEventRows: string[] = [];
  seedCalls = 0; seedCollectMs = 0; seedApplyMs = 0; seedFinishMs = 0;
  seedCollectCalls = 0; seedApplyCalls = 0; previewCollectCalls = 0; authoritativeApplyCalls = 0;
  collectFailureEnabled = false;
  readonly collectFailure: P011CollectFirstFailure = new P011CollectFirstFailure();
  private collectEntry(dt: number,tiltRad: number,substepId: number,seed: number): void {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(350);let txCode=0;try{
    if(!this.collectFailureEnabled){txCode=0;return;}
    const d=this.collectFailure,w=this.work,b=this.sink.budget;
    d.phase='ENTRY';d.m2Code=0;d.hotCode=0;d.rollbackCode=0;
    d.rollbackExecuted=false;
    d.predicate='';d.failureIndex=-1;
    d.entryStepOpen=this.stepOpen;d.entryActive=w.active;d.entryEpoch=w.stepEpoch;
    d.entryBudgetEpoch=b.diagnosticEpoch;d.entryBudgetOpen=b.diagnosticOpenCount;d.entryPageBacklog=w.pageBacklog;
    d.attemptToken=this.attemptToken;d.runToken=this.runToken;d.substepId=substepId;d.seed=seed;d.dt=dt;d.tiltRad=tiltRad;
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,350,txCode);}}
  private collectExit(phase: string,m2Code: number,hotCode: number,rollbackCode: number=0): number {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(351);let txCode=0;try{
    if(this.collectFailureEnabled){const d=this.collectFailure,w=this.work,b=this.sink.budget;
      d.phase=phase;d.m2Code=m2Code;d.hotCode=hotCode;d.rollbackCode=rollbackCode;
      d.rollbackExecuted=phase==='COLLECT_A'||phase==='COLLECT_B'||phase==='COMPARE';
      d.predicate=phase==='BEGIN_HOT'?w.beginFailurePredicate:phase==='COMPARE'?this.compareFailurePredicate():'';
      d.failureIndex=phase==='BEGIN_HOT'?w.beginFailureIndex:phase==='COMPARE'?this.compareFailureIndex():-1;
      d.exitStepOpen=this.stepOpen;d.exitActive=w.active;d.exitEpoch=w.stepEpoch;
      d.exitBudgetEpoch=b.diagnosticEpoch;d.exitBudgetOpen=b.diagnosticOpenCount;d.exitPageBacklog=w.pageBacklog;}
    {const txRc:number=m2Code;txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,351,txCode);}}
  private compareFailurePredicate(): string {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(352);let txCode=0;try{
    const a=this.work.planA,b=this.work.planB;
    if(a.ownerId!==b.ownerId||a.stepEpoch!==b.stepEpoch||a.runToken!==b.runToken){const txRc:string='IDENTITY';txCode=0;return txRc;}
    if(a.requestCount!==b.requestCount||a.motionCount!==b.motionCount){const txRc:string='COUNT';txCode=0;return txRc;}
    for(let i=0;i<a.requestCount;i++){txTrace?.add(39);
      if(a.kind[i]!==b.kind[i]){const txRc:string='KIND';txCode=0;return txRc;}
      if(a.particleId[i]!==b.particleId[i]){const txRc:string='PARTICLE_ID';txCode=0;return txRc;}
      if(a.requested[i]!==b.requested[i]){const txRc:string='REQUESTED';txCode=0;return txRc;}
      if(a.column[i]!==b.column[i]||a.targetColumn[i]!==b.targetColumn[i]){const txRc:string='COLUMN';txCode=0;return txRc;}
    }
    {const txRc:string='OTHER_PLAN_FIELD';txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,352,txCode);}}
  private compareFailureIndex(): number {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(353);let txCode=0;try{
    const a=this.work.planA,b=this.work.planB;
    for(let i=0;i<Math.min(a.requestCount,b.requestCount);i++)if(a.kind[i]!==b.kind[i]||
      a.particleId[i]!==b.particleId[i]||a.requested[i]!==b.requested[i]||
      a.column[i]!==b.column[i]||a.targetColumn[i]!==b.targetColumn[i]){const txRc:number=i;txCode=0;return txRc;}
    {const txRc:number=-1;txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,353,txCode);}}
  constructor(cols: number, capacity: number, waterTop: Float64Array, bh: Float64Array,
    bl: Float64Array, mh: Float64Array, ml: Float64Array, suspH: Float64Array,
    suspL: Float64Array, surface: Float64Array, params: P011ParticleParams, host: P011ParticleM2Host) {
    const config: P011ParticleHotConfig = { cols: cols, nmax: params.nmax,
      requestCapacity: 2048, motionCapacity: params.nmax, eventCapacity: 4096,
      lineageCapacity: 8192, params: params };
    this.work = configureHot(config);
    warmUpHot(this.work);
    this.projection = new P011ParticleM2Projection(this.work);
    this.field = new P011ParticleM2FieldBuffer(cols, capacity, waterTop, bh, bl, mh, ml,
      suspH, suspL, surface);
    this.work.field = this.field;
    this.sink = new P011ParticleM2CommitSink(this.work, host);
  }
  get pool(): P011ParticleField { return this.work.pool; }
  get liveCount(): number { return this.work.pool.totals().count; }
  get active(): boolean { return this.stepOpen !== 0; }
  // The birth callers overwrite every effective row and motion count. The
  // first collect still performs the original frozen-pool/predictor validation;
  // the second collect had no consumed data beyond this plan identity header.
  private collectBirthHeaders(): number {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(4);let txCode=0;try{
    const w = this.work;
    const status = collectParticlePlanInto(w, w.stepEpoch, w.planA);
    if (status !== 0) {const txRc:number=status;txCode=0;return txRc;}
    const a = w.planA, b = w.planB;
    if (!b.storageValidHot()) {const txRc:number=-1;txCode=0;return txRc;}
    b.ownerId = a.ownerId; b.runToken = a.runToken; b.stepEpoch = a.stepEpoch;
    b.substepId = a.substepId; b.attemptToken = a.attemptToken;
    b.paramsRevision = a.paramsRevision; b.inputRevision = a.inputRevision;
    b.seed = a.seed; b.dt = a.dt; b.effectiveTiltRad = a.effectiveTiltRad;
    b.valid = 1; b.consumed = 0;
    {const txRc:number=0;txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,4,txCode);}}
  private dryProgressSpan: number=-1;
  private diagnosticSeedInput(mass: number,x: number,y: number,vx: number,vy: number,
    phase: number,sourceKind: number,seed: number): void {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(355);let txCode=0;try{
    const d=this.work.poolProgress;if(d===null){txCode=0;return;}
    d.nmax=this.work.nmax;d.free=this.pool.freeSlotCount;d.live=d.nmax-d.free;
    d.epoch=this.work.stepEpoch;d.attemptToken=this.attemptToken+1;
    d.context.runToken=this.runToken;d.context.ownerId=this.work.ownerId;
    d.context.mass=mass;d.context.x=x;d.context.y=y;d.context.vx=vx;d.context.vy=vy;
    d.context.column=Math.floor(x);d.context.phase=phase;d.context.sourceKind=sourceKind;d.context.seed=seed;
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,355,txCode);}}
  seed(mass: number,x: number,y: number,vx: number,vy: number,phase: number,sourceKind: number,sourceId: number): number {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(356);let txCode=0;try{
    const d=this.work.poolProgress;let span=-1,code=-1;
    if(d!==null){this.diagnosticSeedInput(mass,x,y,vx,vy,phase,sourceKind,sourceId);
      span=d.begin('SEED');d.increment(14);}
    try{code=this.seedBody(mass,x,y,vx,vy,phase,sourceKind,sourceId);{const txRc:number=code;txCode=0;return txRc;}}
    finally{if(d!==null){d.free=this.pool.freeSlotCount;d.live=d.nmax-d.free;d.endSpan(span,code);}}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,356,txCode);}}
  beginDryBatch(heavy: number,light: number,x: number,y: number,sourceId: number,seed: number,sourceKind: number=0): boolean {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(357);let txCode=0;try{
    const d=this.work.poolProgress;let ok=false;
    if(d!==null){this.diagnosticSeedInput(heavy+light,x,y,0,0,-1,sourceKind,seed);
      this.dryProgressSpan=d.begin('SEED_BATCH');d.increment(14);}
    this.work.derivedV2?.counters.add(18);
    try{ok=this.beginDryBatchBody(heavy,light,x,y,sourceId,seed,sourceKind);{const txRc:boolean=ok;txCode=0;return txRc;}}
    finally{if(!ok&&d!==null){d.endSpan(this.dryProgressSpan,-1);this.dryProgressSpan=-1;}}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,357,txCode);}}
  finishDryBatch(): boolean {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(358);let txCode=0;try{
    const d=this.work.poolProgress;let ok=false;
    try{ok=this.finishDryBatchBody();{const txRc:boolean=ok;txCode=0;return txRc;}}
    finally{if(d!==null){d.free=this.pool.freeSlotCount;d.live=d.nmax-d.free;
      d.endSpan(this.dryProgressSpan,ok?0:-1);this.dryProgressSpan=-1;}}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,358,txCode);}}
  beginCollect(dt: number,tiltRad: number,substepId: number,seed: number,
    sourceHeavy: number,sourceLight: number,sourceEpoch: number): number {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(359);let txCode=0;try{
    const d=this.work.poolProgress;let span=-1,code=-1;
    if(d!==null){d.context.substepId=substepId;d.context.theta=tiltRad;d.context.seed=seed;
      d.context.sourceEpoch=sourceEpoch;d.attemptToken=this.attemptToken+1;
      d.nmax=this.work.nmax;d.free=this.pool.freeSlotCount;d.live=d.nmax-d.free;span=d.begin('COLLECT_ATTEMPT');}
    try{code=this.beginCollectBody(dt,tiltRad,substepId,seed,sourceHeavy,sourceLight,sourceEpoch);{const txRc:number=code;txCode=txRc;return txRc;}}
    finally{d?.endSpan(span,code);}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,359,txCode);}}
  private seedBody(mass: number, x: number, y: number, vx: number, vy: number,
    phase: number, sourceKind: number, sourceId: number): number {this.work.txD1?.role(1);this.work.txD1?.trace.add(41);this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(360);let txCode=0;try{
    if (this.stepOpen !== 0 || this.work.pageBacklog !== 0) {const txRc:number=-1;txCode=0;return txRc;}
    if(this.work.diagnosticEnabled)this.seedCalls++;
    this.attemptToken++;
    const w = this.work;
    const tilt = sourceKind === 0 ? Math.atan2(vx, 0 - vy) : w.effectiveTiltRad;
    if (beginStepHot(w, { runToken: this.runToken, substepId: w.substepId,
      attemptToken: this.attemptToken, dt: 1 / 60, effectiveTiltRad: tilt, seed: sourceId,
      paramsRevision: 1, inputRevision: 1, field: this.field }) !== 0) {const txRc:number=-1;txCode=0;return txRc;}
    this.stepOpen = 1;
    const collectStart=this.work.diagnosticEnabled?(this.work.poolProgress?.clockNow() ?? NaN):0;
    const collected=this.collectBirthHeaders();
    if(this.work.diagnosticEnabled)this.seedCollectCalls++;
    if(this.work.diagnosticEnabled)this.seedCollectMs+=(this.work.poolProgress?.clockNow() ?? NaN)-collectStart;
    if (collected !== 0) { this.rollback(); {const txRc:number=-1;txCode=0;return txRc;} }
    const a = w.planA, b = w.planB;
    a.requestCount = 1; a.motionCount = 0; a.totalRequested = 0;
    a.kind[0] = sourceKind === 0 ? 1 : 4; a.phase[0] = phase; a.sourceKind[0] = sourceKind;
    a.column[0] = Math.floor(x); a.targetColumn[0] = -1; a.particleId[0] = 0;
    a.sourceId[0] = sourceId; a.substep[0] = w.substepId; a.ordinal[0] = 1;
    a.requested[0] = mass; a.totalWeight[0] = 0; a.predictedX[0] = x; a.predictedY[0] = y;
    a.predictedVx[0] = vx; a.predictedVy[0] = vy; a.mergeStart[0] = 0; a.mergeCount[0] = 0;
    b.requestCount = 1; b.motionCount = 0; b.totalRequested = 0;
    copyRequestRowHot(a, 0, b, 0);
    this.sink.birthOnly = sourceKind === 1 ? 1 : 0;
    const applyStart=this.work.diagnosticEnabled?(this.work.poolProgress?.clockNow() ?? NaN):0;
    const applied = applyParticlePlanInto(w, w.stepEpoch, a, this.sink, w.results);
    if(this.work.diagnosticEnabled)this.seedApplyCalls++;
    if(this.work.diagnosticEnabled)this.seedApplyMs+=(this.work.poolProgress?.clockNow() ?? NaN)-applyStart;
    this.sink.birthOnly = 0;
    if (applied !== 0 || w.results.granted[0] !== mass) { this.rollback(); {const txRc:number=-1;txCode=0;return txRc;} }
    const finishStart=this.work.diagnosticEnabled?(this.work.poolProgress?.clockNow() ?? NaN):0;
    const finished=finishStepHot(w, w.stepEpoch);
    if(this.work.diagnosticEnabled)this.seedFinishMs+=(this.work.poolProgress?.clockNow() ?? NaN)-finishStart;
    if (finished !== 0) { this.rollback(); {const txRc:number=-1;txCode=0;return txRc;} }
    this.stepOpen = 0;
    const evidence = this.exportCommittedEvidence();
    for (let i = 0; i < evidence.events.length; i++) {txTrace?.add(39); const row = serializeP011ParticleEvent(evidence.events[i], i); if (row !== null) this.birthEventRows.push(row); }
    {const txRc:number=0;txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,360,txCode);}}
  private beginDryBatchBody(heavy: number, light: number, x: number, y: number, sourceId: number, seed: number, sourceKind: number = 0): boolean {this.work.txD1?.role(1);this.work.txD1?.trace.add(41);this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(361);let txCode=0;try{
    if (this.stepOpen !== 0 || this.work.pageBacklog !== 0 || this.field.mediumMode !== 'DRY_AIR_V1') {const txRc:boolean=false;txCode=0;return txRc;}
    if(this.work.diagnosticEnabled)this.seedCalls++;
    this.attemptToken++; const w=this.work;
    if (beginStepHot(w,{runToken:this.runToken,substepId:w.substepId,attemptToken:this.attemptToken,
      dt:1/60,effectiveTiltRad:0,seed:seed,paramsRevision:1,inputRevision:1,field:this.field})!==0){const txRc:boolean=false;txCode=0;return txRc;}
    this.stepOpen=1;
    const collectStart=this.work.diagnosticEnabled?(this.work.poolProgress?.clockNow() ?? NaN):0;
    const collected=this.collectBirthHeaders();
    if(this.work.diagnosticEnabled)this.seedCollectCalls++;
    if(this.work.diagnosticEnabled)this.seedCollectMs+=(this.work.poolProgress?.clockNow() ?? NaN)-collectStart;
    if(collected!==0){this.rollback();{const txRc:boolean=false;txCode=0;return txRc;}}
    const a=w.planA,b=w.planB;a.requestCount=0;a.motionCount=0;a.totalRequested=0;
    for(let phase=0;phase<2;phase++){txTrace?.add(39);
      const mass=phase===0?heavy:light;if(mass===0)continue;const row=a.requestCount++;
      a.kind[row]=sourceKind===0?1:4;a.phase[row]=phase;a.sourceKind[row]=sourceKind;a.column[row]=Math.floor(x);a.targetColumn[row]=-1;
      a.particleId[row]=0;a.sourceId[row]=sourceId;a.substep[row]=w.substepId;a.ordinal[row]=row+1;
      a.requested[row]=mass;a.totalWeight[row]=0;a.predictedX[row]=x;a.predictedY[row]=y;
      a.predictedVx[row]=0;a.predictedVy[row]=sourceKind===0?0:phase===0?w.params.liftSpeedHeavy:w.params.liftSpeedLight;a.mergeStart[row]=row*2;a.mergeCount[row]=0;
    }
    b.requestCount=a.requestCount;b.motionCount=0;b.totalRequested=0;
    for(let row=0;row<a.requestCount;row++)copyRequestRowHot(a,row,b,row);
    this.sink.birthOnly=sourceKind===1?1:0;
    const applyStart=this.work.diagnosticEnabled?(this.work.poolProgress?.clockNow() ?? NaN):0;
    const applied=applyParticlePlanInto(w,w.stepEpoch,a,this.sink,w.results);this.sink.birthOnly=0;
    if(this.work.diagnosticEnabled)this.seedApplyCalls++;
    if(this.work.diagnosticEnabled)this.seedApplyMs+=(this.work.poolProgress?.clockNow() ?? NaN)-applyStart;
    if(applied!==0){this.rollback();{const txRc:boolean=false;txCode=0;return txRc;}}
    for(let row=0;row<a.requestCount;row++)if(w.results.granted[row]!==a.requested[row]){this.rollback();{const txRc:boolean=false;txCode=0;return txRc;}}
    {const txRc:boolean=true;txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,361,txCode);}}
  private finishDryBatchBody(): boolean {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(362);let txCode=0;try{
    if(this.stepOpen===0){const txRc:boolean=false;txCode=0;return txRc;}
    if(finishStepHot(this.work,this.work.stepEpoch)!==0){this.rollback();{const txRc:boolean=false;txCode=0;return txRc;}}
    this.stepOpen=0;{const txRc:boolean=true;txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,362,txCode);}}
  drainBirthEvents(): string[] {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(363);let txCode=0;try{
    const rows = this.birthEventRows; this.birthEventRows = []; {const txRc:string[]=rows;txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,363,txCode);}}

  private beginCollectBody(dt: number, tiltRad: number, substepId: number, seed: number,
    sourceHeavy: number, sourceLight: number, sourceEpoch: number): number {this.work.txD1?.role(2);this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(5);let txCode=0;try{
    this.collectEntry(dt,tiltRad,substepId,seed);
    if (this.stepOpen !== 0) {const txRc:number=this.collectExit('ENTRY',P011_PARTICLE_M2_BUSY,0);txCode=txRc;return txRc;}
    this.attemptToken++;
    if(this.collectFailureEnabled)this.collectFailure.attemptToken=this.attemptToken;
    if (!Number.isSafeInteger(this.attemptToken)) {const txRc:number=this.collectExit('TOKEN',P011_PARTICLE_M2_STEP_FAILED,0);txCode=txRc;return txRc;}
    this.field.syncSources(sourceHeavy, sourceLight, sourceEpoch);
    const status = beginStepHot(this.work, { runToken: this.runToken, substepId: substepId,
      attemptToken: this.attemptToken, dt: dt, effectiveTiltRad: tiltRad, seed: seed,
      paramsRevision: 1, inputRevision: 1, field: this.field });
    if (status !== 0) {const txRc:number=this.collectExit('BEGIN_HOT',P011_PARTICLE_M2_STEP_FAILED,status);txCode=txRc;return txRc;}
    this.stepOpen = 1;
    const epoch = this.work.stepEpoch;
    txTrace?.add(43);
    const a=collectParticlePlanInto(this.work,epoch,this.work.planA);
    if(this.work.diagnosticEnabled)this.previewCollectCalls++;
    if(a!==0){const rollback=rollbackStepHot(this.work,epoch);this.stepOpen=0;{const txRc:number=this.collectExit('COLLECT_A',P011_PARTICLE_M2_STEP_FAILED,a,rollback);txCode=txRc;return txRc;}}
    txTrace?.add(44);
    const b=collectParticlePlanInto(this.work,epoch,this.work.planB);
    if(this.work.diagnosticEnabled)this.previewCollectCalls++;
    if(b!==0){const rollback=rollbackStepHot(this.work,epoch);this.stepOpen=0;{const txRc:number=this.collectExit('COLLECT_B',P011_PARTICLE_M2_STEP_FAILED,b,rollback);txCode=txRc;return txRc;}}
    const comparison=compareParticlePlansHot(this.work.planA,this.work.planB);
    if(comparison!==0){const rollback=rollbackStepHot(this.work,epoch);this.stepOpen=0;{const txRc:number=this.collectExit('COMPARE',P011_PARTICLE_M2_STEP_FAILED,comparison,rollback);txCode=txRc;return txRc;}}
    {const txRc:number=this.collectExit('READY',0,0);txCode=txRc;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,5,txCode);}}
  requested(): number {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(365);let txCode=0;try{ {const txRc:number=this.stepOpen === 0 ? 0 : this.work.planA.totalRequested;txCode=0;return txRc;} }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,365,txCode);}}
  get totalRemaining(): number { return this.sink.budget.totalRemaining; }
  get totalUsed(): number { return this.sink.budget.totalUsed; }
  registerRun(runId: string, substepId?: number): void {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(366);let txCode=0;try{
    registerParticleHotRun(this.work, this.runToken, runId);
    if (substepId !== undefined) this.work.substepId = substepId;
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,366,txCode);}}
  exportCommittedEvidence(): P011ParticleHotEvidence {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(367);let txCode=0;try{
    {const txRc:P011ParticleHotEvidence=exportParticleEvidence(this.work, this.work.committedPageToken);txCode=0;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,367,txCode);}}
  discardCommittedPage(): void {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(368);let txCode=0;try{
    if (this.work.active === 0 && this.work.pageBacklog === 1 &&
      this.work.committedPageToken > 0) {
      this.work.pageBacklog = 0; this.work.committedPageToken = 0;
    }
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,368,txCode);}}
  apply(totalScale: number, localRemaining: Float64Array, localUsed: Float64Array,
    totalRemaining: number): number {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(369);let txCode=0;try{
    if (this.stepOpen === 0 || localRemaining.length !== this.work.cols ||
      localUsed.length !== this.work.cols || !Number.isFinite(totalScale) || totalScale < 0 || totalScale > 1) {
      {const txRc:number=P011_PARTICLE_M2_STEP_FAILED;txCode=txRc;return txRc;}
    }
    const budget = this.sink.budget;
    budget.scale = totalScale;
    budget.bindLiveBudget(localRemaining, localUsed, totalRemaining, 0);
    const epoch = this.work.stepEpoch;
    const status = applyParticlePlanInto(this.work, epoch, this.work.planA, this.sink, this.work.results);
    if(this.work.diagnosticEnabled)this.authoritativeApplyCalls++;
    if (status !== 0) { this.rollback(); {const txRc:number=P011_PARTICLE_M2_STEP_FAILED;txCode=txRc;return txRc;} }
    if (finishStepHot(this.work, epoch) !== 0) { rollbackStepHot(this.work, epoch); this.stepOpen = 0; {const txRc:number=P011_PARTICLE_M2_STEP_FAILED;txCode=txRc;return txRc;} }
    localRemaining.set(budget.localRemaining); localUsed.set(budget.localUsed);
    this.stepOpen = 0; {const txRc:number=0;txCode=txRc;return txRc;}
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,369,txCode);}}
  rollback(): void {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(370);let txCode=0;try{
    if(this.work.poolProgress!==null&&this.dryProgressSpan>=0){
      this.work.poolProgress.endSpan(this.dryProgressSpan,-1);this.dryProgressSpan=-1;}

    if (this.work.active !== 0) rollbackStepHot(this.work, this.work.stepEpoch);
    this.stepOpen = 0;
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,370,txCode);}}
  reset(): void {this.work.txD1?.syncContext();const txTrace=this.work.txD1===null?null:this.work.txD1.trace;const txScope=txTrace===null?-1:txTrace.enter(371);let txCode=0;try{
    this.rollback(); this.discardCommittedPage(); this.pool.reset();
  }catch(txError){txCode=-1;throw txError as Error;}finally{txTrace?.leave(txScope,371,txCode);}}
}

interface P011ParticleLedgerSnapshot {
  nmax: number; cols: number; liveCount: number; freeCount: number; nextId: number;
  pMass: number[]; pX: number[]; pY: number[]; pVx: number[]; pVy: number[];
  pPhase: number[]; pSource: number[]; pState: number[]; pId: number[]; pSourceId: number[];
  freeSlots: number[]; colParticleMassH: number[]; colParticleMassL: number[];
}
export function serializeP011ParticleSnapshot(s: P011ParticleSnapshot): string {
  const value: P011ParticleLedgerSnapshot = { nmax: s.nmax, cols: s.cols, liveCount: s.liveCount,
    freeCount: s.freeCount, nextId: s.nextId, pMass: Array.from(s.pMass), pX: Array.from(s.pX),
    pY: Array.from(s.pY), pVx: Array.from(s.pVx), pVy: Array.from(s.pVy), pPhase: Array.from(s.pPhase),
    pSource: Array.from(s.pSource), pState: Array.from(s.pState), pId: Array.from(s.pId),
    pSourceId: Array.from(s.pSourceId), freeSlots: Array.from(s.freeSlots),
    colParticleMassH: Array.from(s.colParticleMassH), colParticleMassL: Array.from(s.colParticleMassL) };
  return JSON.stringify(value);
}

// Cold schema-v7 projection. Each event has its own key and retains its transaction key.
export function serializeP011ParticleEvent(e: P011ParticlePrototypeEvent, ordinal: number): string | null {
  const parent = e.requestKey;
  if (e.kind === 'LIFT' && e.particleId === 0) return null;
  if (e.kind === 'SPAWN' && e.sourceKind === 1) { e.kind = 'LIFT'; e.requested = e.mass; e.granted = e.mass; }
  e.requestKey = JSON.stringify([parent, e.kind, e.particleId, ordinal]);
  let result = JSON.stringify(e); result = result.substring(0, result.length - 1);
  result += ',"transactionRequestKey":' + JSON.stringify(parent);
  if (e.kind === 'LIFT') result += ',"fromLayer":"surface"';
  if (e.kind === 'CROSS_COLUMN') result += ',"fromColumn":' + e.column + ',"toColumn":' + e.targetColumn +
    ',"distance":' + Math.abs(e.targetColumn - e.column) + ',"budget":{"local":' + 2 * e.granted + ',"bottom":0,"total":' + e.granted + '}';
  if (e.kind === 'MERGE' || e.kind === 'DEGRADE') result += ',"particleIds":' +
    JSON.stringify(e.mergedIds.length > 0 ? e.mergedIds : e.particleId > 0 ? [e.particleId] : []);
  if (e.kind === 'DEGRADE') result += ',"toColumn":' + e.column + ',"toSuspH":' +
    (e.phase === 0 ? e.granted : 0) + ',"toSuspL":' + (e.phase === 1 ? e.granted : 0);
  return result + '}';
}

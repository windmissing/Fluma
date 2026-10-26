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
export class P011ParticleM2FieldBuffer implements P011ParticleHotFieldBuffer {
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
  private collectEntry(dt: number,tiltRad: number,substepId: number,seed: number): void {
    if(!this.collectFailureEnabled)return;
    const d=this.collectFailure,w=this.work,b=this.sink.budget;
    d.phase='ENTRY';d.m2Code=0;d.hotCode=0;d.rollbackCode=0;
    d.predicate='';d.failureIndex=-1;
    d.entryStepOpen=this.stepOpen;d.entryActive=w.active;d.entryEpoch=w.stepEpoch;
    d.entryBudgetEpoch=b.diagnosticEpoch;d.entryBudgetOpen=b.diagnosticOpenCount;d.entryPageBacklog=w.pageBacklog;
    d.attemptToken=this.attemptToken;d.runToken=this.runToken;d.substepId=substepId;d.seed=seed;d.dt=dt;d.tiltRad=tiltRad;
  }
  private collectExit(phase: string,m2Code: number,hotCode: number,rollbackCode: number=0): number {
    if(this.collectFailureEnabled){const d=this.collectFailure,w=this.work,b=this.sink.budget;
      d.phase=phase;d.m2Code=m2Code;d.hotCode=hotCode;d.rollbackCode=rollbackCode;
      d.predicate=phase==='BEGIN_HOT'?w.beginFailurePredicate:phase==='COMPARE'?this.compareFailurePredicate():'';
      d.failureIndex=phase==='BEGIN_HOT'?w.beginFailureIndex:phase==='COMPARE'?this.compareFailureIndex():-1;
      d.exitStepOpen=this.stepOpen;d.exitActive=w.active;d.exitEpoch=w.stepEpoch;
      d.exitBudgetEpoch=b.diagnosticEpoch;d.exitBudgetOpen=b.diagnosticOpenCount;d.exitPageBacklog=w.pageBacklog;}
    return m2Code;
  }
  private compareFailurePredicate(): string {
    const a=this.work.planA,b=this.work.planB;
    if(a.ownerId!==b.ownerId||a.stepEpoch!==b.stepEpoch||a.runToken!==b.runToken)return 'IDENTITY';
    if(a.requestCount!==b.requestCount||a.motionCount!==b.motionCount)return 'COUNT';
    for(let i=0;i<a.requestCount;i++){
      if(a.kind[i]!==b.kind[i])return 'KIND';
      if(a.particleId[i]!==b.particleId[i])return 'PARTICLE_ID';
      if(a.requested[i]!==b.requested[i])return 'REQUESTED';
      if(a.column[i]!==b.column[i]||a.targetColumn[i]!==b.targetColumn[i])return 'COLUMN';
    }
    return 'OTHER_PLAN_FIELD';
  }
  private compareFailureIndex(): number {
    const a=this.work.planA,b=this.work.planB;
    for(let i=0;i<Math.min(a.requestCount,b.requestCount);i++)if(a.kind[i]!==b.kind[i]||
      a.particleId[i]!==b.particleId[i]||a.requested[i]!==b.requested[i]||
      a.column[i]!==b.column[i]||a.targetColumn[i]!==b.targetColumn[i])return i;
    return -1;
  }
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
  private collectBirthHeaders(): number {
    const w = this.work;
    const status = collectParticlePlanInto(w, w.stepEpoch, w.planA);
    if (status !== 0) return status;
    const a = w.planA, b = w.planB;
    if (!b.storageValidHot()) return -1;
    b.ownerId = a.ownerId; b.runToken = a.runToken; b.stepEpoch = a.stepEpoch;
    b.substepId = a.substepId; b.attemptToken = a.attemptToken;
    b.paramsRevision = a.paramsRevision; b.inputRevision = a.inputRevision;
    b.seed = a.seed; b.dt = a.dt; b.effectiveTiltRad = a.effectiveTiltRad;
    b.valid = 1; b.consumed = 0;
    return 0;
  }
  seed(mass: number, x: number, y: number, vx: number, vy: number,
    phase: number, sourceKind: number, sourceId: number): number {
    if (this.stepOpen !== 0 || this.work.pageBacklog !== 0) return -1;
    if(this.work.diagnosticEnabled)this.seedCalls++;
    this.attemptToken++;
    const w = this.work;
    const tilt = sourceKind === 0 ? Math.atan2(vx, 0 - vy) : w.effectiveTiltRad;
    if (beginStepHot(w, { runToken: this.runToken, substepId: w.substepId,
      attemptToken: this.attemptToken, dt: 1 / 60, effectiveTiltRad: tilt, seed: sourceId,
      paramsRevision: 1, inputRevision: 1, field: this.field }) !== 0) return -1;
    this.stepOpen = 1;
    const collectStart=this.work.diagnosticEnabled?Date.now():0;
    const collected=this.collectBirthHeaders();
    if(this.work.diagnosticEnabled)this.seedCollectCalls++;
    if(this.work.diagnosticEnabled)this.seedCollectMs+=Date.now()-collectStart;
    if (collected !== 0) { this.rollback(); return -1; }
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
    const applyStart=this.work.diagnosticEnabled?Date.now():0;
    const applied = applyParticlePlanInto(w, w.stepEpoch, a, this.sink, w.results);
    if(this.work.diagnosticEnabled)this.seedApplyCalls++;
    if(this.work.diagnosticEnabled)this.seedApplyMs+=Date.now()-applyStart;
    this.sink.birthOnly = 0;
    if (applied !== 0 || w.results.granted[0] !== mass) { this.rollback(); return -1; }
    const finishStart=this.work.diagnosticEnabled?Date.now():0;
    const finished=finishStepHot(w, w.stepEpoch);
    if(this.work.diagnosticEnabled)this.seedFinishMs+=Date.now()-finishStart;
    if (finished !== 0) { this.rollback(); return -1; }
    this.stepOpen = 0;
    const evidence = this.exportCommittedEvidence();
    for (let i = 0; i < evidence.events.length; i++) { const row = serializeP011ParticleEvent(evidence.events[i], i); if (row !== null) this.birthEventRows.push(row); }
    return 0;
  }
  beginDryBatch(heavy: number, light: number, x: number, y: number, sourceId: number, seed: number, sourceKind: number = 0): boolean {
    if (this.stepOpen !== 0 || this.work.pageBacklog !== 0 || this.field.mediumMode !== 'DRY_AIR_V1') return false;
    if(this.work.diagnosticEnabled)this.seedCalls++;
    this.attemptToken++; const w=this.work;
    if (beginStepHot(w,{runToken:this.runToken,substepId:w.substepId,attemptToken:this.attemptToken,
      dt:1/60,effectiveTiltRad:0,seed:seed,paramsRevision:1,inputRevision:1,field:this.field})!==0)return false;
    this.stepOpen=1;
    const collectStart=this.work.diagnosticEnabled?Date.now():0;
    const collected=this.collectBirthHeaders();
    if(this.work.diagnosticEnabled)this.seedCollectCalls++;
    if(this.work.diagnosticEnabled)this.seedCollectMs+=Date.now()-collectStart;
    if(collected!==0){this.rollback();return false;}
    const a=w.planA,b=w.planB;a.requestCount=0;a.motionCount=0;a.totalRequested=0;
    for(let phase=0;phase<2;phase++){
      const mass=phase===0?heavy:light;if(mass===0)continue;const row=a.requestCount++;
      a.kind[row]=sourceKind===0?1:4;a.phase[row]=phase;a.sourceKind[row]=sourceKind;a.column[row]=Math.floor(x);a.targetColumn[row]=-1;
      a.particleId[row]=0;a.sourceId[row]=sourceId;a.substep[row]=w.substepId;a.ordinal[row]=row+1;
      a.requested[row]=mass;a.totalWeight[row]=0;a.predictedX[row]=x;a.predictedY[row]=y;
      a.predictedVx[row]=0;a.predictedVy[row]=sourceKind===0?0:phase===0?w.params.liftSpeedHeavy:w.params.liftSpeedLight;a.mergeStart[row]=row*2;a.mergeCount[row]=0;
    }
    b.requestCount=a.requestCount;b.motionCount=0;b.totalRequested=0;
    for(let row=0;row<a.requestCount;row++)copyRequestRowHot(a,row,b,row);
    this.sink.birthOnly=sourceKind===1?1:0;
    const applyStart=this.work.diagnosticEnabled?Date.now():0;
    const applied=applyParticlePlanInto(w,w.stepEpoch,a,this.sink,w.results);this.sink.birthOnly=0;
    if(this.work.diagnosticEnabled)this.seedApplyCalls++;
    if(this.work.diagnosticEnabled)this.seedApplyMs+=Date.now()-applyStart;
    if(applied!==0){this.rollback();return false;}
    for(let row=0;row<a.requestCount;row++)if(w.results.granted[row]!==a.requested[row]){this.rollback();return false;}
    return true;
  }
  finishDryBatch(): boolean {
    if(this.stepOpen===0)return false;
    if(finishStepHot(this.work,this.work.stepEpoch)!==0){this.rollback();return false;}
    this.stepOpen=0;return true;
  }
  drainBirthEvents(): string[] {
    const rows = this.birthEventRows; this.birthEventRows = []; return rows;
  }

  beginCollect(dt: number, tiltRad: number, substepId: number, seed: number,
    sourceHeavy: number, sourceLight: number, sourceEpoch: number): number {
    this.collectEntry(dt,tiltRad,substepId,seed);
    if (this.stepOpen !== 0) return this.collectExit('ENTRY',P011_PARTICLE_M2_BUSY,0);
    this.attemptToken++;
    if(this.collectFailureEnabled)this.collectFailure.attemptToken=this.attemptToken;
    if (!Number.isSafeInteger(this.attemptToken)) return this.collectExit('TOKEN',P011_PARTICLE_M2_STEP_FAILED,0);
    this.field.syncSources(sourceHeavy, sourceLight, sourceEpoch);
    const status = beginStepHot(this.work, { runToken: this.runToken, substepId: substepId,
      attemptToken: this.attemptToken, dt: dt, effectiveTiltRad: tiltRad, seed: seed,
      paramsRevision: 1, inputRevision: 1, field: this.field });
    if (status !== 0) return this.collectExit('BEGIN_HOT',P011_PARTICLE_M2_STEP_FAILED,status);
    this.stepOpen = 1;
    const epoch = this.work.stepEpoch;
    const a=collectParticlePlanInto(this.work,epoch,this.work.planA);
    if(this.work.diagnosticEnabled)this.previewCollectCalls++;
    if(a!==0){const rollback=rollbackStepHot(this.work,epoch);this.stepOpen=0;return this.collectExit('COLLECT_A',P011_PARTICLE_M2_STEP_FAILED,a,rollback);}
    const b=collectParticlePlanInto(this.work,epoch,this.work.planB);
    if(this.work.diagnosticEnabled)this.previewCollectCalls++;
    if(b!==0){const rollback=rollbackStepHot(this.work,epoch);this.stepOpen=0;return this.collectExit('COLLECT_B',P011_PARTICLE_M2_STEP_FAILED,b,rollback);}
    const comparison=compareParticlePlansHot(this.work.planA,this.work.planB);
    if(comparison!==0){const rollback=rollbackStepHot(this.work,epoch);this.stepOpen=0;return this.collectExit('COMPARE',P011_PARTICLE_M2_STEP_FAILED,comparison,rollback);}
    return this.collectExit('READY',0,0);
  }
  requested(): number { return this.stepOpen === 0 ? 0 : this.work.planA.totalRequested; }
  get totalRemaining(): number { return this.sink.budget.totalRemaining; }
  get totalUsed(): number { return this.sink.budget.totalUsed; }
  registerRun(runId: string, substepId?: number): void {
    registerParticleHotRun(this.work, this.runToken, runId);
    if (substepId !== undefined) this.work.substepId = substepId;
  }
  exportCommittedEvidence(): P011ParticleHotEvidence {
    return exportParticleEvidence(this.work, this.work.committedPageToken);
  }
  discardCommittedPage(): void {
    if (this.work.active === 0 && this.work.pageBacklog === 1 &&
      this.work.committedPageToken > 0) {
      this.work.pageBacklog = 0; this.work.committedPageToken = 0;
    }
  }
  apply(totalScale: number, localRemaining: Float64Array, localUsed: Float64Array,
    totalRemaining: number): number {
    if (this.stepOpen === 0 || localRemaining.length !== this.work.cols ||
      localUsed.length !== this.work.cols || !Number.isFinite(totalScale) || totalScale < 0 || totalScale > 1) {
      return P011_PARTICLE_M2_STEP_FAILED;
    }
    const budget = this.sink.budget;
    budget.scale = totalScale;
    budget.bindLiveBudget(localRemaining, localUsed, totalRemaining, 0);
    const epoch = this.work.stepEpoch;
    const status = applyParticlePlanInto(this.work, epoch, this.work.planA, this.sink, this.work.results);
    if(this.work.diagnosticEnabled)this.authoritativeApplyCalls++;
    if (status !== 0) { this.rollback(); return P011_PARTICLE_M2_STEP_FAILED; }
    if (finishStepHot(this.work, epoch) !== 0) { rollbackStepHot(this.work, epoch); this.stepOpen = 0; return P011_PARTICLE_M2_STEP_FAILED; }
    localRemaining.set(budget.localRemaining); localUsed.set(budget.localUsed);
    this.stepOpen = 0; return 0;
  }
  rollback(): void {
    if (this.work.active !== 0) rollbackStepHot(this.work, this.work.stepEpoch);
    this.stepOpen = 0;
  }
  reset(): void {
    this.rollback(); this.discardCommittedPage(); this.pool.reset();
  }
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

// P-011 D2: stable binding between the fixed HOT executor and canonical field storage.
import { P011ParticleParams, P011ParticleField } from './P011ParticleField';
import { P011ParticleHotConfig, P011ParticleHotWorkspace, P011ParticleHotFieldBuffer,
  configureHot, warmUpHot, beginStepHot, collectParticlePlanInto, compareParticlePlansHot,
  applyParticlePlanInto, finishStepHot, rollbackStepHot } from './P011ParticleHot';
import { P011ParticleHotPrototypeSink } from './P011ParticleHotPrototype';
import { P011ParticleHotEvidence, exportParticleEvidence, registerParticleHotRun } from './P011ParticleHotEvidence';
export const P011_PARTICLE_M2_BUSY = 1;
export const P011_PARTICLE_M2_INVALID_ROUTE = 2;
export const P011_PARTICLE_M2_STEP_FAILED = 3;
export const P011_PARTICLE_M2_ROUTE_CONFLICT = 4;
export function defaultP011ParticleParams(): P011ParticleParams {
  return { massPackage: 0.1, nmax: 512, terminalSpeedHeavy: 64, terminalSpeedLight: 38,
    dragHeavy: 3, dragLight: 2, liftSpeedHeavy: 8, liftSpeedLight: 8, liftSpread: 2,
    mergeColRadius: 1, mergeHeightRadius: 2, contactEps: 1e-6, channelRateCap: 0 };
}
export class P011ParticleM2FieldBuffer implements P011ParticleHotFieldBuffer {
  readonly cols: number; readonly capacity: number;
  readonly waterTop: Float64Array; readonly bh: Float64Array; readonly bl: Float64Array;
  readonly mh: Float64Array; readonly ml: Float64Array; readonly suspH: Float64Array;
  readonly suspL: Float64Array; readonly surface: Float64Array;
  sourceInjectedH = 0; sourceInjectedL = 0; sourceEpoch = 0;
  constructor(cols: number, capacity: number, waterTop: Float64Array, bh: Float64Array,
    bl: Float64Array, mh: Float64Array, ml: Float64Array, suspH: Float64Array,
    suspL: Float64Array, surface: Float64Array) {
    this.cols = cols; this.capacity = capacity; this.waterTop = waterTop;
    this.bh = bh; this.bl = bl; this.mh = mh; this.ml = ml;
    this.suspH = suspH; this.suspL = suspL; this.surface = surface;
  }
}
export class P011ParticleM2FieldBinding {
  readonly work: P011ParticleHotWorkspace;
  readonly field: P011ParticleM2FieldBuffer;
  readonly sink: P011ParticleHotPrototypeSink;
  private runToken = 1; private attemptToken = 0; private stepOpen = 0;
  constructor(cols: number, capacity: number, waterTop: Float64Array, bh: Float64Array,
    bl: Float64Array, mh: Float64Array, ml: Float64Array, suspH: Float64Array,
    suspL: Float64Array, surface: Float64Array, params: P011ParticleParams) {
    const config: P011ParticleHotConfig = { cols: cols, nmax: params.nmax,
      requestCapacity: 2048, motionCapacity: params.nmax, eventCapacity: 4096,
      lineageCapacity: 8192, params: params };
    this.work = configureHot(config); warmUpHot(this.work);
    this.field = new P011ParticleM2FieldBuffer(cols, capacity, waterTop, bh, bl, mh, ml, suspH, suspL, surface);
    this.work.field = this.field; this.sink = new P011ParticleHotPrototypeSink(this.work);
  }
  get pool(): P011ParticleField { return this.work.pool; }
  get liveCount(): number { return this.work.pool.totals().count; }
  get active(): boolean { return this.stepOpen !== 0; }
  get totalRemaining(): number { return this.sink.budget.totalRemaining; }
  get totalUsed(): number { return this.sink.budget.totalUsed; }
  registerRun(runId: string): void { registerParticleHotRun(this.work, this.runToken, runId); }
  exportCommittedEvidence(): P011ParticleHotEvidence {
    return exportParticleEvidence(this.work, this.work.committedPageToken);
  }
  discardCommittedPage(): void {
    if (this.work.active === 0 && this.work.pageBacklog === 1 && this.work.committedPageToken > 0) {
      this.work.pageBacklog = 0; this.work.committedPageToken = 0;
    }
  }
  seed(mass: number, x: number, y: number, vx: number, vy: number,
    phase: number, sourceKind: number, sourceId: number): number {
    if (this.stepOpen !== 0) return -1;
    return this.pool.allocate({ mass: mass, x: x, y: y, vx: vx, vy: vy,
      phase: phase, sourceKind: sourceKind, sourceId: sourceId });
  }
  beginCollect(dt: number, tiltRad: number, substepId: number, seed: number,
    sourceHeavy: number, sourceLight: number, sourceEpoch: number): number {
    if (this.stepOpen !== 0) return P011_PARTICLE_M2_BUSY;
    this.attemptToken++;
    this.field.sourceInjectedH = sourceHeavy; this.field.sourceInjectedL = sourceLight;
    this.field.sourceEpoch = sourceEpoch;
    const status = beginStepHot(this.work, { runToken: this.runToken, substepId: substepId,
      attemptToken: this.attemptToken, dt: dt, effectiveTiltRad: tiltRad, seed: seed,
      paramsRevision: 1, inputRevision: 1, field: this.field });
    if (status !== 0) return P011_PARTICLE_M2_STEP_FAILED;
    this.stepOpen = 1;
    const epoch = this.work.stepEpoch;
    if (collectParticlePlanInto(this.work, epoch, this.work.planA) !== 0 ||
      collectParticlePlanInto(this.work, epoch, this.work.planB) !== 0 ||
      compareParticlePlansHot(this.work.planA, this.work.planB) !== 0) {
      this.rollback(); return P011_PARTICLE_M2_STEP_FAILED;
    }
    return 0;
  }
  requested(): number { return this.stepOpen === 0 ? 0 : this.work.planA.totalRequested; }
  apply(scale: number, localRemaining: Float64Array, localUsed: Float64Array, totalRemaining: number): number {
    if (this.stepOpen === 0 || localRemaining.length !== this.work.cols || localUsed.length !== this.work.cols ||
      !Number.isFinite(scale) || scale < 0 || scale > 1) return P011_PARTICLE_M2_STEP_FAILED;
    const budget = this.sink.budget;
    budget.scale = scale; budget.localRemaining.set(localRemaining); budget.localUsed.set(localUsed);
    budget.totalRemaining = totalRemaining;
    const epoch = this.work.stepEpoch;
    const status = applyParticlePlanInto(this.work, epoch, this.work.planA, this.sink, this.work.results);
    if (status !== 0 || finishStepHot(this.work, epoch) !== 0) {
      this.rollback(); return P011_PARTICLE_M2_STEP_FAILED;
    }
    localRemaining.set(budget.localRemaining); localUsed.set(budget.localUsed);
    this.stepOpen = 0; return 0;
  }
  rollback(): void {
    if (this.work.active !== 0) rollbackStepHot(this.work, this.work.stepEpoch);
    this.stepOpen = 0;
  }
  reset(): void { this.rollback(); this.discardCommittedPage(); this.pool.reset(); }
}

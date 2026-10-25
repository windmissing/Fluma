// P-011 D1-A / dual-field-particle-v1 (2026-10-24).
// Isolated M1 foundations. No field, renderer, budget or guard ownership.
export class P011ParticleStateMass {
  count: number; massH: number; massL: number;
  constructor(count: number, massH: number, massL: number) {
    this.count = count; this.massH = massH; this.massL = massL;
  }
}
export class P011ParticleByState {
  FLYING: P011ParticleStateMass; CONTACT_WAIT: P011ParticleStateMass;
  constructor(flying: P011ParticleStateMass, contactWait: P011ParticleStateMass) {
    this.FLYING = flying; this.CONTACT_WAIT = contactWait;
  }
}
export const P011_PARTICLE_ROUTE_ID = 'dual-field-particle-v1';
export const P011_PARTICLE_NMAX_DEFAULT = 512; // candidate, NOT calibrated
export const P011_PARTICLE_HEAVY = 0;
export const P011_PARTICLE_LIGHT = 1;
export const P011_SOURCE_GESTURE = 0;
export const P011_SOURCE_SHAKE = 1;
const UINT32_MAX = 4294967295;

export interface P011ParticleParams {
  massPackage: number;
  nmax: number;
  terminalSpeedHeavy: number;
  terminalSpeedLight: number;
  dragHeavy: number;
  dragLight: number;
  liftSpeedHeavy: number;
  liftSpeedLight: number;
  liftSpread: number;
  mergeColRadius: number;
  mergeHeightRadius: number;
  contactEps: number;
  channelRateCap: number;
}

export function particleRouteEnabled(routeId: string | null,
  params: P011ParticleParams | null): boolean {
  return routeId === P011_PARTICLE_ROUTE_ID && params !== null;
}

export interface P011ParticleFieldView {
  sandTopAt(c: number): number;
  waterTopAt(c: number): number;
  suspAt(c: number): number;
  freeSpaceAt(c: number): number;
  cols: number;
  capacity: number;
  effectiveTiltRad: number;
}
export function freezeParticleFieldView(view: P011ParticleFieldView): P011ParticleFieldView {
  return new P011FrozenParticleView(view);
}
class P011FrozenParticleView implements P011ParticleFieldView {
  readonly cols: number; readonly capacity: number; readonly effectiveTiltRad: number;
  private sand: Float64Array; private water: Float64Array;
  private susp: Float64Array; private free: Float64Array;
  constructor(view: P011ParticleFieldView) {
    this.cols = view.cols; this.capacity = view.capacity; this.effectiveTiltRad = view.effectiveTiltRad;
    this.sand = new Float64Array(view.cols); this.water = new Float64Array(view.cols);
    this.susp = new Float64Array(view.cols); this.free = new Float64Array(view.cols);
    for (let c = 0; c < view.cols; c++) {
      this.sand[c] = view.sandTopAt(c); this.water[c] = view.waterTopAt(c);
      this.susp[c] = view.suspAt(c); this.free[c] = view.freeSpaceAt(c);
      if (!Number.isFinite(this.sand[c]) || !Number.isFinite(this.water[c]) ||
        !Number.isFinite(this.susp[c]) || !Number.isFinite(this.free[c])) throw new Error('PARTICLE_NON_FINITE');
    }
  }
  sandTopAt(c: number): number { return this.sand[c]; }
  waterTopAt(c: number): number { return this.water[c]; }
  suspAt(c: number): number { return this.susp[c]; }
  freeSpaceAt(c: number): number { return this.free[c]; }
}

export interface P011ParticleSnapshot {
  nmax: number; cols: number; liveCount: number; freeCount: number; nextId: number;
  pMass: Float64Array; pX: Float64Array; pY: Float64Array;
  pVx: Float64Array; pVy: Float64Array;
  pPhase: Uint8Array; pSource: Uint8Array; pState: Uint8Array;
  pId: Uint32Array; pSourceId: Uint32Array; freeSlots: Int32Array;
  colParticleMassH: Float64Array; colParticleMassL: Float64Array;
}
export interface P011ParticleHotTransactionStorage {
  before: P011ParticleSnapshot; after: P011ParticleSnapshot; scratch: P011ParticleSnapshot;
}

export type P011ParticleOp = 'spawn' | 'lift' | 'deposit' | 'crossing' | 'degrade' | 'merge';
export interface P011ParticleRequest {
  requestKey: string; substepId: number; ordinal: number;
  kind: P011ParticleOp; particleId: number; sourceId: number;
  phase: number; sourceKind: number; column: number; targetColumn: number;
  requested: number; totalWeight: number;
  predictedX: number; predictedY: number; predictedVx: number; predictedVy: number;
  mergedIds: number[];
}
export interface P011ParticleStepIdentity {
  runId: string; substepId: number; dt: number; effectiveTiltRad: number;
  seed: number; paramsRevision: number; inputRevision: number;
}
export interface P011ParticlePlan {
  identity: P011ParticleStepIdentity;
  requestCount: number; requests: P011ParticleRequest[];
  totalRequested: number; requestHash: string;
  motions?: P011ParticleMotion[];
  motionHash?: string;
}
export interface P011ParticleCheckpoint {
  state: P011ParticleSnapshot; stepId: number; appliedKeys: string[];
}
export interface P011ParticleMotion {
  particleId: number; column: number; x: number; y: number; vx: number; vy: number;
}
export interface P011ParticleBudgetTicket {
  ticketId: number; requestKey: string; substepId: number;
  source: number; target: number; requested: number; localAllowed: number;
  reserved: number; refunded: number;
}
export interface P011ParticleBudgetSink {
  reserve(request: P011ParticleRequest): P011ParticleBudgetTicket;
  refund(ticketId: number, amount: number): void;
  close(ticketId: number, committed: number): void;
}
export type P011ParticleDisposition = 'PARTICLE' | 'BED' | 'SUSP' | 'MOVE' | 'MERGE' | 'WAIT';
export interface P011ParticleGrant {
  requestKey: string; grantedHeavy: number; grantedLight: number;
  disposition: P011ParticleDisposition;
}
export interface P011ParticlePoolPatch { patchId: number; requestKey: string; }
export interface P011ParticleAtomicPool {
  prepare(request: P011ParticleRequest, grant: P011ParticleGrant): P011ParticlePoolPatch | null;
  publish(patch: P011ParticlePoolPatch): void;
  restore(patch: P011ParticlePoolPatch): void;
  discard(patch: P011ParticlePoolPatch): void;
}
export interface P011ParticleCommitSink {
  apply(request: P011ParticleRequest, pool: P011ParticleAtomicPool): P011ParticleApplyResult;
}
export interface P011ParticleMergeRecord {
  survivor: number; absorbed: number; phase: number; sourceKind: number; sourceId: number;
  column: number; massBefore: number; massAfter: number;
}
export interface P011ParticleRepresentationRecord { kind: 'birth' | 'merge'; index: number; }
export interface P011ParticleBirthRecord extends P011ParticleSeed { particleId: number; }

class P011ParticlePreparedPatch implements P011ParticlePoolPatch {
  patchId: number; requestKey: string; substepId: number;
  before: P011ParticleSnapshot; after: P011ParticleSnapshot;
  published = false;
  merges: P011ParticleMergeRecord[] = [];
  births: P011ParticleBirthRecord[] = [];
  actions: P011ParticleRepresentationRecord[] = [];
  constructor(id: number, key: string, step: number,
    before: P011ParticleSnapshot, after: P011ParticleSnapshot) {
    this.patchId = id; this.requestKey = key; this.substepId = step;
    this.before = before; this.after = after;
  }
}
export interface P011ParticleRequestCollector {
  record(request: P011ParticleRequest): void;
  finish(): P011ParticlePlan;
}
export interface P011ParticleGuardDecision {
  accepted: number; rejected: number; allLegalBefore: boolean;
  limitingEdge: number | null; affectedEdgeIds: number[];
}
export interface P011ParticleApplyResult {
  requestKey: string; committed: boolean; errorCode: string | null;
  requested: number; granted: number; remainder: number;
  budgetClipped: number; guardRejected: number;
  capacityClipped: number; poolClipped: number;
  guard: P011ParticleGuardDecision | null;
  eventBegin: number; eventCount: number;
}

// A-stage sink intentionally has no canonical field-writing capabilities.
// B-stage will add the approved atomic-pool/commit/budget protocol.
export interface P011ParticlePlanSink {
  apply(request: P011ParticleRequest): P011ParticleApplyResult;
}

export interface P011ParticleSeed {
  mass: number; x: number; y: number; vx: number; vy: number;
  phase: number; sourceKind: number; sourceId: number;
}
export interface P011ParticleTotals {
  heavy: number; light: number; count: number; resting: number;
}

function requireFinite(value: number): void {
  if (!Number.isFinite(value)) throw new Error('PARTICLE_NON_FINITE');
}
function requireIndex(value: number, length: number): void {
  if (!Number.isInteger(value) || value < 0 || value >= length) {
    throw new Error('PARTICLE_INVALID_INDEX');
  }
}

export function validateParticleParams(params: P011ParticleParams): void {
  const values: number[] = [params.massPackage, params.nmax, params.terminalSpeedHeavy,
    params.terminalSpeedLight, params.dragHeavy, params.dragLight, params.liftSpeedHeavy,
    params.liftSpeedLight, params.liftSpread, params.mergeColRadius,
    params.mergeHeightRadius, params.contactEps, params.channelRateCap];
  for (let i = 0; i < values.length; i++) if (!Number.isFinite(values[i]) || values[i] < 0) {
    throw new Error('PARTICLE_INVALID_PARAMS');
  }
  if (params.massPackage <= 0 || !Number.isInteger(params.nmax) || params.nmax <= 0 ||
    params.contactEps <= 0 || params.channelRateCap !== 0) throw new Error('PARTICLE_INVALID_PARAMS');
}

export function validateParticleRequest(r: P011ParticleRequest, cols: number): void {
  requireIndex(r.column, cols);
  if (!Number.isFinite(r.requested) || r.requested < 0 || !r.requestKey ||
    (r.phase !== 0 && r.phase !== 1) || (r.sourceKind !== 0 && r.sourceKind !== 1) ||
    !Number.isInteger(r.sourceId) || r.sourceId <= 0 || r.sourceId > UINT32_MAX ||
    !Number.isInteger(r.particleId) || r.particleId < 0 || r.particleId > UINT32_MAX) {
    throw new Error('PARTICLE_INVALID_REQUEST');
  }
  if (r.kind !== 'spawn' && r.kind !== 'lift' && r.kind !== 'deposit' &&
    r.kind !== 'crossing' && r.kind !== 'merge' && r.kind !== 'degrade') {
    throw new Error('PARTICLE_INVALID_OPERATION');
  }
  if ((r.kind === 'spawn' && r.sourceKind !== P011_SOURCE_GESTURE) ||
    (r.kind === 'lift' && r.sourceKind !== P011_SOURCE_SHAKE)) throw new Error('PARTICLE_SOURCE_MISMATCH');
  requireFinite(r.predictedX); requireFinite(r.predictedY);
  requireFinite(r.predictedVx); requireFinite(r.predictedVy);
  const weight = r.kind === 'deposit' || r.kind === 'crossing' ? r.requested : 0;
  if (r.totalWeight !== weight) throw new Error('PARTICLE_INVALID_WEIGHT');
  if (r.kind === 'crossing') {
    requireIndex(r.targetColumn, cols);
    if (Math.abs(r.targetColumn - r.column) !== 1) throw new Error('PARTICLE_CFL');
  }
}

// Canonical pool only. These local primitives do not publish field events;
// D1-B must coordinate pool+field changes atomically before production use.
export class P011ParticleField implements P011ParticleAtomicPool {
  private readonly pMass: Float64Array;
  private readonly pX: Float64Array;
  private readonly pY: Float64Array;
  private readonly pVx: Float64Array;
  private readonly pVy: Float64Array;
  private readonly pPhase: Uint8Array;
  private readonly pSource: Uint8Array;
  private readonly pState: Uint8Array;
  private readonly pId: Uint32Array;
  private readonly pSourceId: Uint32Array;
  private readonly freeSlots: Int32Array;
  private readonly colParticleMassH: Float64Array;
  private readonly colParticleMassL: Float64Array;
  private liveCount = 0;
  private freeCount: number;
  private nextId = 1;
  readonly cols: number;
  readonly nmax: number;
  private stepId = -1;
  private params: P011ParticleParams | null = null;
  private view: P011ParticleFieldView | null = null;
  private patch: P011ParticlePreparedPatch | null = null;
  private nextPatchId = 1;
  private appliedKeys: Set<string> = new Set<string>();
  private stepIdentityKey = '';
  private stepSeed = 0;
  private mergeRecords: P011ParticleMergeRecord[] = [];
  private birthRecords: P011ParticleBirthRecord[] = [];
  private representationRecords: P011ParticleRepresentationRecord[] = [];
  private transactionScratch: P011ParticleField | null = null;
  private transactionBefore: P011ParticleSnapshot | null = null;
  private transactionAfter: P011ParticleSnapshot | null = null;

  constructor(cols: number, nmax: number = P011_PARTICLE_NMAX_DEFAULT) {
    if (!Number.isInteger(cols) || cols <= 0 || !Number.isInteger(nmax) || nmax <= 0) {
      throw new Error('PARTICLE_INVALID_SIZE');
    }
    this.cols = cols; this.nmax = nmax; this.freeCount = nmax;
    this.pMass = new Float64Array(nmax); this.pX = new Float64Array(nmax);
    this.pY = new Float64Array(nmax); this.pVx = new Float64Array(nmax);
    this.pVy = new Float64Array(nmax); this.pPhase = new Uint8Array(nmax);
    this.pSource = new Uint8Array(nmax); this.pState = new Uint8Array(nmax);
    this.pId = new Uint32Array(nmax); this.pSourceId = new Uint32Array(nmax);
    this.freeSlots = new Int32Array(nmax);
    this.colParticleMassH = new Float64Array(cols);
    this.colParticleMassL = new Float64Array(cols);
    for (let i = 0; i < nmax; i++) this.freeSlots[i] = nmax - 1 - i;
  }

  // O(1) allocation, no scan. -1 means capacity refusal, zero writes.
  allocate(seed: P011ParticleSeed): number {
    if (this.patch !== null) throw new Error('PARTICLE_PATCH_BUSY');
    requireFinite(seed.mass); requireFinite(seed.x); requireFinite(seed.y);
    requireFinite(seed.vx); requireFinite(seed.vy);
    if (seed.mass <= 0 || seed.x < 0 || seed.x >= this.cols || seed.y < 0 ||
      (seed.phase !== 0 && seed.phase !== 1) ||
      (seed.sourceKind !== 0 && seed.sourceKind !== 1) ||
      !Number.isInteger(seed.sourceId) || seed.sourceId <= 0 || seed.sourceId > UINT32_MAX) {
      throw new Error('PARTICLE_INVALID_SEED');
    }
    if (this.freeCount === 0) return -1;
    if (this.nextId > UINT32_MAX) throw new Error('PARTICLE_ID_EXHAUSTED');
    const slot = this.freeSlots[--this.freeCount];
    this.pMass[slot] = seed.mass; this.pX[slot] = seed.x; this.pY[slot] = seed.y;
    this.pVx[slot] = seed.vx; this.pVy[slot] = seed.vy;
    this.pPhase[slot] = seed.phase; this.pSource[slot] = seed.sourceKind;
    this.pSourceId[slot] = seed.sourceId; this.pId[slot] = this.nextId++;
    this.pState[slot] = 1; this.liveCount++;
    this.cache(seed.phase)[Math.floor(seed.x)] += seed.mass;
    return slot;
  }

  // CONS-03 primitive: retain exact remainder, including sub-EPS tails.
  // The caller's isolated stub supplies the matching bed delta; production
  // field atomic publication is deliberately deferred to D1-B/D2.
  retainAfterDeposit(slot: number, granted: number, sandTopAfter: number,
    contactEps: number): number {
    if (this.patch !== null) throw new Error('PARTICLE_PATCH_BUSY');
    this.requireLive(slot); requireFinite(granted); requireFinite(sandTopAfter);
    requireFinite(contactEps);
    const mass = this.pMass[slot];
    if (granted < 0 || granted > mass || sandTopAfter < 0 || contactEps <= 0 ||
      !Number.isFinite(sandTopAfter + contactEps)) throw new Error('PARTICLE_INVALID_GRANT');
    const remainder = granted === mass ? 0 : mass - granted;
    this.cache(this.pPhase[slot])[Math.floor(this.pX[slot])] -= granted;
    this.pMass[slot] = remainder;
    if (remainder === 0) {
      this.pState[slot] = 0; this.liveCount--;
      this.freeSlots[this.freeCount++] = slot;
    } else {
      this.pState[slot] = 2; this.pY[slot] = sandTopAfter + contactEps;
      this.pVy[slot] = 0;
    }
    return remainder;
  }

  totals(): P011ParticleTotals {
    let heavy = 0; let light = 0; let resting = 0;
    for (let c = 0; c < this.cols; c++) {
      heavy += this.colParticleMassH[c]; light += this.colParticleMassL[c];
    }
    for (let i = 0; i < this.nmax; i++) if (this.pState[i] === 2) resting++;
    return { heavy: heavy, light: light, count: this.liveCount, resting: resting };
  }
  endpointByState(): P011ParticleByState {
    const flying = new P011ParticleStateMass(0, 0, 0);
    const contactWait = new P011ParticleStateMass(0, 0, 0);
    const result = new P011ParticleByState(flying, contactWait);
    for (let i = 0; i < this.nmax; i++) {
      if (this.pState[i] === 0) continue;
      if ((this.pState[i] !== 1 && this.pState[i] !== 2) ||
        (this.pPhase[i] !== 0 && this.pPhase[i] !== 1) || !Number.isFinite(this.pMass[i]) || this.pMass[i] <= 0) {
        throw new Error('INVALID_LEDGER');
      }
      const group = this.pState[i] === 1 ? result.FLYING : result.CONTACT_WAIT;
      group.count++;
      if (this.pPhase[i] === 0) group.massH += this.pMass[i]; else group.massL += this.pMass[i];
    }
    const totals = this.totals();
    if (result.FLYING.count + result.CONTACT_WAIT.count !== totals.count ||
      Math.abs(result.FLYING.massH + result.CONTACT_WAIT.massH - totals.heavy) > 1e-9 ||
      Math.abs(result.FLYING.massL + result.CONTACT_WAIT.massL - totals.light) > 1e-9) throw new Error('INVALID_LEDGER');
    return result;
  }

  reset(): void {
    if (this.patch !== null) throw new Error('PARTICLE_PATCH_BUSY');
    this.pMass.fill(0); this.pX.fill(0); this.pY.fill(0);
    this.pVx.fill(0); this.pVy.fill(0); this.pPhase.fill(0);
    this.pSource.fill(0); this.pState.fill(0); this.pId.fill(0); this.pSourceId.fill(0);
    this.colParticleMassH.fill(0); this.colParticleMassL.fill(0);
    this.liveCount = 0; this.freeCount = this.nmax;
    for (let i = 0; i < this.nmax; i++) this.freeSlots[i] = this.nmax - 1 - i;
    // IDs never repeat across reset. Exhaustion must not wrap.
    this.appliedKeys.clear(); this.stepId = -1;
  }

  beginStep(identity: P011ParticleStepIdentity, params: P011ParticleParams,
    view: P011ParticleFieldView): void {
    if (this.patch !== null) throw new Error('PARTICLE_PATCH_BUSY');
    validateParticleParams(params);
    if (params.nmax !== this.nmax || view.cols !== this.cols ||
      !Number.isInteger(identity.substepId) || identity.substepId < 0) {
      throw new Error('PARTICLE_INVALID_INPUT');
    }
    if (identity.substepId !== this.stepId) this.appliedKeys.clear();
    this.stepId = identity.substepId;
    this.stepIdentityKey = JSON.stringify(identity);
    this.stepSeed = identity.seed;
    this.params = { massPackage: params.massPackage, nmax: params.nmax,
      terminalSpeedHeavy: params.terminalSpeedHeavy, terminalSpeedLight: params.terminalSpeedLight,
      dragHeavy: params.dragHeavy, dragLight: params.dragLight, liftSpeedHeavy: params.liftSpeedHeavy,
      liftSpeedLight: params.liftSpeedLight, liftSpread: params.liftSpread,
      mergeColRadius: params.mergeColRadius, mergeHeightRadius: params.mergeHeightRadius,
      contactEps: params.contactEps, channelRateCap: params.channelRateCap };
    this.view = freezeParticleFieldView(view);
  }

  collectParticlePlan(identity: P011ParticleStepIdentity, frozenField: P011ParticleFieldView,
    frozenPool: P011ParticleSnapshot, collector: P011ParticleRequestCollector): P011ParticlePlan {
    if (this.params === null) throw new Error('PARTICLE_NOT_CONFIGURED');
    return collectParticlePlan(identity, frozenField, frozenPool, this.params, collector);
  }
  applyParticlePlan(plan: P011ParticlePlan, sink: P011ParticleCommitSink): P011ParticleApplyResult[] {
    if (plan.identity.substepId !== this.stepId || plan.requestCount !== plan.requests.length ||
      JSON.stringify(plan.identity) !== this.stepIdentityKey ||
      JSON.stringify(plan.requests) !== plan.requestHash ||
      (plan.motions !== undefined && JSON.stringify(plan.motions) !== plan.motionHash)) {
      throw new Error('PARTICLE_PLAN_MISMATCH');
    }
    let denominator = 0;
    for (let i = 0; i < plan.requests.length; i++) {
      validateParticleRequest(plan.requests[i], this.cols);
      denominator += plan.requests[i].totalWeight;
    }
    if (denominator !== plan.totalRequested) throw new Error('PARTICLE_PLAN_MISMATCH');
    const results: P011ParticleApplyResult[] = [];
    for (let i = 0; i < plan.requestCount; i++) {
      const r = plan.requests[i];
      const result = sink.apply(r, this);
      const amounts: number[] = [result.requested, result.granted, result.remainder,
        result.budgetClipped, result.guardRejected, result.capacityClipped, result.poolClipped];
      for (let j = 0; j < amounts.length; j++) if (!Number.isFinite(amounts[j]) || amounts[j] < 0) {
        throw new Error('PARTICLE_JOURNAL_MISMATCH');
      }
      if (result.requestKey !== r.requestKey || result.requested !== r.requested ||
        !Number.isFinite(result.granted) || result.granted < 0 ||
        Math.abs(r.requested - result.granted - result.budgetClipped - result.guardRejected -
          result.capacityClipped - result.poolClipped) > 1e-9) throw new Error('PARTICLE_JOURNAL_MISMATCH');
      results.push(result);
      if (!result.committed) return results; // outer owner restores whole substep
    }
    if (plan.motions !== undefined) this.applyMotions(plan.motions);
    return results;
  }

  // Canonical step checkpoints include every SoA, free-stack and ID field.
  // D2 must restore its field/budget/journal checkpoint alongside this one.
  restoreSnapshot(state: P011ParticleSnapshot): void {
    if (this.patch !== null) throw new Error('PARTICLE_PATCH_BUSY');
    if (state.cols !== this.cols || state.nmax !== this.nmax) throw new Error('PARTICLE_INVALID_SIZE');
    this.copySnapshot(state);
  }
  checkpoint(): P011ParticleCheckpoint {
    if (this.patch !== null) throw new Error('PARTICLE_PATCH_BUSY');
    return { state: this.snapshot(), stepId: this.stepId, appliedKeys: Array.from(this.appliedKeys) };
  }
  rollback(checkpoint: P011ParticleCheckpoint): void {
    if (this.patch !== null || checkpoint.stepId !== this.stepId) throw new Error('PARTICLE_STALE_CHECKPOINT');
    this.copySnapshot(checkpoint.state); this.appliedKeys = new Set<string>(checkpoint.appliedKeys);
  }

  private copySnapshot(s: P011ParticleSnapshot): void {
    this.pMass.set(s.pMass); this.pX.set(s.pX); this.pY.set(s.pY);
    this.pVx.set(s.pVx); this.pVy.set(s.pVy); this.pPhase.set(s.pPhase);
    this.pSource.set(s.pSource); this.pState.set(s.pState); this.pId.set(s.pId);
    this.pSourceId.set(s.pSourceId); this.freeSlots.set(s.freeSlots);
    this.colParticleMassH.set(s.colParticleMassH); this.colParticleMassL.set(s.colParticleMassL);
    this.freeCount = s.freeCount; this.liveCount = s.liveCount; this.nextId = s.nextId;
  }

  prepare(request: P011ParticleRequest, grant: P011ParticleGrant): P011ParticlePoolPatch | null {
    if (this.patch !== null) throw new Error('PARTICLE_PATCH_BUSY');
    if (this.params === null || this.view === null || request.substepId !== this.stepId ||
      request.requestKey !== grant.requestKey || this.appliedKeys.has(request.requestKey)) {
      throw new Error('PARTICLE_REQUEST_IDENTITY');
    }
    validateParticleRequest(request, this.cols);
    requireFinite(grant.grantedHeavy); requireFinite(grant.grantedLight);
    const amount = grant.grantedHeavy + grant.grantedLight;
    if (grant.grantedHeavy < 0 || grant.grantedLight < 0 || amount > request.requested + 1e-9) {
      throw new Error('PARTICLE_INVALID_GRANT');
    }
    this.warmUpTransactions();
    const before = this.transactionBefore;
    const after = this.transactionAfter;
    const scratch = this.transactionScratch;
    if (before === null || after === null || scratch === null) throw new Error('PARTICLE_NOT_CONFIGURED');
    this.copyStateTo(before);
    scratch.copySnapshot(before);
    scratch.stepSeed = this.stepSeed;
    scratch.mergeRecords.length = 0; scratch.birthRecords.length = 0;
    scratch.representationRecords.length = 0;
    if (!scratch.edit(request, grant, this.params, this.view)) return null;
    scratch.copyStateTo(after);
    this.patch = new P011ParticlePreparedPatch(this.nextPatchId++, request.requestKey,
      this.stepId, before, after);
    this.patch.merges = scratch.mergeRecords;
    this.patch.births = scratch.birthRecords;
    this.patch.actions = scratch.representationRecords;
    return this.patch;
  }
  patchMerges(patch: P011ParticlePoolPatch): P011ParticleMergeRecord[] {
    return this.requirePatch(patch).merges;
  }
  patchBirths(patch: P011ParticlePoolPatch): P011ParticleBirthRecord[] {
    return this.requirePatch(patch).births;
  }
  patchActions(patch: P011ParticlePoolPatch): P011ParticleRepresentationRecord[] {
    return this.requirePatch(patch).actions;
  }

  publish(patch: P011ParticlePoolPatch): void {
    const active = this.requirePatch(patch);
    if (active.published) throw new Error('PARTICLE_PATCH_ALREADY_PUBLISHED');
    // No allocation or user callback between these copies.
    this.copySnapshot(active.after); active.published = true;
  }
  restore(patch: P011ParticlePoolPatch): void {
    const active = this.requirePatch(patch);
    if (!active.published) throw new Error('PARTICLE_PATCH_NOT_PUBLISHED');
    this.copySnapshot(active.before); this.patch = null;
  }
  discard(patch: P011ParticlePoolPatch): void {
    const active = this.requirePatch(patch);
    if (active.published) throw new Error('PARTICLE_PATCH_ALREADY_PUBLISHED');
    this.patch = null;
  }
  // Finalize only after the sink's field+pool audit and budget close succeed.
  finishPatch(patch: P011ParticlePoolPatch): void {
    const active = this.requirePatch(patch);
    if (!active.published) throw new Error('PARTICLE_PATCH_NOT_PUBLISHED');
    this.appliedKeys.add(active.requestKey); this.patch = null;
  }
  private requirePatch(patch: P011ParticlePoolPatch): P011ParticlePreparedPatch {
    if (this.patch === null || patch !== this.patch || this.patch.substepId !== this.stepId) {
      throw new Error('PARTICLE_STALE_PATCH');
    }
    return this.patch;
  }
  private slotFor(id: number): number {
    for (let i = 0; i < this.nmax; i++) if (this.pState[i] !== 0 && this.pId[i] === id) return i;
    throw new Error('PARTICLE_UNKNOWN_ID');
  }
  private remove(slot: number): void {
    this.cache(this.pPhase[slot])[Math.floor(this.pX[slot])] -= this.pMass[slot];
    this.pMass[slot] = 0; this.pState[slot] = 0;
    this.freeSlots[this.freeCount++] = slot; this.liveCount--;
  }
  private mergePair(a: number, b: number, params: P011ParticleParams): boolean {
    if (a === b || this.pPhase[a] !== this.pPhase[b] || this.pSource[a] !== this.pSource[b] ||
      this.pSourceId[a] !== this.pSourceId[b] || Math.floor(this.pX[a]) !== Math.floor(this.pX[b]) ||
      Math.abs(this.pY[a] - this.pY[b]) > params.mergeHeightRadius || this.pState[a] !== this.pState[b]) return false;
    const ma = this.pMass[a]; const mb = this.pMass[b]; const mass = ma + mb;
    this.representationRecords.push({ kind: 'merge', index: this.mergeRecords.length });
    this.mergeRecords.push({ survivor: this.pId[a], absorbed: this.pId[b],
      phase: this.pPhase[a], sourceKind: this.pSource[a], sourceId: this.pSourceId[a],
      column: Math.floor(this.pX[a]), massBefore: mass, massAfter: mass });
    this.pX[a] = (this.pX[a] * ma + this.pX[b] * mb) / mass;
    this.pY[a] = (this.pY[a] * ma + this.pY[b] * mb) / mass;
    this.pVx[a] = (this.pVx[a] * ma + this.pVx[b] * mb) / mass;
    this.pVy[a] = (this.pVy[a] * ma + this.pVy[b] * mb) / mass;
    this.pMass[a] = mass;
    // Column cache is invariant; remove only the absorbed slot metadata.
    this.pMass[b] = 0; this.pState[b] = 0; this.liveCount--;
    this.freeSlots[this.freeCount++] = b;
    return true;
  }
  private mergeOldest(params: P011ParticleParams): boolean {
    let a = -1; let b = -1;
    for (let i = 0; i < this.nmax; i++) if (this.pState[i] !== 0) {
      for (let j = 0; j < this.nmax; j++) if (this.pState[j] !== 0 && this.pId[i] < this.pId[j] &&
        this.pPhase[i] === this.pPhase[j] && this.pSource[i] === this.pSource[j] &&
        this.pSourceId[i] === this.pSourceId[j] && this.pState[i] === this.pState[j] &&
        Math.floor(this.pX[i]) === Math.floor(this.pX[j]) &&
        Math.abs(this.pY[i] - this.pY[j]) <= params.mergeHeightRadius &&
        (a < 0 || this.pId[i] < this.pId[a] ||
          (this.pId[i] === this.pId[a] && this.pId[j] < this.pId[b]))) { a = i; b = j; }
    }
    return a >= 0 && this.mergePair(a, b, params);
  }
  private edit(r: P011ParticleRequest, g: P011ParticleGrant, params: P011ParticleParams,
    view: P011ParticleFieldView): boolean {
    const amount = g.grantedHeavy + g.grantedLight;
    if (r.kind === 'spawn' || r.kind === 'lift') {
      if (g.disposition === 'SUSP') return r.kind === 'lift'; // field owns fallback
      if (g.disposition === 'WAIT') return amount === 0;
      if (g.disposition !== 'PARTICLE') throw new Error('PARTICLE_INVALID_DISPOSITION');
      for (let phase = 0; phase < 2; phase++) {
        const mass = phase === 0 ? g.grantedHeavy : g.grantedLight;
        const count = Math.floor(mass / params.massPackage);
        const tail = mass - count * params.massPackage;
        for (let k = 0; k < count + (tail > 0 ? 1 : 0); k++) {
          if (this.freeCount === 0 && !this.mergeOldest(params)) return false;
          const packageMass = k < count ? params.massPackage : tail;
          // Counter-based deterministic jitter; no mutable/global RNG.
          const salt = (Math.imul(r.sourceId, 1664525) ^ Math.imul(r.ordinal + k + phase * 8191, 1013904223) ^ this.stepSeed) >>> 0;
          const jitter = (salt / 4294967296 - 0.5) * 0.999;
          const x = r.column + 0.5 + jitter;
          const low = r.kind === 'lift' ? r.predictedY : view.sandTopAt(r.column) + params.contactEps;
          const high = view.waterTopAt(r.column) - params.contactEps;
          if (high < low) return false;
          const speed = phase === 0 ? params.terminalSpeedHeavy : params.terminalSpeedLight;
          const lift = phase === 0 ? params.liftSpeedHeavy : params.liftSpeedLight;
          const birth: P011ParticleSeed = { mass: packageMass, x: x,
            y: r.kind === 'lift' ? low : Math.max(low, Math.min(high, r.predictedY)),
            vx: r.kind === 'lift' ? jitter * 2 * params.liftSpread : speed * Math.sin(view.effectiveTiltRad),
            vy: r.kind === 'lift' ? lift : -speed * Math.cos(view.effectiveTiltRad),
            phase: phase, sourceKind: r.sourceKind, sourceId: r.sourceId };
          const slot = this.allocate(birth);
          this.representationRecords.push({ kind: 'birth', index: this.birthRecords.length });
          this.birthRecords.push({ particleId: this.pId[slot], mass: birth.mass,
            x: birth.x, y: birth.y, vx: birth.vx, vy: birth.vy,
            phase: birth.phase, sourceKind: birth.sourceKind, sourceId: birth.sourceId });
        }
      }
      return true;
    }
    const slot = this.slotFor(r.particleId);
    if (this.pPhase[slot] !== r.phase || this.pSource[slot] !== r.sourceKind ||
      this.pSourceId[slot] !== r.sourceId || Math.floor(this.pX[slot]) !== r.column ||
      this.pMass[slot] !== r.requested || (r.phase === 0 ? g.grantedLight : g.grantedHeavy) !== 0) {
      throw new Error('PARTICLE_SOURCE_MISMATCH');
    }
    if (r.kind === 'deposit') {
      if (g.disposition !== 'BED' && g.disposition !== 'WAIT') throw new Error('PARTICLE_INVALID_DISPOSITION');
      this.retainAfterDeposit(slot, amount, r.predictedY - params.contactEps, params.contactEps);
    } else if (r.kind === 'degrade') {
      if (g.disposition !== 'SUSP' || amount !== this.pMass[slot]) return false;
      this.remove(slot);
    } else if (r.kind === 'crossing') {
      if (g.disposition === 'WAIT' && amount === 0) {
        this.pX[slot] = r.targetColumn > r.column ? r.column + 1 - 1e-9 : r.column + 1e-9;
      } else {
        if (g.disposition !== 'MOVE' || amount !== this.pMass[slot] ||
          Math.floor(r.predictedX) !== r.targetColumn) return false;
        this.cache(r.phase)[r.column] -= amount; this.cache(r.phase)[r.targetColumn] += amount;
        this.pX[slot] = r.predictedX; this.pY[slot] = r.predictedY;
        this.pVx[slot] = r.predictedVx; this.pVy[slot] = r.predictedVy;
      }
    } else if (r.kind === 'merge') {
      if (g.disposition !== 'MERGE' || r.mergedIds.length !== 2 ||
        r.mergedIds[0] !== r.particleId || !this.mergePair(slot, this.slotFor(r.mergedIds[1]), params)) return false;
    }
    return true;
  }

  // Free flight stays in its original column; every crossing is a request.
  // Outer substep rollback owns this publication together with sink fields.
  applyMotions(motions: P011ParticleMotion[]): void {
    if (this.patch !== null) throw new Error('PARTICLE_PATCH_BUSY');
    for (let i = 0; i < motions.length; i++) {
      const m = motions[i]; const slot = this.slotFor(m.particleId);
      if (Math.floor(this.pX[slot]) !== m.column || Math.floor(m.x) !== m.column) {
        throw new Error('PARTICLE_FREE_CROSSING');
      }
      requireFinite(m.x); requireFinite(m.y); requireFinite(m.vx); requireFinite(m.vy);
    }
    for (let i = 0; i < motions.length; i++) {
      const m = motions[i]; const slot = this.slotFor(m.particleId);
      this.pX[slot] = m.x; this.pY[slot] = m.y; this.pVx[slot] = m.vx; this.pVy[slot] = m.vy;
    }
  }

  snapshot(): P011ParticleSnapshot {
    return {
      nmax: this.nmax, cols: this.cols, liveCount: this.liveCount,
      freeCount: this.freeCount, nextId: this.nextId,
      pMass: this.pMass.slice(), pX: this.pX.slice(), pY: this.pY.slice(),
      pVx: this.pVx.slice(), pVy: this.pVy.slice(), pPhase: this.pPhase.slice(),
      pSource: this.pSource.slice(), pState: this.pState.slice(), pId: this.pId.slice(),
      pSourceId: this.pSourceId.slice(), freeSlots: this.freeSlots.slice(),
      colParticleMassH: this.colParticleMassH.slice(),
      colParticleMassL: this.colParticleMassL.slice()
    };
  }

  // Call before the transaction loop. Diagnostic snapshot() still returns
  // an owned copy; production adapters can reuse copyStateTo() buffers.
  warmUpTransactions(): void {
    if (this.transactionScratch !== null) return;
    this.transactionScratch = new P011ParticleField(this.cols, this.nmax);
    this.transactionBefore = this.snapshot(); this.transactionAfter = this.snapshot();
  }
  // COLD binding only. Reuses the existing transaction pool and images;
  // the borrowed wrapper allocates no second set of particle arrays.
  bindHotTransactionStorage(): P011ParticleHotTransactionStorage {
    this.warmUpTransactions();
    const scratch = this.transactionScratch;
    if (scratch === null || this.transactionBefore === null || this.transactionAfter === null) {
      throw new Error('PARTICLE_NOT_CONFIGURED');
    }
    return { before: this.transactionBefore, after: this.transactionAfter, scratch: scratch.borrowHotState() };
  }
  private borrowHotState(): P011ParticleSnapshot {
    return { nmax: this.nmax, cols: this.cols, liveCount: this.liveCount, freeCount: this.freeCount, nextId: this.nextId,
      pMass: this.pMass, pX: this.pX, pY: this.pY, pVx: this.pVx, pVy: this.pVy,
      pPhase: this.pPhase, pSource: this.pSource, pState: this.pState, pId: this.pId,
      pSourceId: this.pSourceId, freeSlots: this.freeSlots,
      colParticleMassH: this.colParticleMassH, colParticleMassL: this.colParticleMassL };
  }
  // HOT numeric copy boundary. All expected errors precede the first write.
  transferHotState(s: P011ParticleSnapshot, restore: boolean): number {
    if (this.patch !== null) return 6;
    if (s.nmax !== this.nmax || s.cols !== this.cols || s.pMass.length !== this.nmax ||
      s.pX.length !== this.nmax || s.pY.length !== this.nmax || s.pVx.length !== this.nmax ||
      s.pVy.length !== this.nmax || s.pPhase.length !== this.nmax || s.pSource.length !== this.nmax ||
      s.pState.length !== this.nmax || s.pId.length !== this.nmax || s.pSourceId.length !== this.nmax ||
      s.freeSlots.length !== this.nmax || s.colParticleMassH.length !== this.cols || s.colParticleMassL.length !== this.cols) return 3;
    if (restore) this.copySnapshot(s);
    else {
      s.liveCount = this.liveCount; s.freeCount = this.freeCount; s.nextId = this.nextId;
      s.pMass.set(this.pMass); s.pX.set(this.pX); s.pY.set(this.pY); s.pVx.set(this.pVx); s.pVy.set(this.pVy);
      s.pPhase.set(this.pPhase); s.pSource.set(this.pSource); s.pState.set(this.pState); s.pId.set(this.pId);
      s.pSourceId.set(this.pSourceId); s.freeSlots.set(this.freeSlots);
      s.colParticleMassH.set(this.colParticleMassH); s.colParticleMassL.set(this.colParticleMassL);
    }
    return 0;
  }
  copyStateTo(out: P011ParticleSnapshot): void {
    if (out.nmax !== this.nmax || out.cols !== this.cols || out.pMass.length !== this.nmax ||
      out.freeSlots.length !== this.nmax || out.colParticleMassH.length !== this.cols) {
      throw new Error('PARTICLE_INVALID_SIZE');
    }
    out.liveCount = this.liveCount; out.freeCount = this.freeCount; out.nextId = this.nextId;
    out.pMass.set(this.pMass); out.pX.set(this.pX); out.pY.set(this.pY);
    out.pVx.set(this.pVx); out.pVy.set(this.pVy); out.pPhase.set(this.pPhase);
    out.pSource.set(this.pSource); out.pState.set(this.pState); out.pId.set(this.pId);
    out.pSourceId.set(this.pSourceId); out.freeSlots.set(this.freeSlots);
    out.colParticleMassH.set(this.colParticleMassH); out.colParticleMassL.set(this.colParticleMassL);
  }

  private cache(phase: number): Float64Array {
    return phase === P011_PARTICLE_HEAVY ? this.colParticleMassH : this.colParticleMassL;
  }
  private requireLive(slot: number): void {
    requireIndex(slot, this.nmax);
    if (this.pState[slot] === 0) throw new Error('PARTICLE_EMPTY_SLOT');
  }
}

// Bounded request journal for A-stage plans. Overflow is an error rather
// than ring overwrite: caller discards the plan and rolls back the substep.
export class P011ParticleJournal implements P011ParticleRequestCollector {
  private readonly requests: P011ParticleRequest[] = [];
  private count = 0;
  private totalRequested = 0;
  private failed = false;
  private finished = false;
  private readonly identity: P011ParticleStepIdentity;
  private readonly limit: number;

  constructor(identity: P011ParticleStepIdentity, limit: number = 4096) {
    if (!Number.isInteger(limit) || limit <= 0) throw new Error('PARTICLE_INVALID_SIZE');
    this.identity = { runId: identity.runId, substepId: identity.substepId, dt: identity.dt,
      effectiveTiltRad: identity.effectiveTiltRad, seed: identity.seed,
      paramsRevision: identity.paramsRevision, inputRevision: identity.inputRevision };
    this.limit = limit;
    for (let i = 0; i < limit; i++) {
      this.requests.push({ requestKey: '', substepId: 0, ordinal: 0,
        kind: 'spawn', particleId: 0, sourceId: 0, phase: 0, sourceKind: 0,
        column: 0, targetColumn: -1, requested: 0, totalWeight: 0,
        predictedX: 0, predictedY: 0, predictedVx: 0, predictedVy: 0, mergedIds: [] });
    }
  }
  record(request: P011ParticleRequest): void {
    if (this.failed || this.finished) throw new Error('PARTICLE_JOURNAL_CLOSED');
    if (this.count === this.limit) {
      this.failed = true; throw new Error('PARTICLE_JOURNAL_OVERFLOW');
    }
    for (let i = 0; i < this.count; i++) {
      if (this.requests[i].requestKey === request.requestKey) {
        this.failed = true; throw new Error('PARTICLE_DUPLICATE_REQUEST');
      }
    }
    const weight = request.kind === 'deposit' || request.kind === 'crossing' ? request.requested : 0;
    if (!Number.isFinite(request.requested) || request.requested < 0 ||
      request.totalWeight !== weight || request.substepId !== this.identity.substepId) {
      this.failed = true; throw new Error('PARTICLE_INVALID_REQUEST');
    }
    const row = this.requests[this.count++];
    row.requestKey = request.requestKey; row.substepId = request.substepId;
    row.ordinal = request.ordinal; row.kind = request.kind;
    row.particleId = request.particleId; row.sourceId = request.sourceId;
    row.phase = request.phase; row.sourceKind = request.sourceKind;
    row.column = request.column; row.targetColumn = request.targetColumn;
    row.requested = request.requested; row.totalWeight = request.totalWeight;
    row.predictedX = request.predictedX; row.predictedY = request.predictedY;
    row.predictedVx = request.predictedVx; row.predictedVy = request.predictedVy;
    row.mergedIds = request.mergedIds.slice();
    this.totalRequested += weight;
  }
  finish(): P011ParticlePlan {
    if (this.failed || this.finished) throw new Error('PARTICLE_JOURNAL_CLOSED');
    this.finished = true;
    // Full canonical serialization avoids hash collisions in A-stage tests.
    const active = this.requests.slice(0, this.count);
    return { identity: this.identity, requestCount: this.count,
      requests: active, totalRequested: this.totalRequested,
      requestHash: JSON.stringify(active) };
  }
}

// Pure candidate predictor only: no live pool or sink is reachable here.
// Candidate water terminal speeds/drag and airborne acceleration are NOT
// calibrated settleSpeed laws. Whole-particle budget/guard commits await B.
export function collectParticlePlan(identity: P011ParticleStepIdentity,
  frozenField: P011ParticleFieldView, frozenPool: P011ParticleSnapshot,
  params: P011ParticleParams, collector: P011ParticleRequestCollector): P011ParticlePlan {
  validateParticleParams(params);
  if (!Number.isFinite(identity.dt) || identity.dt <= 0 ||
    !Number.isFinite(identity.effectiveTiltRad) || frozenField.cols !== frozenPool.cols) {
    throw new Error('PARTICLE_INVALID_INPUT');
  }
  const order: number[] = [];
  for (let i = 0; i < frozenPool.nmax; i++) if (frozenPool.pState[i] !== 0) order.push(i);
  order.sort((a: number, b: number): number => frozenPool.pId[a] - frozenPool.pId[b]);
  // Deposits precede crossings globally. Request predictions live only in
  // this plan, never in the canonical arrays. Free-flight publication is B.
  const deposits: P011ParticleRequest[] = [];
  const crossings: P011ParticleRequest[] = [];
  const motions: P011ParticleMotion[] = [];
  for (let j = 0; j < order.length; j++) {
    const i = order[j]; const x0 = frozenPool.pX[i]; const c = Math.floor(x0);
    const heavy = frozenPool.pPhase[i] === P011_PARTICLE_HEAVY;
    const speed = heavy ? params.terminalSpeedHeavy : params.terminalSpeedLight;
    const drag = heavy ? params.dragHeavy : params.dragLight;
    const wet = frozenPool.pY[i] <= frozenField.waterTopAt(c);
    const blend = 1 - Math.exp(-drag * identity.dt);
    const vx = wet ? frozenPool.pVx[i] +
      (speed * Math.sin(identity.effectiveTiltRad) - frozenPool.pVx[i]) * blend :
      frozenPool.pVx[i] * Math.exp(-0.01 * identity.dt);
    const vy = wet ? frozenPool.pVy[i] +
      (-speed * Math.cos(identity.effectiveTiltRad) - frozenPool.pVy[i]) * blend :
      frozenPool.pVy[i] - 9.8 * identity.dt; // candidate airborne model
    const x = Math.max(0, Math.min(frozenField.cols - 1e-9,
      Math.max(c - 1 + 1e-9, Math.min(c + 2 - 1e-9, x0 + vx * identity.dt))));
    const y = Math.min(frozenField.capacity, frozenPool.pY[i] + vy * identity.dt);
    const contact = frozenPool.pState[i] === 2 || y <= frozenField.sandTopAt(c);
    const target = Math.floor(x);
    if (!contact && target === c) {
      motions.push({ particleId: frozenPool.pId[i], column: c, x: x,
        y: Math.max(y, frozenField.sandTopAt(c) + params.contactEps), vx: vx, vy: vy });
      continue;
    }
    const kind: P011ParticleOp = contact ? 'deposit' : 'crossing';
    const request: P011ParticleRequest = {
      requestKey: JSON.stringify([identity.runId, identity.substepId, kind,
        frozenPool.pId[i], c, contact ? -1 : target, j]),
      substepId: identity.substepId, ordinal: j, kind: kind,
      particleId: frozenPool.pId[i], sourceId: frozenPool.pSourceId[i],
      phase: frozenPool.pPhase[i], sourceKind: frozenPool.pSource[i],
      column: c, targetColumn: contact ? -1 : target,
      requested: frozenPool.pMass[i], totalWeight: frozenPool.pMass[i],
      predictedX: contact ? x0 : x,
      predictedY: contact ? frozenField.sandTopAt(c) + params.contactEps :
        Math.max(y, frozenField.sandTopAt(target) + params.contactEps),
      predictedVx: contact ? 0 : vx, predictedVy: contact ? 0 : vy,
      mergedIds: []
    };
    if (contact) deposits.push(request); else crossings.push(request);
  }
  for (let i = 0; i < deposits.length; i++) collector.record(deposits[i]);
  for (let i = 0; i < crossings.length; i++) collector.record(crossings[i]);
  const plan = collector.finish();
  plan.motions = motions;
  plan.motionHash = JSON.stringify(motions);
  return plan;
}

export function assertParticlePlansEqual(a: P011ParticlePlan, b: P011ParticlePlan): void {
  if (JSON.stringify(a) !== JSON.stringify(b)) throw new Error('PARTICLE_PLAN_MISMATCH');
}

// Skeleton only: the injected stub owns A-stage grants; no product field
// is mutated and this is not the B-stage atomic applyParticlePlan API.
export function applyParticlePlanToStub(plan: P011ParticlePlan,
  sink: P011ParticlePlanSink): P011ParticleApplyResult[] {
  const results: P011ParticleApplyResult[] = [];
  for (let i = 0; i < plan.requestCount; i++) {
    const result = sink.apply(plan.requests[i]);
    results.push(result);
    if (!result.committed) break;
  }
  return results;
}

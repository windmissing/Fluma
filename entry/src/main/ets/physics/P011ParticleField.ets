// P-011 D1-A / dual-field-particle-v1 (2026-10-24).
// Isolated M1 foundations. No field, renderer, budget or guard ownership.
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

export interface P011ParticleSnapshot {
  nmax: number; cols: number; liveCount: number; freeCount: number; nextId: number;
  pMass: Float64Array; pX: Float64Array; pY: Float64Array;
  pVx: Float64Array; pVy: Float64Array;
  pPhase: Uint8Array; pSource: Uint8Array; pState: Uint8Array;
  pId: Uint32Array; pSourceId: Uint32Array; freeSlots: Int32Array;
  colParticleMassH: Float64Array; colParticleMassL: Float64Array;
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

// Canonical pool only. These local primitives do not publish field events;
// D1-B must coordinate pool+field changes atomically before production use.
export class P011ParticleField {
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

  reset(): void {
    this.pMass.fill(0); this.pX.fill(0); this.pY.fill(0);
    this.pVx.fill(0); this.pVy.fill(0); this.pPhase.fill(0);
    this.pSource.fill(0); this.pState.fill(0); this.pId.fill(0); this.pSourceId.fill(0);
    this.colParticleMassH.fill(0); this.colParticleMassL.fill(0);
    this.liveCount = 0; this.freeCount = this.nmax;
    for (let i = 0; i < this.nmax; i++) this.freeSlots[i] = this.nmax - 1 - i;
    // IDs never repeat across reset. Exhaustion must not wrap.
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
    if (!contact && target === c) continue;
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
  return collector.finish();
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

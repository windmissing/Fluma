// P-011 D1-B rev1 (2026-10-24): isolated reference sinks, NOT field wiring.
// D2 replaces storage with live/shadow field adapters, preserving formulas.
import { P011ParticleField, P011ParticleParams, P011ParticleFieldView,
  P011ParticleStepIdentity, P011ParticleRequest, P011ParticleBudgetTicket,
  P011ParticleBudgetSink, P011ParticleCommitSink, P011ParticleAtomicPool,
  P011ParticleApplyResult, P011ParticleGuardDecision, P011ParticleGrant,
  P011ParticlePoolPatch, P011ParticlePlan, P011ParticleDisposition } from './P011ParticleField';
import { p011GuardAdmit, p011GuardConstraintsForDelta,
  p011GuardAffectedEdgesForDelta } from './P011MaterialGuard';
import { P011ParticleRefundAdapter, P011ParticleRefundReceiptBuffer } from './P011ParticleRefundAdapter';

export interface P011ParticleBudgetState {
  localRemaining: Float64Array; localUsed: Float64Array;
  totalRemaining: number; totalUsed: number;
  tickets: P011ParticleBudgetTicket[]; closed: number[]; nextId: number;
  refundReceipt: P011ParticleRefundReceiptBuffer;
}

export class P011ParticlePrototypeBudget implements P011ParticleBudgetSink {
  readonly localRemaining: Float64Array;
  readonly localUsed: Float64Array;
  totalRemaining: number;
  totalUsed = 0;
  readonly bottomUsed = 0; // particles never debit bottom/correction/channel
  private tickets: P011ParticleBudgetTicket[] = [];
  private closed: Set<number> = new Set<number>();
  private nextId = 1;
  readonly substepId: number;
  readonly scale: number;
  readonly refundReceipt = new P011ParticleRefundReceiptBuffer();
  readonly refundAdapter: P011ParticleRefundAdapter;

  constructor(cols: number, dt: number, scale: number, substepId: number) {
    if (!Number.isFinite(dt) || dt <= 0 || !Number.isFinite(scale) || scale < 0 || scale > 1) {
      throw new Error('PARTICLE_INVALID_BUDGET');
    }
    this.localRemaining = new Float64Array(cols); this.localRemaining.fill(28 * dt);
    this.localUsed = new Float64Array(cols); this.totalRemaining = 300 * dt;
    this.substepId = substepId; this.scale = scale;
    this.refundAdapter = new P011ParticleRefundAdapter(this, substepId);
  }
  reserve(r: P011ParticleRequest): P011ParticleBudgetTicket {
    if (r.substepId !== this.substepId || r.column < 0 || r.column >= this.localRemaining.length ||
      !Number.isFinite(r.requested) || r.requested < 0 ||
      (r.kind !== 'deposit' && r.kind !== 'lift' && r.kind !== 'crossing')) {
      throw new Error('PARTICLE_INVALID_TICKET');
    }
    for (let i = 0; i < this.tickets.length; i++) if (this.tickets[i].requestKey === r.requestKey) {
      throw new Error('PARTICLE_DUPLICATE_TICKET');
    }
    const target = r.kind === 'crossing' ? r.targetColumn : -1;
    if (target >= this.localRemaining.length || target === r.column || target < -1) {
      throw new Error('PARTICLE_INVALID_TICKET');
    }
    const l = Math.min(r.requested, this.localRemaining[r.column],
      target >= 0 ? this.localRemaining[target] : Infinity);
    const reserved = Math.min(l, l * this.scale, this.totalRemaining);
    const ticket: P011ParticleBudgetTicket = { ticketId: this.nextId, requestKey: r.requestKey,
      substepId: r.substepId, source: r.column, target: target,
      requested: r.requested, localAllowed: l, reserved: reserved, refunded: 0 };
    this.refundAdapter.registerTicket(ticket);
    this.localRemaining[r.column] -= reserved; this.localUsed[r.column] += reserved;
    if (target >= 0) { this.localRemaining[target] -= reserved; this.localUsed[target] += reserved; }
    this.totalRemaining -= reserved; this.totalUsed += reserved;
    this.nextId++;
    this.tickets.push(ticket);
    return this.copyTicket(ticket);
  }
  refund(ticketId: number, amount: number): void {
    const t = this.ticket(ticketId);
    this.refundAdapter.refundParticleTicket(ticketId, amount, this.refundReceipt);
    t.refunded += amount;
  }
  close(ticketId: number, committed: number): void {
    const t = this.ticket(ticketId);
    this.refundAdapter.closeParticleTicket(ticketId, committed, this.refundReceipt);
    this.closed.add(ticketId);
  }
  private ticket(id: number): P011ParticleBudgetTicket {
    const t = this.tickets.find((value: P011ParticleBudgetTicket): boolean => value.ticketId === id);
    if (t === undefined || this.closed.has(id) || t.substepId !== this.substepId) {
      throw new Error('PARTICLE_STALE_TICKET');
    }
    return t;
  }
  private copyTicket(t: P011ParticleBudgetTicket): P011ParticleBudgetTicket {
    return { ticketId: t.ticketId, requestKey: t.requestKey, substepId: t.substepId,
      source: t.source, target: t.target, requested: t.requested,
      localAllowed: t.localAllowed, reserved: t.reserved, refunded: t.refunded };
  }
  snapshot(): P011ParticleBudgetState {
    const receipt = new P011ParticleRefundReceiptBuffer(); receipt.copyFrom(this.refundReceipt);
    return { localRemaining: this.localRemaining.slice(), localUsed: this.localUsed.slice(),
      totalRemaining: this.totalRemaining, totalUsed: this.totalUsed,
      tickets: this.tickets.map((t: P011ParticleBudgetTicket): P011ParticleBudgetTicket => this.copyTicket(t)),
      closed: Array.from(this.closed), nextId: this.nextId, refundReceipt: receipt };
  }
  restore(s: P011ParticleBudgetState): void {
    this.localRemaining.set(s.localRemaining); this.localUsed.set(s.localUsed);
    this.totalRemaining = s.totalRemaining; this.totalUsed = s.totalUsed;
    this.tickets = s.tickets.map((t: P011ParticleBudgetTicket): P011ParticleBudgetTicket => this.copyTicket(t));
    this.closed = new Set<number>(s.closed); this.nextId = s.nextId;
    this.refundAdapter.resetForRestore();
    for (let i = 0; i < this.tickets.length; i++) {
      this.refundAdapter.registerTicket(this.tickets[i], this.closed.has(this.tickets[i].ticketId));
    }
    this.refundReceipt.copyFrom(s.refundReceipt);
  }
}

export interface P011ParticlePrototypeEvent {
  kind: string; requestKey: string; particleId: number; sourceId: number;
  phase: number; sourceKind: number; column: number; targetColumn: number;
  mass: number; requested: number; granted: number; remainder: number;
  budgetClipped: number; guardRejected: number; capacityClipped: number; poolClipped: number;
  deltaS: number; mergedIds: number[]; flag: string;
  x?: number; y?: number; vx?: number; vy?: number;
  admitted?: number; massAfter?: number; released?: boolean;
  remainderDisposition?: string; guardPreview?: P011ParticleGuardDecision | null;
  budgetLocal?: number; budgetTotal?: number; bottom?: number;
  massBefore?: number; positionMomentPreserved?: boolean;
}
export interface P011ParticlePrototypeState {
  bh: Float64Array; bl: Float64Array; mh: Float64Array; ml: Float64Array;
  suspH: Float64Array; suspL: Float64Array; waterTop: Float64Array;
  eventCount: number; budget: P011ParticleBudgetState;
  appliedKeys: string[];
}
export interface P011ParticleExecutionResult {
  ok: boolean; errorCode: string | null; results: P011ParticleApplyResult[];
}

// A Node-replaceable complete field stage. No WaterSandField import or wiring.
export class P011ParticlePrototypeSink implements P011ParticleCommitSink {
  readonly bh: Float64Array; readonly bl: Float64Array;
  readonly mh: Float64Array; readonly ml: Float64Array;
  readonly suspH: Float64Array; readonly suspL: Float64Array;
  readonly waterTop: Float64Array;
  readonly events: P011ParticlePrototypeEvent[] = [];
  readonly budgets: P011ParticlePrototypeBudget;
  readonly pool: P011ParticleField;
  readonly params: P011ParticleParams;
  readonly identity: P011ParticleStepIdentity;
  readonly capacity: number;
  readonly eventLimit: number;
  failAfterPublish = false; // explicit isolated fault-injection hook
  readonly failedAttempts: string[] = [];
  private applied: Set<string> = new Set<string>();

  constructor(pool: P011ParticleField, params: P011ParticleParams,
    identity: P011ParticleStepIdentity, budgets: P011ParticlePrototypeBudget,
    capacity: number = 512, eventLimit: number = 4096) {
    this.pool = pool; this.params = params; this.identity = identity;
    this.budgets = budgets; this.capacity = capacity; this.eventLimit = eventLimit;
    const cols = pool.cols;
    this.bh = new Float64Array(cols); this.bl = new Float64Array(cols);
    this.mh = new Float64Array(cols); this.ml = new Float64Array(cols);
    this.suspH = new Float64Array(cols); this.suspL = new Float64Array(cols);
    this.waterTop = new Float64Array(cols); this.waterTop.fill(capacity);
    this.pool.beginStep(identity, params, this.view());
  }
  sand(c: number): number { return this.bh[c] + this.bl[c] + this.mh[c] + this.ml[c]; }
  view(): P011ParticleFieldView {
    return new P011ParticlePrototypeView(this);
  }
  snapshot(): P011ParticlePrototypeState {
    return { bh: this.bh.slice(), bl: this.bl.slice(), mh: this.mh.slice(), ml: this.ml.slice(),
      suspH: this.suspH.slice(), suspL: this.suspL.slice(), waterTop: this.waterTop.slice(),
      eventCount: this.events.length, budget: this.budgets.snapshot(), appliedKeys: Array.from(this.applied) };
  }
  restore(s: P011ParticlePrototypeState): void {
    this.bh.set(s.bh); this.bl.set(s.bl); this.mh.set(s.mh); this.ml.set(s.ml);
    this.suspH.set(s.suspH); this.suspL.set(s.suspL); this.waterTop.set(s.waterTop);
    this.events.length = s.eventCount; this.budgets.restore(s.budget);
    this.applied = new Set<string>(s.appliedKeys);
  }
  executeParticlePlan(plan: P011ParticlePlan): P011ParticleExecutionResult {
    const before = this.snapshot(); const particles = this.pool.checkpoint();
    try {
      const results = this.pool.applyParticlePlan(plan, this);
      for (let i = 0; i < results.length; i++) if (!results[i].committed) {
        const code = results[i].errorCode;
        throw new Error(code === null ? 'PARTICLE_COMMIT_FAILED' : code);
      }
      this.audit();
      return { ok: true, errorCode: null, results: results };
    } catch (error) {
      this.pool.rollback(particles); this.restore(before);
      const code = `${error}`;
      this.failedAttempts.push(code); // failure evidence survives canonical rollback
      return { ok: false, errorCode: code, results: [] };
    }
  }
  guard(amount: number, column: number, sign: number): P011ParticleGuardDecision {
    const delta = new Map<number, number>(); delta.set(column, sign);
    const constraints = p011GuardConstraintsForDelta(delta,
      (edge: number): number => this.sand(edge + 1) - this.sand(edge) -
        Math.tan(this.identity.effectiveTiltRad), this.pool.cols - 1);
    const edges = p011GuardAffectedEdgesForDelta(delta, this.pool.cols - 1);
    const decision = p011GuardAdmit(amount, constraints, Math.tan(Math.PI / 6));
    return { accepted: decision.accepted, rejected: decision.rejected,
      allLegalBefore: decision.allLegalBefore,
      limitingEdge: decision.limitingEdge === null ? null : edges[decision.limitingEdge],
      affectedEdgeIds: edges };
  }
  audit(): void {
    const p = this.pool.snapshot();
    for (let c = 0; c < this.pool.cols; c++) {
      const values: number[] = [this.bh[c], this.bl[c], this.mh[c], this.ml[c],
        this.suspH[c], this.suspL[c], p.colParticleMassH[c], p.colParticleMassL[c], this.waterTop[c]];
      for (let j = 0; j < values.length; j++) if (!Number.isFinite(values[j]) || values[j] < -1e-9) {
        throw new Error('PARTICLE_INVENTORY_AUDIT');
      }
      if (this.sand(c) + this.suspH[c] + this.suspL[c] + p.colParticleMassH[c] +
        p.colParticleMassL[c] > this.waterTop[c] + 1e-9 || this.waterTop[c] > this.capacity) {
        throw new Error('PARTICLE_CAPACITY_AUDIT');
      }
    }
    for (let i = 0; i < p.nmax; i++) if (p.pState[i] !== 0 &&
      (!Number.isFinite(p.pX[i]) || !Number.isFinite(p.pY[i]) || p.pY[i] < 0 || !Number.isFinite(p.pVx[i]) ||
      !Number.isFinite(p.pVy[i]) || !Number.isFinite(p.pMass[i]) || p.pMass[i] <= 0 || p.pY[i] > this.capacity)) {
      throw new Error('PARTICLE_NON_FINITE');
    }
  }
  private split(column: number, amount: number): number[] {
    // Same mixed -> light -> heavy extraction formula as splitTopV2.
    const fromMix = Math.min(amount, this.mh[column] + this.ml[column]);
    const scale = fromMix / Math.max(this.mh[column] + this.ml[column], 1e-9);
    let h = this.mh[column] * scale; let l = this.ml[column] * scale;
    let remaining = amount - fromMix;
    const fromLight = Math.min(remaining, this.bl[column]); l += fromLight; remaining -= fromLight;
    const fromHeavy = Math.min(remaining, this.bh[column]); h += fromHeavy;
    this.mh[column] -= this.mh[column] * scale; this.ml[column] -= this.ml[column] * scale;
    this.bl[column] -= fromLight; this.bh[column] -= fromHeavy;
    return [h, l];
  }
  apply(r: P011ParticleRequest, atomic: P011ParticleAtomicPool): P011ParticleApplyResult {
    if (atomic !== this.pool || r.substepId !== this.identity.substepId || this.applied.has(r.requestKey)) {
      throw new Error('PARTICLE_REQUEST_IDENTITY');
    }
    const before = this.snapshot();
    let patch: P011ParticlePoolPatch | null = null; let published = false;
    let granted = r.requested; let budgetClipped = 0; let guardRejected = 0;
    let capacityClipped = 0; let poolClipped = 0;
    let guard: P011ParticleGuardDecision | null = null;
    let ticket: P011ParticleBudgetTicket | null = null;
    let disposition: P011ParticleDisposition = 'PARTICLE';
    const c = r.column;
    let gh = r.phase === 0 ? granted : 0; let gl = r.phase === 1 ? granted : 0;
    let degraded = false;
    const originalPool = this.pool.snapshot();
    try {
      this.pool.beginStep(this.identity, this.params, this.view());
      if (r.kind === 'crossing' && this.view().freeSpaceAt(r.targetColumn) + 1e-9 < r.requested) {
        capacityClipped = r.requested; granted = 0;
      } else if (r.kind === 'spawn' && this.view().freeSpaceAt(c) + 1e-9 < r.requested) {
        capacityClipped = r.requested; granted = 0;
      } else if (r.kind === 'deposit' || r.kind === 'lift' || r.kind === 'crossing') {
        ticket = this.budgets.reserve(r); granted = ticket.reserved;
        if (r.kind === 'crossing' && granted < r.requested) {
          this.budgets.refund(ticket.ticketId, granted); granted = 0; budgetClipped = r.requested;
        } else {
          budgetClipped = r.requested - granted;
          if (r.kind !== 'crossing') {
            guard = this.guard(granted, c, r.kind === 'deposit' ? 1 : -1);
            this.budgets.refund(ticket.ticketId, guard.rejected);
            guardRejected = guard.rejected; granted = guard.accepted;
          }
        }
      }
      gh = r.phase === 0 ? granted : 0; gl = r.phase === 1 ? granted : 0;
      // A field stage is represented by this isolated sink's private arrays;
      // no callbacks/events are published until both field and pool audit.
      if (r.kind === 'deposit') {
        disposition = granted > 0 ? 'BED' : 'WAIT';
        this.mh[c] += gh; this.ml[c] += gl;
      } else if (r.kind === 'lift') {
        const available = this.sand(c);
        if (granted > available) {
          const clipped = granted - available; capacityClipped += clipped;
          if (ticket !== null) this.budgets.refund(ticket.ticketId, clipped);
          granted = available;
        }
        const split = this.split(c, granted); gh = split[0]; gl = split[1];
        disposition = granted > 0 ? 'PARTICLE' : 'WAIT';
      } else if (r.kind === 'degrade') disposition = 'SUSP';
      else if (r.kind === 'crossing') disposition = granted > 0 ? 'MOVE' : 'WAIT';
      else if (r.kind === 'merge') disposition = 'MERGE';
      else if (granted === 0) disposition = 'WAIT';
      // Stage-dependent contact/birth height, while keeping request identity
      // and requested mass unchanged. D2 performs the same calculation.
      const staged: P011ParticleRequest = { requestKey: r.requestKey,
        substepId: r.substepId, ordinal: r.ordinal, kind: r.kind, particleId: r.particleId,
        sourceId: r.sourceId, phase: r.phase, sourceKind: r.sourceKind,
        column: c, targetColumn: r.targetColumn, requested: r.requested, totalWeight: r.totalWeight,
        predictedX: r.predictedX, predictedY: r.kind === 'deposit' || r.kind === 'lift' ?
          this.sand(c) + this.params.contactEps : r.predictedY,
        predictedVx: r.predictedVx, predictedVy: r.predictedVy, mergedIds: r.mergedIds };
      const grant: P011ParticleGrant = { requestKey: r.requestKey,
        grantedHeavy: gh, grantedLight: gl, disposition: disposition };
      patch = atomic.prepare(staged, grant);
      if (patch === null && r.kind === 'lift' && granted > 0) {
        grant.disposition = 'SUSP'; disposition = 'SUSP'; degraded = true;
        patch = atomic.prepare(staged, grant);
      } else if (patch === null && r.kind === 'spawn') {
        poolClipped = granted; granted = 0; gh = 0; gl = 0;
        grant.grantedHeavy = 0; grant.grantedLight = 0; grant.disposition = 'WAIT';
        patch = atomic.prepare(staged, grant);
      }
      if (patch === null) throw new Error('PARTICLE_PATCH_REFUSED');
      if (r.kind === 'degrade' || degraded) { this.suspH[c] += gh; this.suspL[c] += gl; }
      const merges = this.pool.patchMerges(patch);
      const births = this.pool.patchBirths(patch);
      const actions = this.pool.patchActions(patch);
      // One main event plus each actual merged pair; T2 fallback has a
      // separate disposition event, never a second source admission.
      const mainCount = r.kind === 'merge' || (r.kind === 'spawn' && births.length > 0) ? 0 : 1;
      const count = mainCount + births.length + merges.length + (degraded ? 1 : 0);
      if (this.events.length + count > this.eventLimit) throw new Error('PARTICLE_JOURNAL_OVERFLOW');
      atomic.publish(patch); published = true;
      this.audit();
      const after = this.pool.snapshot();
      for (let phase = 0; phase < 2; phase++) {
        let beforeMass = 0; let afterMass = 0;
        for (let col = 0; col < this.pool.cols; col++) {
          beforeMass += phase === 0 ? before.bh[col] + before.mh[col] + before.suspH[col] +
            originalPool.colParticleMassH[col] : before.bl[col] + before.ml[col] + before.suspL[col] + originalPool.colParticleMassL[col];
          afterMass += phase === 0 ? this.bh[col] + this.mh[col] + this.suspH[col] + after.colParticleMassH[col] :
            this.bl[col] + this.ml[col] + this.suspL[col] + after.colParticleMassL[col];
        }
        const external = r.kind === 'spawn' ? (phase === 0 ? gh : gl) : 0;
        if (Math.abs(afterMass - beforeMass - external) > 1e-9) throw new Error('PARTICLE_CONSERVATION_AUDIT');
      }
      if (this.failAfterPublish) throw new Error('PARTICLE_INJECTED_FAILURE');
      if (ticket !== null) this.budgets.close(ticket.ticketId, granted);
      const begin = this.events.length;
      for (let action = 0; action < actions.length; action++) {
        if (actions[action].kind === 'merge') {
        const m = merges[actions[action].index];
        this.events.push({ kind: 'MERGE', requestKey: r.requestKey, particleId: m.survivor,
          sourceId: m.sourceId, phase: m.phase, sourceKind: m.sourceKind, column: m.column,
          targetColumn: -1, mass: m.massBefore, requested: m.massBefore, granted: m.massAfter,
          remainder: 0, budgetClipped: 0, guardRejected: 0, capacityClipped: 0, poolClipped: 0,
          deltaS: 0, mergedIds: [m.survivor, m.absorbed], flag: '' });
        this.events[this.events.length - 1].massBefore = m.massBefore;
        this.events[this.events.length - 1].massAfter = m.massAfter;
        this.events[this.events.length - 1].positionMomentPreserved = true;
        } else {
        const b = births[actions[action].index];
        this.events.push({ kind: 'SPAWN', requestKey: r.requestKey, particleId: b.particleId,
          sourceId: b.sourceId, phase: b.phase, sourceKind: b.sourceKind, column: c, targetColumn: -1,
          mass: b.mass, requested: b.mass, granted: b.mass, admitted: b.mass,
          remainder: 0, budgetClipped: 0, guardRejected: 0, capacityClipped: 0, poolClipped: 0,
          deltaS: 0, mergedIds: [], flag: '', x: b.x, y: b.y, vx: b.vx, vy: b.vy });
        }
      }
      const kind = r.kind === 'deposit' ? 'LAND' : r.kind === 'crossing' ? 'CROSS_COLUMN' :
        r.kind === 'lift' ? 'LIFT' : r.kind === 'degrade' ? 'DEGRADE' : r.kind === 'merge' ? 'MERGE' : 'SPAWN';
      if (mainCount > 0) this.events.push({ kind: kind, requestKey: r.requestKey, particleId: r.particleId,
        sourceId: r.sourceId, phase: r.phase, sourceKind: r.sourceKind, column: c,
        targetColumn: r.targetColumn, mass: r.requested, requested: r.requested, granted: granted,
        remainder: r.requested - granted, budgetClipped: budgetClipped, guardRejected: guardRejected,
        capacityClipped: capacityClipped, poolClipped: poolClipped,
        deltaS: r.kind === 'deposit' ? granted : r.kind === 'lift' ? -granted : 0,
        mergedIds: r.mergedIds, flag: r.kind === 'degrade' ? 'PARTICLE_DEGRADED_TO_COLUMN' : '',
        massAfter: r.requested - granted, released: r.requested === granted,
        remainderDisposition: r.kind === 'deposit' && r.requested > granted ? 'RETAIN_PARTICLE' : 'NONE',
        guardPreview: guard, budgetLocal: granted, budgetTotal: ticket === null ? 0 : granted, bottom: 0 });
      if (degraded) this.events.push({ kind: 'DEGRADE', requestKey: r.requestKey, particleId: 0,
        sourceId: r.sourceId, phase: r.phase, sourceKind: r.sourceKind, column: c, targetColumn: -1,
        mass: granted, requested: granted, granted: granted, remainder: 0, budgetClipped: 0,
        guardRejected: 0, capacityClipped: 0, poolClipped: 0, deltaS: 0, mergedIds: [],
        flag: 'PARTICLE_DEGRADED_TO_COLUMN' });
      this.pool.finishPatch(patch); this.applied.add(r.requestKey);
      return { requestKey: r.requestKey, committed: true, errorCode: null,
        requested: r.requested, granted: granted, remainder: r.requested - granted,
        budgetClipped: budgetClipped, guardRejected: guardRejected, capacityClipped: capacityClipped,
        poolClipped: poolClipped, guard: guard, eventBegin: begin, eventCount: count };
    } catch (error) {
      if (patch !== null) { if (published) atomic.restore(patch); else atomic.discard(patch); }
      this.restore(before);
      return { requestKey: r.requestKey, committed: false, errorCode: `${error}`,
        requested: r.requested, granted: 0, remainder: r.requested,
        budgetClipped: 0, guardRejected: 0, capacityClipped: 0, poolClipped: r.requested,
        guard: null, eventBegin: before.eventCount, eventCount: 0 };
    }
  }
}

class P011ParticlePrototypeView implements P011ParticleFieldView {
  readonly cols: number; readonly capacity: number; readonly effectiveTiltRad: number;
  private sand: Float64Array; private water: Float64Array;
  private susp: Float64Array; private free: Float64Array;
  constructor(sink: P011ParticlePrototypeSink) {
    this.cols = sink.pool.cols; this.capacity = sink.capacity;
    this.effectiveTiltRad = sink.identity.effectiveTiltRad;
    this.sand = new Float64Array(this.cols); this.water = sink.waterTop.slice();
    this.susp = new Float64Array(this.cols); this.free = new Float64Array(this.cols);
    const particles = sink.pool.snapshot();
    for (let c = 0; c < this.cols; c++) {
      this.sand[c] = sink.sand(c); this.susp[c] = sink.suspH[c] + sink.suspL[c];
      this.free[c] = this.water[c] - this.sand[c] - this.susp[c] -
        particles.colParticleMassH[c] - particles.colParticleMassL[c];
    }
  }
  sandTopAt(c: number): number { return this.sand[c]; }
  waterTopAt(c: number): number { return this.water[c]; }
  suspAt(c: number): number { return this.susp[c]; }
  freeSpaceAt(c: number): number { return this.free[c]; }
}

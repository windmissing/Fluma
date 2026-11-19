/**
 * P-011 architecture §19.5 pre-commit material writer guard.
 *
 * Runs before every individual material increment of each named writer
 * (settle / percolate / bottomCreep / relax / lift). For a candidate grant q
 * with per-edge signed residual d_e and per-unit-grant influence k_e, the
 * guard solves the closed-form interval intersection
 *   -L_e <= d_e + k_e * q <= L_e   (L_e = H for legal edges, |d_e| for
 *   already-violating edges: non-worsening), intersected with [0, qGranted],
 * yielding the exact maximum admissible qAccepted without iteration or
 * bisection (the column delta of each transaction is linear in q).
 *
 * The guard never relaxes budget ceilings or physics formulas; it only
 * clips a granted amount before material is written and reports the
 * reduction so the caller can refund its budget ledgers (local/bottom/
 * total) with the rejected part — no re-issue, no carry-over.
 */

export interface P011GuardEdgeConstraint {
  /** Signed residual before this increment: (S[e+1]-S[e]) - tan(effectiveTilt). */
  residual: number;
  /** Signed per-unit-grant slope influence: (dDeltaS[e+1]-dDeltaS[e]) / dx. */
  influence: number;
}

export interface P011GuardOutcome {
  /** Maximum admissible grant in [0, requested]. */
  accepted: number;
  /** Rejected part = requested - accepted (>= 0). */
  rejected: number;
  /** True when every inspected edge was legal before the increment. */
  allLegalBefore: boolean;
  /** Smallest interfaceIndex among edges that bound the accepted grant. */
  limitingEdge: number | null;
}

// T174 / T170 §3: owned fixed storage for the M2 single-column host guard.
// The generic Map/Set reference below remains unchanged for other writers.
export class P011GuardNumericOutcome implements P011GuardOutcome {
  accepted: number = 0; rejected: number = 0;
  allLegalBefore: boolean = true; limitingEdge: number | null = null;
}
export class P011ParticleColumnGuardWorkspace {
  readonly edgeId: Float64Array = new Float64Array(3);
  readonly residual: Float64Array = new Float64Array(3);
  readonly influence: Float64Array = new Float64Array(3);
  readonly out: P011GuardNumericOutcome = new P011GuardNumericOutcome();
  count: number = 0;
  private readonly edgeBinding: ArrayBuffer = this.edgeId.buffer as ArrayBuffer;
  private readonly residualBinding: ArrayBuffer = this.residual.buffer as ArrayBuffer;
  private readonly influenceBinding: ArrayBuffer = this.influence.buffer as ArrayBuffer;
  private readonly outBinding: P011GuardNumericOutcome = this.out;
  storageValid(): boolean {
    return this.edgeId.buffer === this.edgeBinding && this.edgeId.length === 3 &&
      this.residual.buffer === this.residualBinding && this.residual.length === 3 &&
      this.influence.buffer === this.influenceBinding && this.influence.length === 3 && this.out === this.outBinding;
  }
}

/** Same RN operation sequence/early exits/ties as p011GuardAdmit, into owned out. */
export function p011GuardAdmitNumericInto(requested: number, residual: Float64Array,
  influence: Float64Array, count: number, slopeHardMax: number, out: P011GuardNumericOutcome): number {
  if (!p011GuardArmed(slopeHardMax) || !Number.isFinite(requested) || requested <= 0) {
    out.accepted = Math.max(0, Number.isFinite(requested) ? requested : 0);
    out.rejected = 0; out.allLegalBefore = true; out.limitingEdge = null; return 0;
  }
  let qLo = 0; let qHi = Math.max(0, requested); let allLegal = true;
  let limitingEdge: number | null = null;
  for (let e = 0; e < count; e++) {
    const d = residual[e], k = influence[e];
    if (!Number.isFinite(d) || !Number.isFinite(k)) {
      out.accepted = 0; out.rejected = requested; out.allLegalBefore = false; out.limitingEdge = e; return 0;
    }
    const legal = Math.abs(d) <= slopeHardMax;
    if (!legal) allLegal = false;
    const bound = legal ? slopeHardMax : Math.abs(d);
    if (k === 0) {
      if (Math.abs(d) > bound) {
        out.accepted = 0; out.rejected = requested; out.allLegalBefore = allLegal; out.limitingEdge = e; return 0;
      }
      continue;
    }
    let lo = (-bound - d) / k; let hi = (bound - d) / k;
    if (lo > hi) { const swap = lo; lo = hi; hi = swap; }
    if (lo > qLo) { qLo = lo; limitingEdge = e; }
    if (hi < qHi) { qHi = hi; limitingEdge = e; }
  }
  if (qLo > qHi) {
    out.accepted = 0; out.rejected = requested; out.allLegalBefore = allLegal; out.limitingEdge = limitingEdge; return 0;
  }
  const accepted = Math.max(0, Math.min(requested, qHi));
  out.accepted = accepted; out.rejected = Math.max(0, requested - accepted);
  out.allLegalBefore = allLegal; out.limitingEdge = limitingEdge; return 0;
}

/** Whether the guard is armed: slopeHardMax tan(thetaHardMaxDeg), finite. */
export function p011GuardArmed(slopeHardMax: number): boolean {
  return Number.isFinite(slopeHardMax) && slopeHardMax > 0;
}

/**
 * Closed-form admission for one candidate increment.
 *
 * constraint[].residual/influence describe every edge whose slope changes
 * with this grant. Legal edges (|residual| <= H) must stay within [-H, H]
 * after the increment; already-violating edges must not worsen
 * (|d'(q)| <= |d|). k = 0 edges either do not constrain q or make the
 * candidate infeasible.
 */
export function p011GuardAdmit(requested: number,
  constraints: P011GuardEdgeConstraint[],
  slopeHardMax: number): P011GuardOutcome {
  if (!p011GuardArmed(slopeHardMax) || !Number.isFinite(requested) ||
    requested <= 0) {
    return {
      accepted: Math.max(0, Number.isFinite(requested) ? requested : 0),
      rejected: 0,
      allLegalBefore: true,
      limitingEdge: null
    };
  }
  let qLo = 0;
  let qHi = Math.max(0, requested);
  let allLegal = true;
  let limitingEdge: number | null = null;
  for (let e = 0; e < constraints.length; e++) {
    const d = constraints[e].residual;
    const k = constraints[e].influence;
    if (!Number.isFinite(d) || !Number.isFinite(k)) {
      return { accepted: 0, rejected: requested, allLegalBefore: false,
        limitingEdge: e };
    }
    const legal = Math.abs(d) <= slopeHardMax;
    if (!legal) {
      allLegal = false;
    }
    const bound = legal ? slopeHardMax : Math.abs(d);
    if (k === 0) {
      if (Math.abs(d) > bound) {
        // Edge cannot be fixed by this grant and already violates: infeasible.
        return { accepted: 0, rejected: requested, allLegalBefore: allLegal,
          limitingEdge: e };
      }
      continue;
    }
    // -bound <= d + k*q <= bound  →  q interval [lo, hi]
    let lo = (-bound - d) / k;
    let hi = (bound - d) / k;
    if (lo > hi) {
      const swap = lo;
      lo = hi;
      hi = swap;
    }
    if (lo > qLo) {
      qLo = lo;
      limitingEdge = e;
    }
    if (hi < qHi) {
      qHi = hi;
      limitingEdge = e;
    }
  }
  if (qLo > qHi) {
    return { accepted: 0, rejected: requested, allLegalBefore: allLegal,
      limitingEdge: limitingEdge };
  }
  const accepted = Math.max(0, Math.min(requested, qHi));
  return {
    accepted: accepted,
    rejected: Math.max(0, requested - accepted),
    allLegalBefore: allLegal,
    limitingEdge: limitingEdge
  };
}

/**
 * Build the closed-form constraint list for one candidate increment from a
 * per-column delta map (column -> signed per-unit-grant S change, e.g.
 * {donor: -1, receiver: +1} for pair moves or {c: 1} for settle). This is
 * the single derivation shared by the apply-side guard wrapper and the
 * preview allocator's guard mirror (§23.1 R-3-a): both must derive the
 * affected-edge set and per-edge influence coefficients identically.
 *
 * residualOf(edge) supplies the signed residual (S[e+1]-S[e])-tan(theta) of
 * the edge at the moment of the decision; the caller controls which S
 * snapshot it reads (apply side: the live sandTopScratch; preview side: the
 * shadow S derived from substepSandTopBefore).
 */
export function p011GuardConstraintsForDelta(
  deltaColumns: Map<number, number>,
  residualOf: (edge: number) => number,
  edgeCount: number): P011GuardEdgeConstraint[] {
  const affected = new Set<number>();
  for (const column of deltaColumns.keys()) {
    if (column - 1 >= 0 && column - 1 < edgeCount) {
      affected.add(column - 1);
    }
    if (column >= 0 && column < edgeCount) {
      affected.add(column);
    }
  }
  const constraints: P011GuardEdgeConstraint[] = [];
  for (const edge of affected) {
    const influence = (deltaColumns.get(edge + 1) ?? 0) -
      (deltaColumns.get(edge) ?? 0);
    constraints.push({
      residual: residualOf(edge),
      influence: influence
    });
  }
  return constraints;
}

/**
 * GAP-4 (§24.4 affectedEdgeIds): the physical interface indices behind a
 * constraint list, in the SAME order p011GuardConstraintsForDelta produced
 * them. p011GuardAdmit's limitingEdge is the position in the constraints
 * array; mapping it through this list yields the physical edge id. The
 * helper re-derives the affected set with the identical rule so callers
 * cannot drift from the constraint builder.
 */
export function p011GuardAffectedEdgesForDelta(
  deltaColumns: Map<number, number>,
  edgeCount: number): number[] {
  const affected = new Set<number>();
  for (const column of deltaColumns.keys()) {
    if (column - 1 >= 0 && column - 1 < edgeCount) {
      affected.add(column - 1);
    }
    if (column >= 0 && column < edgeCount) {
      affected.add(column);
    }
  }
  const edges: number[] = [];
  for (const edge of affected) {
    edges.push(edge);
  }
  return edges;
}

/**
 * Budget ledger refund after a guard clip (§19.5 four-layer write-back).
 * The rejected part (granted - accepted) returns to local remaining (both
 * endpoints for pair moves, single endpoint for column moves), bottom
 * remaining (creepBottom only), and total remaining; used ledgers are
 * reduced by the same amount so used + remaining stays consistent with the
 * final accepted grant. There is no channel-layer ledger in the current
 * engine (channelRateCap = 0); the channel layer is covered by the
 * per-writer telemetry.
 */
export class P011GuardLedgerRefund {
  private readonly localRemaining: Float64Array;
  private readonly localUsed: Float64Array;

  constructor(localRemaining: Float64Array, localUsed: Float64Array) {
    this.localRemaining = localRemaining;
    this.localUsed = localUsed;
  }

  /**
   * §28 refund-risk fix (option b — refund only what was ACTUALLY
   * debited): the latent defect had bottom-creep refunds crediting
   * localRemaining unconditionally even though creepBottom never debits
   * local — a pure budget increase (never triggered historically: the
   * guard only rejects "k=0 && edge already violating" grants, which the
   * bottom request evolution never produced; plan-3's advection channel
   * raises the construction probability, hence the pre-fix). Per column
   * the refund is now capped by the column's own used amount: each column
   * refunds min(share, localUsed[c]) so used+remaining closes EXACTLY and
   * un-debited columns refund zero. (Semantics change: the refund can no
   * longer exceed the actual local debit — no admission rule changes.)
   */
  refundLocal(columns: number[], amount: number): void {
    if (amount <= 0) {
      return;
    }
    if (columns.length === 0) {
      return;
    }
    const share = amount / columns.length;
    let unrefundable = 0;
    for (const c of columns) {
      if (c >= 0 && c < this.localRemaining.length) {
        const refundable = Math.min(share, this.localUsed[c]);
        this.localRemaining[c] += refundable;
        this.localUsed[c] -= refundable;
        unrefundable += share - refundable;
      } else {
        unrefundable += share;
      }
    }
    // The unrefundable part is NOT redistributed: those columns never
    // held the debit, so crediting others would re-create the pure
    // increase. It silently vanishes (conservative — the caller's
    // rejected amount exceeded the actual local debit).
    if (unrefundable > 0) {
      // Intentionally no-op; documented for the reader.
    }
  }
}

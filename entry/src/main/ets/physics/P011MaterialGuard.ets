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

  refundLocal(columns: number[], amount: number): void {
    if (amount <= 0) {
      return;
    }
    for (const c of columns) {
      if (c >= 0 && c < this.localRemaining.length) {
        this.localRemaining[c] += amount;
        this.localUsed[c] = Math.max(0, this.localUsed[c] - amount);
      }
    }
  }
}

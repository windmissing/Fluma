/**
 * P-011 relax-correction-weighted-v1 (architecture §24, designer algorithm
 * doc v1): an independent, reservation-first, contribution-weighted budget
 * sub-channel for the relax slot.
 *
 * Semantics (approved budget ruling): independent ledger + priority
 * reservation + full total accounting. The correction budget is RESERVED
 * from the existing total motion budget B = totalMotionBudget*dt, never
 * added on top of it (ordinaryUsed + correctionUsed <= B stays invariant);
 * correction grants are NOT multiplied by the ordinary channels'
 * totalMotionBudgetScale.
 *
 * Pipeline per substep (pure functions, no ArkUI):
 *   1. p011CorrectionReserve — on the candidate-angle snapshot (shadow for
 *      preview, live for apply): per eligible edge compute residual r,
 *      overshoot e = max(0, |r| − h), net contribution derivative
 *      D = −Σ e·sign(r)·k over the three affected edges, curvature
 *      K = Σ k², qDescent = max(0, D)/K, qNatural = min(e/2, flux, stock,
 *      capacity), guard-admissible cap, and degree-aware local caps
 *      u = min(all caps, L_donor/degree, L_recv/degree). R_limit =
 *      min(B, Σu). Then p011CorrectionAllocate distributes R_limit by
 *      capped weighting p = min(u, λ·w), w = e·max(0, D)·dx/2 (numerically
 *      stable form of e²γ; γ itself diverges as e→0⁺ so the product form is
 *      mandatory), resolved by ascending u/w sweep without bisection.
 *   2. p011CorrectionApplyOnce — per candidate edge in the frozen
 *      (−w, edgeId) order, re-derive direction/stock/capacity/D on the
 *      CURRENT state, cap by the reservation p, debit the caller's ledgers,
 *      admit via the shared p011GuardAdmit closed-form interval on the
 *      three affected edges, refund the rejected part to every debited
 *      ledger, and let the caller apply the material move. Direction
 *      reversal or D<=0 cancels the slot (no stop semantics change).
 *
 * Uniform-field boundary: three interior same-sign equal-e edges give
 * D = 0 — such edges get no reservation weight; if every edge stalls the
 * caller reports CORRECTION_STALLED (honest, no fake progress).
 */

import {
  P011GuardEdgeConstraint,
  p011GuardAdmit,
  p011GuardConstraintsForDelta
} from './P011MaterialGuard';

/** Frozen correction parameters passed by the caller per substep. */
export interface P011CorrectionParams {
  /** Approved relax target residual bound h = tan(angleDeg) (current engine). */
  relaxBoundSlope: number;
  /** Per-edge avalanche cap = maxAvalancheFlux * dt (column-height units). */
  maxAvalancheFluxDt: number;
  /** Per-column local budget L = vSandTopMax * dt. */
  localBudget: number;
  /** Total motion budget B = totalMotionBudget * dt. */
  totalBudget: number;
  /** Substep dt (diagnostics only). */
  dt: number;
}

/** One candidate correction edge, computed on the reservation snapshot. */
export interface P011CorrectionCandidate {
  /** Physical edge index (min(donor, receiver)). */
  edge: number;
  donor: number;
  receiver: number;
  /** Signed residual r[e] against the accepted tilt. */
  r: number;
  /** Overshoot e = max(0, |r| − h). */
  e: number;
  /** Net contribution derivative D (positive = this move reduces E). */
  d: number;
  /** Curvature K = Σ k² over affected edges. */
  k: number;
  /** Natural request min(e/2, flux, stock, capacity). */
  qNatural: number;
  /** Descent cap max(0, D)/K. */
  qDescent: number;
  /** Guard-admissible cap on the snapshot (three-edge closed form). */
  guardAdmissible: number;
  /** Capped reservation u = min(all caps, local/degree caps). */
  u: number;
  /** Weight w = e · max(0, D) · dx / 2 (numerically stable e²γ). */
  w: number;
  /** Allocated reservation p ≤ u (filled by p011CorrectionAllocate). */
  p: number;
}

/** Reservation result for one substep snapshot. */
export interface P011CorrectionReservation {
  candidates: P011CorrectionCandidate[];
  /** R_limit = min(B, Σu). */
  rLimit: number;
  /** Per-column reservation P_i = Σ (p of candidates touching i). */
  pByColumn: Float64Array;
  /** Execution order: candidate indices sorted by (−w, edge). */
  order: number[];
  /** Sum of ordinary relax requests on the snapshot (for scale folding). */
  ordinaryRelaxRequested: number;
}

/** Stock reader abstraction so preview (shadow) and apply (live) share code. */
export interface P011CorrectionStockView {
  sandTop: Float64Array;
  effectiveTiltDeg: number;
  /** Donor movable stock (donor column height). */
  donorStock(column: number): number;
  /** Receiver capacity (water surface headroom or container capacity). */
  receiverCapacity(column: number): number;
  /** Three-edge guard residuals read from the CURRENT sandTop. */
  guardResidual(edge: number): number;
}

/** Guard admit on the current view (shared by apply and preview). */
function correctionGuardAdmissible(view: P011CorrectionStockView,
  donor: number, receiver: number, requested: number,
  slopeHardMax: number, edgeCount: number): number {
  if (requested <= 0) {
    return 0;
  }
  const deltaColumns = new Map<number, number>();
  deltaColumns.set(donor, -1);
  deltaColumns.set(receiver, 1);
  const constraints = p011GuardConstraintsForDelta(deltaColumns,
    (edge: number): number => view.guardResidual(edge), edgeCount);
  const outcome = p011GuardAdmit(requested, constraints, slopeHardMax);
  return outcome.accepted;
}

/**
 * Reservation pass on one snapshot. Eligibility mirrors the existing relax
 * trigger (|r| > h) plus the local-contribution requirement D > 0; e > 0 is
 * implied by |r| > h. Edges with D <= 0 are excluded (uniform-field
 * cancellation, NO_POSITIVE_LOCAL_CONTRIBUTION) — no artificial epsilon
 * weights activate zero-contribution edges.
 */
export function p011CorrectionReserve(
  view: P011CorrectionStockView,
  params: P011CorrectionParams,
  slopeHardMax: number,
  ordinaryRelaxRequested: number): P011CorrectionReservation {
  const cols = view.sandTop.length;
  const edgeCount = cols - 1;
  const tanTilt = Math.tan(view.effectiveTiltDeg * Math.PI / 180);
  const candidates: P011CorrectionCandidate[] = [];
  // degree_i = number of candidate edges touching column i (for local caps).
  const degree = new Int32Array(cols);

  for (let edge = 0; edge < edgeCount; edge++) {
    const r = view.sandTop[edge + 1] - view.sandTop[edge] - tanTilt;
    if (Math.abs(r) <= params.relaxBoundSlope) {
      continue; // not an eligible relax edge under the existing trigger
    }
    const e = Math.abs(r) - params.relaxBoundSlope;
    // Direction: r > 0 -> right column is too high -> donor = edge+1.
    const donor = r > 0 ? edge + 1 : edge;
    const receiver = r > 0 ? edge : edge + 1;
    // Influence coefficients (dx = 1): k[c] = −2·s, k[c±1] = +s.
    const s = r > 0 ? 1 : -1;
    const kCenter = -2 * s;
    const kSide = s;
    // D = −Σ e·sign(r)·k over affected edges (e overhang uses each edge's
    // own residual sign; edges inside the bound contribute 0).
    const contrib = (edgeIndex: number): number => {
      if (edgeIndex < 0 || edgeIndex >= edgeCount) {
        return 0;
      }
      const rr = view.sandTop[edgeIndex + 1] - view.sandTop[edgeIndex] - tanTilt;
      if (Math.abs(rr) <= params.relaxBoundSlope) {
        return 0;
      }
      const ee = Math.abs(rr) - params.relaxBoundSlope;
      return ee * (rr > 0 ? 1 : -1);
    };
    const d = -(contrib(edge - 1) * kSide +
      contrib(edge) * kCenter +
      contrib(edge + 1) * kSide);
    // K = Σ k² over the existing affected edges (interior 6, boundary 5).
    let k = kCenter * kCenter + 2 * kSide * kSide;
    if (edge === 0 || edge === edgeCount - 1) {
      k = kCenter * kCenter + kSide * kSide;
    }
    if (d <= 1e-12 || e <= 0) {
      // No positive local contribution: uniform-field cancellation (D is
      // exactly zero on uniform interior edges in exact arithmetic; the
      // 1e-12 floor absorbs the float noise of the contribution sum).
      continue;
    }
    const qDescent = d / k;
    const qNatural = Math.min(e / 2, params.maxAvalancheFluxDt,
      Math.max(0, view.donorStock(donor)),
      Math.max(0, view.receiverCapacity(receiver)));
    const guardAdmissible = correctionGuardAdmissible(view, donor, receiver,
      Math.min(qNatural, qDescent), slopeHardMax, edgeCount);
    candidates.push({
      edge: edge,
      donor: donor,
      receiver: receiver,
      r: r,
      e: e,
      d: d,
      k: k,
      qNatural: qNatural,
      qDescent: qDescent,
      guardAdmissible: guardAdmissible,
      u: 0,
      w: e * d / 2, // w = e·D·dx/2 (stable product form of e²γ)
      p: 0
    });
    degree[donor] += 1;
    degree[receiver] += 1;
  }

  // Local degree caps + u.
  for (const candidate of candidates) {
    const localCapDonor = degree[candidate.donor] > 0
      ? params.localBudget / degree[candidate.donor] : params.localBudget;
    const localCapReceiver = degree[candidate.receiver] > 0
      ? params.localBudget / degree[candidate.receiver] : params.localBudget;
    candidate.u = Math.max(0, Math.min(candidate.qNatural, candidate.qDescent,
      candidate.guardAdmissible, localCapDonor, localCapReceiver));
  }

  const sumU = candidates.reduce((sum, c) => sum + c.u, 0);
  const rLimit = Math.min(params.totalBudget, sumU);

  const pByColumn = new Float64Array(cols);
  const order = candidates.map((c, index) => index)
    .sort((a, b) => {
      const wa = candidates[a].w;
      const wb = candidates[b].w;
      if (wa !== wb) {
        return wb - wa; // weight descending
      }
      return candidates[a].edge - candidates[b].edge; // edge ascending
    });

  const reservation: P011CorrectionReservation = {
    candidates: candidates,
    rLimit: rLimit,
    pByColumn: pByColumn,
    order: order,
    ordinaryRelaxRequested: ordinaryRelaxRequested
  };
  p011CorrectionAllocate(reservation);
  return reservation;
}

/**
 * Capped weighted allocation p = min(u, λ·w) with Σp = R_limit (or Σp = Σu
 * when R ≥ Σu): ascending u/w sweep — an item saturates when u ≤ λ·w with
 * the CURRENT λ = remaining/weightSum (it cannot use more than u, so fix
 * it, shrink both remaining and weightSum, and continue); the first item
 * that does NOT saturate proves λ is now too small to saturate ANY
 * remaining item (they are sorted by ascending u/w), so every remaining
 * item takes p = λ·w with the same single λ and the sweep stops. No
 * bisection, single pass.
 */
export function p011CorrectionAllocate(reservation: P011CorrectionReservation): void {
  const candidates = reservation.candidates;
  if (candidates.length === 0) {
    return;
  }
  const sumU = candidates.reduce((sum, c) => sum + c.u, 0);
  const rLimit = Math.min(reservation.rLimit, sumU);
  reservation.rLimit = rLimit;
  for (const c of candidates) {
    c.p = 0;
  }
  if (rLimit <= 0) {
    return;
  }
  // Active set sorted by u/w ascending (saturate cheapest weight first).
  const byRatio = candidates
    .filter(c => c.u > 0 && c.w > 0)
    .sort((a, b) => a.u / a.w - b.u / b.w);
  let remaining = rLimit;
  let weightSum = byRatio.reduce((sum, c) => sum + c.w, 0);
  for (const c of byRatio) {
    if (weightSum <= 0 || remaining <= 0) {
      break;
    }
    const lambda = remaining / weightSum;
    if (c.u <= lambda * c.w) {
      // Saturates: fix at u, shrink the active set.
      c.p = c.u;
      remaining -= c.u;
      weightSum -= c.w;
    } else {
      // λ too small to saturate this item; every remaining item (sorted by
      // ascending u/w, so larger ratios) is also unsaturated — one shared λ.
      for (const rest of byRatio) {
        if (rest.p === 0 && rest.u > 0 && rest.w > 0) {
          rest.p = lambda * rest.w;
        }
      }
      remaining = 0;
      break;
    }
  }
  // Per-column reservation P_i.
  for (const c of candidates) {
    if (c.p > 0) {
      reservation.pByColumn[c.donor] += c.p;
      reservation.pByColumn[c.receiver] += c.p;
    }
  }
}

/** One apply-step outcome for a correction candidate. */
export interface P011CorrectionApplyOutcome {
  /** Index into reservation.candidates. */
  index: number;
  /** Reservation p at entry. */
  reserved: number;
  /** Final accepted grant (0 when cancelled / guard-rejected). */
  accepted: number;
  /** Requested-at-apply (min(p, refreshed caps)); may be 0 when cancelled. */
  requested: number;
  /** Rejected by the guard (requested − accepted while requested > 0). */
  rejected: number;
  cancelled: boolean;
  /** Cancellation / stall reason (diagnostics). */
  reason: string | null;
}

/** Ledger debit/refund surface the caller must provide per candidate. */
export interface P011CorrectionLedger {
  /** Debit q from local (donor+receiver) + total + correction ledgers. */
  debit(donor: number, receiver: number, amount: number): void;
  /** Refund the rejected part to the same ledgers (exactly once each). */
  refund(donor: number, receiver: number, amount: number): void;
}

/**
 * Apply ONE correction candidate on the current state. Shared verbatim by
 * the real apply (live arrays) and the preview mirror (shadow arrays) per
 * §24.2 collaboration requirement. The caller owns the material write; this
 * function only computes the admissible q and performs ledger
 * debit/refund so both sides stay bit-identical.
 */
export function p011CorrectionApplyOnce(
  reservation: P011CorrectionReservation,
  index: number,
  view: P011CorrectionStockView,
  params: P011CorrectionParams,
  slopeHardMax: number,
  ledger: P011CorrectionLedger): P011CorrectionApplyOutcome {
  const candidate = reservation.candidates[index];
  const reserved = candidate.p;
  const outcome: P011CorrectionApplyOutcome = {
    index: index,
    reserved: reserved,
    accepted: 0,
    requested: 0,
    rejected: 0,
    cancelled: true,
    reason: null
  };
  if (reserved <= 0) {
    outcome.reason = 'ZERO_RESERVATION';
    return outcome;
  }
  // Refresh everything on the CURRENT state (no stale snapshot reuse).
  const cols = view.sandTop.length;
  const edgeCount = cols - 1;
  const tanTilt = Math.tan(view.effectiveTiltDeg * Math.PI / 180);
  const r = view.sandTop[candidate.edge + 1] - view.sandTop[candidate.edge] - tanTilt;
  if (Math.abs(r) <= params.relaxBoundSlope) {
    outcome.reason = 'WITHIN_BOUND';
    return outcome;
  }
  const e = Math.abs(r) - params.relaxBoundSlope;
  // Direction reversal cancels this slot.
  const directionNow = r > 0 ? 1 : -1;
  const directionThen = candidate.r > 0 ? 1 : -1;
  if (directionNow !== directionThen) {
    outcome.reason = 'DIRECTION_REVERSED';
    return outcome;
  }
  const donor = candidate.donor;
  const receiver = candidate.receiver;
  const s = directionNow;
  const kCenter = -2 * s;
  const kSide = s;
  const contrib = (edgeIndex: number): number => {
    if (edgeIndex < 0 || edgeIndex >= edgeCount) {
      return 0;
    }
    const rr = view.sandTop[edgeIndex + 1] - view.sandTop[edgeIndex] - tanTilt;
    if (Math.abs(rr) <= params.relaxBoundSlope) {
      return 0;
    }
    const ee = Math.abs(rr) - params.relaxBoundSlope;
    return ee * (rr > 0 ? 1 : -1);
  };
  const d = -(contrib(candidate.edge - 1) * kSide +
    contrib(candidate.edge) * kCenter +
    contrib(candidate.edge + 1) * kSide);
  if (d <= 1e-12) {
    outcome.reason = 'NO_POSITIVE_LOCAL_CONTRIBUTION';
    return outcome;
  }
  let k = kCenter * kCenter + 2 * kSide * kSide;
  if (candidate.edge === 0 || candidate.edge === edgeCount - 1) {
    k = kCenter * kCenter + kSide * kSide;
  }
  const qDescent = d / k;
  const qNatural = Math.min(e / 2, params.maxAvalancheFluxDt,
    Math.max(0, view.donorStock(donor)),
    Math.max(0, view.receiverCapacity(receiver)));
  const requested = Math.min(reserved, qDescent, qNatural);
  outcome.requested = requested;
  if (requested <= 0) {
    outcome.reason = 'STOCK_OR_CAPACITY_EXHAUSTED';
    return outcome;
  }
  // Debit all ledgers, then admit on the CURRENT three-edge residuals.
  ledger.debit(donor, receiver, requested);
  const accepted = correctionGuardAdmissible(view, donor, receiver, requested,
    slopeHardMax, edgeCount);
  const rejected = requested - accepted;
  if (rejected > 0) {
    ledger.refund(donor, receiver, rejected);
  }
  outcome.accepted = accepted;
  outcome.rejected = rejected;
  outcome.cancelled = false;
  outcome.reason = accepted > 0 ? null : 'GUARD_REJECTED';
  return outcome;
}

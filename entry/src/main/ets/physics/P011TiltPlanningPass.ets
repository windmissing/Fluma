/**
 * P-011 architecture §20 / v2.14 dynamic tilt feasibility planning pass.
 * Implements the two-pass, three-candidate control law of §20.2/§20.6:
 *   pass 1 preview at thetaRequested -> feasible interval -> clamp or infeasible
 *   pass 2 replan at theta1 (clamped) -> old-angle fallback -> BLOCKED
 * This module holds the pure math, plan/result types and the deterministic
 * candidate ledger. WaterSandField drives the passes with its own authoritative
 * channel machinery and attaches itself as P011PlanningPassDelegate for request
 * collection and preview allocation, so preview and commit share one code path
 * (§20.6.4). Status note: maxTiltOmega remains UNCALIBRATED; the value supplied
 * by the run configuration is used as-is and no default is invented here.
 */

export type P011ExistingChannelId =
  'settle' | 'percolate' | 'bottomCreep' | 'relax' | 'lift' | 'suspensionDiffusion' |
  // §24 relax-correction-weighted-v1: independent reservation-first budget
  // sub-channel replacing the relax slot's budget allocation.
  'correction' |
  // S4 deep-creep (architecture-P011-S4-deep-creep-v1.md §1.1): the
  // bh-interface potential-gradient reverse-exchange channel. Rank 6;
  // correction shifts to 7 (rank is a pure lexicographic tool — preview
  // ordering only, never in the ledger).
  'deepCreep' |
  // P-011 plan-3 stage-a (architecture-P011-advection-v1.md §1.1): the
  // suspension advection channel — directed transport of EXISTING
  // suspension under four budget layers (S-invariant). Rank 8 (after
  // correction's 7); rank stays a lexicographic preview tool only.
  'advection';

export interface P011ExistingRequestShape {
  requested: number;
}

export interface P011ExistingGrantShape {
  granted: number;
  clipped: number;
  rejected: number;
}

export interface P011ExistingBudgetUse {
  local: number;
  bottom: number;
  total: number;
  channelRateCap: number;
}

export interface P011ExistingChannelRateCapRemaining {
  settle: number;
  percolate: number;
  bottomCreep: number;
  relax: number;
  lift: number;
  suspensionDiffusion: number;
  correction: number;
  /** S4 §1.2: booked but NOT capped in the first version (observe then
   * calibrate — the cap candidate lands after the sweep). */
  deepCreep: number;
  /** Advection §1.3: the channel rate cap IS a real truncation dimension
   * (the fifth q_admit min term — designer §2), unlike S4's observe-only
   * booking. */
  advection: number;
}

export interface P011ExistingBudgetSnapshot {
  localRemaining: number[];
  bottomRemaining: number;
  totalRemaining: number;
  channelRateCapRemaining: P011ExistingChannelRateCapRemaining;
}

export interface P011ExistingChannelStateSnapshot {
  sliding: boolean[];
}

export interface P011TiltFeasibilityInput {
  runId: string;
  substepId: number;
  dt: number;
  thetaBefore: number;
  targetTilt: number;
  maxTiltOmega: number;
  sandTopBefore: Float64Array;
  budgetsBefore: P011ExistingBudgetSnapshot;
  channelStateBefore: P011ExistingChannelStateSnapshot;
}

export interface P011ChannelPlan {
  channelId: P011ExistingChannelId;
  requested: P011ExistingRequestShape;
  plannedGrant: P011ExistingGrantShape;
  deltaSandTopByColumn: Float64Array;
  budgetUse: P011ExistingBudgetUse;
  admissibility: 'admissible' | 'clipped' | 'rejected';
}

export interface P011TiltFeasibilityResult {
  thetaRequested: number;
  thetaAccepted: number;
  feasibleThetaMin: number;
  feasibleThetaMax: number;
  limitingEdge: number | null;
  plans: P011ChannelPlan[];
  passCount: 1 | 2;
  fallbackUsed: boolean;
  status: 'accepted' | 'clamped' | 'held' | 'blocked';
  blockedReason: string | null;
}

export interface P011PlanningPassPreview {
  input: P011TiltFeasibilityInput;
  result: P011TiltFeasibilityResult;
  discard(): void;
  commit(): void;
}

export const P011_PLANNING_PASS_TAN_HARD_MAX = Math.tan(30 * Math.PI / 180);

/**
 * Angle-domain comparison tolerance for feasibility interval decisions.
 * The exact-equality rule of §20.6.4 applies to plan comparison only; the
 * interval intersection must absorb float noise (atan round-trip, boundary
 * residuals hugging the hard max after repeated excess/2 relax steps) or a
 * previously committed boundary angle would be spuriously rejected.
 */
export const P011_PLANNING_PASS_ANGLE_EPS = 1e-9;

export const P011_BLOCKED_EMPTY_FEASIBLE_INTERVAL = 'EMPTY_FEASIBLE_INTERVAL';
export const P011_BLOCKED_NO_SAFE_OLD_TILT_PLAN = 'NO_SAFE_OLD_TILT_PLAN';
export const P011_BLOCKED_PREEXISTING_INFEASIBLE_STATE = 'PREEXISTING_INFEASIBLE_STATE';
export const P011_BLOCKED_PREVIEW_COMMIT_MISMATCH = 'PREVIEW_COMMIT_MISMATCH';

/** Fixed channel rank for deterministic ordering (§20.6.2). */
export function p011ChannelRank(id: P011ExistingChannelId): number {
  switch (id) {
    case 'settle':
      return 0;
    case 'percolate':
      return 1;
    case 'bottomCreep':
      return 2;
    case 'relax':
      return 3;
    case 'lift':
      return 4;
    case 'suspensionDiffusion':
      return 5;
    case 'deepCreep':
      return 6;
    case 'correction':
      return 7;
    case 'advection':
      return 8;
    default:
      return 8;
  }
}

function normalizeZero(value: number): number {
  return value === 0 ? 0 : value;
}

/**
 * Feasible reference-angle interval for one edge, in slope space:
 * theta must satisfy |g0[c] + deltaG[c] - tan(theta)| <= H.
 * Returns [loSlope, hiSlope] or null when the edge input is not finite.
 */
export function p011EdgeSlopeInterval(g0PlusDeltaG: number,
  hardMax: number): [number, number] | null {
  const lo = g0PlusDeltaG - hardMax;
  const hi = g0PlusDeltaG + hardMax;
  if (!(lo <= hi) || !Number.isFinite(lo) || !Number.isFinite(hi)) {
    return null;
  }
  return [lo, hi];
}

/**
 * Deterministic feasibility decision for one candidate angle against the
 * per-edge slope intervals. Returns the clamped angle plus the limiting edge
 * (smallest interfaceIndex on ties, §20.6.3), or infeasible. The accepted
 * angle is additionally intersected with the max-omega step interval and the
 * directed interval from thetaBefore toward targetTilt (§20.2).
 */
export interface P011CandidateDecision {
  feasible: boolean;
  thetaClamped: number | null;
  limitingEdge: number | null;
  intervalLoSlope: number;
  intervalHiSlope: number;
}

export function p011DecideCandidate(thetaCandidate: number,
  g0: Float64Array, deltaS: Float64Array, hardMax: number,
  dt: number, thetaBefore: number, maxTiltOmega: number,
  targetTilt: number): P011CandidateDecision {
  const cols = g0.length;
  let loSlope = Number.NEGATIVE_INFINITY;
  let hiSlope = Number.POSITIVE_INFINITY;
  let limitingEdge: number | null = null;
  for (let c = 0; c < cols - 1; c++) {
    const gNext = g0[c + 1] - g0[c] + (deltaS[c + 1] - deltaS[c]);
    const interval = p011EdgeSlopeInterval(gNext, hardMax);
    if (interval === null) {
      return {
        feasible: false,
        thetaClamped: null,
        limitingEdge: c,
        intervalLoSlope: Number.NaN,
        intervalHiSlope: Number.NaN
      };
    }
    // §20.6.3 tie-break: strict comparison keeps the smallest interfaceIndex
    // when multiple edges give the same constraint value (ascending scan).
    if (interval[0] > loSlope) {
      loSlope = interval[0];
      limitingEdge = c;
    }
    if (interval[1] < hiSlope) {
      hiSlope = interval[1];
      limitingEdge = c;
    }
  }
  if (!(loSlope <= hiSlope)) {
    return {
      feasible: false,
      thetaClamped: null,
      limitingEdge: limitingEdge,
      intervalLoSlope: loSlope,
      intervalHiSlope: hiSlope
    };
  }
  const maxStep = maxTiltOmega * dt;
  const stepLo = thetaBefore - maxStep;
  const stepHi = thetaBefore + maxStep;
  const directedLo = targetTilt >= thetaBefore ? thetaBefore : targetTilt;
  const directedHi = targetTilt >= thetaBefore ? targetTilt : thetaBefore;
  const loAngle = Math.max(Math.atan(loSlope) * 180 / Math.PI, stepLo, directedLo);
  const hiAngle = Math.min(Math.atan(hiSlope) * 180 / Math.PI, stepHi, directedHi);
  if (loAngle > hiAngle + P011_PLANNING_PASS_ANGLE_EPS) {
    return {
      feasible: false,
      thetaClamped: null,
      limitingEdge: limitingEdge,
      intervalLoSlope: loSlope,
      intervalHiSlope: hiSlope
    };
  }
  const clamped = Math.max(loAngle, Math.min(hiAngle, thetaCandidate));
  return {
    feasible: true,
    thetaClamped: normalizeZero(clamped),
    limitingEdge: limitingEdge,
    intervalLoSlope: loSlope,
    intervalHiSlope: hiSlope
  };
}

/** Exact equality check for preview vs commit (§20.6.4). -0 normalized, NaN rejected. */
export function p011PreviewCommitConsistent(preview: P011TiltFeasibilityResult,
  committedTheta: number, committedPlans: P011ChannelPlan[]): boolean {
  if (!Number.isFinite(preview.thetaAccepted) || !Number.isFinite(committedTheta)) {
    return false;
  }
  if (normalizeZero(preview.thetaAccepted) !== normalizeZero(committedTheta)) {
    return false;
  }
  if (preview.plans.length !== committedPlans.length) {
    return false;
  }
  for (let index = 0; index < preview.plans.length; index++) {
    const p = preview.plans[index];
    const c = committedPlans[index];
    if (p.channelId !== c.channelId) {
      return false;
    }
    if (normalizeZero(p.plannedGrant.granted) !== normalizeZero(c.plannedGrant.granted)) {
      return false;
    }
    if (normalizeZero(p.plannedGrant.clipped) !== normalizeZero(c.plannedGrant.clipped)) {
      return false;
    }
    if (normalizeZero(p.plannedGrant.rejected) !== normalizeZero(c.plannedGrant.rejected)) {
      return false;
    }
    if (normalizeZero(p.budgetUse.local) !== normalizeZero(c.budgetUse.local) ||
      normalizeZero(p.budgetUse.bottom) !== normalizeZero(c.budgetUse.bottom) ||
      normalizeZero(p.budgetUse.total) !== normalizeZero(c.budgetUse.total) ||
      normalizeZero(p.budgetUse.channelRateCap) !== normalizeZero(c.budgetUse.channelRateCap)) {
      return false;
    }
    if (p.deltaSandTopByColumn.length !== c.deltaSandTopByColumn.length) {
      return false;
    }
    for (let k = 0; k < p.deltaSandTopByColumn.length; k++) {
      const pv = p.deltaSandTopByColumn[k];
      const cv = c.deltaSandTopByColumn[k];
      if (!Number.isFinite(pv) || !Number.isFinite(cv)) {
        return false;
      }
      if (normalizeZero(pv) !== normalizeZero(cv)) {
        return false;
      }
    }
  }
  return true;
}

/**
 * Delegate interface: WaterSandField provides the authoritative request
 * collection (its own collect phase at a candidate angle) and the preview
 * allocation (mirroring its own caps in fixed channel order). The service
 * owns only the deterministic two-pass candidate control flow.
 */
export interface P011PlanningPassDelegate {
  /**
   * Run one pure preview at thetaCandidate: collect all channel requests from
   * the S0 snapshot, allocate against an independent copy of B0 with the
   * existing channel order and caps, and return the complete plan list. Must
   * not mutate any physical or budget state.
   */
  previewAt(thetaCandidate: number): P011ChannelPlan[] | null;

  /** Copy of the substep-start sandTop (S0). */
  sandTopSnapshot(): Float64Array;

  /** True when the S0 state already violates the hard max at thetaBefore. */
  preexistingViolation(hardMax: number): boolean;
}

function blockedResult(input: P011TiltFeasibilityInput, reason: string,
  passCount: 1 | 2): P011TiltFeasibilityResult {
  return {
    thetaRequested: input.thetaBefore,
    thetaAccepted: input.thetaBefore,
    feasibleThetaMin: Number.NaN,
    feasibleThetaMax: Number.NaN,
    limitingEdge: null,
    plans: [],
    passCount: passCount,
    fallbackUsed: reason === P011_BLOCKED_NO_SAFE_OLD_TILT_PLAN ||
      reason === P011_BLOCKED_PREEXISTING_INFEASIBLE_STATE,
    status: 'blocked',
    blockedReason: reason
  };
}

export class P011TiltPlanningPassService {
  private delegate: P011PlanningPassDelegate | null = null;
  private lastCommitted: P011PlanningPassPreview | null = null;

  attach(delegate: P011PlanningPassDelegate): void {
    this.delegate = delegate;
  }

  detach(): void {
    this.delegate = null;
    this.lastCommitted = null;
  }

  createPreview(input: P011TiltFeasibilityInput,
    hardMaxSlope: number): P011PlanningPassPreview {
    const delegate = this.delegate;
    if (delegate === null) {
      const result = blockedResult(input, P011_BLOCKED_NO_SAFE_OLD_TILT_PLAN, 1);
      return this.wrapBlocked(input, result);
    }
    if (delegate.preexistingViolation(hardMaxSlope)) {
      const result = blockedResult(input, P011_BLOCKED_PREEXISTING_INFEASIBLE_STATE, 1);
      return this.wrapBlocked(input, result);
    }
    const maxStep = input.maxTiltOmega * input.dt;
    const thetaRequested = input.targetTilt >= input.thetaBefore
      ? Math.min(input.targetTilt, input.thetaBefore + maxStep)
      : Math.max(input.targetTilt, input.thetaBefore - maxStep);
    // PASS 1: preview at thetaRequested.
    const g0 = delegate.sandTopSnapshot();
    const plans1 = delegate.previewAt(thetaRequested);
    if (plans1 === null) {
      const result = blockedResult(input, P011_BLOCKED_NO_SAFE_OLD_TILT_PLAN, 1);
      return this.wrapBlocked(input, result);
    }
    const deltaS1 = this.combineDeltaS(plans1, g0.length);
    const decision1 = p011DecideCandidate(thetaRequested, g0, deltaS1, hardMaxSlope,
      input.dt, input.thetaBefore, input.maxTiltOmega, input.targetTilt);
    if (decision1.feasible && decision1.thetaClamped !== null &&
      Math.abs(decision1.thetaClamped - thetaRequested) <= P011_PLANNING_PASS_ANGLE_EPS) {
      const result: P011TiltFeasibilityResult = {
        thetaRequested: thetaRequested,
        thetaAccepted: thetaRequested,
        feasibleThetaMin: Math.atan(decision1.intervalLoSlope) * 180 / Math.PI,
        feasibleThetaMax: Math.atan(decision1.intervalHiSlope) * 180 / Math.PI,
        limitingEdge: decision1.limitingEdge,
        plans: plans1,
        passCount: 1,
        fallbackUsed: false,
        status: 'accepted',
        blockedReason: null
      };
      return this.wrapCommitted(input, result);
    }
    // PASS 1 clamped: theta1 candidate requires pass 2 replan (§20.2).
    if (decision1.feasible && decision1.thetaClamped !== null) {
      const theta1 = decision1.thetaClamped;
      const plans2 = delegate.previewAt(theta1);
      if (plans2 !== null) {
        const deltaS2 = this.combineDeltaS(plans2, g0.length);
        const decision2 = p011DecideCandidate(theta1, g0, deltaS2, hardMaxSlope,
          input.dt, input.thetaBefore, input.maxTiltOmega, input.targetTilt);
        if (decision2.feasible && decision2.thetaClamped !== null &&
          Math.abs(decision2.thetaClamped - theta1) <= P011_PLANNING_PASS_ANGLE_EPS) {
          const result: P011TiltFeasibilityResult = {
            thetaRequested: thetaRequested,
            thetaAccepted: theta1,
            feasibleThetaMin: Math.atan(decision2.intervalLoSlope) * 180 / Math.PI,
            feasibleThetaMax: Math.atan(decision2.intervalHiSlope) * 180 / Math.PI,
            limitingEdge: decision2.limitingEdge,
            plans: plans2,
            passCount: 2,
            fallbackUsed: false,
            status: 'clamped',
            blockedReason: null
          };
          return this.wrapCommitted(input, result);
        }
      }
    }
    // Old-angle fallback evaluated within pass 2 against a separate B0 copy.
    const plansFallback = delegate.previewAt(input.thetaBefore);
    if (plansFallback !== null) {
      const deltaSFallback = this.combineDeltaS(plansFallback, g0.length);
      const decisionFallback = p011DecideCandidate(input.thetaBefore, g0, deltaSFallback,
        hardMaxSlope, input.dt, input.thetaBefore, input.maxTiltOmega, input.targetTilt);
      if (decisionFallback.feasible && decisionFallback.thetaClamped !== null &&
        Math.abs(decisionFallback.thetaClamped - input.thetaBefore) <= P011_PLANNING_PASS_ANGLE_EPS) {
        const result: P011TiltFeasibilityResult = {
          thetaRequested: thetaRequested,
          thetaAccepted: input.thetaBefore,
          feasibleThetaMin: Math.atan(decisionFallback.intervalLoSlope) * 180 / Math.PI,
          feasibleThetaMax: Math.atan(decisionFallback.intervalHiSlope) * 180 / Math.PI,
          limitingEdge: decisionFallback.limitingEdge,
          plans: plansFallback,
          passCount: 2,
          fallbackUsed: true,
          status: 'held',
          blockedReason: null
        };
        return this.wrapCommitted(input, result);
      }
    }
    const result = blockedResult(input, P011_BLOCKED_NO_SAFE_OLD_TILT_PLAN, 2);
    return this.wrapBlocked(input, result);
  }

  private combineDeltaS(plans: P011ChannelPlan[], cols: number): Float64Array {
    const deltaS = new Float64Array(cols);
    for (const plan of plans) {
      for (let c = 0; c < cols; c++) {
        deltaS[c] += plan.deltaSandTopByColumn[c];
      }
    }
    return deltaS;
  }

  private wrapCommitted(input: P011TiltFeasibilityInput,
    result: P011TiltFeasibilityResult): P011PlanningPassPreview {
    const preview = new P011PlanningPassPreviewHandle(this, input, result, true);
    this.lastCommitted = preview;
    return preview;
  }

  private wrapBlocked(input: P011TiltFeasibilityInput,
    result: P011TiltFeasibilityResult): P011PlanningPassPreview {
    return new P011PlanningPassPreviewHandle(this, input, result, false);
  }

  internalClear(handle: P011PlanningPassPreviewHandle): void {
    if (this.lastCommitted === handle) {
      this.lastCommitted = null;
    }
  }

  internalAssertActive(handle: P011PlanningPassPreviewHandle): void {
    if (this.lastCommitted !== handle) {
      throw new Error('P011_PLANNING_PASS_COMMIT_WITHOUT_PREVIEW');
    }
  }
}

/**
 * Class-based preview handle (ArkTS forbids untyped object literals with
 * function properties). Committable handles register with the service so the
 * single-commit rule can verify the preview is still the selected candidate.
 */
export class P011PlanningPassPreviewHandle implements P011PlanningPassPreview {
  readonly input: P011TiltFeasibilityInput;
  readonly result: P011TiltFeasibilityResult;
  private readonly owner: P011TiltPlanningPassService;
  private readonly committable: boolean;

  constructor(owner: P011TiltPlanningPassService, input: P011TiltFeasibilityInput,
    result: P011TiltFeasibilityResult, committable: boolean) {
    this.owner = owner;
    this.input = input;
    this.result = result;
    this.committable = committable;
  }

  discard(): void {
    if (this.committable) {
      this.owner.internalClear(this);
    }
  }

  commit(): void {
    if (!this.committable) {
      throw new Error('P011_PLANNING_PASS_BLOCKED');
    }
    this.owner.internalAssertActive(this);
  }
}

/**
 * P-011 architecture §19.3 natural-relax-hysteresis-v1 dual-threshold state
 * machine (observation-3 v3 execution identity, GAP-1).
 *
 * Pure functions and record types only — no ArkUI, AppStorage, sensor or file
 * imports. The state array itself lives in WaterSandField (sliding[]);
 * this module owns the transition function, the threshold derivation, the
 * request formula and the decision-observation records (protocol v3 §2.1).
 *
 * Semantics (§19.3 interface contract, frozen):
 *   d[c] = S[c+1] - S[c] - tan(effectiveTilt) * dx   (dx = 1)
 *   r[c] = |d[c]|                                     (normalized residual)
 *   slopeStart = tan(thetaStartDeg), slopeStop = tan(thetaStopDeg)
 *   inactive && r >= slopeStart -> enter sliding (START_THRESHOLD_CROSSED)
 *   active   && r <= slopeStop  -> exit sliding  (STOP_THRESHOLD_CROSSED)
 *   between thresholds          -> hold current state (HOLD_ACTIVE/HOLD_INACTIVE)
 *   requested = max(0, r - slopeStop) * dx / 2 when activeAfter, clipped by
 *   maxAvalancheFlux*dt, donor stock and receiver capacity.
 *
 * Decision sampling point (protocol v3 §2.1): the residual the state machine
 * actually reads when choosing the branch — recorded per decision with
 * decisionG (signed slope), decisionR (signed residual), decisionAlpha =
 * atan(|decisionR|) in degrees, the threshold branch actually consulted,
 * and a transitionReason. Never the whole-step stateBefore nor a post-apply
 * residual.
 *
 * LEGACY mode: when the run has no candidate thresholds configured
 * (thetaStartDeg/thetaStopDeg null), WaterSandField keeps the pre-v3.1
 * relax behavior bit-for-bit (single tan(angleDeg) bound, requested>EPS
 * active proxy). The hysteresis state machine is OFF and
 * p011RelaxHysteresisEnabled reports false; executedThresholds stays null so
 * checkers can separate legacy runs from candidate runs.
 */

export const P011_RELAX_HYSTERESIS_VERSION = 'natural-relax-hysteresis-v1';

/** §19.3 P011RelaxHysteresisParams — candidate thresholds, degrees. */
export interface P011RelaxHysteresisParams {
  thetaStartDeg: number;
  thetaStopDeg: number;
  thetaHardMaxDeg: number;
}

/** §19.3 thresholds — tan-space bounds the state machine actually reads. */
export interface P011RelaxHysteresisThresholds {
  slopeStart: number;
  slopeStop: number;
  slopeHardMax: number;
}

/**
 * §19.3 threshold derivation with the frozen parameter invariant
 * 0 <= thetaStopDeg < thetaStartDeg <= thetaHardMaxDeg <= 30.
 * Throws P011RelaxHysteresisParamError on violation — construction/config
 * failure, never a silent default (§19.9).
 */
export class P011RelaxHysteresisParamError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'P011RelaxHysteresisParamError';
  }
}

export function p011RelaxHysteresisThresholds(
  params: P011RelaxHysteresisParams
): P011RelaxHysteresisThresholds {
  const values = [params.thetaStartDeg, params.thetaStopDeg, params.thetaHardMaxDeg];
  for (const value of values) {
    if (!Number.isFinite(value)) {
      throw new P011RelaxHysteresisParamError('non-finite hysteresis threshold');
    }
  }
  if (!(0 <= params.thetaStopDeg && params.thetaStopDeg < params.thetaStartDeg &&
    params.thetaStartDeg <= params.thetaHardMaxDeg && params.thetaHardMaxDeg <= 30)) {
    throw new P011RelaxHysteresisParamError(
      'hysteresis invariant violated: 0 <= stop < start <= hardMax <= 30');
  }
  return {
    slopeStart: Math.tan(params.thetaStartDeg * Math.PI / 180),
    slopeStop: Math.tan(params.thetaStopDeg * Math.PI / 180),
    slopeHardMax: Math.tan(params.thetaHardMaxDeg * Math.PI / 180)
  };
}

/**
 * §27.1: the ACTIVE threshold configuration — the single object every
 * consumer reads (state machine collect branch, planner preview mirror,
 * real apply, executedThresholdsForLedger). Lives as a private field on
 * WaterSandField (same instance, no cross-object sync seam). dual-*=non
 * null; legacy-single=null (relaxHysteresisThresholds derived from it).
 */
export interface P011ActiveThresholdConfig {
  thetaStartDeg: number;
  thetaStopDeg: number;
  /** tan derivatives (deriveHysteresisThresholds output). */
  slopeStart: number;
  slopeStop: number;
  relaxMode: 'dual-user-configured' | 'dual-protocol' | 'legacy-single';
  /** §27.4: monotonic per-configure revision (identity event). */
  configRevision: number;
  thresholdSource: 'factory-recommended' | 'user' | 'protocol-candidate' | 'legacy';
  /** 'v1' — only meaningful for factory-recommended. */
  recommendationVersion: string | null;
}

/** Protocol v3 §2.1 thresholdUsed — which branch the decision consulted. */
export interface P011RelaxHysteresisThresholdUsed {
  branch: 'start' | 'stop';
  angleDeg: number;
  slopeLimit: number;
  comparator: '>=' | '<=';
}

/** Protocol v3 §2.1 transitionReason vocabulary (frozen). */
export type P011RelaxHysteresisTransitionReason =
  'START_THRESHOLD_CROSSED'
  | 'STOP_THRESHOLD_CROSSED'
  | 'HOLD_INACTIVE'
  | 'HOLD_ACTIVE';

/**
 * Protocol v3 §2.1 per-edge decision record, captured at the state machine's
 * read-the-residual-pick-the-branch point. decisionR is SIGNED (residual with
 * sign); decisionAlpha = atan(|decisionR|) * 180 / pi.
 */
export interface P011RelaxHysteresisDecision {
  interfaceIndex: number;
  /** Same-substep decision order (1-based, per edge identity). */
  decisionOrdinal: number;
  /** Signed slope (S[c+1]-S[c])/dx at the sampling point. */
  decisionG: number;
  /** Signed residual decisionG - tan(effectiveTilt). */
  decisionR: number;
  /** atan(|decisionR|) in degrees. */
  decisionAlpha: number;
  activeBefore: boolean;
  activeAfter: boolean;
  transitionReason: P011RelaxHysteresisTransitionReason;
  thresholdUsed: P011RelaxHysteresisThresholdUsed;
}

/** One decision evaluation result (activeAfter + full observation record). */
export interface P011RelaxHysteresisNextActiveResult {
  activeAfter: boolean;
  transitionReason: P011RelaxHysteresisTransitionReason;
  thresholdUsed: P011RelaxHysteresisThresholdUsed;
}

/**
 * §19.3 three-state transition for one edge. Pure; the caller supplies
 * activeBefore from the persistent sliding[] array and receives the next
 * state plus the decision metadata (recorded by the caller into the ledger).
 */
export function p011RelaxHysteresisNextActive(
  activeBefore: boolean,
  residual: number,
  thresholds: P011RelaxHysteresisThresholds,
  thetaStartDeg: number,
  thetaStopDeg: number
): P011RelaxHysteresisNextActiveResult {
  if (!Number.isFinite(residual)) {
    // Non-finite residual: hold the previous state (never invent a flip).
    return {
      activeAfter: activeBefore,
      transitionReason: activeBefore ? 'HOLD_ACTIVE' : 'HOLD_INACTIVE',
      thresholdUsed: activeBefore
        ? { branch: 'stop', angleDeg: thetaStopDeg, slopeLimit: thresholds.slopeStop,
          comparator: '<=' }
        : { branch: 'start', angleDeg: thetaStartDeg, slopeLimit: thresholds.slopeStart,
          comparator: '>=' }
    };
  }
  const r = Math.abs(residual);
  if (!activeBefore && r >= thresholds.slopeStart) {
    return {
      activeAfter: true,
      transitionReason: 'START_THRESHOLD_CROSSED',
      thresholdUsed: { branch: 'start', angleDeg: thetaStartDeg,
        slopeLimit: thresholds.slopeStart, comparator: '>=' }
    };
  }
  if (activeBefore && r <= thresholds.slopeStop) {
    return {
      activeAfter: false,
      transitionReason: 'STOP_THRESHOLD_CROSSED',
      thresholdUsed: { branch: 'stop', angleDeg: thetaStopDeg,
        slopeLimit: thresholds.slopeStop, comparator: '<=' }
    };
  }
  return {
    activeAfter: activeBefore,
    transitionReason: activeBefore ? 'HOLD_ACTIVE' : 'HOLD_INACTIVE',
    thresholdUsed: activeBefore
      ? { branch: 'stop', angleDeg: thetaStopDeg, slopeLimit: thresholds.slopeStop,
        comparator: '<=' }
      : { branch: 'start', angleDeg: thetaStartDeg, slopeLimit: thresholds.slopeStart,
        comparator: '>=' }
  };
}

/**
 * §19.3 request formula: only active edges request; the amount is the
 * overshoot above the STOP bound — max(0, r - slopeStop) * dx / 2 — capped
 * by maxAvalancheFlux*dt, donor movable stock and receiver capacity.
 * Matches the legacy excess/2 shape (excess = |deviation| - limit) so the
 * candidate-run magnitude profile stays comparable to legacy behavior.
 */
export function p011RelaxHysteresisRequested(
  residual: number,
  thresholds: P011RelaxHysteresisThresholds,
  dt: number,
  maxAvalancheFlux: number,
  donorSand: number,
  receiverCapacity: number
): number {
  if (!Number.isFinite(residual)) {
    return 0;
  }
  const r = Math.abs(residual);
  const overshoot = r - thresholds.slopeStop;
  if (overshoot <= 0) {
    return 0;
  }
  const natural = Math.min(overshoot / 2, maxAvalancheFlux * dt,
    Math.max(0, donorSand), Math.max(0, receiverCapacity));
  return Math.max(0, natural);
}

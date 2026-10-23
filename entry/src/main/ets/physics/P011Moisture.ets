// P-011 S3 moisture profile (architecture-P011-S3-moisture-v1.md):
// the per-column moisture field + local dual-angle threshold plugin.
//
// THRESHOLD-LAYER PLUGIN, NOT A PHYSICS CHANNEL: the moisture field only
// feeds the relaxAngle state machine's per-edge threshold pairs — the
// material channels, the §19.5 guard, the 300/s budget and the §20
// planner keep their interfaces untouched (moisture changes REQUESTS,
// never the admission constants).
//
// s IS NOT A SECOND WATER LEDGER (red line): s[c,k] is a proxy profile;
// it never enters the water accounting, conservation audits or the
// snapshot's water fields.

/** Model identity (route-isolated: 's3-moisture-profile-v1'). */
export const P011_MOISTURE_MODEL_VERSION = 's3-moisture-profile-v1';
export const P011_MOISTURE_ROUTE_ID = 's3-moisture-profile-v1';

/** First-version parameter set (all values are calibration candidates,
 * explicitly NOT frozen — the task brief's red line). */
export interface P011MoistureParams {
  /** Vertical bins per column (performance tier: 8 default, 16 arm). */
  k: 8 | 16;
  /** s_base(q) = sBase0 + sBaseSlope·q (linear family, first version). */
  sBase0: number;
  sBaseSlope: number;
  /** β(q) = betaSlope·q (profile steepness, linear family). */
  betaSlope: number;
  /** μ_shake ceiling (the mixing envelope). */
  muShakeMax: number;
  /** Reserved τ candidate (first version derives μ from shakeEnergy
   * directly — the shakeDecay constant drives the fall-off). */
  muShakeTau: number;
  /** Reference water fraction (the compatibility anchor: q = qRef with
   * μ=0 yields Δ_e ≡ 0 — the ON/default same-trajectory guarantee). */
  qRef: number;
  /** Threshold angle gains: Δ_e = aC·(C(s_e)−C_ref) − aB·(B(s_e)−B_ref). */
  aC: number;
  aB: number;
  /** C(s) piecewise breakpoints (project calibration, NOT paper values). */
  cBreaks: number[];
  /** B(s) piecewise breakpoints. */
  bBreaks: number[];
  /** Donor surface-layer thickness (material units) for s_e. */
  tTop: number;
}

export function defaultP011MoistureParams(): P011MoistureParams {
  return {
    k: 8,
    // Default scene water fraction: Vw=128·160, Vs=128·200 → q=0.444…
    // (measured from the default fixture; the compatibility anchor).
    qRef: 0.444,
    sBase0: 0.35,
    sBaseSlope: 0.25,
    betaSlope: 0.9,
    muShakeMax: 0.85,
    muShakeTau: 0.45,
    aC: 3.0,
    aB: 2.0,
    cBreaks: [0.25, 0.5, 0.85],
    bBreaks: [0.3, 0.6, 0.9],
    tTop: 2.0
  };
}

/**
 * The equilibrium saturation at height fraction z (0 = column bottom,
 * 1 = sand top) for water fraction q: a base level plus a bottom-heavy
 * profile. First-version linear family:
 *   s_eq(z, q) = clamp(s_base(q) + β(q)·(1 − z), 0, 1)
 * (higher bins are drier; more water raises the whole profile).
 */
export function p011MoistureEq(z: number, q: number,
  params: P011MoistureParams): number {
  const sBase = params.sBase0 + params.sBaseSlope * q;
  const beta = params.betaSlope * q;
  const raw = sBase + beta * (1 - z);
  return Math.max(0, Math.min(1, raw));
}

/**
 * The mixed saturation after shaking:
 *   s = (1 − μ)·s_eq + μ·s_mean(q),  s_mean = ∫s_eq dz ≈ s_base + β/2.
 * μ = muShakeMax·clamp(shakeEnergy, 0, 1). The mix NEVER changes the
 * water amount (μ only re-reads the profile — red line: shake keeps Vw).
 */
export function p011MoistureMixed(z: number, q: number, mu: number,
  params: P011MoistureParams): number {
  const eq = p011MoistureEq(z, q, params);
  const sBase = params.sBase0 + params.sBaseSlope * q;
  const beta = params.betaSlope * q;
  const mean = Math.max(0, Math.min(1, sBase + beta / 2));
  return (1 - mu) * eq + mu * mean;
}

/**
 * Cohesion C(s): the c257-topology piecewise map (project calibration).
 * Three segments over [0, s3]: [0,s1) linear rise 0→c1; [s1,s2) near
 * constant c1→c2; [s2,s3] linear fall to 0; > s3 → 0. Normalized so
 * C ∈ [0, 1] (c1 = 0.8, c2 = 1.0 fixed shape; the breaks are params).
 */
export function p011MoistureCohesion(s: number, breaks: number[]): number {
  const s1 = breaks[0];
  const s2 = breaks[1];
  const s3 = breaks[2];
  if (s <= 0 || s >= s3) {
    return 0;
  }
  if (s < s1) {
    return 0.8 * (s / Math.max(s1, 1e-9));
  }
  if (s < s2) {
    return 0.8 + 0.2 * ((s - s1) / Math.max(s2 - s1, 1e-9));
  }
  return 1.0 * (1 - (s - s2) / Math.max(s3 - s2, 1e-9));
}

/**
 * Buoyancy B(s): monotonically non-decreasing (wet sand lubricates),
 * piecewise linear over the same topology with its own breaks; B ∈ [0,1].
 */
export function p011MoistureBuoyancy(s: number, breaks: number[]): number {
  const b1 = breaks[0];
  const b2 = breaks[1];
  const b3 = breaks[2];
  if (s <= 0) {
    return 0;
  }
  if (s >= b3) {
    return 1;
  }
  if (s < b1) {
    return 0.5 * (s / Math.max(b1, 1e-9));
  }
  if (s < b2) {
    return 0.5 + 0.3 * ((s - b1) / Math.max(b2 - b1, 1e-9));
  }
  return 0.8 + 0.2 * ((s - b2) / Math.max(b3 - b2, 1e-9));
}

/**
 * The per-edge threshold shift Δ_e from the donor surface saturation
 * (§4.2): Δ = aC·(C(s_e) − C(s_ref)) − aB·(B(s_e) − B(s_ref)) where the
 * reference saturations are the q=qRef equilibrium at the same effective
 * depth (so q=qRef, μ=0 ⇒ Δ ≡ 0 — the compatibility anchor).
 */
export function p011MoistureDelta(sEdge: number, sEdgeRef: number,
  params: P011MoistureParams): number {
  const cNow = p011MoistureCohesion(sEdge, params.cBreaks);
  const cRef = p011MoistureCohesion(sEdgeRef, params.cBreaks);
  const bNow = p011MoistureBuoyancy(sEdge, params.bBreaks);
  const bRef = p011MoistureBuoyancy(sEdgeRef, params.bBreaks);
  return params.aC * (cNow - cRef) - params.aB * (bNow - bRef);
}

/**
 * The feasibility clip (§4.4, constructive proof): the local pair
 *   θ_stop,e = θ_stop_base + Δ_e, θ_start,e = θ_start_base + Δ_e
 * stays inside 0 ≤ stop < start ≤ 30 and stop > deadzone exactly when
 * Δ ∈ [deadzone + ε − θ_stop_base, 30 − θ_start_base]. This function
 * computes the clip window; an EMPTY window (θ_stop_base ≤ deadzone)
 * is a configuration rejection (fail-closed — the caller refuses to
 * arm moisture, zero state change, §27.2 precedent).
 */
/** The feasibility window (ArkTS: classes, not object literals). */
export class P011MoistureDeltaWindowResult {
  lo: number = 0;
  hi: number = 0;
}

export function p011MoistureDeltaWindow(thetaStopBase: number,
  thetaStartBase: number, deadzoneDeg: number): P011MoistureDeltaWindowResult {
  const result = new P011MoistureDeltaWindowResult();
  result.lo = deadzoneDeg + 1e-6 - thetaStopBase;
  result.hi = 30 - thetaStartBase;
  return result;
}

/** Clip Δ into the feasibility window (assumes non-empty; the caller
 * checked). */
export function p011MoistureClipDelta(delta: number, lo: number,
  hi: number): number {
  if (hi < lo) {
    return lo; // defensive: empty window — the caller rejects upstream
  }
  return Math.max(lo, Math.min(hi, delta));
}

/** The reference saturation for an edge (the q=qRef equilibrium at the
 * donor's effective surface depth — used by Δ's anchor term). */
export function p011MoistureRefSaturation(sEdge: number, q: number,
  params: P011MoistureParams): number {
  // The reference model: the same mixed profile evaluated at qRef with
  // μ=0, positioned so that the CURRENT edge's saturation maps to itself
  // when q=qRef — i.e. invert the current s back to the z that produces
  // it under qRef. First-version simplification: use the same z-mapping
  // via the ratio of profiles (monotone families keep this invertible).
  const sBaseRef = params.sBase0 + params.sBaseSlope * params.qRef;
  const betaRef = params.betaSlope * params.qRef;
  const sBaseNow = params.sBase0 + params.sBaseSlope * q;
  const betaNow = params.betaSlope * q;
  // s = sBase + β(1−z) → 1−z = (s − sBase)/β → z shared.
  if (Math.abs(betaNow) < 1e-9 || Math.abs(betaRef) < 1e-9) {
    return sEdge;
  }
  const oneMinusZ = (sEdge - sBaseNow) / betaNow;
  return Math.max(0, Math.min(1, sBaseRef + betaRef * oneMinusZ));
}

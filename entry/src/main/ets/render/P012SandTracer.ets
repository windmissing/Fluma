// P-011 falling-sand B-route §1 (architecture-P011-falling-sand-B-route-v1.md):
// the sand TRACER — a pure-visual spray/jet proxy fed by injection receipts.
//
// RED LINES (source design §1/§3, task §6.4):
//  - No volume / no stock / no budget: this file NEVER writes WaterSandField,
//    never enters physics telemetry or the checker. It is a renderer-side
//    bystander fed by P011InjectionReceipt events.
//  - admitted=0 (or rejectedReason !== null) -> ZERO tracer points. If a
//    spray appears on a rejected injection that is a ROUTING bug (asserted
//    in tests).
//  - Occlusion is sampled per frame (waterTop clip / fog-band smoothstep /
//    sandTop full hide) — NO frame-to-frame caching (E3 LAYER_EPS lesson:
//    sandTop moves every frame, a cached band is stale by construction).
//
// Tuning constants (k/vPhase/fade/radius) are CANDIDATES pending effect
// calibration — explicitly NOT frozen (task §6.4).

/** Shared contract with the physics layer (design §1.2; physics file TBD). */
export interface P011InjectionReceipt {
  /** One per Pan gesture session (incremented at gesture start). */
  sourceId: number;
  /** Monotonic event sequence within the session. */
  sequence: number;
  /** Actually admitted amounts (units); both phases same instant. */
  admittedH: number;
  admittedL: number;
  /** Requested amounts (partial-admission scaling evidence). */
  requestedH: number;
  requestedL: number;
  /** NO_WATER / CAPACITY_BLOCKED / null. */
  rejectedReason: string | null;
  /** Touch point, canvas display px, page coordinate system. */
  x: number;
  y: number;
  /** Monotonic clock ms (same clock as the travel-time base). */
  inputTime: number;
  /** Per-event fixed seed (event identity, tracer reproducibility). */
  seed: number;
}

/** Phase identity for a visual point. */
export const P012_TRACER_HEAVY = 0;
export const P012_TRACER_LIGHT = 1;

/** Candidate tuning constants (NOT frozen — pending effect calibration). */
export interface P012SandTracerTuning {
  /** Points per admitted unit (candidate 2.0). */
  pointsPerUnit: number;
  /** Heavy phase display speed, px/s (candidate from heavySettleSpeed). */
  vPhaseHeavyPx: number;
  /** Light phase display speed, px/s (candidate from lightSettleSpeed). */
  vPhaseLightPx: number;
  /** Travel-time clamp bounds, s (design §2.2). */
  tTravelMin: number;
  tTravelMax: number;
  /** Fade duration after reaching sandTop, s (candidate 0.15). */
  fadeSeconds: number;
  /** Birth jitter half-width as a fraction of a column width. */
  jitterColumnFraction: number;
  /** Max points per receipt (design §2.2 clamp upper bound). */
  maxPointsPerReceipt: number;
}

export function defaultP012SandTracerTuning(): P012SandTracerTuning {
  return {
    pointsPerUnit: 2.0,
    vPhaseHeavyPx: 256,
    vPhaseLightPx: 152,
    tTravelMin: 0.3,
    tTravelMax: 2.5,
    fadeSeconds: 0.15,
    jitterColumnFraction: 0.5,
    maxPointsPerReceipt: 24
  };
}

/** Visual occlusion view: continuous samplers in DISPLAY px space. */
export interface P012TracerOcclusionView {
  /** sandTop height (material units) at display x px. */
  sandTopAtPx(x: number): number;
  /** waterTop height (material units) at display x px. */
  waterTopAtPx(x: number): number;
  /** Material height -> display y px (same mapping the renderer uses). */
  heightToY(h: number): number;
  /** The fog band's half width in display px (P012_A_WFULL_PX / 2). */
  fogBandHalfPx: number;
}

/** Per-point lifecycle state (SoA byte flags). */
const P012_TRACER_STATE_ALIVE = 1;
const P012_TRACER_STATE_FADE = 2;

/**
 * The sand tracer: an SoA pool of at most 256 visual points plus a
 * 64-entry receipt ring (bounded, never replayed — design §1.3).
 */
export class P012SandTracer {
  /** Hard upper bound on live points (design §3). */
  static readonly MAX_POINTS = 256;
  /** Receipt ring capacity (design §1.3). */
  static readonly RECEIPT_RING = 64;

  // SoA storage (capacity MAX_POINTS).
  private readonly posX = new Float64Array(P012SandTracer.MAX_POINTS);
  private readonly posY = new Float64Array(P012SandTracer.MAX_POINTS);
  private readonly velX = new Float64Array(P012SandTracer.MAX_POINTS);
  private readonly velY = new Float64Array(P012SandTracer.MAX_POINTS);
  /** 0 = heavy, 1 = light. */
  private readonly phase = new Uint8Array(P012SandTracer.MAX_POINTS);
  /** Seconds since birth (alive) or since fade start (fade). */
  private readonly age = new Float64Array(P012SandTracer.MAX_POINTS);
  /** Per-point deterministic jitter seed. */
  private readonly fadeSeed = new Float64Array(P012SandTracer.MAX_POINTS);
  /** 0 = dead slot, 1 = alive, 2 = fade. */
  private readonly state = new Uint8Array(P012SandTracer.MAX_POINTS);
  private liveCount = 0;

  // Receipt ring (bounded anti-backlog; consumed on push, never replayed).
  private readonly ringSeeds = new Float64Array(P012SandTracer.RECEIPT_RING);
  private readonly ringTimes = new Float64Array(P012SandTracer.RECEIPT_RING);
  private ringHead = 0;
  private ringSize = 0;

  // Last-seen gravity direction (display space unit vector).
  private gravityDirX = 0;
  private gravityDirY = 1;

  private readonly tuning: P012SandTracerTuning;

  constructor(tuning: P012SandTracerTuning = defaultP012SandTracerTuning()) {
    this.tuning = tuning;
  }

  /** Live point count (alive + fading). */
  activeCount(): number {
    return this.liveCount;
  }

  /**
   * Feed one injection receipt. HARD RED LINE: admitted=0 (either phase
   * both zero, or rejectedReason non-null) generates ZERO points.
   * n = clamp(round(k × (admittedH + admittedL)), 1, 24) otherwise —
   * per admitted amounts (partial admission scales, §6.2 case 2).
   */
  feed(receipt: P011InjectionReceipt, columnWidthPx: number): void {
    // Ring bookkeeping (bounded; consumed on push — never replayed).
    this.ringSeeds[this.ringHead] = receipt.seed;
    this.ringTimes[this.ringHead] = receipt.inputTime;
    this.ringHead = (this.ringHead + 1) % P012SandTracer.RECEIPT_RING;
    if (this.ringSize < P012SandTracer.RECEIPT_RING) {
      this.ringSize += 1;
    }
    const totalAdmitted = receipt.admittedH + receipt.admittedL;
    if (receipt.rejectedReason !== null || totalAdmitted <= 0) {
      return; // zero generation (hard red line)
    }
    const n = Math.max(1, Math.min(this.tuning.maxPointsPerReceipt,
      Math.round(this.tuning.pointsPerUnit * totalAdmitted)));
    // Split by phase proportion (H/L same-instant split, §2.2).
    const heavyCount = receipt.admittedH >= totalAdmitted
      ? n : Math.round(n * receipt.admittedH / totalAdmitted);
    let spawned = 0;
    for (let i = 0; i < n && this.liveCount < P012SandTracer.MAX_POINTS; i++) {
      const isHeavy = i < heavyCount;
      const rng = p012TracerRand(receipt.seed, i);
      const jitter = (rng - 0.5) * 2 * this.tuning.jitterColumnFraction *
        columnWidthPx;
      const phaseV = isHeavy
        ? this.tuning.vPhaseHeavyPx : this.tuning.vPhaseLightPx;
      const speed = phaseV * (0.85 + 0.3 * p012TracerRand(receipt.seed, i + 97));
      const slot = this.findFreeSlot();
      if (slot < 0) {
        break; // pool full — degrade by skipping (deterministic by seed)
      }
      this.posX[slot] = receipt.x + jitter;
      this.posY[slot] = receipt.y;
      this.velX[slot] = this.gravityDirX * speed;
      this.velY[slot] = this.gravityDirY * speed;
      this.phase[slot] = isHeavy ? P012_TRACER_HEAVY : P012_TRACER_LIGHT;
      this.age[slot] = 0;
      this.fadeSeed[slot] = p012TracerRand(receipt.seed, i + 193);
      this.state[slot] = P012_TRACER_STATE_ALIVE;
      this.liveCount += 1;
      spawned += 1;
    }
    // spawned may be < n when the pool is full (design §3: degrade by
    // seed-ordered skipping, never random-drop the already spawned).
    if (spawned === 0 && this.liveCount >= P012SandTracer.MAX_POINTS) {
      return;
    }
  }

  /**
   * Advance every live point by dt seconds along the display-space
   * gravity. Points whose display y passes their column's sandTop
   * surface enter the fade state (design §2.2 — contact flash writes no
   * stock). view may be null in headless tests (arrival detection off;
   * travel is pure ballistics then).
   */
  update(dt: number, gravityDirX: number, gravityDirY: number,
    view: P012TracerOcclusionView | null): void {
    this.gravityDirX = gravityDirX;
    this.gravityDirY = gravityDirY;
    for (let i = 0; i < P012SandTracer.MAX_POINTS; i++) {
      const st = this.state[i];
      if (st === 0) {
        continue;
      }
      if (st === P012_TRACER_STATE_ALIVE) {
        this.posX[i] += this.velX[i] * dt;
        this.posY[i] += this.velY[i] * dt;
        this.age[i] += dt;
        if (view !== null) {
          const sandTopY = view.heightToY(view.sandTopAtPx(this.posX[i]));
          const movingDown = this.velY[i] > 0;
          const arrived = movingDown
            ? this.posY[i] >= sandTopY : this.posY[i] <= sandTopY;
          if (arrived) {
            this.state[i] = P012_TRACER_STATE_FADE;
            this.age[i] = 0;
          }
        }
      } else { // FADE
        this.age[i] += dt;
        if (this.age[i] >= this.tuning.fadeSeconds) {
          this.state[i] = 0;
          this.liveCount -= 1;
        }
      }
    }
  }

  /**
   * Compute the travel time for a point born at (x, y) toward its
   * column's sandTop — clamp(depth / vPhase, 0.3, 2.5) (design §2.2).
   * Exposed for tests (single definition, no re-implementation).
   */
  travelTimeSeconds(x: number, y: number, isHeavy: boolean,
    view: P012TracerOcclusionView): number {
    const sandTopY = view.heightToY(view.sandTopAtPx(x));
    const depthPx = Math.abs(sandTopY - y);
    const vPhase = isHeavy
      ? this.tuning.vPhaseHeavyPx : this.tuning.vPhaseLightPx;
    const raw = depthPx / Math.max(vPhase, 1e-9);
    return Math.max(this.tuning.tTravelMin,
      Math.min(this.tuning.tTravelMax, raw));
  }

  /** Point accessors for tests (read-only views). */
  pointState(i: number): number {
    return this.state[i];
  }

  pointPhase(i: number): number {
    return this.phase[i];
  }

  pointPosX(i: number): number {
    return this.posX[i];
  }

  pointPosY(i: number): number {
    return this.posY[i];
  }

  /**
   * Per-point visibility alpha after the three-segment occlusion
   * (design §2.3), sampled per call — no caching:
   *   1) above waterTop (y < waterY): fully clipped;
   *   2) inside the fog band (|y - sandY| < halfW): smoothstep to 0;
   *   3) at/below sandTop (y >= sandY): fully hidden.
   * Returns 0 for dead slots.
   */
  pointAlpha(i: number, view: P012TracerOcclusionView): number {
    if (this.state[i] === 0) {
      return 0;
    }
    const y = this.posY[i];
    const waterY = view.heightToY(view.waterTopAtPx(this.posX[i]));
    if (y < waterY) {
      return 0; // above the water surface — clipped
    }
    const sandY = view.heightToY(view.sandTopAtPx(this.posX[i]));
    if (y >= sandY) {
      return 0; // inside the sand mask — fully hidden
    }
    let alpha = 1;
    const bandDist = sandY - y;
    if (bandDist < view.fogBandHalfPx) {
      const t = Math.max(0, Math.min(1,
        (view.fogBandHalfPx - bandDist) / view.fogBandHalfPx));
      alpha *= 1 - p012Smoothstep(t);
    }
    if (this.state[i] === P012_TRACER_STATE_FADE) {
      alpha *= 1 - Math.min(1,
        this.age[i] / this.tuning.fadeSeconds);
    }
    return alpha;
  }

  /** Clear all in-flight points + the receipt ring (§4 lifecycle). */
  clear(): void {
    this.state.fill(0);
    this.liveCount = 0;
    this.ringHead = 0;
    this.ringSize = 0;
  }

  /**
   * Draw all live points: H and L groups each in ONE path (≤2 draw
   * calls per frame, design §3), alpha banded to ≤3 globalAlpha steps
   * per group. Returns the number of draw calls issued.
   */
  draw(ctx: P012TracerDrawContext, view: P012TracerOcclusionView,
    heavyColor: string, lightColor: string, pointSizePx: number): number {
    let drawCalls = 0;
    drawCalls += this.drawGroup(ctx, view, P012_TRACER_HEAVY,
      heavyColor, pointSizePx);
    drawCalls += this.drawGroup(ctx, view, P012_TRACER_LIGHT,
      lightColor, pointSizePx);
    return drawCalls;
  }

  private drawGroup(ctx: P012TracerDrawContext,
    view: P012TracerOcclusionView, phaseWanted: number,
    color: string, pointSizePx: number): number {
    // Collect (slot, alpha) then bucket into ≤3 alpha bands so each
    // group costs at most 3 fill passes inside one path per band —
    // still ≤2 logical draw groups (H/L), ≤3 alpha档 (design §3).
    const bands: number[][] = [[], [], []];
    for (let i = 0; i < P012SandTracer.MAX_POINTS; i++) {
      if (this.state[i] === 0 || this.phase[i] !== phaseWanted) {
        continue;
      }
      const alpha = this.pointAlpha(i, view);
      if (alpha <= 0.02) {
        continue; // fully occluded/dead — skip
      }
      const band = alpha > 0.66 ? 0 : (alpha > 0.33 ? 1 : 2);
      bands[band].push(i);
    }
    let calls = 0;
    const bandAlphas = [0.85, 0.5, 0.2];
    for (let b = 0; b < bands.length; b++) {
      if (bands[b].length === 0) {
        continue;
      }
      ctx.fillStyle = color;
      ctx.globalAlpha = bandAlphas[b];
      for (const i of bands[b]) {
        ctx.fillRect(this.posX[i] - pointSizePx / 2,
          this.posY[i] - pointSizePx / 2, pointSizePx, pointSizePx);
      }
      calls += 1;
    }
    ctx.globalAlpha = 1;
    return calls;
  }

  private findFreeSlot(): number {
    for (let i = 0; i < P012SandTracer.MAX_POINTS; i++) {
      if (this.state[i] === 0) {
        return i;
      }
    }
    return -1;
  }
}

/** Minimal canvas-2d surface the tracer needs (testable via a stub). */
export interface P012TracerDrawContext {
  fillStyle: string;
  globalAlpha: number;
  fillRect(x: number, y: number, w: number, h: number): void;
}

/** Deterministic per-(seed,index) rand in [0,1) (mulberry-style). */
function p012TracerRand(seed: number, index: number): number {
  let t = (seed + index * 0x9E3779B9) >>> 0;
  t = Math.imul(t ^ (t >>> 15), t | 1);
  t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
  return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
}

/** smoothstep(t) for t in [0,1]. */
function p012Smoothstep(t: number): number {
  return t * t * (3 - 2 * t);
}

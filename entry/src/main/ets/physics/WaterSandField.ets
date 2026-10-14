/**
 * WaterSandField - P-003/P-005 scene 2 three-phase column model, algorithm v2.
 * Canonical ledger: waterTop/bh/bl/mh/ml/suspH/suspL; hs/hl/sandTop are derived.
 */

export enum WaterSandPhase {
  WATER = 0,
  HEAVY = 1,
  LIGHT = 2
}

export enum WaterSandInjectionState {
  DEPOSITED = 0,
  SUSPENDED = 1
}

export enum WaterSandErrorCode {
  NEGATIVE_VALUE = 1,
  NON_FINITE_VALUE = 2,
  CAPACITY_OVERFLOW = 3,
  WATER_SOLVE_FAILED = 4,
  UNKNOWN = 99
}

export class WaterSandParamError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'WaterSandParamError';
  }
}

export class WaterSandSnapshotError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'WaterSandSnapshotError';
  }
}

export interface WaterSandParams {
  angleDeg: number;
  heavySettleSpeed: number;
  lightSettleSpeed: number;
  percolationRate: number;
  waterLevelRate: number;
  heavyDiffusion: number;
  lightDiffusion: number;
  shakeDecay: number;
  heavyLiftRate: number;
  lightLiftRate: number;
  maxAvalancheFlux: number;
  avalanchePasses: number;
  maxTiltOmegaDegPerSec: number;
}

export interface WaterSandRegion {
  startCol: number;
  endColInclusive: number;
  bottom: number;
  height: number;
}

export interface WaterSandScene {
  waterRegions: WaterSandRegion[];
  heavyDepositedRegions: WaterSandRegion[];
  lightDepositedRegions: WaterSandRegion[];
  heavySuspendedRegions: WaterSandRegion[];
  lightSuspendedRegions: WaterSandRegion[];
}

export interface WaterSandInjection {
  phase: WaterSandPhase;
  amount: number;
  startCol: number;
  endColInclusive: number;
  state: WaterSandInjectionState;
}

export interface WaterSandSplitResult {
  heavy: number;
  light: number;
}

export interface WaterSandTotals {
  water: number;
  heavy: number;
  light: number;
}

export interface WaterSandDrift {
  water: number;
  heavy: number;
  light: number;
}

export interface WaterSandSeparationProfile {
  lowerHeavyShare: number | null;
  upperHeavyShare: number | null;
  separationRatio: number | null;
}

export interface WaterSandMetrics {
  ledgerVersion: 2;
  driftWater: number;
  driftHeavy: number;
  driftLight: number;
  depositedHeavyFraction: number;
  depositedLightFraction: number;
  suspensionFraction: number;
  lowerHeavyShare: number | null;
  upperHeavyShare: number | null;
  separationRatio: number | null;
  t90Heavy: number;
  t90Light: number;
  maxSlopeDeg: number;
  maxCurvature: number;
  waterSlopeDeg: number;
  waterAmplitude: number;
  waterAmplitudePx: number;
  phaseOrderScore: number;
  maxSandTopDeltaPerSecond: number;
  physicsTimeMs: number;
  dormant: boolean;
}

export interface WaterSandStepResult {
  ok: boolean;
  stepped: boolean;
  substeps: number;
  moved: boolean;
  dormant: boolean;
  physicsTimeMs: number;
  errorCode: WaterSandErrorCode | null;
  errorColumn: number;
}

export interface WaterSandSnapshot {
  schemaVersion: 2;
  cols: number;
  capacity: number;
  waterTop: Float64Array;
  bh: Float64Array;
  bl: Float64Array;
  mh: Float64Array;
  ml: Float64Array;
  suspH: Float64Array;
  suspL: Float64Array;
  hs: Float64Array;
  hl: Float64Array;
  sandTop: Float64Array;
}

const EPS = 1e-9;
const DRIFT_LIMIT = 0.001;

export function defaultWaterSandParams(): WaterSandParams {
  return {
    angleDeg: 30,
    heavySettleSpeed: 16,
    lightSettleSpeed: 6,
    percolationRate: 6,
    waterLevelRate: 8,
    heavyDiffusion: 4,
    lightDiffusion: 10,
    shakeDecay: 2.2,
    heavyLiftRate: 0.35,
    lightLiftRate: 0.55,
    maxAvalancheFlux: 24,
    avalanchePasses: 5,
    maxTiltOmegaDegPerSec: 180
  };
}

function finite(value: number): boolean {
  return Number.isFinite(value);
}

export class WaterSandField {
  readonly cols: number;
  readonly capacity: number;

  private params: WaterSandParams = defaultWaterSandParams();
  private waterTop: Float64Array;
  private bh: Float64Array;
  private bl: Float64Array;
  private mh: Float64Array;
  private ml: Float64Array;
  private suspH: Float64Array;
  private suspL: Float64Array;
  private sandTopScratch: Float64Array;
  private targetTopScratch: Float64Array;
  private errorScratch: Float64Array;
  private transferScratch: Float64Array;
  private smoothScratch: Float64Array;
  private backup: WaterSandSnapshot;
  private initialWater = 0;
  private initialHeavy = 0;
  private initialLight = 0;
  private targetTilt = 0;
  private effectiveTilt = 0;
  private shakeInput = 0;
  private shakeEnergy = 0;
  private simulationTime = 0;
  private t90Heavy = -1;
  private t90Light = -1;
  private maxSandTopDeltaPerSecond = 0;
  private lastPhysicsTimeMs = 0;
  private lastError: WaterSandErrorCode | null = null;
  private frozen = false;
  private dormant = false;

  constructor(cols: number, capacity: number) {
    if (!Number.isInteger(cols) || !Number.isInteger(capacity) || cols < 2 || capacity < 2) {
      throw new WaterSandParamError('cols and capacity must be integers >= 2');
    }
    this.cols = cols;
    this.capacity = capacity;
    this.waterTop = new Float64Array(cols);
    this.bh = new Float64Array(cols);
    this.bl = new Float64Array(cols);
    this.mh = new Float64Array(cols);
    this.ml = new Float64Array(cols);
    this.suspH = new Float64Array(cols);
    this.suspL = new Float64Array(cols);
    this.sandTopScratch = new Float64Array(cols);
    this.targetTopScratch = new Float64Array(cols);
    this.errorScratch = new Float64Array(cols);
    this.transferScratch = new Float64Array(cols);
    this.smoothScratch = new Float64Array(cols);
    this.backup = this.createEmptySnapshot();
    this.initialize(this.defaultScene());
  }

  configure(params: WaterSandParams): void {
    if (params.angleDeg <= 0 || params.angleDeg >= 45 ||
      params.lightSettleSpeed <= 0 || params.heavySettleSpeed <= params.lightSettleSpeed ||
      params.percolationRate <= 0 || params.waterLevelRate <= 0 ||
      params.heavyDiffusion < 0 || params.lightDiffusion < 0 || params.shakeDecay <= 0 ||
      params.heavyLiftRate < 0 || params.lightLiftRate < 0 ||
      params.maxAvalancheFlux <= 0 || params.avalanchePasses < 4 || params.avalanchePasses > 6 ||
      params.maxTiltOmegaDegPerSec <= 0) {
      throw new WaterSandParamError('invalid WaterSandParams');
    }
    const values: number[] = [params.angleDeg, params.heavySettleSpeed, params.lightSettleSpeed,
      params.percolationRate, params.waterLevelRate, params.heavyDiffusion, params.lightDiffusion,
      params.shakeDecay, params.heavyLiftRate, params.lightLiftRate, params.maxAvalancheFlux,
      params.avalanchePasses, params.maxTiltOmegaDegPerSec];
    for (const value of values) {
      if (!finite(value)) {
        throw new WaterSandParamError('non-finite WaterSandParams value');
      }
    }
    this.params = {
      angleDeg: params.angleDeg,
      heavySettleSpeed: params.heavySettleSpeed,
      lightSettleSpeed: params.lightSettleSpeed,
      percolationRate: params.percolationRate,
      waterLevelRate: params.waterLevelRate,
      heavyDiffusion: params.heavyDiffusion,
      lightDiffusion: params.lightDiffusion,
      shakeDecay: params.shakeDecay,
      heavyLiftRate: params.heavyLiftRate,
      lightLiftRate: params.lightLiftRate,
      maxAvalancheFlux: params.maxAvalancheFlux,
      avalanchePasses: params.avalanchePasses,
      maxTiltOmegaDegPerSec: params.maxTiltOmegaDegPerSec
    };
  }

  initialize(scene: WaterSandScene): void {
    this.validateScene(scene);
    this.waterTop.fill(0);
    this.bh.fill(0);
    this.bl.fill(0);
    this.mh.fill(0);
    this.ml.fill(0);
    this.suspH.fill(0);
    this.suspL.fill(0);
    this.fillRegions(this.waterTop, scene.waterRegions, false);
    this.fillRegions(this.bh, scene.heavyDepositedRegions, true);
    this.fillRegions(this.bl, scene.lightDepositedRegions, true);
    this.fillRegions(this.suspH, scene.heavySuspendedRegions, true);
    this.fillRegions(this.suspL, scene.lightSuspendedRegions, true);
    this.updateSandTop();
    const totals = this.getTotals();
    if (this.audit(false) !== null) {
      throw new WaterSandParamError('scene exceeds capacity');
    }
    this.initialWater = totals.water;
    this.initialHeavy = totals.heavy;
    this.initialLight = totals.light;
    this.simulationTime = 0;
    this.t90Heavy = -1;
    this.t90Light = -1;
    this.shakeEnergy = 0;
    this.shakeInput = 0;
    this.maxSandTopDeltaPerSecond = 0;
    this.frozen = false;
    this.dormant = false;
    this.lastError = null;
  }

  reset(scene?: WaterSandScene): void {
    this.initialize(scene === undefined ? this.defaultScene() : scene);
  }

  setTilt(deg: number): void {
    if (!finite(deg)) {
      throw new WaterSandParamError('tilt must be finite');
    }
    this.targetTilt = Math.max(-89, Math.min(89, deg));
    this.dormant = false;
  }

  setShakeInput(value: number): void {
    if (!finite(value)) {
      throw new WaterSandParamError('shake input must be finite');
    }
    this.shakeInput = Math.max(0, Math.min(1, value));
    this.dormant = false;
  }

  inject(spec: WaterSandInjection): void {
    if (!finite(spec.amount) || spec.amount < 0) {
      throw new WaterSandParamError('injection amount must be finite and non-negative');
    }
    if (spec.startCol < 0 || spec.endColInclusive >= this.cols || spec.startCol > spec.endColInclusive) {
      throw new WaterSandParamError('injection columns are out of range');
    }
    const columns = spec.endColInclusive - spec.startCol + 1;
    const amountPerColumn = spec.amount / columns;
    this.saveBackup();
    if (spec.phase === WaterSandPhase.WATER) {
      for (let c = spec.startCol; c <= spec.endColInclusive; c++) {
        this.waterTop[c] += amountPerColumn;
      }
    } else {
      const isHeavy = spec.phase === WaterSandPhase.HEAVY;
      const suspended = spec.state === WaterSandInjectionState.SUSPENDED;
      for (let c = spec.startCol; c <= spec.endColInclusive; c++) {
        if (isHeavy) {
          if (suspended) {
            this.suspH[c] += amountPerColumn;
          } else {
            this.mh[c] += amountPerColumn;
          }
        } else {
          if (suspended) {
            this.suspL[c] += amountPerColumn;
          } else {
            this.ml[c] += amountPerColumn;
          }
        }
      }
    }
    this.updateSandTop();
    if (this.audit(false) !== null) {
      this.restoreBackup();
      throw new WaterSandParamError('injection exceeds capacity');
    }
    const totals = this.getTotals();
    this.initialWater = totals.water;
    this.initialHeavy = totals.heavy;
    this.initialLight = totals.light;
  }

  invert180(): void {
    if (this.frozen) {
      return;
    }
    const newWaterTop = new Float64Array(this.cols);
    const newSuspH = new Float64Array(this.cols);
    const newSuspL = new Float64Array(this.cols);
    for (let c = 0; c < this.cols; c++) {
      const mirrored = this.cols - 1 - c;
      const waterVolume = this.getWaterVolume(c);
      newSuspH[mirrored] = this.bh[c] + this.mh[c] + this.suspH[c];
      newSuspL[mirrored] = this.bl[c] + this.ml[c] + this.suspL[c];
      newWaterTop[mirrored] = waterVolume + newSuspH[mirrored] + newSuspL[mirrored];
    }
    this.waterTop.set(newWaterTop);
    this.bh.fill(0);
    this.bl.fill(0);
    this.mh.fill(0);
    this.ml.fill(0);
    this.suspH.set(newSuspH);
    this.suspL.set(newSuspL);
    this.updateSandTop();
    this.shakeEnergy = Math.max(this.shakeEnergy, 0.8);
  }

  step(dt: number): WaterSandStepResult {
    const start = Date.now();
    const result: WaterSandStepResult = {
      ok: true,
      stepped: false,
      substeps: 0,
      moved: false,
      dormant: false,
      physicsTimeMs: 0,
      errorCode: null,
      errorColumn: -1
    };
    if (this.frozen || !finite(dt) || dt <= 0) {
      result.physicsTimeMs = Date.now() - start;
      return result;
    }
    if (this.dormant && Math.abs(this.targetTilt - this.effectiveTilt) <= EPS &&
      this.shakeEnergy <= EPS && this.shakeInput <= EPS) {
      result.dormant = true;
      result.physicsTimeMs = Date.now() - start;
      this.lastPhysicsTimeMs = result.physicsTimeMs;
      return result;
    }
    const clipped = Math.min(dt, 1 / 30);
    const substeps = clipped <= 1 / 60 ? 1 : 2;
    const actualDt = substeps === 1 ? clipped : clipped / 2;
    for (let index = 0; index < substeps; index++) {
      this.saveBackup();
      this.updateSandTop();
      this.transferScratch.set(this.sandTopScratch);
      const errorCode = this.substep(actualDt);
      if (errorCode !== null) {
        this.restoreBackup();
        this.frozen = true;
        this.lastError = errorCode;
        result.ok = false;
        result.errorCode = errorCode;
        result.errorColumn = this.firstInvalidColumn();
        break;
      }
      result.stepped = true;
      result.substeps++;
      result.moved = this.changedSinceBackup() || result.moved;
    }
    this.updateMetrics(actualDt);
    result.dormant = this.dormant;
    result.physicsTimeMs = Date.now() - start;
    this.lastPhysicsTimeMs = result.physicsTimeMs;
    return result;
  }

  getTotals(): WaterSandTotals {
    let water = 0;
    let heavy = 0;
    let light = 0;
    for (let c = 0; c < this.cols; c++) {
      water += this.getWaterVolume(c);
      heavy += this.bh[c] + this.mh[c] + this.suspH[c];
      light += this.bl[c] + this.ml[c] + this.suspL[c];
    }
    return { water, heavy, light };
  }

  getDrift(): WaterSandDrift {
    const totals = this.getTotals();
    return {
      water: Math.abs(totals.water - this.initialWater) / Math.max(this.initialWater, EPS),
      heavy: Math.abs(totals.heavy - this.initialHeavy) / Math.max(this.initialHeavy, EPS),
      light: Math.abs(totals.light - this.initialLight) / Math.max(this.initialLight, EPS)
    };
  }

  getMetrics(pixelScale?: number): WaterSandMetrics {
    const scale = pixelScale === undefined ? 1 : pixelScale;
    const totals = this.getTotals();
    const drift = this.getDrift();
    const profile = this.computeSeparationProfile();
    let suspended = 0;
    let depositedHeavy = 0;
    let depositedLight = 0;
    for (let c = 0; c < this.cols; c++) {
      suspended += this.suspH[c] + this.suspL[c];
      depositedHeavy += this.bh[c] + this.mh[c];
      depositedLight += this.bl[c] + this.ml[c];
    }
    const totalSand = Math.max(totals.heavy + totals.light, EPS);
    const phaseOrderScore = profile.lowerHeavyShare === null || profile.upperHeavyShare === null
      ? 0 : profile.lowerHeavyShare - profile.upperHeavyShare;
    return {
      ledgerVersion: 2,
      driftWater: drift.water,
      driftHeavy: drift.heavy,
      driftLight: drift.light,
      depositedHeavyFraction: depositedHeavy / Math.max(this.initialHeavy, EPS),
      depositedLightFraction: depositedLight / Math.max(this.initialLight, EPS),
      suspensionFraction: suspended / totalSand,
      lowerHeavyShare: profile.lowerHeavyShare,
      upperHeavyShare: profile.upperHeavyShare,
      separationRatio: profile.separationRatio,
      t90Heavy: this.t90Heavy,
      t90Light: this.t90Light,
      maxSlopeDeg: this.maxSlopeDeg(),
      maxCurvature: this.maxCurvature(),
      waterSlopeDeg: this.waterSlopeDeg(),
      waterAmplitude: this.waterAmplitude(),
      waterAmplitudePx: this.waterAmplitude() * scale,
      phaseOrderScore,
      maxSandTopDeltaPerSecond: this.maxSandTopDeltaPerSecond,
      physicsTimeMs: this.lastPhysicsTimeMs,
      dormant: this.dormant
    };
  }

  snapshot(): WaterSandSnapshot {
    const target = this.createEmptySnapshot();
    this.copyStateTo(target);
    return target;
  }

  copyStateTo(target: WaterSandSnapshot): void {
    if (target.schemaVersion !== 2 || target.cols !== this.cols || target.capacity !== this.capacity) {
      throw new WaterSandSnapshotError('snapshot schema, cols, or capacity mismatch');
    }
    if (target.waterTop.length !== this.cols || target.bh.length !== this.cols ||
      target.bl.length !== this.cols || target.mh.length !== this.cols ||
      target.ml.length !== this.cols || target.suspH.length !== this.cols ||
      target.suspL.length !== this.cols || target.hs.length !== this.cols ||
      target.hl.length !== this.cols || target.sandTop.length !== this.cols) {
      throw new WaterSandSnapshotError('snapshot array length mismatch');
    }
    this.updateSandTop();
    target.waterTop.set(this.waterTop);
    target.bh.set(this.bh);
    target.bl.set(this.bl);
    target.mh.set(this.mh);
    target.ml.set(this.ml);
    target.suspH.set(this.suspH);
    target.suspL.set(this.suspL);
    for (let c = 0; c < this.cols; c++) {
      target.hs[c] = this.bh[c] + this.mh[c];
      target.hl[c] = this.bl[c] + this.ml[c];
      target.sandTop[c] = target.hs[c] + target.hl[c];
    }
  }

  getLastError(): WaterSandErrorCode | null {
    return this.lastError;
  }

  private substep(dt: number): WaterSandErrorCode | null {
    this.updateTilt(dt);
    this.updateSandTop();
    this.waterLevel(dt);
    this.diffuseSuspension(dt);
    this.settleSandV2(dt);
    this.percolateHeavyConservative(dt);
    this.relaxAngle(dt);
    this.liftByShake(dt);
    this.updateSandTop();
    return this.audit(true);
  }

  private updateTilt(dt: number): void {
    const maxStep = this.params.maxTiltOmegaDegPerSec * dt;
    const difference = Math.max(-maxStep, Math.min(maxStep, this.targetTilt - this.effectiveTilt));
    this.effectiveTilt += difference;
  }

  private waterLevel(dt: number): void {
    const slope = Math.tan(this.effectiveTilt * Math.PI / 180);
    if (Math.abs(this.effectiveTilt) < EPS) {
      let minimum = Number.POSITIVE_INFINITY;
      let maximum = Number.NEGATIVE_INFINITY;
      for (let c = 0; c < this.cols; c++) {
        minimum = Math.min(minimum, this.waterTop[c]);
        maximum = Math.max(maximum, this.waterTop[c]);
      }
      if (maximum - minimum <= EPS) {
        return;
      }
    }
    const totals = this.getTotals();
    let suspended = 0;
    for (let c = 0; c < this.cols; c++) {
      suspended += this.suspH[c] + this.suspL[c];
    }
    const sum = totals.water + suspended;
    let low = -this.capacity;
    let high = 2 * this.capacity;
    for (let iteration = 0; iteration < 40; iteration++) {
      const base = (low + high) / 2;
      let volume = 0;
      for (let c = 0; c < this.cols; c++) {
        const target = Math.max(this.sandTopScratch[c],
          Math.min(this.capacity, base + slope * (c - (this.cols - 1) / 2)));
        volume += target - this.sandTopScratch[c];
      }
      if (volume < sum) {
        low = base;
      } else {
        high = base;
      }
    }
    const base = (low + high) / 2;
    let positiveError = 0;
    for (let c = 0; c < this.cols; c++) {
      const target = Math.max(this.sandTopScratch[c],
        Math.min(this.capacity, base + slope * (c - (this.cols - 1) / 2)));
      this.targetTopScratch[c] = target;
      this.errorScratch[c] = this.waterTop[c] - target;
      if (this.errorScratch[c] > 0) {
        positiveError += this.errorScratch[c];
      }
    }
    let budget = Math.min(positiveError, positiveError * Math.min(1, this.params.waterLevelRate * dt) + EPS);
    for (let donor = 0; donor < this.cols && budget > EPS; donor++) {
      if (this.errorScratch[donor] <= EPS) {
        continue;
      }
      for (let receiver = 0; receiver < this.cols && budget > EPS; receiver++) {
        if (this.errorScratch[receiver] >= -EPS) {
          continue;
        }
        const amount = Math.min(budget, this.errorScratch[donor], -this.errorScratch[receiver],
          this.waterTop[donor] - this.sandTopScratch[donor] - this.suspH[donor] - this.suspL[donor],
          this.capacity - this.waterTop[receiver]);
        if (amount <= EPS) {
          continue;
        }
        this.waterTop[donor] -= amount;
        this.waterTop[receiver] += amount;
        this.errorScratch[donor] -= amount;
        this.errorScratch[receiver] += amount;
        budget -= amount;
      }
    }
  }

  private diffuseSuspension(dt: number): void {
    let hasSuspension = false;
    for (let c = 0; c < this.cols; c++) {
      if (this.suspH[c] > EPS || this.suspL[c] > EPS) {
        hasSuspension = true;
        break;
      }
    }
    if (!hasSuspension) {
      return;
    }
    for (let c = 0; c < this.cols - 1; c++) {
      const heavyDifference = this.suspH[c + 1] - this.suspH[c];
      const lightDifference = this.suspL[c + 1] - this.suspL[c];
      const heavyFlux = Math.min(this.params.heavyDiffusion * dt, Math.abs(heavyDifference) / 2);
      const lightFlux = Math.min(this.params.lightDiffusion * dt, Math.abs(lightDifference) / 2);
      if (heavyDifference > 0) {
        this.suspH[c + 1] -= heavyFlux;
        this.suspH[c] += heavyFlux;
      } else if (heavyDifference < 0) {
        this.suspH[c] -= heavyFlux;
        this.suspH[c + 1] += heavyFlux;
      }
      if (lightDifference > 0) {
        this.suspL[c + 1] -= lightFlux;
        this.suspL[c] += lightFlux;
      } else if (lightDifference < 0) {
        this.suspL[c] -= lightFlux;
        this.suspL[c + 1] += lightFlux;
      }
    }
  }

  private settleSandV2(dt: number): void {
    for (let c = 0; c < this.cols; c++) {
      if (this.suspH[c] <= EPS && this.suspL[c] <= EPS) {
        continue;
      }
      const depth = Math.max(this.waterTop[c] - this.sandTopScratch[c], EPS);
      const heavy = this.suspH[c] * Math.min(1, this.params.heavySettleSpeed * dt / depth);
      const light = this.suspL[c] * Math.min(1, this.params.lightSettleSpeed * dt / depth);
      this.suspH[c] -= heavy;
      this.mh[c] += heavy;
      this.suspL[c] -= light;
      this.ml[c] += light;
    }
    this.updateSandTop();
  }

  private percolateHeavyConservative(dt: number): void {
    for (let c = 0; c < this.cols; c++) {
      if (this.mh[c] <= EPS) {
        continue;
      }
      const through = Math.min(this.mh[c], this.bl[c]);
      const pair = Math.min(this.mh[c], this.ml[c]);
      if (through + pair <= EPS) {
        continue;
      }
      const budget = this.params.percolationRate * dt * (through + pair);
      const q = Math.min(through, 0.5 * through, budget);
      const r = Math.min(pair, 0.5 * pair, Math.max(0, budget - q), this.mh[c] - q);
      this.mh[c] -= q + r;
      this.bh[c] += q + r;
      this.bl[c] += r - q;
      this.ml[c] += q - r;
    }
    this.updateSandTop();
  }

  private relaxAngle(dt: number): void {
    const limit = Math.tan(this.params.angleDeg * Math.PI / 180);
    let maximumDifference = 0;
    for (let c = 0; c < this.cols - 1; c++) {
      maximumDifference = Math.max(maximumDifference,
        Math.abs(this.sandTopScratch[c + 1] - this.sandTopScratch[c]));
    }
    if (maximumDifference <= limit) {
      return;
    }
    const maxFlux = this.params.maxAvalancheFlux * dt;
    for (let pass = 0; pass < this.params.avalanchePasses; pass++) {
      const forward = pass % 2 === 0;
      for (let index = 0; index < this.cols - 1; index++) {
        const c = forward ? index : this.cols - 2 - index;
        const next = c + 1;
        const difference = this.sandTopScratch[next] - this.sandTopScratch[c];
        if (Math.abs(difference) <= limit) {
          continue;
        }
        const high = difference > 0 ? c : next;
        const low = difference > 0 ? next : c;
        const excess = Math.abs(difference) - limit;
        const amount = Math.min(excess / 2, maxFlux,
          this.sandTopScratch[high], this.capacity - this.sandTopScratch[low]);
        if (amount <= EPS) {
          continue;
        }
        const split = this.splitTopV2(high, amount);
        this.mh[low] += split.heavy;
        this.ml[low] += split.light;
        this.updateSandTop();
      }
    }
  }

  private liftByShake(dt: number): void {
    this.shakeEnergy = Math.max(0, Math.min(1,
      this.shakeEnergy * Math.exp(-this.params.shakeDecay * dt) + this.shakeInput * dt));
    if (this.shakeEnergy <= EPS) {
      this.shakeInput = 0;
      return;
    }
    const bandBase = Math.max(0.5, Math.tan(this.params.angleDeg * Math.PI / 180));
    for (let c = 0; c < this.cols; c++) {
      const band = Math.min(this.sandTopScratch[c], bandBase);
      if (band <= EPS) {
        continue;
      }
      const surfaceHeavyShare = this.mh[c] / Math.max(this.mh[c] + this.ml[c], EPS);
      const amount = this.shakeEnergy * band * dt *
        (this.params.heavyLiftRate * surfaceHeavyShare +
          this.params.lightLiftRate * (1 - surfaceHeavyShare));
      const capacity = this.waterTop[c] - this.sandTopScratch[c] + amount -
        this.suspH[c] - this.suspL[c];
      const clipped = Math.max(0, Math.min(amount, capacity));
      if (clipped <= EPS) {
        continue;
      }
      const split = this.splitTopV2(c, clipped);
      this.suspH[c] += split.heavy;
      this.suspL[c] += split.light;
      this.updateSandTop();
    }
    this.shakeInput = 0;
  }

  private splitTopV2(column: number, amount: number): WaterSandSplitResult {
    const fromMix = Math.min(amount, this.mh[column] + this.ml[column]);
    const mixTotal = Math.max(this.mh[column] + this.ml[column], EPS);
    const mixScale = fromMix / mixTotal;
    let heavy = this.mh[column] * mixScale;
    let light = this.ml[column] * mixScale;
    let remaining = amount - fromMix;
    const fromLight = Math.min(remaining, this.bl[column]);
    light += fromLight;
    remaining -= fromLight;
    const fromHeavy = Math.min(remaining, this.bh[column]);
    heavy += fromHeavy;
    this.mh[column] -= this.mh[column] * mixScale;
    this.ml[column] -= this.ml[column] * mixScale;
    this.bl[column] -= fromLight;
    this.bh[column] -= fromHeavy;
    this.updateSandTop();
    return { heavy, light };
  }

  private computeSeparationProfile(): WaterSandSeparationProfile {
    this.updateSandTop();
    let lowHeavy = 0;
    let lowLight = 0;
    let highHeavy = 0;
    let highLight = 0;
    let hasHeavy = false;
    let hasLight = false;
    for (let c = 0; c < this.cols; c++) {
      hasHeavy = hasHeavy || this.bh[c] + this.mh[c] > EPS;
      hasLight = hasLight || this.bl[c] + this.ml[c] > EPS;
      const lowerBound = this.sandTopScratch[c] / 3;
      const upperBound = 2 * this.sandTopScratch[c] / 3;
      const mixStart = this.bh[c] + this.bl[c];
      const mixTotal = this.mh[c] + this.ml[c];
      const lowerMix = this.mixedOverlap(0, lowerBound, mixStart, this.sandTopScratch[c]);
      const upperMix = this.mixedOverlap(upperBound, this.sandTopScratch[c], mixStart, this.sandTopScratch[c]);
      lowHeavy += Math.min(this.bh[c], lowerBound) +
        lowerMix * this.mh[c] / Math.max(mixTotal, EPS);
      lowLight += Math.max(0, Math.min(this.bh[c] + this.bl[c], lowerBound) - this.bh[c]) +
        lowerMix * this.ml[c] / Math.max(mixTotal, EPS);
      highHeavy += Math.max(0, Math.min(this.bh[c], this.sandTopScratch[c]) - upperBound) +
        upperMix * this.mh[c] / Math.max(mixTotal, EPS);
      highLight += Math.max(0, Math.min(this.bh[c] + this.bl[c], this.sandTopScratch[c]) -
        Math.max(this.bh[c], upperBound)) +
        upperMix * this.ml[c] / Math.max(mixTotal, EPS);
    }
    if (!hasHeavy || !hasLight || lowHeavy + lowLight <= EPS || highHeavy + highLight <= EPS) {
      return { lowerHeavyShare: null, upperHeavyShare: null, separationRatio: null };
    }
    const lowerHeavyShare = lowHeavy / (lowHeavy + lowLight);
    const upperHeavyShare = highHeavy / (highHeavy + highLight);
    return {
      lowerHeavyShare,
      upperHeavyShare,
      separationRatio: upperHeavyShare / Math.max(lowerHeavyShare, EPS)
    };
  }

  private mixedOverlap(start: number, end: number, mixStart: number, sandTop: number): number {
    return Math.max(0, Math.min(end, sandTop) - Math.max(start, mixStart));
  }

  private updateMetrics(dt: number): void {
    let maxDelta = 0;
    for (let c = 0; c < this.cols; c++) {
      const delta = Math.abs(this.sandTopScratch[c] - this.transferScratch[c]);
      if (delta > maxDelta) {
        maxDelta = delta;
      }
    }
    this.maxSandTopDeltaPerSecond = Math.max(this.maxSandTopDeltaPerSecond,
      maxDelta / Math.max(dt, EPS));
    this.simulationTime += dt;
    let depositedHeavy = 0;
    let depositedLight = 0;
    let suspended = 0;
    for (let c = 0; c < this.cols; c++) {
      depositedHeavy += this.bh[c] + this.mh[c];
      depositedLight += this.bl[c] + this.ml[c];
      suspended += this.suspH[c] + this.suspL[c];
    }
    if (this.t90Heavy < 0 && depositedHeavy / Math.max(this.initialHeavy, EPS) >= 0.9) {
      this.t90Heavy = this.simulationTime;
    }
    if (this.t90Light < 0 && depositedLight / Math.max(this.initialLight, EPS) >= 0.9) {
      this.t90Light = this.simulationTime;
    }
    const suspensionFraction = suspended / Math.max(this.initialHeavy + this.initialLight, EPS);
    this.dormant = suspensionFraction <= 0.001 && this.shakeEnergy <= EPS &&
      this.maxSlopeDeg() <= this.params.angleDeg + 0.01 && this.waterAmplitude() <= 0.01;
  }

  private audit(checkDrift: boolean): WaterSandErrorCode | null {
    for (let c = 0; c < this.cols; c++) {
      if (!finite(this.waterTop[c]) || !finite(this.bh[c]) || !finite(this.bl[c]) ||
        !finite(this.mh[c]) || !finite(this.ml[c]) || !finite(this.suspH[c]) ||
        !finite(this.suspL[c])) {
        this.lastError = WaterSandErrorCode.NON_FINITE_VALUE;
        return WaterSandErrorCode.NON_FINITE_VALUE;
      }
      if (this.waterTop[c] < -EPS || this.bh[c] < -EPS || this.bl[c] < -EPS ||
        this.mh[c] < -EPS || this.ml[c] < -EPS || this.suspH[c] < -EPS ||
        this.suspL[c] < -EPS) {
        this.lastError = WaterSandErrorCode.NEGATIVE_VALUE;
        return WaterSandErrorCode.NEGATIVE_VALUE;
      }
      const total = this.bh[c] + this.bl[c] + this.mh[c] + this.ml[c] +
        this.suspH[c] + this.suspL[c] + this.getWaterVolume(c);
      if (total > this.capacity + 1e-7 || this.suspH[c] + this.suspL[c] >
        this.waterTop[c] - this.sandTopScratch[c] + EPS) {
        this.lastError = WaterSandErrorCode.CAPACITY_OVERFLOW;
        return WaterSandErrorCode.CAPACITY_OVERFLOW;
      }
    }
    if (checkDrift) {
      const totals = this.getTotals();
      const driftWater = Math.abs(totals.water - this.initialWater) /
        Math.max(this.initialWater, EPS);
      const driftHeavy = Math.abs(totals.heavy - this.initialHeavy) /
        Math.max(this.initialHeavy, EPS);
      const driftLight = Math.abs(totals.light - this.initialLight) /
        Math.max(this.initialLight, EPS);
      if (driftWater >= DRIFT_LIMIT || driftHeavy >= DRIFT_LIMIT || driftLight >= DRIFT_LIMIT) {
        this.lastError = WaterSandErrorCode.UNKNOWN;
        return WaterSandErrorCode.UNKNOWN;
      }
    }
    return null;
  }

  private getWaterVolume(c: number): number {
    return Math.max(0, this.waterTop[c] - this.sandTopScratch[c] - this.suspH[c] - this.suspL[c]);
  }

  private maxSlopeDeg(): number {
    let maximum = 0;
    for (let c = 0; c < this.cols - 1; c++) {
      maximum = Math.max(maximum, Math.abs(this.sandTopScratch[c + 1] - this.sandTopScratch[c]));
    }
    return Math.atan(maximum) * 180 / Math.PI;
  }

  private maxCurvature(): number {
    let maximum = 0;
    for (let c = 1; c < this.cols - 1; c++) {
      maximum = Math.max(maximum, Math.abs(
        this.sandTopScratch[c + 1] - 2 * this.sandTopScratch[c] + this.sandTopScratch[c - 1]));
    }
    return maximum;
  }

  private waterSlopeDeg(): number {
    let maximum = 0;
    const start = Math.floor(this.cols * 0.1);
    const end = this.cols - 1 - start;
    for (let c = start; c < end; c++) {
      maximum = Math.max(maximum, Math.abs(this.waterTop[c + 1] - this.waterTop[c]));
    }
    return Math.atan(maximum) * 180 / Math.PI;
  }

  private waterAmplitude(): number {
    const start = Math.floor(this.cols * 0.1);
    const end = this.cols - start;
    let minimum = Number.POSITIVE_INFINITY;
    let maximum = Number.NEGATIVE_INFINITY;
    for (let c = start; c < end; c++) {
      minimum = Math.min(minimum, this.waterTop[c]);
      maximum = Math.max(maximum, this.waterTop[c]);
    }
    return maximum - minimum;
  }

  private updateSandTop(): void {
    for (let c = 0; c < this.cols; c++) {
      this.sandTopScratch[c] = this.bh[c] + this.bl[c] + this.mh[c] + this.ml[c];
    }
  }

  private validateScene(scene: WaterSandScene): void {
    const groups = [
      scene.waterRegions, scene.heavyDepositedRegions, scene.lightDepositedRegions,
      scene.heavySuspendedRegions, scene.lightSuspendedRegions
    ];
    for (const regions of groups) {
      for (const region of regions) {
        if (!Number.isInteger(region.startCol) || !Number.isInteger(region.endColInclusive) ||
          region.startCol < 0 || region.endColInclusive >= this.cols ||
          region.startCol > region.endColInclusive || region.bottom < 0 ||
          !finite(region.height) || region.height < 0) {
          throw new WaterSandParamError('invalid scene region');
        }
      }
    }
  }

  private fillRegions(target: Float64Array, regions: WaterSandRegion[], additive: boolean): void {
    for (const region of regions) {
      for (let c = region.startCol; c <= region.endColInclusive; c++) {
        if (additive) {
          target[c] += region.height;
        } else {
          target[c] = Math.max(target[c], region.bottom + region.height);
        }
      }
    }
  }

  private defaultScene(): WaterSandScene {
    const all: WaterSandRegion = { startCol: 0, endColInclusive: this.cols - 1, bottom: 0, height: 110 };
    return {
      waterRegions: [all],
      heavyDepositedRegions: [],
      lightDepositedRegions: [],
      heavySuspendedRegions: [{ startCol: 0, endColInclusive: this.cols - 1, bottom: 0, height: 24 }],
      lightSuspendedRegions: [{ startCol: 0, endColInclusive: this.cols - 1, bottom: 0, height: 16 }]
    };
  }

  private createEmptySnapshot(): WaterSandSnapshot {
    return {
      schemaVersion: 2,
      cols: this.cols,
      capacity: this.capacity,
      waterTop: new Float64Array(this.cols),
      bh: new Float64Array(this.cols),
      bl: new Float64Array(this.cols),
      mh: new Float64Array(this.cols),
      ml: new Float64Array(this.cols),
      suspH: new Float64Array(this.cols),
      suspL: new Float64Array(this.cols),
      hs: new Float64Array(this.cols),
      hl: new Float64Array(this.cols),
      sandTop: new Float64Array(this.cols)
    };
  }

  private saveBackup(): void {
    this.backup.waterTop.set(this.waterTop);
    this.backup.bh.set(this.bh);
    this.backup.bl.set(this.bl);
    this.backup.mh.set(this.mh);
    this.backup.ml.set(this.ml);
    this.backup.suspH.set(this.suspH);
    this.backup.suspL.set(this.suspL);
  }

  private restoreBackup(): void {
    this.waterTop.set(this.backup.waterTop);
    this.bh.set(this.backup.bh);
    this.bl.set(this.backup.bl);
    this.mh.set(this.backup.mh);
    this.ml.set(this.backup.ml);
    this.suspH.set(this.backup.suspH);
    this.suspL.set(this.backup.suspL);
    this.updateSandTop();
  }

  private changedSinceBackup(): boolean {
    for (let c = 0; c < this.cols; c++) {
      if (Math.abs(this.waterTop[c] - this.backup.waterTop[c]) > EPS ||
        Math.abs(this.bh[c] - this.backup.bh[c]) > EPS ||
        Math.abs(this.bl[c] - this.backup.bl[c]) > EPS ||
        Math.abs(this.mh[c] - this.backup.mh[c]) > EPS ||
        Math.abs(this.ml[c] - this.backup.ml[c]) > EPS ||
        Math.abs(this.suspH[c] - this.backup.suspH[c]) > EPS ||
        Math.abs(this.suspL[c] - this.backup.suspL[c]) > EPS) {
        return true;
      }
    }
    return false;
  }

  private firstInvalidColumn(): number {
    const state = [this.waterTop, this.bh, this.bl, this.mh, this.ml, this.suspH, this.suspL];
    for (let c = 0; c < this.cols; c++) {
      for (const array of state) {
        if (!finite(array[c]) || array[c] < -EPS) {
          return c;
        }
      }
    }
    return -1;
  }
}

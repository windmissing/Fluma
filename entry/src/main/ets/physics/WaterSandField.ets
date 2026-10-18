/**
 * WaterSandField - P-003/P-005 scene 2 three-phase column model, algorithm v2.2.
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
  routeId: string;
  angleDeg: number;
  heavySettleSpeed: number;
  lightSettleSpeed: number;
  percolationRate: number;
  waterLevelRate: number;
  heavyDiffusion: number;
  lightDiffusion: number;
  shakeDecay: number;
  shakeRampRate: number;
  heavyLiftRate: number;
  lightLiftRate: number;
  vSandTopMax: number;
  totalMotionBudget: number;
  maxAvalancheFlux: number;
  avalanchePasses: number;
  bottomCreepRate: number;
  vBottomCreepMax: number;
  bottomCreepVelocityGain: number;
  bottomCreepDeadZone: number;
  directionHoldTime: number;
  inputSmoothingAlpha: number;
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
  maxSandTopSubstepDeltaObserved: number;
  maxSandTopSpeedObserved: number;
  substepBudgetUsedMax: number;
  substepBudgetClipped: boolean;
  physicsTimeMs: number;
  dormant: boolean;
}

export interface WaterSandStepResult {
  ok: boolean;
  stepped: boolean;
  substeps: number;
  substepDt: number;
  moved: boolean;
  dormant: boolean;
  maxSandTopSubstepDeltaObserved: number;
  maxSandTopSpeedObserved: number;
  substepBudgetUsedMax: number;
  substepBudgetClipped: boolean;
  physicsTimeMs: number;
  errorCode: WaterSandErrorCode | null;
  errorColumn: number;
}

export interface WaterSandP011Metric {
  routeId: string;
  runId: string;
  sequence: number;
  frameId: number;
  substepId: number;
  physicsTime: number;
  physicsDt: number;
  targetTilt: number;
  effectiveTilt: number;
  effectiveTiltRate: number;
  bottomCreepRate: number;
  bottomFlux: number;
  bottomCreepBudgetUsed: number;
  bottomCreepBudgetLimit: number;
  bottomCreepBudgetClipped: number;
  totalMotionBudgetUsed: number;
  totalMotionBudgetLimit: number;
  totalMotionBudgetClipped: number;
  directionCorrect: boolean;
  waterTotal: number;
  heavyTotal: number;
  lightTotal: number;
  commandPhaseId: number;
  commandPhaseSign: number;
  commandPhaseStartPhysicsTime: number;
  heavyFlux: number;
  lightFlux: number;
  heavyActualDisplacement: number;
  lightActualDisplacement: number;
  heavyIdealDisplacement: number;
  lightIdealDisplacement: number;
  heavyResidual: number;
  lightResidual: number;
  heavyShapeRms: number;
  lightShapeRms: number;
  heavyShapeMaxDelta: number;
  lightShapeMaxDelta: number;
  maxSubstepBh: number;
  maxSubstepBl: number;
  heavyShare: number | null;
  heavyShareDeficit: number;
}

export interface WaterSandP011Bottom {
  routeId: string;
  runId: string;
  sequence: number;
  frameId: number;
  substepId: number;
  physicsTime: number;
  bh: number[];
  bl: number[];
  // P-011 bottom-creep-v2.8-heavy-interface: diagnostic displacement arrays.
  bottomCreepDonorEffectiveDisplacement: number[];
  bottomCreepReceiverEffectiveDisplacement: number[];
}

export type WaterSandP011InputSource = 'script' | 'sensor' | 'gesture';

export type WaterSandP011Hook = (metric: WaterSandP011Metric, bottom: WaterSandP011Bottom) => void;

let lastP011TimeMs = 0;
let lastP011RunCounter = 0;

export function nextP011TimeMs(): number {
  const wallTime = Date.now();
  const time = wallTime > lastP011TimeMs ? wallTime : lastP011TimeMs + 1;
  lastP011TimeMs = time;
  return time;
}

function nextP011RunId(): string {
  lastP011RunCounter += 1;
  return `p011-run-${Date.now()}-${lastP011RunCounter}`;
}

export interface WaterSandFrameHookPayload {
  frameTimeMs: number;
  physicsTimeMs: number;
  substeps: number;
  substepDt: number;
  maxSandTopSubstepDeltaObserved: number;
  maxSandTopSpeedObserved: number;
  substepBudgetUsedMax: number;
  substepBudgetClipped: boolean;
  dormant: boolean;
}

export function waterSandFrameHookPayload(
  frameTimeMs: number,
  result: WaterSandStepResult
): WaterSandFrameHookPayload {
  return {
    frameTimeMs,
    physicsTimeMs: result.physicsTimeMs,
    substeps: result.substeps,
    substepDt: result.substepDt,
    maxSandTopSubstepDeltaObserved: result.maxSandTopSubstepDeltaObserved,
    maxSandTopSpeedObserved: result.maxSandTopSpeedObserved,
    substepBudgetUsedMax: result.substepBudgetUsedMax,
    substepBudgetClipped: result.substepBudgetClipped,
    dormant: result.dormant
  };
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
const P011_TARGET_SHAPE_K = 0.5;
const P011_TARGET_SHAPE_TAU_H_SECONDS = 0.100;
const P011_TARGET_SHAPE_TAU_L_SECONDS = 0.060;
const P011_HEAVY_SHARE_MIN = 0.35;
const P011_HEAVY_SHARE_MAX = 0.65;
const P011_HEAVY_SHARE_BIAS = 0.10;
const P011_CENTRAL_ROI_FRACTION = 0.80;
const P011_BOTTOM_ROUTE_ID = 'bottom-creep-v2.8-heavy-interface';

export function defaultWaterSandParams(): WaterSandParams {
  return {
    routeId: P011_BOTTOM_ROUTE_ID,
    angleDeg: 30,
    heavySettleSpeed: 64,
    lightSettleSpeed: 38,
    percolationRate: 6,
    waterLevelRate: 8,
    heavyDiffusion: 4,
    lightDiffusion: 10,
    shakeDecay: 2.2,
    shakeRampRate: 8,
    heavyLiftRate: 36,
    lightLiftRate: 40,
    vSandTopMax: 28,
    totalMotionBudget: 300,
    maxAvalancheFlux: 24,
    avalanchePasses: 5,
    bottomCreepRate: 96,
    vBottomCreepMax: 10,
    bottomCreepVelocityGain: 36,
    bottomCreepDeadZone: 1,
    directionHoldTime: 80,
    inputSmoothingAlpha: 0.6,
    maxTiltOmegaDegPerSec: 720
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
  private smoothScratch: Float64Array;
  private substepMotionBudgetRemaining: Float64Array;
  private substepMotionBudgetUsed: Float64Array;
  private substepSandTopBefore: Float64Array;
  private bottomCreepRequestAmounts: Float64Array;
  private bottomCreepRequestDonors: Int32Array;
  private bottomCreepRequestReceivers: Int32Array;
  // P-011 bottom-creep-v2.8-heavy-interface: substep-local and phase telemetry state.
  private bottomCreepDonorEffectiveDisplacement: Float64Array;
  private bottomCreepReceiverEffectiveDisplacement: Float64Array;
  private phaseHeavyBaseline: Float64Array;
  private phaseLightBaseline: Float64Array;
  private substepBhBefore: Float64Array;
  private substepBlBefore: Float64Array;
  private settleRequestAmounts: Float64Array;
  private percolationRequestAmounts: Float64Array;
  private slipRequestAmounts: Float64Array;
  private slipRequestDonors: Int32Array;
  private slipRequestReceivers: Int32Array;
  private riseRequestAmounts: Float64Array;
  private backup: WaterSandSnapshot;
  private motionSnapshot: WaterSandSnapshot;
  private initialWater = 0;
  private initialHeavy = 0;
  private initialLight = 0;
  private targetTilt = 0;
  private effectiveTilt = 0;
  private effectiveTiltRate = 0;
  private bottomCreepDirection = 0;
  private lastBottomDirectionChange = 0;
  private lastNonZeroTargetTime = 0;
  private lastBottomFlux = 0;
  private lastBottomCreepRate = 0;
  private bottomCreepBudgetUsed = 0;
  private bottomCreepBudgetLimit = 0;
  private bottomCreepBudgetClipped = 0;
  private bottomCreepBudgetRequested = 0;
  private targetShapeIdealDisplacement = 0;
  private targetShapeActualDisplacement = 0;
  private targetShapeRequestRate = 0;
  private commandPhaseId = 0;
  private commandPhaseSign = 0;
  private commandPhaseStartPhysicsTime = 0;
  private phaseHeavyIdeal = 0;
  private phaseLightIdeal = 0;
  private phaseHeavyActual = 0;
  private phaseLightActual = 0;
  private lastHeavyFlux = 0;
  private lastLightFlux = 0;
  private lastHeavyShapeRms = 0;
  private lastLightShapeRms = 0;
  private lastHeavyShapeMaxDelta = 0;
  private lastLightShapeMaxDelta = 0;
  private lastMaxSubstepBh = 0;
  private lastMaxSubstepBl = 0;
  private lastBottomSplitError: WaterSandErrorCode | null = null;
  private bottomCreepRequestCount = 0;
  private slipRequestCount = 0;
  private totalMotionBudgetRemaining = 0;
  private totalMotionBudgetUsed = 0;
  private totalMotionBudgetLimit = 0;
  private totalMotionBudgetClipped = 0;
  private totalMotionBudgetRequested = 0;
  private totalMotionBudgetScale = 1;
  private collectingTotalMotionRequests = false;
  private lastP011DirectionCorrect = true;
  private lastP011PhysicsTime = 0;
  private lastP011PhysicsDt = 0;
  private p011RunId = nextP011RunId();
  private p011SampleSequence = 0;
  private p011InputSequence = 0;
  private p011FrameId = 0;
  private p011SubstepId = 0;
  private zeroTargetSince = -1;
  private shakeInput = 0;
  private shakeEnergy = 0;
  private simulationTime = 0;
  private t90Heavy = -1;
  private t90Light = -1;
  private maxSandTopSubstepDeltaObserved = 0;
  private maxSandTopSpeedObserved = 0;
  private substepBudgetUsedMax = 0;
  private substepBudgetClipped = false;
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
    this.smoothScratch = new Float64Array(cols);
    this.substepMotionBudgetRemaining = new Float64Array(cols);
    this.substepMotionBudgetUsed = new Float64Array(cols);
    this.substepSandTopBefore = new Float64Array(cols);
    this.bottomCreepRequestAmounts = new Float64Array(cols - 1);
    this.bottomCreepRequestDonors = new Int32Array(cols - 1);
    this.bottomCreepRequestReceivers = new Int32Array(cols - 1);
    this.bottomCreepDonorEffectiveDisplacement = new Float64Array(cols);
    this.bottomCreepReceiverEffectiveDisplacement = new Float64Array(cols);
    this.phaseHeavyBaseline = new Float64Array(cols);
    this.phaseLightBaseline = new Float64Array(cols);
    this.substepBhBefore = new Float64Array(cols);
    this.substepBlBefore = new Float64Array(cols);
    this.settleRequestAmounts = new Float64Array(cols);
    this.percolationRequestAmounts = new Float64Array(cols);
    this.slipRequestAmounts = new Float64Array(6 * (cols - 1));
    this.slipRequestDonors = new Int32Array(6 * (cols - 1));
    this.slipRequestReceivers = new Int32Array(6 * (cols - 1));
    this.riseRequestAmounts = new Float64Array(cols);
    this.backup = this.createEmptySnapshot();
    this.motionSnapshot = this.createEmptySnapshot();
    this.initialize(this.defaultScene());
  }

  configure(params: WaterSandParams): void {
    if (params.routeId !== P011_BOTTOM_ROUTE_ID ||
      params.angleDeg <= 0 || params.angleDeg >= 45 ||
      params.lightSettleSpeed <= 0 || params.heavySettleSpeed <= params.lightSettleSpeed ||
      params.percolationRate <= 0 || params.waterLevelRate <= 0 ||
      params.heavyDiffusion < 0 || params.lightDiffusion < 0 || params.shakeDecay <= 0 ||
      params.shakeRampRate <= 0 || params.heavyLiftRate < 0 ||
      params.heavyLiftRate > params.lightLiftRate || params.vSandTopMax <= 0 ||
      params.vSandTopMax > 30 ||
      params.totalMotionBudget <= 0 ||
      params.maxAvalancheFlux <= 0 || params.avalanchePasses < 4 || params.avalanchePasses > 6 ||
      params.bottomCreepRate < 0 ||
      params.vBottomCreepMax <= 0 || params.vBottomCreepMax > 12 ||
      params.bottomCreepVelocityGain < 0 || params.bottomCreepDeadZone < 0 ||
      params.bottomCreepDeadZone >= params.angleDeg || params.directionHoldTime < 0 ||
      params.inputSmoothingAlpha <= 0 || params.inputSmoothingAlpha > 1 ||
      params.maxTiltOmegaDegPerSec <= 0) {
      throw new WaterSandParamError('invalid WaterSandParams');
    }
    const values: number[] = [params.angleDeg, params.heavySettleSpeed, params.lightSettleSpeed,
      params.percolationRate, params.waterLevelRate, params.heavyDiffusion, params.lightDiffusion,
      params.shakeDecay, params.shakeRampRate, params.heavyLiftRate, params.lightLiftRate,
      params.vSandTopMax, params.totalMotionBudget, params.maxAvalancheFlux, params.avalanchePasses,
      params.bottomCreepRate, params.vBottomCreepMax, params.bottomCreepVelocityGain, params.bottomCreepDeadZone,
      params.directionHoldTime, params.inputSmoothingAlpha, params.maxTiltOmegaDegPerSec];
    for (const value of values) {
      if (!finite(value)) {
        throw new WaterSandParamError('non-finite WaterSandParams value');
      }
    }
    this.params = {
      routeId: params.routeId,
      angleDeg: params.angleDeg,
      heavySettleSpeed: params.heavySettleSpeed,
      lightSettleSpeed: params.lightSettleSpeed,
      percolationRate: params.percolationRate,
      waterLevelRate: params.waterLevelRate,
      heavyDiffusion: params.heavyDiffusion,
      lightDiffusion: params.lightDiffusion,
      shakeDecay: params.shakeDecay,
      shakeRampRate: params.shakeRampRate,
      heavyLiftRate: params.heavyLiftRate,
      lightLiftRate: params.lightLiftRate,
      vSandTopMax: params.vSandTopMax,
      totalMotionBudget: params.totalMotionBudget,
      maxAvalancheFlux: params.maxAvalancheFlux,
      avalanchePasses: params.avalanchePasses,
      bottomCreepRate: params.bottomCreepRate,
      vBottomCreepMax: params.vBottomCreepMax,
      bottomCreepVelocityGain: params.bottomCreepVelocityGain,
      bottomCreepDeadZone: params.bottomCreepDeadZone,
      directionHoldTime: params.directionHoldTime,
      inputSmoothingAlpha: params.inputSmoothingAlpha,
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
    this.maxSandTopSubstepDeltaObserved = 0;
    this.maxSandTopSpeedObserved = 0;
    this.substepBudgetUsedMax = 0;
    this.substepBudgetClipped = false;
    this.lastBottomFlux = 0;
    this.lastBottomCreepRate = 0;
    this.targetShapeIdealDisplacement = 0;
    this.targetShapeActualDisplacement = 0;
    this.commandPhaseId = 0;
    this.commandPhaseSign = 0;
    this.commandPhaseStartPhysicsTime = 0;
    this.phaseHeavyIdeal = 0;
    this.phaseLightIdeal = 0;
    this.phaseHeavyActual = 0;
    this.phaseLightActual = 0;
    this.lastHeavyFlux = 0;
    this.lastLightFlux = 0;
    this.lastHeavyShapeRms = 0;
    this.lastLightShapeRms = 0;
    this.lastHeavyShapeMaxDelta = 0;
    this.lastLightShapeMaxDelta = 0;
    this.lastMaxSubstepBh = 0;
    this.lastMaxSubstepBl = 0;
    this.lastBottomSplitError = null;
    this.phaseHeavyBaseline.set(this.bh);
    this.phaseLightBaseline.set(this.bl);
    this.lastP011PhysicsTime = 0;
    this.lastP011PhysicsDt = 0;
    this.p011RunId = nextP011RunId();
    this.p011SampleSequence = 0;
    this.p011InputSequence = 0;
    this.p011FrameId = 0;
    this.p011SubstepId = 0;
    this.frozen = false;
    this.dormant = false;
    this.lastError = null;
  }

  reset(scene?: WaterSandScene): void {
    this.initialize(scene === undefined ? this.defaultScene() : scene);
  }

  // P-011 dual-input-v1: O(1) mode boundary; preserves materials, budgets and run identity.
  resetInputState(): void {
    this.targetTilt = 0;
    this.effectiveTilt = 0;
    this.effectiveTiltRate = 0;
    this.bottomCreepDirection = 0;
    this.lastBottomDirectionChange = this.simulationTime;
    this.lastNonZeroTargetTime = this.simulationTime;
    this.zeroTargetSince = this.simulationTime;
    this.dormant = false;
  }

  setTilt(deg: number): void {
    if (!finite(deg)) {
      throw new WaterSandParamError('tilt must be finite');
    }
    this.targetTilt = Math.max(-89, Math.min(89, deg));
    if (Math.abs(this.targetTilt) <= EPS) {
      if (this.zeroTargetSince < 0) {
        this.zeroTargetSince = this.simulationTime;
      }
    } else {
      this.zeroTargetSince = -1;
      this.lastNonZeroTargetTime = this.simulationTime;
    }
    const targetSign = this.targetTilt > EPS ? 1 : this.targetTilt < -EPS ? -1 : this.commandPhaseSign;
    if (targetSign !== 0 && targetSign !== this.commandPhaseSign) {
      this.beginCommandPhase(targetSign);
    }
    this.dormant = false;
  }

  private beginCommandPhase(sign: number): void {
    this.commandPhaseId += 1;
    this.commandPhaseSign = sign;
    this.commandPhaseStartPhysicsTime = this.lastP011PhysicsTime;
    this.targetShapeIdealDisplacement = 0;
    this.targetShapeActualDisplacement = 0;
    this.phaseHeavyIdeal = 0;
    this.phaseLightIdeal = 0;
    this.phaseHeavyActual = 0;
    this.phaseLightActual = 0;
    this.phaseHeavyBaseline.set(this.bh);
    this.phaseLightBaseline.set(this.bl);
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

  step(dt: number, p011Hook?: WaterSandP011Hook): WaterSandStepResult {
    const start = Date.now();
    const result: WaterSandStepResult = {
      ok: true,
      stepped: false,
      substeps: 0,
      substepDt: 0,
      moved: false,
      dormant: false,
      maxSandTopSubstepDeltaObserved: 0,
      maxSandTopSpeedObserved: 0,
      substepBudgetUsedMax: 0,
      substepBudgetClipped: false,
      physicsTimeMs: 0,
      errorCode: null,
      errorColumn: -1
    };
    if (this.frozen || !finite(dt) || dt <= 0) {
      result.physicsTimeMs = Date.now() - start;
      return result;
    }
    this.p011FrameId += 1;
    if (this.dormant && Math.abs(this.targetTilt - this.effectiveTilt) <= EPS &&
      this.shakeEnergy <= EPS && this.shakeInput <= EPS) {
      result.dormant = true;
      result.physicsTimeMs = Date.now() - start;
      this.lastPhysicsTimeMs = result.physicsTimeMs;
      return result;
    }
    const clipped = Math.min(dt, 1 / 30);
    const substeps = clipped <= 1 / 60 ? 1 : 2;
    const substepDt = substeps === 1 ? clipped : clipped / 2;
    result.substepDt = substepDt;
    this.maxSandTopSubstepDeltaObserved = 0;
    this.maxSandTopSpeedObserved = 0;
    this.substepBudgetUsedMax = 0;
    this.substepBudgetClipped = false;
    for (let index = 0; index < substeps; index++) {
      this.saveBackup();
      this.updateSandTop();
      this.beginSubstepMotionBudget(substepDt, true, 1);
      this.substepSandTopBefore.set(this.sandTopScratch);
      this.substepBhBefore.set(this.bh);
      this.substepBlBefore.set(this.bl);
      this.lastBottomFlux = 0;
      this.lastBottomCreepRate = 0;
      this.targetShapeRequestRate = 0;
      this.lastHeavyFlux = 0;
      this.lastLightFlux = 0;
      this.lastMaxSubstepBh = 0;
      this.lastMaxSubstepBl = 0;
      this.lastBottomSplitError = null;
      this.lastP011DirectionCorrect = true;
      let errorCode = this.substep(substepDt);
      if (errorCode === null) {
        this.restoreMotionSnapshot();
        this.lastBottomFlux = 0;
        const scale = this.totalMotionBudgetRequested <= this.totalMotionBudgetLimit
          ? 1 : this.totalMotionBudgetLimit / this.totalMotionBudgetRequested;
        this.beginMotionApplication(substepDt, scale);
        errorCode = this.applyMotionChannels(substepDt);
      }
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
      let substepDelta = 0;
      let budgetUsed = 0;
      for (let c = 0; c < this.cols; c++) {
        substepDelta = Math.max(substepDelta,
          Math.abs(this.sandTopScratch[c] - this.substepSandTopBefore[c]));
        budgetUsed = Math.max(budgetUsed, this.substepMotionBudgetUsed[c]);
      }
      this.maxSandTopSubstepDeltaObserved = Math.max(
        this.maxSandTopSubstepDeltaObserved, substepDelta);
      this.maxSandTopSpeedObserved = Math.max(
        this.maxSandTopSpeedObserved, substepDelta / Math.max(substepDt, EPS));
      this.substepBudgetUsedMax = Math.max(this.substepBudgetUsedMax, budgetUsed);
      this.updateMetrics(substepDt);
      this.lastP011PhysicsTime = nextP011TimeMs();
      this.lastP011PhysicsDt = substepDt;
      this.p011SubstepId += 1;
      this.updatePhaseShapeTelemetry();
      if (p011Hook !== undefined) {
        p011Hook(this.getP011Metric(), this.getP011Bottom());
      }
    }
    result.maxSandTopSubstepDeltaObserved = this.maxSandTopSubstepDeltaObserved;
    result.maxSandTopSpeedObserved = this.maxSandTopSpeedObserved;
    result.substepBudgetUsedMax = this.substepBudgetUsedMax;
    result.substepBudgetClipped = this.substepBudgetClipped;
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

  private updatePhaseShapeTelemetry(): void {
    const margin = Math.floor(this.cols * (1 - P011_CENTRAL_ROI_FRACTION) / 2);
    const start = margin;
    const endExclusive = this.cols - margin;
    const count = Math.max(1, endExclusive - start);
    let heavySquareSum = 0;
    let lightSquareSum = 0;
    let heavyMaxDelta = 0;
    let lightMaxDelta = 0;
    for (let column = start; column < endExclusive; column++) {
      const heavyDelta = this.bh[column] - this.phaseHeavyBaseline[column];
      const lightDelta = this.bl[column] - this.phaseLightBaseline[column];
      heavySquareSum += heavyDelta * heavyDelta;
      lightSquareSum += lightDelta * lightDelta;
      heavyMaxDelta = Math.max(heavyMaxDelta, Math.abs(heavyDelta));
      lightMaxDelta = Math.max(lightMaxDelta, Math.abs(lightDelta));
    }
    this.lastHeavyShapeRms = Math.sqrt(heavySquareSum / count);
    this.lastLightShapeRms = Math.sqrt(lightSquareSum / count);
    this.lastHeavyShapeMaxDelta = heavyMaxDelta;
    this.lastLightShapeMaxDelta = lightMaxDelta;
    this.lastMaxSubstepBh = 0;
    this.lastMaxSubstepBl = 0;
    for (let column = 0; column < this.cols; column++) {
      this.lastMaxSubstepBh = Math.max(this.lastMaxSubstepBh,
        Math.abs(this.bh[column] - this.substepBhBefore[column]));
      this.lastMaxSubstepBl = Math.max(this.lastMaxSubstepBl,
        Math.abs(this.bl[column] - this.substepBlBefore[column]));
    }
  }

  getP011Metric(): WaterSandP011Metric {
    const totals = this.getTotals();
    this.updatePhaseShapeTelemetry();
    const heavyShare = this.lastBottomFlux > EPS ? this.lastHeavyFlux / this.lastBottomFlux : null;
    this.p011SampleSequence += 1;
    return {
      routeId: this.params.routeId,
      runId: this.p011RunId,
      sequence: this.p011SampleSequence,
      frameId: this.p011FrameId,
      substepId: this.p011SubstepId,
      physicsTime: this.lastP011PhysicsTime,
      physicsDt: this.lastP011PhysicsDt,
      targetTilt: this.targetTilt,
      effectiveTilt: this.effectiveTilt,
      effectiveTiltRate: this.effectiveTiltRate,
      bottomCreepRate: this.lastBottomCreepRate,
      bottomFlux: this.lastBottomFlux,
      bottomCreepBudgetUsed: this.bottomCreepBudgetUsed,
      bottomCreepBudgetLimit: this.bottomCreepBudgetLimit,
      bottomCreepBudgetClipped: this.bottomCreepBudgetClipped,
      totalMotionBudgetUsed: this.totalMotionBudgetUsed,
      totalMotionBudgetLimit: this.totalMotionBudgetLimit,
      totalMotionBudgetClipped: this.totalMotionBudgetClipped,
      directionCorrect: this.lastP011DirectionCorrect,
      waterTotal: totals.water,
      heavyTotal: totals.heavy,
      lightTotal: totals.light,
      commandPhaseId: this.commandPhaseId,
      commandPhaseSign: this.commandPhaseSign,
      commandPhaseStartPhysicsTime: this.commandPhaseStartPhysicsTime,
      heavyFlux: this.lastHeavyFlux,
      lightFlux: this.lastLightFlux,
      heavyActualDisplacement: this.phaseHeavyActual,
      lightActualDisplacement: this.phaseLightActual,
      heavyIdealDisplacement: this.phaseHeavyIdeal,
      lightIdealDisplacement: this.phaseLightIdeal,
      heavyResidual: this.phaseHeavyIdeal - this.phaseHeavyActual,
      lightResidual: this.phaseLightIdeal - this.phaseLightActual,
      heavyShapeRms: this.lastHeavyShapeRms,
      lightShapeRms: this.lastLightShapeRms,
      heavyShapeMaxDelta: this.lastHeavyShapeMaxDelta,
      lightShapeMaxDelta: this.lastLightShapeMaxDelta,
      maxSubstepBh: this.lastMaxSubstepBh,
      maxSubstepBl: this.lastMaxSubstepBl,
      heavyShare,
      heavyShareDeficit: heavyShare === null ? 0 : Math.max(0, P011_HEAVY_SHARE_MIN - heavyShare)
    };
  }

  getP011Bottom(): WaterSandP011Bottom {
    return {
      routeId: this.params.routeId,
      runId: this.p011RunId,
      sequence: this.p011SampleSequence,
      frameId: this.p011FrameId,
      substepId: this.p011SubstepId,
      physicsTime: this.lastP011PhysicsTime,
      bh: Array.from(this.bh),
      bl: Array.from(this.bl),
      bottomCreepDonorEffectiveDisplacement: Array.from(this.bottomCreepDonorEffectiveDisplacement),
      bottomCreepReceiverEffectiveDisplacement: Array.from(this.bottomCreepReceiverEffectiveDisplacement)
    };
  }

  getP011RunId(): string {
    return this.p011RunId;
  }

  nextP011InputSequence(): number {
    this.p011InputSequence += 1;
    return this.p011InputSequence;
  }

  emitP011NoopTelemetry(p011Hook?: WaterSandP011Hook): void {
    // P-011 bottom-creep-v2.8-heavy-interface: align dormant commands by physicsTime.
    this.p011FrameId += 1;
    this.p011SubstepId += 1;
    this.lastP011PhysicsTime = nextP011TimeMs();
    this.lastBottomFlux = 0;
    this.lastBottomCreepRate = 0;
    this.lastP011PhysicsDt = 0;
    this.lastHeavyFlux = 0;
    this.lastLightFlux = 0;
    this.lastMaxSubstepBh = 0;
    this.lastMaxSubstepBl = 0;
    this.bottomCreepBudgetUsed = 0;
    this.bottomCreepBudgetLimit = 0;
    this.bottomCreepBudgetClipped = 0;
    this.totalMotionBudgetUsed = 0;
    this.totalMotionBudgetLimit = 0;
    this.totalMotionBudgetClipped = 0;
    this.lastP011DirectionCorrect = true;
    this.bottomCreepDonorEffectiveDisplacement.fill(0);
    this.bottomCreepReceiverEffectiveDisplacement.fill(0);
    this.substepBhBefore.set(this.bh);
    this.substepBlBefore.set(this.bl);
    this.updatePhaseShapeTelemetry();
    if (p011Hook !== undefined) {
      p011Hook(this.getP011Metric(), this.getP011Bottom());
    }
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
      maxSandTopSubstepDeltaObserved: this.maxSandTopSubstepDeltaObserved,
      maxSandTopSpeedObserved: this.maxSandTopSpeedObserved,
      substepBudgetUsedMax: this.substepBudgetUsedMax,
      substepBudgetClipped: this.substepBudgetClipped,
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

  private beginSubstepMotionBudget(substepDt: number, collectRequests: boolean,
    totalMotionScale: number): void {
    // P-003/P-005 v2.2 and P-011 v2.8: substep-local motion budgets.
    const budget = this.params.vSandTopMax * substepDt;
    this.substepMotionBudgetRemaining.fill(budget);
    this.substepMotionBudgetUsed.fill(0);
    this.bottomCreepBudgetLimit = this.params.vBottomCreepMax * substepDt;
    this.bottomCreepBudgetUsed = 0;
    this.bottomCreepBudgetClipped = 0;
    if (collectRequests) {
      this.settleRequestAmounts.fill(0);
      this.percolationRequestAmounts.fill(0);
      this.slipRequestAmounts.fill(0);
      this.slipRequestDonors.fill(-1);
      this.slipRequestReceivers.fill(-1);
      this.riseRequestAmounts.fill(0);
      this.slipRequestCount = 0;
      this.bottomCreepBudgetRequested = 0;
      this.bottomCreepRequestCount = 0;
      this.bottomCreepRequestAmounts.fill(0);
      this.bottomCreepRequestDonors.fill(-1);
      this.bottomCreepRequestReceivers.fill(-1);
      this.bottomCreepDonorEffectiveDisplacement.fill(0);
      this.bottomCreepReceiverEffectiveDisplacement.fill(0);
    }
    this.totalMotionBudgetLimit = this.params.totalMotionBudget * substepDt;
    this.totalMotionBudgetRemaining = this.totalMotionBudgetLimit;
    this.totalMotionBudgetUsed = 0;
    this.totalMotionBudgetClipped = 0;
    this.totalMotionBudgetRequested = 0;
    this.collectingTotalMotionRequests = collectRequests;
    this.totalMotionBudgetScale = collectRequests ? 1 : totalMotionScale;
  }

  private beginMotionApplication(substepDt: number, totalMotionScale: number): void {
    const budget = this.params.vSandTopMax * substepDt;
    this.substepMotionBudgetRemaining.fill(budget);
    this.substepMotionBudgetUsed.fill(0);
    this.bottomCreepBudgetUsed = 0;
    this.totalMotionBudgetRemaining = this.totalMotionBudgetLimit;
    this.totalMotionBudgetUsed = 0;
    this.totalMotionBudgetClipped = 0;
    this.collectingTotalMotionRequests = false;
    this.totalMotionBudgetScale = totalMotionScale;
  }

  private saveMotionSnapshot(): void {
    this.motionSnapshot.waterTop.set(this.waterTop);
    this.motionSnapshot.bh.set(this.bh);
    this.motionSnapshot.bl.set(this.bl);
    this.motionSnapshot.mh.set(this.mh);
    this.motionSnapshot.ml.set(this.ml);
    this.motionSnapshot.suspH.set(this.suspH);
    this.motionSnapshot.suspL.set(this.suspL);
    this.updateSandTop();
  }

  private restoreMotionSnapshot(): void {
    this.waterTop.set(this.motionSnapshot.waterTop);
    this.bh.set(this.motionSnapshot.bh);
    this.bl.set(this.motionSnapshot.bl);
    this.mh.set(this.motionSnapshot.mh);
    this.ml.set(this.motionSnapshot.ml);
    this.suspH.set(this.motionSnapshot.suspH);
    this.suspL.set(this.motionSnapshot.suspL);
    this.updateSandTop();
  }

  private clipSubstepMotion(source: number, target: number, requested: number): number {
    let locallyAllowed = Math.max(0, requested);
    if (target === -1) {
      locallyAllowed = Math.min(locallyAllowed, this.substepMotionBudgetRemaining[source]);
    } else {
      locallyAllowed = Math.min(locallyAllowed,
        this.substepMotionBudgetRemaining[source], this.substepMotionBudgetRemaining[target]);
    }
    if (locallyAllowed < requested - EPS) {
      this.markSubstepBudgetClipped();
    }
    const allowed = this.clipTotalMotion(locallyAllowed, locallyAllowed);
    this.substepMotionBudgetRemaining[source] -= allowed;
    if (target !== -1) {
      this.substepMotionBudgetRemaining[target] -= allowed;
    }
    this.substepMotionBudgetUsed[source] += allowed;
    if (target !== -1) {
      this.substepMotionBudgetUsed[target] += allowed;
    }
    return allowed;
  }

  private clipTotalMotion(locallyAllowed: number, requestedForTotal: number): number {
    const locallyAllowedValue = Math.max(0, locallyAllowed);
    const requested = Math.max(0, requestedForTotal);
    const totalAllowed = Math.min(requested * this.totalMotionBudgetScale,
      this.totalMotionBudgetRemaining);
    const allowed = Math.min(locallyAllowedValue, totalAllowed);
    this.totalMotionBudgetUsed += allowed;
    this.totalMotionBudgetClipped += requested - totalAllowed;
    this.totalMotionBudgetRemaining -= allowed;
    return allowed;
  }

  private clipBottomCreepMotion(requested: number): number {
    const allowed = this.clipTotalMotion(requested, requested);
    this.bottomCreepBudgetUsed += allowed;
    this.targetShapeActualDisplacement += allowed;
    return allowed;
  }

  private markSubstepBudgetClipped(): void {
    this.substepBudgetClipped = true;
  }

  private substep(dt: number): WaterSandErrorCode | null {
    this.updateTilt(dt);
    this.updateSandTop();
    this.waterLevel(dt);
    this.diffuseSuspension(dt);
    if (this.collectingTotalMotionRequests) {
      this.saveMotionSnapshot();
    }
    this.settleSandV2(dt);
    this.percolateHeavyConservative(dt);
    const creepError = this.creepBottom(dt);
    if (creepError !== null) {
      return creepError;
    }
    this.relaxAngle(dt);
    this.liftByShake(dt);
    this.updateSandTop();
    return this.audit(true);
  }

  private applyMotionChannels(dt: number): WaterSandErrorCode | null {
    this.settleSandV2(dt);
    this.percolateHeavyConservative(dt);
    const creepError = this.creepBottom(dt);
    if (creepError !== null) {
      return creepError;
    }
    this.relaxAngle(dt);
    this.liftByShake(dt);
    this.updateSandTop();
    return this.audit(true);
  }

  private updateTilt(dt: number): void {
    const maxStep = this.params.maxTiltOmegaDegPerSec * dt;
    const difference = Math.max(-maxStep, Math.min(maxStep, this.targetTilt - this.effectiveTilt));
    this.effectiveTilt += difference;
    this.effectiveTiltRate = difference / dt;
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
      if (heavyDifference > 0) {
        const flux = Math.min(this.params.heavyDiffusion * dt, heavyDifference / 2,
          this.suspensionCapacity(c));
        this.suspH[c + 1] -= flux;
        this.suspH[c] += flux;
      } else if (heavyDifference < 0) {
        const flux = Math.min(this.params.heavyDiffusion * dt, -heavyDifference / 2,
          this.suspensionCapacity(c + 1));
        this.suspH[c] -= flux;
        this.suspH[c + 1] += flux;
      }
      if (lightDifference > 0) {
        const flux = Math.min(this.params.lightDiffusion * dt, lightDifference / 2,
          this.suspensionCapacity(c));
        this.suspL[c + 1] -= flux;
        this.suspL[c] += flux;
      } else if (lightDifference < 0) {
        const flux = Math.min(this.params.lightDiffusion * dt, -lightDifference / 2,
          this.suspensionCapacity(c + 1));
        this.suspL[c] -= flux;
        this.suspL[c + 1] += flux;
      }
    }
  }

  private suspensionCapacity(column: number): number {
    return Math.max(0, this.waterTop[column] - this.sandTopScratch[column] -
      this.suspH[column] - this.suspL[column]);
  }

  private settleSandV2(dt: number): void {
    for (let c = 0; c < this.cols; c++) {
      if (this.suspH[c] <= EPS && this.suspL[c] <= EPS) {
        continue;
      }
      const depth = Math.max(this.waterTop[c] - this.sandTopScratch[c], EPS);
      const desiredHeavy = this.suspH[c] * Math.min(1, this.params.heavySettleSpeed * dt / depth);
      const desiredLight = this.suspL[c] * Math.min(1, this.params.lightSettleSpeed * dt / depth);
      const requested = desiredHeavy + desiredLight;
      if (this.collectingTotalMotionRequests) {
        this.settleRequestAmounts[c] = requested;
        this.totalMotionBudgetRequested += Math.max(0, requested);
        continue;
      }
      const originalRequested = this.settleRequestAmounts[c];
      if (originalRequested <= EPS) {
        continue;
      }
      const allowed = this.clipSubstepMotion(c, -1, originalRequested);
      const scale = allowed / Math.max(originalRequested, EPS);
      const heavy = desiredHeavy * scale;
      const light = desiredLight * scale;
      this.suspH[c] -= heavy;
      this.mh[c] += heavy;
      this.suspL[c] -= light;
      this.ml[c] += light;
    }
    this.updateSandTop();
  }

  private percolateHeavyConservative(dt: number): void {
    for (let c = 0; c < this.cols; c++) {
      if (this.collectingTotalMotionRequests) {
        const depth = Math.max(this.waterTop[c] - this.sandTopScratch[c], EPS);
        const rawHeavy = this.suspH[c] * Math.min(1, this.params.heavySettleSpeed * dt / depth);
        const rawLight = this.suspL[c] * Math.min(1, this.params.lightSettleSpeed * dt / depth);
        const rawSettled = rawHeavy + rawLight;
        const settleScale = rawSettled <= EPS
          ? 0 : this.settleRequestAmounts[c] / rawSettled;
        const projectedHeavy = this.mh[c] + rawHeavy * settleScale;
        const projectedLight = this.ml[c] + rawLight * settleScale;
        const through = Math.min(projectedHeavy, this.bl[c]);
        const pair = Math.min(projectedHeavy, projectedLight);
        const budget = this.params.percolationRate * dt * (through + pair);
        const requested = Math.min(0.5 * through + 0.5 * pair, budget);
        this.percolationRequestAmounts[c] = requested;
        this.totalMotionBudgetRequested += Math.max(0, requested);
        continue;
      }
      const through = Math.min(this.mh[c], this.bl[c]);
      const pair = Math.min(this.mh[c], this.ml[c]);
      const budget = this.params.percolationRate * dt * (through + pair);
      const requested = Math.min(0.5 * through + 0.5 * pair, budget);
      const originalRequested = this.percolationRequestAmounts[c];
      if (originalRequested <= EPS) {
        continue;
      }
      // P-011 bottom-creep-v2.7: percolation participates in the shared total.
      const allowed = this.clipTotalMotion(requested, originalRequested);
      const q = Math.min(through, 0.5 * through, allowed);
      const r = Math.min(pair, 0.5 * pair, Math.max(0, allowed - q), this.mh[c] - q);
      this.mh[c] -= q + r;
      this.bh[c] += q + r;
      this.bl[c] += r - q;
      this.ml[c] += q - r;
    }
    this.updateSandTop();
  }

  private relaxAngle(dt: number): void {
    const limit = Math.tan(this.params.angleDeg * Math.PI / 180);
    const targetSlope = Math.tan(this.effectiveTilt * Math.PI / 180);
    let maximumDeviation = 0;
    for (let c = 0; c < this.cols - 1; c++) {
      const deviation = this.sandTopScratch[c + 1] - this.sandTopScratch[c] - targetSlope;
      maximumDeviation = Math.max(maximumDeviation, Math.abs(deviation));
    }
    if (maximumDeviation <= limit) {
      return;
    }
    const maxFlux = this.params.maxAvalancheFlux * dt;
    if (this.collectingTotalMotionRequests) {
      for (let index = 0; index < this.cols - 1; index++) {
        const c = index;
        const next = c + 1;
        const deviation = this.sandTopScratch[next] - this.sandTopScratch[c] - targetSlope;
        if (Math.abs(deviation) <= limit) {
          continue;
        }
        const donor = deviation > 0 ? next : c;
        const receiver = deviation > 0 ? c : next;
        const excess = Math.abs(deviation) - limit;
        const receiverSuspension = this.suspH[receiver] + this.suspL[receiver];
        const receiverCapacity = this.waterTop[receiver] > EPS
          ? this.waterTop[receiver] - this.sandTopScratch[receiver] - receiverSuspension
          : this.capacity - this.sandTopScratch[receiver];
        const naturalAmount = Math.min(excess / 2, maxFlux,
          this.sandTopScratch[donor], receiverCapacity);
        this.totalMotionBudgetRequested += Math.max(0, naturalAmount);
        if (naturalAmount > EPS && this.slipRequestCount < this.slipRequestAmounts.length) {
          this.slipRequestAmounts[this.slipRequestCount] = naturalAmount;
          this.slipRequestDonors[this.slipRequestCount] = donor;
          this.slipRequestReceivers[this.slipRequestCount] = receiver;
          this.slipRequestCount++;
        }
      }
      return;
    }
    for (let index = 0; index < this.slipRequestCount; index++) {
      const requested = this.slipRequestAmounts[index];
      const donor = this.slipRequestDonors[index];
      const receiver = this.slipRequestReceivers[index];
      const amount = this.clipSubstepMotion(donor, receiver, requested);
      if (amount <= EPS) {
        continue;
      }
      const split = this.splitTopV2(donor, amount);
      this.mh[receiver] += split.heavy;
      this.ml[receiver] += split.light;
      this.updateSandTop();
    }
  }

  private creepBottom(dt: number): WaterSandErrorCode | null {
    const tiltMagnitude = Math.abs(this.targetTilt);
    if (tiltMagnitude >= this.params.bottomCreepDeadZone) {
      let requestRate = this.targetShapeRequestRate;
      if (this.collectingTotalMotionRequests) {
        const residualHeavy = this.phaseHeavyIdeal - this.phaseHeavyActual;
        const residualLight = this.phaseLightIdeal - this.phaseLightActual;
        const residual = Math.max(residualHeavy / P011_TARGET_SHAPE_TAU_H_SECONDS,
          residualLight / P011_TARGET_SHAPE_TAU_L_SECONDS);
        requestRate = Math.max(0, Math.min(this.params.vBottomCreepMax,
          P011_TARGET_SHAPE_K * tiltMagnitude +
          residual));
        this.targetShapeRequestRate = requestRate;
        const idealStep =
          P011_TARGET_SHAPE_K * tiltMagnitude * dt;
        this.targetShapeIdealDisplacement += idealStep;
        this.phaseHeavyIdeal += idealStep;
        this.phaseLightIdeal += idealStep;
      }
      this.lastBottomCreepRate = requestRate;
      if (requestRate > EPS) {
        return this.creepBottomRequests(dt, requestRate);
      }
    }
    this.lastBottomCreepRate = 0;
    this.lastP011DirectionCorrect = true;
    this.targetShapeRequestRate = 0;
    return null;
  }

  private creepBottomRequests(dt: number, requestRate: number,
    ): WaterSandErrorCode | null {
    const requestedDirection = this.targetTilt > 0 ? 1 : -1;
    const holdSeconds = this.params.directionHoldTime / 1000;
    if (this.targetTilt === 0 && this.zeroTargetSince >= 0 &&
      this.simulationTime - this.zeroTargetSince >= holdSeconds) {
      this.bottomCreepDirection = 0;
      this.lastBottomCreepRate = 0;
      this.lastP011DirectionCorrect = true;
      return null;
    }
    if (this.bottomCreepDirection === 0) {
      this.bottomCreepDirection = requestedDirection;
      this.lastBottomDirectionChange = this.simulationTime;
    } else if (requestedDirection !== this.bottomCreepDirection &&
      this.simulationTime - this.lastBottomDirectionChange < holdSeconds) {
      // P-011 v2.8: hold the last valid direction within the debounce window.
    } else if (requestedDirection !== this.bottomCreepDirection) {
      this.bottomCreepDirection = requestedDirection;
      this.lastBottomDirectionChange = this.simulationTime;
    }
    if (this.bottomCreepDirection === 0) {
      this.lastBottomCreepRate = 0;
      this.lastP011DirectionCorrect = true;
      return null;
    }
    this.lastP011DirectionCorrect = requestedDirection === this.bottomCreepDirection;
    const maxFlux = requestRate * dt;
    // P-011 v2.8: positive tilt sends mass left-to-right; negative is symmetric.
    for (let offset = 0; offset < this.cols - 1; offset++) {
      const index = this.bottomCreepDirection > 0 ? this.cols - 2 - offset : offset;
      const left = index;
      const right = index + 1;
      const donor = this.bottomCreepDirection > 0 ? left : right;
      const receiver = this.bottomCreepDirection > 0 ? right : left;
      const donorDeposited = this.bh[donor] + this.bl[donor];
      if (donorDeposited <= EPS) {
        continue;
      }
      const receiverSuspension = this.suspH[receiver] + this.suspL[receiver];
      const receiverCapacity = this.waterTop[receiver] > EPS
        ? this.waterTop[receiver] - this.sandTopScratch[receiver] - receiverSuspension
        : this.capacity - this.sandTopScratch[receiver];
      const requested = Math.min(maxFlux, donorDeposited, receiverCapacity);
      if (this.collectingTotalMotionRequests) {
        this.bottomCreepRequestAmounts[offset] = requested;
        this.bottomCreepRequestDonors[offset] = donor;
        this.bottomCreepRequestReceivers[offset] = receiver;
        this.bottomCreepBudgetRequested += requested;
        continue;
      }
    }
    this.bottomCreepRequestCount = this.cols - 1;
    // P-011 v2.8: allocate the independent budget by request share.
    const independentScale = this.bottomCreepBudgetRequested <= EPS
      ? 0 : Math.min(1, this.bottomCreepBudgetLimit / this.bottomCreepBudgetRequested);
    this.bottomCreepBudgetClipped = Math.max(0,
      this.bottomCreepBudgetRequested - this.bottomCreepBudgetLimit);
    if (this.collectingTotalMotionRequests) {
      this.totalMotionBudgetRequested += Math.min(this.bottomCreepBudgetRequested,
        this.bottomCreepBudgetLimit);
      return null;
    }
    for (let index = 0; index < this.bottomCreepRequestCount; index++) {
      const requested = this.bottomCreepRequestAmounts[index];
      const donor = this.bottomCreepRequestDonors[index];
      const receiver = this.bottomCreepRequestReceivers[index];
      const donorDeposited = this.bh[donor] + this.bl[donor];
      const receiverSuspension = this.suspH[receiver] + this.suspL[receiver];
      const receiverCapacity = this.waterTop[receiver] > EPS
        ? this.waterTop[receiver] - this.sandTopScratch[receiver] - receiverSuspension
        : this.capacity - this.sandTopScratch[receiver];
      const independentGrant = requested * independentScale;
      const amount = this.clipBottomCreepMotion(
        Math.max(0, Math.min(independentGrant, donorDeposited, receiverCapacity)));
      if (amount <= EPS) {
        continue;
      }
      const heavyResidual = this.phaseHeavyIdeal - this.phaseHeavyActual;
      const lightResidual = this.phaseLightIdeal - this.phaseLightActual;
      const split = this.splitBottomPhaseBalanced(donor, amount, heavyResidual, lightResidual);
      if (this.lastBottomSplitError !== null) {
        return this.lastBottomSplitError;
      }
      this.bh[receiver] += split.heavy;
      this.bl[receiver] += split.light;
      // P-011 v2.8: signed global left/right effective displacement.
      const directionSign = this.bottomCreepDirection > 0 ? 1 : -1;
      this.bottomCreepDonorEffectiveDisplacement[donor] -= directionSign * amount;
      this.bottomCreepReceiverEffectiveDisplacement[receiver] += directionSign * amount;
      this.lastBottomFlux += amount;
      this.lastHeavyFlux += split.heavy;
      this.lastLightFlux += split.light;
      this.phaseHeavyActual += directionSign * split.heavy;
      this.phaseLightActual += directionSign * split.light;
      this.updateSandTop();
    }
    return null;
  }

  private liftByShake(dt: number): void {
    const requestEnergy = this.shakeEnergy;
    if (!this.collectingTotalMotionRequests) {
      if (this.shakeInput > EPS) {
        const target = Math.max(0, Math.min(1, this.shakeInput));
        const maximumChange = this.params.shakeRampRate * dt;
        const change = Math.max(-maximumChange, Math.min(maximumChange, target - this.shakeEnergy));
        this.shakeEnergy = Math.max(0, Math.min(1, this.shakeEnergy + change));
      } else {
        this.shakeEnergy *= Math.exp(-this.params.shakeDecay * dt);
      }
    }
    if (requestEnergy <= EPS) {
      return;
    }
    const bandBase = Math.max(0.5, Math.tan(this.params.angleDeg * Math.PI / 180));
    for (let c = 0; c < this.cols; c++) {
      const band = Math.min(this.sandTopScratch[c], bandBase);
      if (band <= EPS) {
        continue;
      }
      const surfaceHeavyShare = this.mh[c] / Math.max(this.mh[c] + this.ml[c], EPS);
      const desired = requestEnergy * band * dt *
        (this.params.heavyLiftRate * surfaceHeavyShare +
          this.params.lightLiftRate * (1 - surfaceHeavyShare));
      const capacity = this.waterTop[c] - this.sandTopScratch[c] + desired -
        this.suspH[c] - this.suspL[c];
      const requested = Math.max(0, Math.min(desired, this.sandTopScratch[c], capacity));
      if (this.collectingTotalMotionRequests) {
        const amount = requested;
        this.riseRequestAmounts[c] = amount;
        this.totalMotionBudgetRequested += Math.max(0, amount);
        continue;
      }
      const originalRequested = this.riseRequestAmounts[c];
      if (originalRequested <= EPS) {
        continue;
      }
      const amount = this.clipSubstepMotion(c, -1, originalRequested);
      if (amount <= EPS) {
        continue;
      }
      const split = this.splitTopV2(c, amount);
      this.suspH[c] += split.heavy;
      this.suspL[c] += split.light;
      this.updateSandTop();
    }
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

  private splitBottom(column: number, amount: number): WaterSandSplitResult {
    const deposited = this.bh[column] + this.bl[column];
    const scale = amount / Math.max(deposited, EPS);
    const heavy = this.bh[column] * scale;
    const light = this.bl[column] * scale;
    this.bh[column] -= heavy;
    this.bl[column] -= light;
    this.updateSandTop();
    return { heavy, light };
  }

  private splitBottomPhaseBalanced(
    donor: number,
    amount: number,
    heavyResidual: number,
    lightResidual: number
  ): WaterSandSplitResult {
    const deposited = Math.max(this.bh[donor] + this.bl[donor], EPS);
    const baseShare = Math.max(P011_HEAVY_SHARE_MIN,
      Math.min(P011_HEAVY_SHARE_MAX, this.bh[donor] / deposited));
    const biasedShare = Math.max(P011_HEAVY_SHARE_MIN,
      Math.min(P011_HEAVY_SHARE_MAX,
        baseShare + P011_HEAVY_SHARE_BIAS * (heavyResidual - lightResidual)));
    let heavy = Math.min(amount * biasedShare, this.bh[donor], amount);
    let light = Math.min(amount - heavy, this.bl[donor]);
    let remainder = amount - heavy - light;
    const extraHeavy = Math.min(remainder, this.bh[donor] - heavy);
    heavy += extraHeavy;
    remainder -= extraHeavy;
    const extraLight = Math.min(remainder, this.bl[donor] - light);
    light += extraLight;
    remainder -= extraLight;
    if (remainder > EPS) {
      this.lastBottomSplitError = WaterSandErrorCode.CAPACITY_OVERFLOW;
      return { heavy: 0, light: 0 };
    }
    this.bh[donor] -= heavy;
    this.bl[donor] -= light;
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

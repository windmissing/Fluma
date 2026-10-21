/**
 * WaterSandField - P-003/P-005 scene 2 three-phase column model, algorithm v2.2.
 * Canonical ledger: waterTop/bh/bl/mh/ml/suspH/suspL; hs/hl/sandTop are derived.
 */
import {
  P011NaturalObservationAudit,
  P011NaturalObservationBudgets,
  P011NaturalObservationExecutedThresholds,
  P011NaturalObservationProbe,
  P011NaturalObservationRunRole,
  P011NaturalObservationState,
  P011RelaxHysteresisDecisionSample,
  P011_CORRECTION_ROUTE_ID,
  P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION,
  P011_NATURAL_OBSERVATION_ROUTE_ID,
  p011NaturalObservationPhaseForCommand,
  p011NaturalObservationPhaseForV3Segment
} from './P011NaturalObservation';
import { P011_SCHEDULE_DRIFT_MARKER_MS } from './P011SchedulePlayer';
import {
  P011ChannelPlan,
  P011ExistingBudgetSnapshot,
  P011ExistingChannelId,
  P011ExistingChannelRateCapRemaining,
  P011ExistingChannelStateSnapshot,
  P011PlanningPassDelegate,
  P011TiltFeasibilityInput,
  P011TiltPlanningPassService,
  P011_PLANNING_PASS_TAN_HARD_MAX,
  p011PreviewCommitConsistent
} from './P011TiltPlanningPass';
import {
  P011GuardLedgerRefund,
  P011GuardOutcome,
  p011GuardAdmit,
  p011GuardAffectedEdgesForDelta,
  p011GuardConstraintsForDelta
} from './P011MaterialGuard';
import {
  P011CorrectionLedger,
  P011CorrectionParams,
  P011CorrectionReservation,
  P011CorrectionStockView,
  p011CorrectionApplyOnce,
  p011CorrectionReserve
} from './P011RelaxCorrection';
import {
  P011RelaxHysteresisThresholds,
  P011RelaxHysteresisNextActiveResult,
  P011_RELAX_HYSTERESIS_VERSION,
  p011RelaxHysteresisNextActive,
  p011RelaxHysteresisRequested,
  p011RelaxHysteresisThresholds as deriveHysteresisThresholds
} from './P011RelaxHysteresis';

/**
 * §24 relax-correction-weighted-v1 view/ledger adapters. ArkTS forbids
 * object literals with function properties, so the stock views and ledger
 * implementations are classes.
 */
class LiveCorrectionStockView implements P011CorrectionStockView {
  private readonly field: WaterSandField;
  constructor(field: WaterSandField) {
    this.field = field;
  }
  get sandTop(): Float64Array {
    return this.field.internalSandTopScratch();
  }
  get effectiveTiltDeg(): number {
    return this.field.internalEffectiveTilt();
  }
  donorStock(column: number): number {
    return this.field.internalSandTopScratch()[column];
  }
  receiverCapacity(column: number): number {
    const suspension = this.field.suspH[column] + this.field.suspL[column];
    return this.field.waterTop[column] > EPS
      ? this.field.waterTop[column] - this.field.internalSandTopScratch()[column] - suspension
      : this.field.capacity - this.field.internalSandTopScratch()[column];
  }
  guardResidual(edge: number): number {
    const s = this.field.internalSandTopScratch();
    return s[edge + 1] - s[edge] -
      Math.tan(this.field.internalEffectiveTilt() * Math.PI / 180);
  }
}

class S0CorrectionStockView implements P011CorrectionStockView {
  private readonly field: WaterSandField;
  constructor(field: WaterSandField) {
    this.field = field;
  }
  get sandTop(): Float64Array {
    return this.field.internalSubstepSandTopBefore();
  }
  get effectiveTiltDeg(): number {
    return this.field.internalEffectiveTilt();
  }
  donorStock(column: number): number {
    return this.field.internalSubstepSandTopBefore()[column];
  }
  receiverCapacity(column: number): number {
    const suspension = this.field.suspH[column] + this.field.suspL[column];
    return this.field.waterTop[column] > EPS
      ? this.field.waterTop[column] - this.field.internalSubstepSandTopBefore()[column] - suspension
      : this.field.capacity - this.field.internalSubstepSandTopBefore()[column];
  }
  guardResidual(edge: number): number {
    const s = this.field.internalSubstepSandTopBefore();
    return s[edge + 1] - s[edge] -
      Math.tan(this.field.internalEffectiveTilt() * Math.PI / 180);
  }
}

class LiveCorrectionLedger implements P011CorrectionLedger {
  private readonly field: WaterSandField;
  constructor(field: WaterSandField) {
    this.field = field;
  }
  debit(donor: number, receiver: number, amount: number): void {
    this.field.substepMotionBudgetRemaining[donor] -= amount;
    this.field.substepMotionBudgetRemaining[receiver] -= amount;
    this.field.substepMotionBudgetUsed[donor] += amount;
    this.field.substepMotionBudgetUsed[receiver] += amount;
    this.field.totalMotionBudgetUsed += amount;
    this.field.totalMotionBudgetRemaining -= amount;
    this.field.correctionBudgetUsed += amount;
    this.field.correctionBudgetRemaining -= amount;
  }
  refund(donor: number, receiver: number, amount: number): void {
    this.field.substepMotionBudgetRemaining[donor] += amount;
    this.field.substepMotionBudgetRemaining[receiver] += amount;
    this.field.substepMotionBudgetUsed[donor] = Math.max(0,
      this.field.substepMotionBudgetUsed[donor] - amount);
    this.field.substepMotionBudgetUsed[receiver] = Math.max(0,
      this.field.substepMotionBudgetUsed[receiver] - amount);
    this.field.totalMotionBudgetRemaining += amount;
    this.field.totalMotionBudgetUsed = Math.max(0,
      this.field.totalMotionBudgetUsed - amount);
    this.field.correctionBudgetRemaining += amount;
    this.field.correctionBudgetUsed = Math.max(0,
      this.field.correctionBudgetUsed - amount);
  }
}

/**
 * Shadow ledger for the preview mirror: local arrays by reference plus
 * total/correction scalars captured through accessors (ArkTS closures in
 * constructors are fine; object-literal function properties are not).
 */
class ShadowCorrectionLedger implements P011CorrectionLedger {
  private readonly localRemaining: Float64Array;
  private readonly localUsed: Float64Array;
  private readonly readTotal: () => number;
  private readonly addTotal: (delta: number) => void;
  private readonly readCorrection: () => number;
  private readonly addCorrection: (delta: number) => void;
  constructor(localRemaining: Float64Array, localUsed: Float64Array,
    readTotal: () => number, addTotal: (delta: number) => void,
    readCorrection: () => number, addCorrection: (delta: number) => void) {
    this.localRemaining = localRemaining;
    this.localUsed = localUsed;
    this.readTotal = readTotal;
    this.addTotal = addTotal;
    this.readCorrection = readCorrection;
    this.addCorrection = addCorrection;
  }
  debit(donor: number, receiver: number, amount: number): void {
    this.localRemaining[donor] -= amount;
    this.localRemaining[receiver] -= amount;
    this.localUsed[donor] += amount;
    this.localUsed[receiver] += amount;
    this.addTotal(-amount);
    this.addCorrection(-amount);
  }
  refund(donor: number, receiver: number, amount: number): void {
    this.localRemaining[donor] += amount;
    this.localRemaining[receiver] += amount;
    this.localUsed[donor] = Math.max(0, this.localUsed[donor] - amount);
    this.localUsed[receiver] = Math.max(0, this.localUsed[receiver] - amount);
    this.addTotal(amount);
    this.addCorrection(amount);
  }
}

/** Shadow-array stock view for the preview mirror's correction slot. */
class ShadowCorrectionStockView implements P011CorrectionStockView {
  private readonly field: WaterSandField;
  private readonly shadowS: Float64Array;
  private readonly shadowSuspH: Float64Array;
  private readonly shadowSuspL: Float64Array;
  constructor(field: WaterSandField, shadowS: Float64Array,
    shadowSuspH: Float64Array, shadowSuspL: Float64Array) {
    this.field = field;
    this.shadowS = shadowS;
    this.shadowSuspH = shadowSuspH;
    this.shadowSuspL = shadowSuspL;
  }
  get sandTop(): Float64Array {
    return this.shadowS;
  }
  get effectiveTiltDeg(): number {
    return this.field.internalEffectiveTilt();
  }
  donorStock(column: number): number {
    return this.shadowS[column];
  }
  receiverCapacity(column: number): number {
    const suspension = this.shadowSuspH[column] + this.shadowSuspL[column];
    return this.field.waterTop[column] > EPS
      ? this.field.waterTop[column] - this.shadowS[column] - suspension
      : this.field.capacity - this.shadowS[column];
  }
  guardResidual(edge: number): number {
    return this.shadowS[edge + 1] - this.shadowS[edge] -
      Math.tan(this.field.internalEffectiveTilt() * Math.PI / 180);
  }
}

/** P-011 v2.14 §20: scalar state saved/restored around planning previews. */
class PlanningScalars {
  targetShapeRequestRate = 0;
  targetShapeIdealDisplacement = 0;
  phaseHeavyIdeal = 0;
  phaseLightIdeal = 0;
  bottomCreepDirection = 0;
  lastBottomDirectionChange = 0;
  lastP011DirectionCorrect = true;
  lastBottomCreepRate = 0;
  effectiveTilt = 0;
  effectiveTiltRate = 0;
  totalMotionBudgetRequested = 0;
  bottomCreepBudgetRequested = 0;
}

/**
 * Class-based planning-pass delegate (ArkTS forbids untyped object literals
 * with function properties). Forwards to WaterSandField's authoritative
 * collect machinery; declared after WaterSandField to avoid forward refs.
 */
class WaterSandFieldPlanningDelegate implements P011PlanningPassDelegate {
  private readonly field: WaterSandField;

  constructor(field: WaterSandField) {
    this.field = field;
  }

  previewAt(thetaCandidate: number): P011ChannelPlan[] | null {
    return this.field.internalPreviewChannelPlansAt(thetaCandidate);
  }

  sandTopSnapshot(): Float64Array {
    return this.field.internalSandTopSnapshot();
  }

  preexistingViolation(hardMax: number): boolean {
    return this.field.internalPreexistingViolation(hardMax);
  }
}

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
  TILT_PLANNING_BLOCKED = 5,
  PREVIEW_COMMIT_MISMATCH = 6,
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

/** P-011 v2.14 §20: last committed substep's planning-pass diagnostics. */
export interface P011PlanningTelemetry {
  status: string | null;
  blockedReason: string | null;
  passCount: number;
  fallbackUsed: boolean;
  thetaRequested: number;
  limitingEdge: number | null;
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
  waterTop: Float64Array;
  private bh: Float64Array;
  private bl: Float64Array;
  private mh: Float64Array;
  private ml: Float64Array;
  suspH: Float64Array;
  suspL: Float64Array;
  private sandTopScratch: Float64Array;
  private targetTopScratch: Float64Array;
  private errorScratch: Float64Array;
  private smoothScratch: Float64Array;
  substepMotionBudgetRemaining: Float64Array;
  substepMotionBudgetUsed: Float64Array;
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
  totalMotionBudgetRemaining = 0;
  totalMotionBudgetUsed = 0;
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
  private naturalObservationProbe: P011NaturalObservationProbe | null = null;
  private naturalObservationStateBefore: P011NaturalObservationState | null = null;
  // P-011 v2.14 §20: planning pass state. The preview flag detaches the probe
  // and suppresses telemetry so discarded passes leave zero side effects.
  private planningPassService = new P011TiltPlanningPassService();
  private planningPreviewActive = false;
  private lastPlanningStatus: string | null = null;
  private lastPlanningBlockedReason: string | null = null;
  private lastPlanningPassCount = 0;
  private lastPlanningFallbackUsed = false;
  private lastPlanningThetaRequested = 0;
  private lastPlanningLimitingEdge: number | null = null;
  private lastPlanningCommittedPlans: P011ChannelPlan[] | null = null;
  private lastPlanningCommittedTheta = 0;
  // §24 relax-correction-weighted-v1 ledger: independent reservation-first
  // sub-channel for the relax slot. Lifecycle aligns with
  // beginSubstepMotionBudget / beginMotionApplication; grants are NOT
  // scaled by totalMotionBudgetScale (reservation semantics).
  correctionBudgetUsed = 0;
  correctionBudgetRemaining = 0;
  private correctionBudgetRequested = 0;
  private correctionBudgetClipped = 0;
  private correctionReservation: P011CorrectionReservation | null = null;
  private correctionStalledCount = 0;
  private planningDelegate: P011PlanningPassDelegate =
    new WaterSandFieldPlanningDelegate(this);

  internalPreviewChannelPlansAt(thetaCandidate: number): P011ChannelPlan[] | null {
    return this.previewChannelPlansAt(thetaCandidate);
  }

  // §24 correction adapter accessors (same-file classes cannot touch
  // private members under ArkTS class-level privacy).
  internalSandTopScratch(): Float64Array {
    return this.sandTopScratch;
  }

  internalEffectiveTilt(): number {
    return this.effectiveTilt;
  }

  internalSubstepSandTopBefore(): Float64Array {
    return this.substepSandTopBefore;
  }

  internalSandTopSnapshot(): Float64Array {
    const copy = new Float64Array(this.cols);
    copy.set(this.substepSandTopBefore);
    return copy;
  }

  internalPreexistingViolation(hardMax: number): boolean {
    for (let c = 0; c + 1 < this.cols; c++) {
      const residual = this.substepSandTopBefore[c + 1] - this.substepSandTopBefore[c] -
        Math.tan(this.effectiveTilt * Math.PI / 180);
      if (Math.abs(residual) > hardMax + 1e-12) {
        return true;
      }
    }
    return false;
  }
  private naturalObservationPhysicsDt = 0;
  private naturalObservationTotalRequested = 0;
  private naturalObservationRelaxCandidateCount = 0;
  private naturalObservationRelaxRequestedTotal = 0;
  private naturalObservationRunRole: P011NaturalObservationRunRole | null = null;
  private naturalObservationThresholdCalibrationId: string | null = null;
  private naturalObservationThetaStartDeg: number | null = null;
  private naturalObservationThetaStopDeg: number | null = null;
  private naturalObservationScheduleId: string | null = null;
  // D-1 explicit route isolation: the physics-apply route binding (null or
  // natural-relax-hysteresis-v1 keeps the legacy relaxAngle apply).
  private naturalObservationRouteId: string | null = null;
  private naturalObservationSliding: Uint8Array;
  private naturalObservationSlidingBefore: Uint8Array;
  private naturalObservationSlidingAfter: Uint8Array;
  private naturalObservationSlipSeen: Uint8Array;
  // ── GAP-1 dual-threshold state machine state ──
  // Physically effective sliding[] (drives relax eligibility; distinct from
  // the observation-shadow arrays above which only mirror it into records).
  // reset/initialize clears it; budget clipping never does (§19.4-2/3).
  private relaxSliding: Uint8Array;
  private relaxHysteresisThresholds: P011RelaxHysteresisThresholds | null = null;
  // ── GAP-4 attempt audit ──
  private p011AttemptCounter = 0;
  // ── GAP-5 executed identity (bound once per run via want) ──
  private naturalObservationExecSourceCommit: string | null = null;
  private naturalObservationExecSourceFileSha256: string | null = null;
  private naturalObservationExecBuildSha256: string | null = null;
  // ── GAP-2 v3 schedule binding for the CURRENT command ──
  private p011CurrentLegId: string | null = null;
  private p011CurrentSegmentId: string | null = null;
  private p011CurrentPhaseKindV3: 'platform' | 'ramp' | null = null;
  // §26.4 decision 2: schedule player dispatch drift on the attempt layer.
  private p011ScheduleDriftMs: number | null = null;
  private p011ScheduleDriftExceeded = false;
  // ── GAP-4 attempt outcome of the current substep ──
  private p011CurrentAttemptId: string | null = null;
  private p011CurrentCommitStatus: 'committed' | 'blocked' | 'rolled_back'
    | 'rejected' | null = null;
  private p011CurrentRollbackReason: string | null = null;

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
    this.naturalObservationSliding = new Uint8Array(cols - 1);
    this.naturalObservationSlidingBefore = new Uint8Array(cols - 1);
    this.naturalObservationSlidingAfter = new Uint8Array(cols - 1);
    this.naturalObservationSlipSeen = new Uint8Array(cols - 1);
    this.relaxSliding = new Uint8Array(cols - 1);
    this.relaxDecisionOrdinals = new Int32Array(cols - 1);
    this.backup = this.createEmptySnapshot();
    this.motionSnapshot = this.createEmptySnapshot();
    this.planningPassService.attach(this.planningDelegate);
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
    this.naturalObservationSliding.fill(0);
    this.naturalObservationSlidingBefore.fill(0);
    this.naturalObservationSlidingAfter.fill(0);
    this.naturalObservationSlipSeen.fill(0);
    // §19.4-3: reset / geometry rebuild clears the physical sliding state.
    this.relaxSliding.fill(0);
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
      const planningError = this.planAndCommitTilt(substepDt);
      let errorCode: WaterSandErrorCode | null = planningError;
      if (errorCode !== null) {
        // GAP-4: a BLOCKED substep still emits an auditable attempt record —
        // begin/finish the observation with the blocked outcome so the
        // ledger carries attemptId/commitStatus='blocked'/rollbackReason and
        // the v3 checker can verify "BLOCKED attempts also land in the
        // ledger". State is identical before/after (nothing committed).
        this.beginNaturalObservation(substepDt);
      }
      if (errorCode === null) {
        this.beginNaturalObservation(substepDt);
        errorCode = this.substep(substepDt);
      }
      if (errorCode === null) {
        // §20.6.4: verify the authoritative collect against the preview plans
        // before any material is applied; mismatch freezes the substep.
        errorCode = this.verifyPreviewCommitConsistency(substepDt);
      }
      if (errorCode === null) {
        this.restoreMotionSnapshot();
        this.lastBottomFlux = 0;
        // D-1 route isolation: the §24 correction reservation (and the
        // B − R_limit fold of the ordinary budget) only exists on runs that
        // explicitly bind P011_CORRECTION_ROUTE_ID with the state machine
        // armed. Every other run uses the pre-correction budget scaling,
        // bit-for-bit as 5e60f2d.
        let scale: number;
        if (this.correctionRouteActive()) {
          this.computeCorrectionReservation(substepDt);
          const ordinaryBudget = this.totalMotionBudgetLimit -
            this.correctionReservation!.rLimit;
          scale = this.totalMotionBudgetRequested <= ordinaryBudget
            ? 1 : ordinaryBudget / Math.max(this.totalMotionBudgetRequested, EPS);
        } else {
          this.correctionReservation = null;
          this.correctionStalledCount = 0;
          scale = this.totalMotionBudgetRequested <= this.totalMotionBudgetLimit
            ? 1 : this.totalMotionBudgetLimit / this.totalMotionBudgetRequested;
        }
        if (this.naturalObservationProbe !== null) {
          this.naturalObservationTotalRequested = this.totalMotionBudgetRequested;
        }
        this.beginMotionApplication(substepDt, scale);
        errorCode = this.applyMotionChannels(substepDt);
        if (errorCode === null && this.naturalObservationProbe !== null) {
          // Manager ruling A-1 ledger reconciliation: clipped must satisfy
          // abs(clipped - (requested - used)) <= 1e-9 per substep. Recompute
          // both ledger fields from their same-source request/used pairs so
          // the reported amounts cover every clip layer (independent bottom
          // scale, stock/capacity caps, total proportional scale). This only
          // rewrites telemetry fields; budget ceilings and physics grants
          // are untouched.
          this.bottomCreepBudgetClipped = Math.max(0,
            this.bottomCreepBudgetRequested - this.bottomCreepBudgetUsed);
          this.totalMotionBudgetClipped = Math.max(0,
            this.naturalObservationTotalRequested - this.totalMotionBudgetUsed);
        }
      }
      if (errorCode !== null) {
        this.finishNaturalObservation(errorCode);
        this.restoreBackup();
        // §20.4: a planning BLOCKED substep marks the run invalid via
        // ok=false, but unlike transaction errors it must NOT freeze the
        // field — the caller may choose an explicit invalid continuation
        // (the blocked evidence stays in lastPlanning* telemetry).
        if (errorCode !== WaterSandErrorCode.TILT_PLANNING_BLOCKED &&
          errorCode !== WaterSandErrorCode.PREVIEW_COMMIT_MISMATCH) {
          this.frozen = true;
        }
        this.lastError = errorCode;
        result.ok = false;
        result.errorCode = errorCode;
        result.errorColumn = this.firstInvalidColumn();
        break;
      }
      this.finishNaturalObservation(null);
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

  setNaturalObservationProbe(probe: P011NaturalObservationProbe | null): void {
    this.naturalObservationProbe = probe;
    this.naturalObservationStateBefore = null;
  }

  setNaturalObservationRunMetadata(runRole: P011NaturalObservationRunRole | null,
    thresholdCalibrationId: string | null, thetaStartDeg: number | null,
    thetaStopDeg: number | null): void {
    this.naturalObservationRunRole = runRole;
    this.naturalObservationThresholdCalibrationId = thresholdCalibrationId;
    this.naturalObservationThetaStartDeg = thetaStartDeg;
    this.naturalObservationThetaStopDeg = thetaStopDeg;
    this.applyRelaxHysteresisConfiguration();
  }

  /**
   * GAP-5: bind the execution identity (source commit / per-file SHA-256 /
   * build SHA-256) that executedThresholds reports. Callers supply these
   * from the run manifest; the state machine version is the compiled-in
   * P011_RELAX_HYSTERESIS_VERSION constant.
   */
  setNaturalObservationExecutionIdentity(sourceCommit: string | null,
    sourceFileSha256: string | null, buildSha256: string | null): void {
    this.naturalObservationExecSourceCommit = sourceCommit;
    this.naturalObservationExecSourceFileSha256 = sourceFileSha256;
    this.naturalObservationExecBuildSha256 = buildSha256;
  }

  /**
   * GAP-2: bind the v3 schedule segment identity of the CURRENT command.
   * Called from the want path (SandCanvas) when a schedule-driven command is
   * consumed; legId/segmentId/phaseKind flow into every ledger context of
   * the substeps executing that command. Nulls clear the binding.
   */
  setP011CurrentCommandSegment(legId: string | null, segmentId: string | null,
    phaseKind: 'platform' | 'ramp' | null): void {
    this.p011CurrentLegId = legId;
    this.p011CurrentSegmentId = segmentId;
    this.p011CurrentPhaseKindV3 = phaseKind;
  }

  /**
   * §26.4 decision 2: record the driving schedule command's dispatch drift
   * (actual − scheduled, monotonic clock) on the attempt layer. Null clears
   * the binding (single-command / gesture substeps carry null). The player
   * records, never chases; the checker consumes the field under the
   * existing input-timing gates.
   */
  setP011ScheduleDrift(driftMs: number | null): void {
    this.p011ScheduleDriftMs = driftMs;
    this.p011ScheduleDriftExceeded = driftMs !== null &&
      driftMs > P011_SCHEDULE_DRIFT_MARKER_MS;
  }

  /**
   * GAP-1: derive the state machine thresholds from the configured candidate
   * (thetaStartDeg/thetaStopDeg). When both are present the dual-threshold
   * state machine is armed; when absent (legacy run) it stays off and the
   * pre-v3.1 relax behavior (single tan(angleDeg) bound + requested>EPS
   * active proxy) runs bit-for-bit unchanged. Parameter invariant violations
   * throw P011RelaxHysteresisParamError — construction/config failure, never
   * a silent default (§19.9). thetaHardMaxDeg is fixed at 30 (tan30 hard
   * gate); no new physics threshold is introduced.
   */
  private applyRelaxHysteresisConfiguration(): void {
    const start = this.naturalObservationThetaStartDeg;
    const stop = this.naturalObservationThetaStopDeg;
    if (start === null || stop === null) {
      this.relaxHysteresisThresholds = null;
      return;
    }
    this.relaxHysteresisThresholds = deriveHysteresisThresholds({
      thetaStartDeg: start,
      thetaStopDeg: stop,
      thetaHardMaxDeg: 30
    });
  }

  /** True when the dual-threshold state machine drives relax eligibility. */
  relaxHysteresisEnabled(): boolean {
    return this.relaxHysteresisThresholds !== null;
  }

  /**
   * GAP-5: executedThresholds as the state machine actually read them (the
   * same object the transition function uses), with implementation identity.
   * Null on legacy runs.
   */
  private executedThresholdsForLedger(): P011NaturalObservationExecutedThresholds | null {
    const thresholds = this.relaxHysteresisThresholds;
    if (thresholds === null) {
      return null;
    }
    const start = this.naturalObservationThetaStartDeg as number;
    const stop = this.naturalObservationThetaStopDeg as number;
    return {
      thetaStartDeg: start,
      thetaStopDeg: stop,
      slopeStart: thresholds.slopeStart,
      slopeStop: thresholds.slopeStop,
      stateMachineVersion: P011_RELAX_HYSTERESIS_VERSION,
      sourceCommit: this.naturalObservationExecSourceCommit ?? '',
      sourceFileSha256: this.naturalObservationExecSourceFileSha256 ?? '',
      buildSha256: this.naturalObservationExecBuildSha256 ?? ''
    };
  }

  // Observation-3 v2 remediation: the command schedule identifier selects the
  // phase-name mapping table (nine-phase 30° v2 vs legacy seven-phase 45° v1).
  // Null keeps the legacy mapping (backward compatible).
  setNaturalObservationScheduleId(scheduleId: string | null): void {
    this.naturalObservationScheduleId = scheduleId;
  }

  /**
   * D-1 (Option A + explicit route isolation): bind the physics-apply route
   * this run executes. Only an explicit P011_CORRECTION_ROUTE_ID binding —
   * on top of the armed dual-threshold state machine — admits the §24
   * correction allocator (applyCorrectionSlot). Every other value (or null)
   * keeps the pre-correction relaxAngle apply semantics for ALL runs,
   * including candidate natural-relax-hysteresis-v1 runs: the v3 identity
   * is not an implicit correction switch.
   */
  setNaturalObservationRouteId(routeId: string | null): void {
    this.naturalObservationRouteId = routeId;
  }

  getNaturalObservationRouteId(): string | null {
    return this.naturalObservationRouteId;
  }

  /** True only when BOTH gate conditions hold (necessary + sufficient). */
  correctionRouteActive(): boolean {
    return this.relaxHysteresisThresholds !== null &&
      this.naturalObservationRouteId === P011_CORRECTION_ROUTE_ID;
  }

  getNaturalObservationScheduleId(): string | null {
    return this.naturalObservationScheduleId;
  }

  private naturalObservationState(): P011NaturalObservationState {
    this.updateSandTop();
    return {
      bh: Array.from(this.bh),
      bl: Array.from(this.bl),
      mh: Array.from(this.mh),
      ml: Array.from(this.ml),
      suspH: Array.from(this.suspH),
      suspL: Array.from(this.suspL),
      S: Array.from(this.sandTopScratch),
      waterTop: Array.from(this.waterTop)
    };
  }

  private beginNaturalObservation(substepDt: number): void {
    if (this.naturalObservationProbe === null) {
      return;
    }
    this.naturalObservationPhysicsDt = substepDt;
    const state = this.naturalObservationState();
    this.naturalObservationStateBefore = state;
    this.naturalObservationSlidingBefore.set(this.naturalObservationSliding);
    this.naturalObservationSlidingAfter.fill(0);
    this.naturalObservationSlipSeen.fill(0);
    const scheduleId = this.naturalObservationScheduleId;
    const phaseD = scheduleId === 'nine-phase-30-v3'
      ? p011NaturalObservationPhaseForV3Segment(this.p011CurrentSegmentId)
      : p011NaturalObservationPhaseForCommand(this.p011InputSequence,
        this.targetTilt, scheduleId);
    this.naturalObservationProbe.beginSubstep({
      observationRouteId: this.naturalObservationRouteId !== null
        ? this.naturalObservationRouteId : P011_NATURAL_OBSERVATION_ROUTE_ID,
      ledgerSchemaVersion: P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION,
      runId: this.p011RunId,
      frameId: this.p011FrameId,
      substepId: this.p011SubstepId + 1,
      sequence: this.p011SampleSequence + 1,
      physicsTime: nextP011TimeMs(),
      physicsDt: substepDt,
      commandSequence: this.p011InputSequence,
      commandPhaseId: this.commandPhaseId,
      commandPhaseSign: this.commandPhaseSign,
      phaseD: phaseD,
      targetTilt: this.targetTilt,
      effectiveTilt: this.effectiveTilt,
      effectiveTiltRate: this.effectiveTiltRate,
      shakeEnergy: this.shakeEnergy,
      runRole: this.naturalObservationRunRole,
      thresholdCalibrationId: this.naturalObservationThresholdCalibrationId,
      thetaStartDeg: this.naturalObservationThetaStartDeg,
      thetaStopDeg: this.naturalObservationThetaStopDeg,
      transitionTelemetryAvailable: true,
      // Schema v3 supplement: planner visibility per substep.
      thetaAccepted: this.lastPlanningStatus === null
        ? null : this.lastPlanningCommittedTheta,
      requestRejected: this.lastPlanningStatus === null
        ? null : (this.lastPlanningStatus === 'blocked' ||
          this.lastPlanningStatus === 'clamped' ||
          this.lastPlanningStatus === 'held'),
      plannerHold: this.lastPlanningStatus === null
        ? null : this.lastPlanningStatus === 'held',
      // Observation-3 v3 (GAP-2/4/5): real per-step proposal, v3 schedule
      // segment binding, attempt audit identity and executed thresholds.
      thetaRequested: this.lastPlanningThetaRequested,
      legId: this.p011CurrentLegId,
      segmentId: this.p011CurrentSegmentId,
      phaseKindV3: this.p011CurrentPhaseKindV3,
      attemptId: this.p011CurrentAttemptId,
      commitStatus: this.p011CurrentCommitStatus,
      rollbackReason: this.p011CurrentRollbackReason,
      // §26.4 decision 2: schedule drift rides the attempt layer (null on
      // non-schedule-driven substeps).
      scheduleDriftMs: this.p011ScheduleDriftMs,
      scheduleDriftExceeded: this.p011ScheduleDriftExceeded
        ? true : null,
      executedThresholds: this.executedThresholdsForLedger()
    }, state);
    this.naturalObservationRelaxCandidateCount = 0;
    this.naturalObservationRelaxRequestedTotal = 0;
  }

  private naturalObservationBudgets(): P011NaturalObservationBudgets {
    const localUsed = Array.from(this.substepMotionBudgetUsed);
    const localRemaining = Array.from(this.substepMotionBudgetRemaining);
    const localLimit = localUsed.map((used, index) => used + localRemaining[index]);
    return {
      localLimit: localLimit,
      localUsed: localUsed,
      localRemaining: localRemaining,
      bottomRequested: this.bottomCreepBudgetRequested,
      bottomUsed: this.bottomCreepBudgetUsed,
      bottomLimit: this.bottomCreepBudgetLimit,
      bottomClipped: this.bottomCreepBudgetClipped,
      totalRequested: this.naturalObservationTotalRequested,
      totalUsed: this.totalMotionBudgetUsed,
      totalLimit: this.totalMotionBudgetLimit,
      totalRemaining: this.totalMotionBudgetRemaining,
      totalClipped: this.totalMotionBudgetClipped
    };
  }

  private finishNaturalObservation(errorCode: WaterSandErrorCode | null): void {
    const probe = this.naturalObservationProbe;
    const before = this.naturalObservationStateBefore;
    if (probe === null || before === null) {
      return;
    }
    if (errorCode === null) {
      // GAP-1: candidate runs persist the state machine's own array (the
      // machine was advanced during the collect scan); legacy runs keep the
      // requested>EPS mirror semantics.
      if (this.relaxHysteresisThresholds !== null) {
        this.naturalObservationSliding.set(this.relaxSliding);
      } else {
        this.naturalObservationSliding.set(this.naturalObservationSlidingAfter);
      }
    }
    const after = this.naturalObservationState();
    let finite = true;
    let range = true;
    let capacity = true;
    let stock = true;
    let waterBefore = 0;
    let heavyBefore = 0;
    let lightBefore = 0;
    let waterAfter = 0;
    let heavyAfter = 0;
    let lightAfter = 0;
    let maxSubstepBh = 0;
    let maxSubstepBl = 0;
    let maxSandTopSubstepDelta = 0;
    let maxSandTopInvariantError = 0;
    for (let c = 0; c < this.cols; c++) {
      const values = [before.bh[c], before.bl[c], before.mh[c], before.ml[c],
        before.suspH[c], before.suspL[c], before.S[c], before.waterTop[c],
        after.bh[c], after.bl[c], after.mh[c], after.ml[c],
        after.suspH[c], after.suspL[c], after.S[c], after.waterTop[c]];
      for (const value of values) {
        finite = finite && Number.isFinite(value);
        stock = stock && value >= -EPS;
        range = range && value <= this.capacity + EPS;
      }
      capacity = capacity && after.S[c] + after.suspH[c] + after.suspL[c] <= this.capacity + EPS;
      waterBefore += before.waterTop[c];
      heavyBefore += before.bh[c] + before.mh[c] + before.suspH[c];
      lightBefore += before.bl[c] + before.ml[c] + before.suspL[c];
      waterAfter += after.waterTop[c];
      heavyAfter += after.bh[c] + after.mh[c] + after.suspH[c];
      lightAfter += after.bl[c] + after.ml[c] + after.suspL[c];
      maxSubstepBh = Math.max(maxSubstepBh, Math.abs(after.bh[c] - before.bh[c]));
      maxSubstepBl = Math.max(maxSubstepBl, Math.abs(after.bl[c] - before.bl[c]));
      maxSandTopSubstepDelta = Math.max(maxSandTopSubstepDelta,
        Math.abs(after.S[c] - before.S[c]));
      if (c + 1 < this.cols) {
        maxSandTopInvariantError = Math.max(maxSandTopInvariantError,
          Math.abs(after.S[c + 1] - after.S[c] - Math.tan(this.effectiveTilt * Math.PI / 180)));
      }
    }
    range = range && maxSandTopInvariantError <= Math.tan(30 * Math.PI / 180) + EPS;
    const conservation = Math.abs(waterAfter - waterBefore) <= 1e-6 &&
      Math.abs(heavyAfter - heavyBefore) <= 1e-6 &&
      Math.abs(lightAfter - lightBefore) <= 1e-6;
    const budgets = this.naturalObservationBudgets();
    let maxLocalBudgetExcess = 0;
    for (let c = 0; c < this.cols; c++) {
      maxLocalBudgetExcess = Math.max(maxLocalBudgetExcess,
        budgets.localUsed[c] - budgets.localLimit[c]);
    }
    const audit: P011NaturalObservationAudit = {
      finite: finite,
      range: range,
      capacity: capacity,
      stock: stock,
      conservation: conservation,
      waterTotalBefore: waterBefore,
      waterTotalAfter: waterAfter,
      heavyTotalBefore: heavyBefore,
      heavyTotalAfter: heavyAfter,
      lightTotalBefore: lightBefore,
      lightTotalAfter: lightAfter,
      maxSubstepBh: maxSubstepBh,
      maxSubstepBl: maxSubstepBl,
      maxSandTopSubstepDelta: maxSandTopSubstepDelta,
      maxSandTopInvariantError: maxSandTopInvariantError,
      bottomBudgetExcess: Math.max(0, budgets.bottomUsed - budgets.bottomLimit),
      totalBudgetExcess: Math.max(0, budgets.totalUsed - budgets.totalLimit),
      maxLocalBudgetExcess: Math.max(0, maxLocalBudgetExcess),
      relaxCandidateCount: this.naturalObservationRelaxCandidateCount,
      relaxTriggered: this.naturalObservationRelaxRequestedTotal > EPS,
      relaxRequestedTotal: this.naturalObservationRelaxRequestedTotal,
      relaxAppliedTotal: 0,
      errorCode: errorCode === null ? null : `${errorCode}`
    };
    probe.finishSubstep(after, budgets, audit);
    this.naturalObservationStateBefore = null;
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
    // GAP-3: decision order is substep-local (decisionOrdinal restarts at 1
    // for every substep; cross-substep continuity is via substepId).
    this.relaxDecisionOrdinals.fill(0);
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
    // D-1 route isolation: the correction sub-ledgers are substep-local and
    // only meaningful on explicitly-routed correction runs; reset them every
    // substep either way.
    this.correctionBudgetUsed = 0;
    this.correctionBudgetRemaining = 0;
    this.correctionBudgetRequested = 0;
    this.correctionBudgetClipped = 0;
    this.correctionReservation = null;
    this.correctionStalledCount = 0;
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
    // D-1 route isolation: the §24 reservation-first correction sub-channel
    // replaces the relax slot's budget allocation ONLY on runs that
    // explicitly bind P011_CORRECTION_ROUTE_ID with the state machine armed
    // (necessary + sufficient, manager ruling 2026-10-21). Every other run —
    // legacy defaults, SLIP verification, observation ①b, and v3 candidate
    // natural-relax-hysteresis-v1 runs — keeps the pre-correction
    // relaxAngle apply semantics (bit-for-bit 5e60f2d).
    if (this.correctionRouteActive()) {
      this.applyCorrectionSlot(dt);
    } else {
      this.relaxAngle(dt);
    }
    this.liftByShake(dt);
    this.updateSandTop();
    return this.audit(true);
  }

  /**
   * §24 computeCorrectionReservation: on the candidate-angle snapshot (post
   * restoreMotionSnapshot, pre beginMotionApplication) compute the
   * independent correction reservation — candidates, R_limit, per-column
   * P_i and the frozen execution order. Ordinary channels then see a
   * folded total budget B − R_limit via the scale in the step loop.
   */
  private computeCorrectionReservation(substepDt: number): void {
    this.correctionReservation = null;
    this.correctionStalledCount = 0;
    const hysteresis = this.relaxHysteresisThresholds;
    const params: P011CorrectionParams = {
      // GAP-1: candidate runs use the state machine's STOP bound as the
      // correction channel's residual bound (§19.3 requested formula);
      // legacy keeps tan(angleDeg).
      relaxBoundSlope: hysteresis !== null
        ? hysteresis.slopeStop
        : Math.tan(this.params.angleDeg * Math.PI / 180),
      maxAvalancheFluxDt: this.params.maxAvalancheFlux * substepDt,
      localBudget: this.params.vSandTopMax * substepDt,
      totalBudget: this.params.totalMotionBudget * substepDt,
      dt: substepDt,
      slidingEligible: hysteresis !== null ? this.relaxSliding : null
    };
    const view = this.correctionStockView();
    const reservation = p011CorrectionReserve(view, params,
      this.materialGuardHardMax(), this.totalMotionBudgetRequested);
    this.correctionReservation = reservation;
    this.correctionBudgetRequested = reservation.rLimit;
    this.correctionBudgetRemaining = reservation.rLimit;
    this.correctionBudgetUsed = 0;
    this.correctionBudgetClipped = 0;
  }

  /** Live-field stock view for the reservation and the apply slot. */
  private correctionStockView(): P011CorrectionStockView {
    return new LiveCorrectionStockView(this);
  }

  /**
   * §24 applyCorrectionSlot: replaces relaxAngle's budget allocation in the
   * apply chain. Executes the frozen (−w, edgeId) order; every candidate
   * re-derives direction/e/D/K and stock on the CURRENT field, debits
   * local+total+correction ledgers, admits via the shared three-edge
   * closed-form guard, refunds the rejected part, and applies the accepted
   * material move with splitTopV2 semantics (mh/ml proportional mixed
   * slice first, then bl/bh fallbacks) so updateSandTop recomputes
   * bit-identically to the preview mirror.
   */
  private applyCorrectionSlot(substepDt: number): void {
    const reservation = this.correctionReservation;
    if (reservation === null || reservation.candidates.length === 0) {
      this.recordNaturalRelaxInactiveEdges();
      this.recordNaturalRelaxSummary(0);
      return;
    }
    const params: P011CorrectionParams = {
      relaxBoundSlope: Math.tan(this.params.angleDeg * Math.PI / 180),
      maxAvalancheFluxDt: this.params.maxAvalancheFlux * substepDt,
      localBudget: this.params.vSandTopMax * substepDt,
      totalBudget: this.params.totalMotionBudget * substepDt,
      dt: substepDt
    };
    const view = this.correctionStockView();
    const ledger: P011CorrectionLedger = new LiveCorrectionLedger(this);
    const hysteresisActive = this.relaxHysteresisThresholds !== null;
    let appliedTotal = 0;
    let stalled = 0;
    for (const index of reservation.order) {
      const candidate = reservation.candidates[index];
      const outcome = p011CorrectionApplyOnce(reservation, index, view,
        params, this.materialGuardHardMax(), ledger);
      if (outcome.cancelled || outcome.reason !== null) {
        if (outcome.reason === 'NO_POSITIVE_LOCAL_CONTRIBUTION' ||
          outcome.reason === 'DIRECTION_REVERSED' ||
          outcome.reason === 'GUARD_REJECTED') {
          stalled += 1;
        }
      }
      const edge = candidate.edge;
      const activeBefore = hysteresisActive
        ? (this.relaxSliding[edge] !== 0)
        : this.naturalObservationSlidingBefore[edge] !== 0;
      this.naturalObservationSlipSeen[edge] = 1;
      // Transition semantics: on candidate runs the state machine owns the
      // state (a budget-limited or guard-rejected edge STAYS active — the
      // next collect boundary re-evaluates; §19.5 "budget-limited active
      // edges stay active"). Legacy keeps the requested>0 <-> requested=0
      // contract (obs①b fix B).
      const activeAfter = hysteresisActive ? activeBefore : candidate.p > EPS;
      this.naturalObservationSlidingAfter[edge] = activeAfter ? 1 : 0;
      if (outcome.accepted > EPS) {
        const donor = candidate.donor;
        const receiver = candidate.receiver;
        // splitTopV2-exact material write (proportional mixed slice first).
        const mixAvail = this.mh[donor] + this.ml[donor];
        const fromMix = Math.min(outcome.accepted, mixAvail);
        const mixScale = fromMix / Math.max(mixAvail, EPS);
        let heavy = this.mh[donor] * mixScale;
        let light = this.ml[donor] * mixScale;
        let remaining = outcome.accepted - fromMix;
        const fromLight = Math.min(remaining, this.bl[donor]);
        light += fromLight;
        remaining -= fromLight;
        const fromHeavy = Math.min(remaining, this.bh[donor]);
        heavy += fromHeavy;
        const donorBhBefore = this.bh[donor];
        const donorBlBefore = this.bl[donor];
        const donorMhBefore = this.mh[donor];
        const donorMlBefore = this.ml[donor];
        this.mh[donor] -= this.mh[donor] * mixScale;
        this.ml[donor] -= this.ml[donor] * mixScale;
        this.bl[donor] -= fromLight;
        this.bh[donor] -= fromHeavy;
        this.mh[receiver] += heavy;
        this.ml[receiver] += light;
        appliedTotal += outcome.accepted;
        this.updateSandTop();
        if (this.naturalObservationProbe !== null) {
          this.naturalObservationProbe.recordSlip({
            interfaceIndex: edge,
            donor: donor,
            receiver: receiver,
            directionSign: donor < receiver ? 1 : -1,
            requested: candidate.p,
            allowed: outcome.accepted,
            heavy: heavy,
            light: light,
            activeBefore: activeBefore,
            activeAfter: activeAfter,
            hardGuardRejected: outcome.rejected,
            hardGuardClipped: outcome.rejected > 0 && outcome.accepted > EPS
              ? outcome.rejected : 0,
            hardGuardAvailable: true
          });
          this.recordNaturalTopSplit(edge, donor, outcome.accepted,
            donorBhBefore, donorBlBefore, donorMhBefore, donorMlBefore,
            { heavy: heavy, light: light });
        }
      } else if (candidate.p > EPS && this.naturalObservationProbe !== null) {
        this.naturalObservationProbe.recordSlip({
          interfaceIndex: edge,
          donor: candidate.donor,
          receiver: candidate.receiver,
          directionSign: candidate.donor < candidate.receiver ? 1 : -1,
          requested: candidate.p,
          allowed: 0,
          heavy: 0,
          light: 0,
          activeBefore: activeBefore,
          activeAfter: activeAfter,
          hardGuardRejected: outcome.rejected,
          hardGuardClipped: 0,
          hardGuardAvailable: true
        });
      }
    }
    this.correctionStalledCount = stalled;
    this.recordNaturalRelaxInactiveEdges();
    this.recordNaturalRelaxSummary(appliedTotal);
  }

  private updateTilt(dt: number): void {
    const maxStep = this.params.maxTiltOmegaDegPerSec * dt;
    const difference = Math.max(-maxStep, Math.min(maxStep, this.targetTilt - this.effectiveTilt));
    this.effectiveTilt += difference;
    this.effectiveTiltRate = difference / dt;
  }

  // ==========================================================================
  // P-011 §19.5 pre-commit material guard: runs before each individual
  // material increment of every named writer; clips the granted amount via
  // closed-form edge-interval intersection (legal state keeps every edge
  // within ±H; violating edges are non-worsening) and refunds the rejected
  // part to the local/bottom/total budget ledgers. No ceiling changes, no
  // physics changes, no re-issue.
  // ==========================================================================

  private static readonly MATERIAL_GUARD_HARD_MAX = Math.tan(30 * Math.PI / 180);

  private materialGuardHardMax(): number {
    return WaterSandField.MATERIAL_GUARD_HARD_MAX;
  }

  /** Signed residual of edge e against the committed effectiveTilt. */
  private materialGuardResidual(edge: number): number {
    return this.sandTopScratch[edge + 1] - this.sandTopScratch[edge] -
      Math.tan(this.effectiveTilt * Math.PI / 180);
  }

  /**
   * Build the closed-form constraints for one increment and admit it.
   * deltaColumns maps column -> signed per-unit-grant S change (e.g.
   * {donor: -1, receiver: +1}). Every edge whose slope changes with the
   * grant — for pair moves the up to three adjacent edges, for column
   * moves the up to two adjacent edges (§19.5 table) — is constrained.
   */
  private materialGuardAdmitIncrement(
    requested: number,
    deltaColumns: Map<number, number>): P011GuardOutcome {
    if (requested <= 0) {
      return { accepted: 0, rejected: 0, allLegalBefore: true,
        limitingEdge: null };
    }
    // Shared derivation (§23.1 R-3-a): the apply side and the preview
    // allocator's guard mirror use the same affected-edge set and influence
    // coefficients, so the mirror cannot drift from the real guard.
    const constraints = p011GuardConstraintsForDelta(deltaColumns,
      (edge: number): number => this.materialGuardResidual(edge), this.cols - 1);
    const outcome = p011GuardAdmit(requested, constraints, this.materialGuardHardMax());
    this.recordMaterialGuardEvidence('guard', deltaColumns, requested, outcome);
    return outcome;
  }

  /**
   * GAP-4 (§24.4 affectedEdgeIds): record per-writer guard evidence with
   * the physical-edge mapping. p011GuardAdmit reports limitingEdge as a
   * constraints-array position; the same-order physical ids come from
   * p011GuardAffectedEdgesForDelta (identical set-derivation rule).
   */
  private recordMaterialGuardEvidence(channel: string,
    deltaColumns: Map<number, number>, requested: number,
    outcome: P011GuardOutcome): void {
    const probe = this.naturalObservationProbe;
    if (probe === null || outcome.rejected <= 0) {
      return;
    }
    const affected = p011GuardAffectedEdgesForDelta(deltaColumns, this.cols - 1);
    const limitingPhysical = outcome.limitingEdge !== null &&
      outcome.limitingEdge < affected.length
      ? affected[outcome.limitingEdge] : null;
    probe.recordMaterialGuardEvidence({
      channel: channel,
      limitingEdge: limitingPhysical,
      affectedEdgeIds: affected,
      rejected: outcome.rejected,
      clipped: Math.max(0, requested - outcome.accepted)
    });
  }

  /**
   * Refund the guard-rejected part of a clipped increment to the budget
   * ledgers (§19.5 four-layer write-back): local (both endpoints for pair
   * moves), bottom (creepBottom only), and total. Used ledgers are reduced
   * by the same amount so used+remaining matches the final accepted grant.
   */
  private materialGuardRefund(rejected: number, localColumns: number[],
    bottomChannel: boolean): void {
    if (rejected <= 0) {
      return;
    }
    const refund = new P011GuardLedgerRefund(this.substepMotionBudgetRemaining,
      this.substepMotionBudgetUsed);
    refund.refundLocal(localColumns, rejected);
    this.totalMotionBudgetRemaining += rejected;
    this.totalMotionBudgetUsed = Math.max(0, this.totalMotionBudgetUsed - rejected);
    if (bottomChannel) {
      this.bottomCreepBudgetUsed = Math.max(0, this.bottomCreepBudgetUsed - rejected);
      // bottomCreepBudgetLimit is not a "remaining" ledger: used stays <= limit.
    }
  }

  // ==========================================================================
  // P-011 v2.14 §20: two-pass planning pass with dynamic tilt feasibility.
  // §20.6.1 candidate closure {thetaRequested, theta1, theta0}; §20.6.2 fixed
  // channel order settle→percolate→bottomCreep→relax→lift with (rank, index)
  // lexicographic allocation; §20.6.4 preview/commit share this code path.
  // ==========================================================================

  private planningScalarsBefore: PlanningScalars = new PlanningScalars();
  // GAP-1: the preview's relaxAngle collect pass advances the physical
  // sliding[] state machine; the discarded preview must not leak those
  // transitions into the authoritative collect (preview purity, §20.7-□2).
  private planningSlidingBackup: Uint8Array = new Uint8Array(0);

  private savePlanningScalars(): void {
    this.planningScalarsBefore.targetShapeRequestRate = this.targetShapeRequestRate;
    this.planningScalarsBefore.targetShapeIdealDisplacement = this.targetShapeIdealDisplacement;
    this.planningScalarsBefore.phaseHeavyIdeal = this.phaseHeavyIdeal;
    this.planningScalarsBefore.phaseLightIdeal = this.phaseLightIdeal;
    this.planningScalarsBefore.bottomCreepDirection = this.bottomCreepDirection;
    this.planningScalarsBefore.lastBottomDirectionChange = this.lastBottomDirectionChange;
    this.planningScalarsBefore.lastP011DirectionCorrect = this.lastP011DirectionCorrect;
    this.planningScalarsBefore.lastBottomCreepRate = this.lastBottomCreepRate;
    this.planningScalarsBefore.effectiveTilt = this.effectiveTilt;
    this.planningScalarsBefore.effectiveTiltRate = this.effectiveTiltRate;
    this.planningScalarsBefore.totalMotionBudgetRequested = this.totalMotionBudgetRequested;
    this.planningScalarsBefore.bottomCreepBudgetRequested = this.bottomCreepBudgetRequested;
  }

  private restorePlanningScalars(): void {
    this.targetShapeRequestRate = this.planningScalarsBefore.targetShapeRequestRate;
    this.targetShapeIdealDisplacement = this.planningScalarsBefore.targetShapeIdealDisplacement;
    this.phaseHeavyIdeal = this.planningScalarsBefore.phaseHeavyIdeal;
    this.phaseLightIdeal = this.planningScalarsBefore.phaseLightIdeal;
    this.bottomCreepDirection = this.planningScalarsBefore.bottomCreepDirection;
    this.lastBottomDirectionChange = this.planningScalarsBefore.lastBottomDirectionChange;
    this.lastP011DirectionCorrect = this.planningScalarsBefore.lastP011DirectionCorrect;
    this.lastBottomCreepRate = this.planningScalarsBefore.lastBottomCreepRate;
    this.effectiveTilt = this.planningScalarsBefore.effectiveTilt;
    this.effectiveTiltRate = this.planningScalarsBefore.effectiveTiltRate;
    this.totalMotionBudgetRequested = this.planningScalarsBefore.totalMotionBudgetRequested;
    this.bottomCreepBudgetRequested = this.planningScalarsBefore.bottomCreepBudgetRequested;
  }

  /**
   * One pure preview at thetaCandidate: temporarily detach the probe, set the
   * candidate reference angle, run the authoritative collect phase, then
   * allocate the collected requests against independent budget copies with the
   * fixed channel order. Restores every touched scalar and material field.
   */
  private previewChannelPlansAt(thetaCandidate: number): P011ChannelPlan[] | null {
    const substepDt = this.lastPlanningSubstepDt;
    const probe = this.naturalObservationProbe;
    this.savePlanningScalars();
    // GAP-1 preview purity: snapshot the state machine before the preview's
    // collect pass can advance it; restore in the finally block.
    if (this.planningSlidingBackup.length !== this.relaxSliding.length) {
      this.planningSlidingBackup = new Uint8Array(this.relaxSliding.length);
    }
    this.planningSlidingBackup.set(this.relaxSliding);
    this.naturalObservationProbe = null;
    this.planningPreviewActive = true;
    this.effectiveTilt = thetaCandidate;
    this.effectiveTiltRate = (thetaCandidate - this.planningScalarsBefore.effectiveTilt) /
      Math.max(substepDt, EPS);
    let plans: P011ChannelPlan[] | null = null;
    try {
      // The substep-start backup (saved by the step loop before planning) holds
      // the exact S0 material state; restore from it to discard any preview
      // mutation from an earlier candidate run.
      this.restoreBackup();
      this.updateSandTop();
      this.lastBottomFlux = 0;
      this.lastBottomSplitError = null;
      this.beginSubstepMotionBudget(substepDt, true, 1);
      const collectError = this.substep(substepDt);
      if (collectError === null) {
        plans = this.allocatePreviewPlans(substepDt);
      }
      this.restoreBackup();
      this.updateSandTop();
    } finally {
      this.planningPreviewActive = false;
      this.naturalObservationProbe = probe;
      this.restorePlanningScalars();
      // GAP-1 preview purity: discard the preview's state machine transitions.
      this.relaxSliding.set(this.planningSlidingBackup);
      // Reset every request ledger so the authoritative collect (which runs
      // after planning and no longer passes through beginSubstepMotionBudget)
      // starts from the same clean state the step loop originally created.
      // Without this, incremental counters like slipRequestCount continue from
      // the preview's value and the apply phase double-counts preview requests.
      this.beginSubstepMotionBudget(substepDt, true, 1);
    }
    return plans;
  }

  private lastPlanningSubstepDt = 0;

  /**
   * Shadow allocation of the collected requests — the single §20.6.4 pure
   * planning function shared by the preview and the verify pass, exactly
   * mirroring the authoritative apply-phase semantics (§20.6.2 channel
   * order settle → percolate → bottomCreep → relax → lift, clipSubstepMotion
   * local caps first, then clipTotalMotion's
   * min(locallyAllowed, requestedForTotal * totalScale, totalRemaining),
   * bottomCreep's independent budget first) plus the §19.5 guard mirror per
   * §23.1 R-3-a: every candidate grant is admitted with the same
   * closed-form interval intersection used by materialGuardAdmitIncrement
   * (shared derivation p011GuardConstraintsForDelta), and the rejected part
   * is refunded to the shadow local/total/bottom ledgers exactly like
   * materialGuardRefund.
   *
   * The mirror runs against shadow copies of the material arrays
   * (bh/bl/mh/ml/suspH/suspL plus a recomputed surface) and reproduces the
   * whole apply-chain state evolution, because the real apply's per-channel
   * decisions read evolved state: percolate reads post-settle mh/ml,
   * creepBottom reads post-settle suspension and post-percolate deposited
   * stock, relax's early-return gate reads the post-creep surface, and the
   * guard residuals read whatever the preceding grants have already moved.
   * Split formulas (splitTopV2 / splitBottomPhaseBalanced) are replicated
   * exactly and the shadow surface is recomputed from the layer sums
   * (recomputeShadowS, same formula as updateSandTop) after every material
   * write, because equal-sum shortcuts drift in floating point and flip
   * boundary-equality gates such as relax's maximumDeviation == tan(30°).
   * This closes the R-3 gap (preview promised a repair that the apply's
   * guard clipped away while tilt advanced) at the source: the preview's
   * promised deltaG is bit-identical to what the real apply produces.
   *
   * Both invocations (preview at thetaCandidate, verify at thetaAccepted)
   * read the same post-collect request ledgers and the same S0, so they are
   * bit-identical.
   */
  private allocatePreviewPlans(substepDt: number): P011ChannelPlan[] {
    const cols = this.cols;
    const plans: P011ChannelPlan[] = [];
    const localRemaining = new Float64Array(cols);
    localRemaining.fill(this.params.vSandTopMax * substepDt);
    const localUsed = new Float64Array(cols);
    let bottomRemaining = this.params.vBottomCreepMax * substepDt;
    let totalRemaining = this.params.totalMotionBudget * substepDt;
    const totalRequested = Math.max(0, this.totalMotionBudgetRequested);
    // D-1 route isolation: the §24 correction reservation mirror (and the
    // B − R fold) exists only on explicitly-routed correction runs; every
    // other run mirrors the pre-correction relax slot and total scaling,
    // bit-for-bit as 5e60f2d.
    const correctionRouteActive = this.correctionRouteActive();
    let shadowReservation: P011CorrectionReservation | null = null;
    let correctionParams: P011CorrectionParams | null = null;
    let totalScale: number;
    if (correctionRouteActive) {
      const correctionHysteresis = this.relaxHysteresisThresholds;
      correctionParams = {
        relaxBoundSlope: correctionHysteresis !== null
          ? correctionHysteresis.slopeStop
          : Math.tan(this.params.angleDeg * Math.PI / 180),
        maxAvalancheFluxDt: this.params.maxAvalancheFlux * substepDt,
        localBudget: this.params.vSandTopMax * substepDt,
        totalBudget: this.params.totalMotionBudget * substepDt,
        dt: substepDt,
        slidingEligible: correctionHysteresis !== null ? this.relaxSliding : null
      };
      const headCorrectionView: P011CorrectionStockView =
        new S0CorrectionStockView(this);
      headCorrectionView.sandTop.set(this.substepSandTopBefore);
      shadowReservation = p011CorrectionReserve(headCorrectionView,
        correctionParams, this.materialGuardHardMax(), this.totalMotionBudgetRequested);
      const ordinaryBudget = this.params.totalMotionBudget * substepDt -
        shadowReservation.rLimit;
      totalScale = totalRequested <= ordinaryBudget
        ? 1 : ordinaryBudget / Math.max(totalRequested, EPS);
    } else {
      totalScale = totalRequested <= this.params.totalMotionBudget * substepDt
        ? 1 : this.params.totalMotionBudget * substepDt / Math.max(totalRequested, EPS);
    }

    // Shadow material arrays: initialize from the live post-collect state.
    const shadowS = new Float64Array(cols);
    shadowS.set(this.substepSandTopBefore);
    const shadowBh = new Float64Array(cols);
    shadowBh.set(this.bh);
    const shadowBl = new Float64Array(cols);
    shadowBl.set(this.bl);
    const shadowMh = new Float64Array(cols);
    shadowMh.set(this.mh);
    const shadowMl = new Float64Array(cols);
    shadowMl.set(this.ml);
    const shadowSuspH = new Float64Array(cols);
    shadowSuspH.set(this.suspH);
    const shadowSuspL = new Float64Array(cols);
    shadowSuspL.set(this.suspL);
    const slopeHardMax = this.materialGuardHardMax();
    // Bit-exact updateSandTop mirror: recompute S from the full layer sums,
    // never by delta addition (the real apply's updateSandTop recomputes
    // S = bh+bl+mh+ml after each material write; delta adds accumulate
    // different float rounding and diverge from the real surface on
    // boundary-equality gates like the relax maximumDeviation check).
    const recomputeShadowS = (): void => {
      for (let c = 0; c < cols; c++) {
        shadowS[c] = shadowBh[c] + shadowBl[c] + shadowMh[c] + shadowMl[c];
      }
    };
    // Guard mirror residual against the shadow surface and candidate tilt.
    const guardResidual = (edge: number): number =>
      shadowS[edge + 1] - shadowS[edge] -
      Math.tan(this.effectiveTilt * Math.PI / 180);
    // Guard mirror admission + shadow ledger refund (local/total; the creep
    // block additionally refunds the bottom ledger).
    const guardAdmit = (granted: number,
      deltaColumns: Map<number, number>): number => {
      if (granted <= EPS) {
        return granted;
      }
      const constraints = p011GuardConstraintsForDelta(deltaColumns,
        guardResidual, cols - 1);
      const outcome = p011GuardAdmit(granted, constraints, slopeHardMax);
      const rejected = outcome.rejected;
      if (rejected > 0) {
        for (const column of deltaColumns.keys()) {
          if (column >= 0 && column < cols) {
            localRemaining[column] += rejected;
            localUsed[column] = Math.max(0, localUsed[column] - rejected);
          }
        }
        totalRemaining += rejected;
      }
      return outcome.accepted;
    };

    // clipTotalMotion shadow (two-arg form): allowed =
    // min(locallyAllowed, requestedForTotal * totalScale, totalRemaining).
    const clipTotal = (locallyAllowed: number, requestedForTotal: number): number => {
      const locally = Math.max(0, locallyAllowed);
      const requested = Math.max(0, requestedForTotal);
      const totalAllowed = Math.min(requested * totalScale, totalRemaining);
      const allowed = Math.min(locally, totalAllowed);
      totalRemaining -= allowed;
      return allowed;
    };
    // clipSubstepMotion shadow for single-column channels (target === -1):
    // clipTotalMotion(locallyAllowed, locallyAllowed).
    const clipLocal = (source: number, requested: number): number => {
      const locally = Math.max(0, Math.min(requested, localRemaining[source]));
      const allowed = clipTotal(locally, locally);
      localRemaining[source] -= allowed;
      localUsed[source] += allowed;
      return allowed;
    };
    // clipSubstepMotion shadow for two-column channels (relax).
    const clipLocalPair = (donor: number, receiver: number,
      requested: number): number => {
      const locally = Math.max(0, Math.min(requested, localRemaining[donor],
        localRemaining[receiver]));
      const allowed = clipTotal(locally, locally);
      localRemaining[donor] -= allowed;
      localRemaining[receiver] -= allowed;
      localUsed[donor] += allowed;
      localUsed[receiver] += allowed;
      return allowed;
    };

    const makePlan = (channelId: P011ExistingChannelId,
      requestedTotal: number, grantedTotal: number, bottomUse: number,
      totalUse: number, deltaS: Float64Array): P011ChannelPlan => {
      const clipped = Math.max(0, requestedTotal - grantedTotal);
      const plan: P011ChannelPlan = {
        channelId: channelId,
        requested: { requested: requestedTotal },
        plannedGrant: {
          granted: grantedTotal,
          clipped: clipped,
          rejected: 0
        },
        deltaSandTopByColumn: deltaS,
        budgetUse: {
          local: Math.max(0, grantedTotal),
          bottom: Math.max(0, bottomUse),
          total: Math.max(0, totalUse),
          channelRateCap: 0
        },
        admissibility: requestedTotal <= EPS ? 'admissible'
          : (clipped <= EPS ? 'admissible' : 'clipped')
      };
      return plan;
    };

    // --- settle: column ascending. The apply recomputes desiredHeavy/
    // desiredLight from the current suspension and depth, grants
    // clipSubstepMotion + guard, and splits by scale — mirror all of it and
    // advance the shadow mh/ml/susp stocks (needed by percolate's q/r and
    // creep's receiver-capacity reads). Every settle guard decision sees the
    // channel-start surface (the apply calls updateSandTop only after the
    // whole loop), so shadowS stays frozen during this loop. ---
    {
      const deltaS = new Float64Array(cols);
      let requestedTotal = 0;
      let grantedTotal = 0;
      let totalUse = 0;
      for (let c = 0; c < cols; c++) {
        const originalRequested = this.settleRequestAmounts[c];
        if (originalRequested <= EPS) {
          continue;
        }
        requestedTotal += originalRequested;
        // Re-derive the heavy/light split exactly like the apply branch
        // (desired from the current — post-diffuse — suspension and depth).
        const depth = Math.max(this.waterTop[c] - shadowS[c], EPS);
        const desiredHeavy = shadowSuspH[c] *
          Math.min(1, this.params.heavySettleSpeed * substepDt / depth);
        const desiredLight = shadowSuspL[c] *
          Math.min(1, this.params.lightSettleSpeed * substepDt / depth);
        let allowed = clipLocal(c, originalRequested);
        if (allowed > EPS) {
          const deltaColumns = new Map<number, number>();
          deltaColumns.set(c, 1);
          allowed = guardAdmit(allowed, deltaColumns);
        }
        const scale = allowed / Math.max(originalRequested, EPS);
        const heavy = desiredHeavy * scale;
        const light = desiredLight * scale;
        shadowSuspH[c] -= heavy;
        shadowMh[c] += heavy;
        shadowSuspL[c] -= light;
        shadowMl[c] += light;
        grantedTotal += allowed;
        totalUse += allowed;
        deltaS[c] += allowed;
      }
      // Mirror the apply's end-of-loop updateSandTop (recompute from layer sums).
      recomputeShadowS();
      plans.push(makePlan('settle', requestedTotal, grantedTotal, 0, totalUse, deltaS));
    }
    // --- percolate: S-invariant. The apply re-derives through/pair from the
    // post-settle layers and clips as clipTotalMotion(requested,
    // originalRequested); q/r reshuffle the shadow stocks (bh += q+r,
    // bl += r-q, mh -= q+r, ml += q-r). No guard: ΔS = 0. ---
    {
      const deltaS = new Float64Array(cols);
      let requestedTotal = 0;
      let grantedTotal = 0;
      let totalUse = 0;
      for (let c = 0; c < cols; c++) {
        const originalRequested = this.percolationRequestAmounts[c];
        if (originalRequested <= EPS) {
          continue;
        }
        const through = Math.min(shadowMh[c], shadowBl[c]);
        const pair = Math.min(shadowMh[c], shadowMl[c]);
        const budget = this.params.percolationRate * substepDt * (through + pair);
        const requested = Math.min(0.5 * through + 0.5 * pair, budget);
        requestedTotal += originalRequested;
        const allowed = clipTotal(requested, originalRequested);
        const q = Math.min(through, 0.5 * through, allowed);
        const r = Math.min(pair, 0.5 * pair, Math.max(0, allowed - q),
          shadowMh[c] - q);
        shadowMh[c] -= q + r;
        shadowBh[c] += q + r;
        shadowBl[c] += r - q;
        shadowMl[c] += q - r;
        grantedTotal += allowed;
        totalUse += allowed;
        // deltaS stays zero: percolate is S-invariant in exact arithmetic.
      }
      // The apply phase calls updateSandTop after the percolate loop: the
      // layer reshuffle (mh→bh, bl↔ml) re-rounds the layer sums, so the
      // recomputed surface differs in floating point from the pre-channel
      // value even though the exact sum is unchanged. Recompute here too,
      // or boundary-equality gates downstream (relax maximumDeviation) see
      // a different float than the real apply.
      recomputeShadowS();
      plans.push(makePlan('percolate', requestedTotal, grantedTotal, 0, totalUse, deltaS));
    }
    // --- bottomCreep: independent bottom budget, then shared total, then
    // the guard mirror. Stock reads use the shadow arrays (post-settle
    // suspension, post-percolate deposited stock, shadow surface); the
    // shadow stock advances per applied grant exactly like the apply's
    // per-grant material writes + updateSandTop. ---
    {
      const deltaS = new Float64Array(cols);
      let requestedTotal = 0;
      let grantedTotal = 0;
      let bottomUse = 0;
      let totalUse = 0;
      const independentScale = this.bottomCreepBudgetRequested <= EPS
        ? 0 : Math.min(1, bottomRemaining / Math.max(this.bottomCreepBudgetRequested, EPS));
      // The real creep's split reads phaseHeavyActual/phaseLightActual which
      // its own loop updates after every applied grant; mirror those
      // accumulators so each split sees the same residuals the real loop
      // sees (frozen at-entry values drift the biased share).
      let shadowPhaseHeavyActual = this.phaseHeavyActual;
      let shadowPhaseLightActual = this.phaseLightActual;
      const creepDirectionSign = this.bottomCreepDirection > 0 ? 1 : -1;
      for (let index = 0; index < this.bottomCreepRequestCount; index++) {
        const requested = this.bottomCreepRequestAmounts[index];
        const donor = this.bottomCreepRequestDonors[index];
        const receiver = this.bottomCreepRequestReceivers[index];
        if (requested <= EPS) {
          continue;
        }
        requestedTotal += requested;
        const donorDeposited = shadowBh[donor] + shadowBl[donor];
        const receiverSuspension = shadowSuspH[receiver] + shadowSuspL[receiver];
        const receiverCapacity = this.waterTop[receiver] > EPS
          ? this.waterTop[receiver] - shadowS[receiver] - receiverSuspension
          : this.capacity - shadowS[receiver];
        const independentGrant = Math.max(0, Math.min(requested * independentScale,
          donorDeposited, receiverCapacity, bottomRemaining));
        let allowed = clipTotal(independentGrant, independentGrant);
        if (allowed > EPS) {
          const deltaColumns = new Map<number, number>();
          deltaColumns.set(donor, -1);
          deltaColumns.set(receiver, 1);
          const beforeGuard = allowed;
          allowed = guardAdmit(allowed, deltaColumns);
          if (beforeGuard - allowed > 0) {
            // materialGuardRefund also returns the rejected part to the
            // bottom ledger (creep-only channel).
            bottomRemaining += beforeGuard - allowed;
          }
        }
        bottomRemaining -= allowed;
        grantedTotal += allowed;
        bottomUse += allowed;
        totalUse += allowed;
        deltaS[donor] -= allowed;
        deltaS[receiver] += allowed;
        // Mirror the apply's exact material writes + updateSandTop: the real
        // creep splits the amount via splitBottomPhaseBalanced (phase-biased
        // heavy share clamped to [0.35, 0.65], then proportional fallbacks
        // within the donor's deposited stock), subtracts from the donor's
        // bh/bl and adds to the receiver's bh/bl, then recomputes S from the
        // layer sums. Replicating the exact split keeps the recomputed
        // shadowS bit-identical to the real surface (equal-sum shortcuts
        // drift on boundary-equality gates like the relax maxDeviation).
        const creepDeposited = Math.max(shadowBh[donor] + shadowBl[donor], EPS);
        const creepBaseShare = Math.max(P011_HEAVY_SHARE_MIN,
          Math.min(P011_HEAVY_SHARE_MAX, shadowBh[donor] / creepDeposited));
        const creepBiasedShare = Math.max(P011_HEAVY_SHARE_MIN,
          Math.min(P011_HEAVY_SHARE_MAX,
            creepBaseShare + P011_HEAVY_SHARE_BIAS *
              (this.phaseHeavyIdeal - shadowPhaseHeavyActual -
                (this.phaseLightIdeal - shadowPhaseLightActual))));
        let creepHeavy = Math.min(allowed * creepBiasedShare, shadowBh[donor], allowed);
        let creepLight = Math.min(allowed - creepHeavy, shadowBl[donor]);
        let creepRemainder = allowed - creepHeavy - creepLight;
        const creepExtraHeavy = Math.min(creepRemainder, shadowBh[donor] - creepHeavy);
        creepHeavy += creepExtraHeavy;
        creepRemainder -= creepExtraHeavy;
        const creepExtraLight = Math.min(creepRemainder, shadowBl[donor] - creepLight);
        creepLight += creepExtraLight;
        shadowBh[donor] -= creepHeavy;
        shadowBl[donor] -= creepLight;
        shadowBh[receiver] += creepHeavy;
        shadowBl[receiver] += creepLight;
        shadowPhaseHeavyActual += creepDirectionSign * creepHeavy;
        shadowPhaseLightActual += creepDirectionSign * creepLight;
        recomputeShadowS();
      }
      plans.push(makePlan('bottomCreep', requestedTotal, grantedTotal, bottomUse,
        totalUse, deltaS));
    }
    // --- relax/correction slot mirror (D-1 route isolated). On explicitly
    // routed correction runs the apply chain replaced relaxAngle's budget
    // allocation with the reservation-first correction sub-channel, so the
    // mirror does the same on the shadow arrays: reserve with
    // p011CorrectionReserve (stock view over the shadow layers), then
    // execute the frozen (-w, edgeId) order with the SAME
    // p011CorrectionApplyOnce the real apply uses, debiting the shadow
    // ledgers and writing splitTopV2-exact material so recomputeShadowS
    // stays bit-identical to the real apply's updateSandTop. On every other
    // run the mirror replicates the pre-correction relax slot
    // bit-for-bit (5e60f2d): slip requests in collect order (static),
    // local pair then total, then the guard mirror; the apply-phase
    // maximumDeviation re-check at relax-apply entry is mirrored exactly —
    // when the shadow surface's maximum deviation is within the limit the
    // relax channel grants nothing. ---
    if (shadowReservation !== null && correctionParams !== null) {
      const deltaS = new Float64Array(cols);
      let grantedTotal = 0;
      let totalUse = 0;
      // The reservation came from the head (identical to the real one);
      // the per-candidate applyOnce view reads the EVOLVED shadow arrays.
      const headView: P011CorrectionStockView =
        new ShadowCorrectionStockView(this, shadowS, shadowSuspH, shadowSuspL);
      let shadowCorrectionRemaining = shadowReservation.rLimit;
      const correctionLedger: P011CorrectionLedger =
        new ShadowCorrectionLedger(localRemaining, localUsed, () => totalRemaining,
          (delta: number): void => { totalRemaining += delta; },
          () => shadowCorrectionRemaining,
          (delta: number): void => { shadowCorrectionRemaining += delta; });
      const requestedTotal = shadowReservation.rLimit;
      for (const index of shadowReservation.order) {
        const candidate = shadowReservation.candidates[index];
        const outcome = p011CorrectionApplyOnce(shadowReservation, index,
          headView, correctionParams, this.materialGuardHardMax(),
          correctionLedger);
        const allowed = outcome.accepted;
        grantedTotal += allowed;
        totalUse += allowed;
        if (allowed > EPS) {
          const donor = candidate.donor;
          const receiver = candidate.receiver;
          deltaS[donor] -= allowed;
          deltaS[receiver] += allowed;
          // splitTopV2-exact shadow material write.
          const mixAvail = shadowMh[donor] + shadowMl[donor];
          const fromMix = Math.min(allowed, mixAvail);
          const mixScale = fromMix / Math.max(mixAvail, EPS);
          let heavy = shadowMh[donor] * mixScale;
          let light = shadowMl[donor] * mixScale;
          let remaining = allowed - fromMix;
          const fromLight = Math.min(remaining, shadowBl[donor]);
          light += fromLight;
          remaining -= fromLight;
          const fromHeavy = Math.min(remaining, shadowBh[donor]);
          heavy += fromHeavy;
          shadowMh[donor] -= shadowMh[donor] * mixScale;
          shadowMl[donor] -= shadowMl[donor] * mixScale;
          shadowBl[donor] -= fromLight;
          shadowBh[donor] -= fromHeavy;
          shadowMh[receiver] += heavy;
          shadowMl[receiver] += light;
          recomputeShadowS();
        }
      }
      plans.push(makePlan('correction', requestedTotal, grantedTotal, 0, totalUse, deltaS));
    } else {
      const deltaS = new Float64Array(cols);
      let requestedTotal = 0;
      let grantedTotal = 0;
      let totalUse = 0;
      const relaxLimit = Math.tan(this.params.angleDeg * Math.PI / 180);
      const relaxTargetSlope = Math.tan(this.effectiveTilt * Math.PI / 180);
      let relaxMaxDeviation = 0;
      for (let c = 0; c < cols - 1; c++) {
        const deviation = shadowS[c + 1] - shadowS[c] - relaxTargetSlope;
        relaxMaxDeviation = Math.max(relaxMaxDeviation, Math.abs(deviation));
      }
      if (relaxMaxDeviation > relaxLimit) {
        for (let index = 0; index < this.slipRequestCount; index++) {
          const requested = this.slipRequestAmounts[index];
          const donor = this.slipRequestDonors[index];
          const receiver = this.slipRequestReceivers[index];
          if (requested <= EPS) {
            continue;
          }
          requestedTotal += requested;
          let allowed = clipLocalPair(donor, receiver, requested);
          if (allowed > EPS) {
            const deltaColumns = new Map<number, number>();
            deltaColumns.set(donor, -1);
            deltaColumns.set(receiver, 1);
            allowed = guardAdmit(allowed, deltaColumns);
          }
          grantedTotal += allowed;
          totalUse += allowed;
          deltaS[donor] -= allowed;
          deltaS[receiver] += allowed;
          // Mirror splitTopV2 + material write + updateSandTop exactly.
          const mixAvail = shadowMh[donor] + shadowMl[donor];
          const fromMix = Math.min(allowed, mixAvail);
          const mixScale = fromMix / Math.max(mixAvail, EPS);
          let relaxHeavy = shadowMh[donor] * mixScale;
          let relaxLight = shadowMl[donor] * mixScale;
          let relaxRemaining = allowed - fromMix;
          const relaxFromLight = Math.min(relaxRemaining, shadowBl[donor]);
          relaxLight += relaxFromLight;
          relaxRemaining -= relaxFromLight;
          const relaxFromHeavy = Math.min(relaxRemaining, shadowBh[donor]);
          relaxHeavy += relaxFromHeavy;
          shadowMh[donor] -= shadowMh[donor] * mixScale;
          shadowMl[donor] -= shadowMl[donor] * mixScale;
          shadowBl[donor] -= relaxFromLight;
          shadowBh[donor] -= relaxFromHeavy;
          shadowMh[receiver] += relaxHeavy;
          shadowMl[receiver] += relaxLight;
          recomputeShadowS();
        }
      }
      plans.push(makePlan('relax', requestedTotal, grantedTotal, 0, totalUse, deltaS));
    }
    // --- lift: column ascending, static request, local then total, then
    // the guard mirror; shadow surface advances per applied grant. ---
    {
      const deltaS = new Float64Array(cols);
      let requestedTotal = 0;
      let grantedTotal = 0;
      let totalUse = 0;
      for (let c = 0; c < cols; c++) {
        const requested = this.riseRequestAmounts[c];
        if (requested <= EPS) {
          continue;
        }
        requestedTotal += requested;
        let allowed = clipLocal(c, requested);
        if (allowed > EPS) {
          const deltaColumns = new Map<number, number>();
          deltaColumns.set(c, -1);
          allowed = guardAdmit(allowed, deltaColumns);
        }
        grantedTotal += allowed;
        totalUse += allowed;
        deltaS[c] -= allowed;
        // Mirror splitTopV2 + updateSandTop exactly (proportional mixed
        // slice first, then bl/bh fallbacks).
        const liftMixAvail = shadowMh[c] + shadowMl[c];
        const liftFromMix = Math.min(allowed, liftMixAvail);
        const liftMixScale = liftFromMix / Math.max(liftMixAvail, EPS);
        let liftHeavy = shadowMh[c] * liftMixScale;
        let liftLight = shadowMl[c] * liftMixScale;
        let liftRemaining = allowed - liftFromMix;
        const liftFromLight = Math.min(liftRemaining, shadowBl[c]);
        liftLight += liftFromLight;
        liftRemaining -= liftFromLight;
        const liftFromHeavy = Math.min(liftRemaining, shadowBh[c]);
        liftHeavy += liftFromHeavy;
        shadowMh[c] -= shadowMh[c] * liftMixScale;
        shadowMl[c] -= shadowMl[c] * liftMixScale;
        shadowBl[c] -= liftFromLight;
        shadowBh[c] -= liftFromHeavy;
        recomputeShadowS();
      }
      plans.push(makePlan('lift', requestedTotal, grantedTotal, 0, totalUse, deltaS));
    }
    return plans;
  }

/**
   * Replace the unconditional updateTilt commit (§20.3: the reference angle
   * must not be committed before grants/audit). Runs the two-pass planning
   * control law and leaves the field in the state selected by the single
   * commit: effectiveTilt = thetaAccepted. On BLOCKED the reference angle is
   * held at thetaBefore and the substep is marked failed via lastPlanning*.
   */
  private planAndCommitTilt(dt: number): WaterSandErrorCode | null {
    const thetaBefore = this.effectiveTilt;
    this.lastPlanningSubstepDt = dt;
    // GAP-4: every planning evaluation is one attempt. attemptId is
    // monotonic per field instance and unique per substep.
    this.p011AttemptCounter += 1;
    this.p011CurrentAttemptId = `${this.p011RunId}-a${this.p011AttemptCounter}`;
    this.p011CurrentRollbackReason = null;
    this.p011CurrentCommitStatus = 'committed';
    const rateCapRemaining: P011ExistingChannelRateCapRemaining = {
      settle: 0,
      percolate: 0,
      bottomCreep: 0,
      relax: 0,
      lift: 0,
      suspensionDiffusion: 0,
      correction: 0
    };
    const budgetsBefore: P011ExistingBudgetSnapshot = {
      localRemaining: Array.from(this.substepMotionBudgetRemaining),
      bottomRemaining: this.params.vBottomCreepMax * dt,
      totalRemaining: this.params.totalMotionBudget * dt,
      channelRateCapRemaining: rateCapRemaining
    };
    const slidingState = new Array<boolean>(Math.max(1, this.cols - 1)).fill(false);
    const channelStateBefore: P011ExistingChannelStateSnapshot = {
      sliding: slidingState
    };
    const input: P011TiltFeasibilityInput = {
      runId: this.p011RunId,
      substepId: this.p011SubstepId + 1,
      dt: dt,
      thetaBefore: thetaBefore,
      targetTilt: this.targetTilt,
      maxTiltOmega: this.params.maxTiltOmegaDegPerSec,
      sandTopBefore: new Float64Array(this.cols),
      budgetsBefore: budgetsBefore,
      channelStateBefore: channelStateBefore
    };
    input.sandTopBefore.set(this.substepSandTopBefore);
    const preview = this.planningPassService.createPreview(input,
      P011_PLANNING_PASS_TAN_HARD_MAX);
    const result = preview.result;
    this.lastPlanningStatus = result.status;
    this.lastPlanningBlockedReason = result.blockedReason;
    this.lastPlanningPassCount = result.passCount;
    this.lastPlanningFallbackUsed = result.fallbackUsed;
    this.lastPlanningThetaRequested = result.thetaRequested;
    this.lastPlanningLimitingEdge = result.limitingEdge;
    if (result.status === 'blocked') {
      // §20.4: BLOCKED is run-level invalid, not a silent hold. Freeze the
      // substep with an explicit error code so step() reports ok=false and
      // the run cannot satisfy zero-violation acceptance.
      preview.discard();
      this.effectiveTilt = thetaBefore;
      this.effectiveTiltRate = 0;
      this.lastError = WaterSandErrorCode.TILT_PLANNING_BLOCKED;
      // GAP-4: the blocked attempt still lands in the ledger.
      this.p011CurrentCommitStatus = 'blocked';
      this.p011CurrentRollbackReason = result.blockedReason;
      return WaterSandErrorCode.TILT_PLANNING_BLOCKED;
    }
    // Commit the accepted angle, then verify preview/commit exact equality
    // (§20.6.4): the authoritative collect re-runs at thetaAccepted inside
    // the step loop; capture its allocation here by re-collecting once and
    // comparing against the preview plans with p011PreviewCommitConsistent.
    preview.commit();
    this.effectiveTilt = result.thetaAccepted;
    this.effectiveTiltRate = (result.thetaAccepted - thetaBefore) / Math.max(dt, EPS);
    this.lastPlanningCommittedPlans = result.plans;
    this.lastPlanningCommittedTheta = result.thetaAccepted;
    // GAP-4 attempt semantics: clamped (committed at a smaller feasible
    // angle) and held (committed at the old angle) are real commits —
    // substeps do run — so all non-blocked statuses map to 'committed'.
    // The clamped-vs-held distinction is deliberately NOT carried here; it
    // lands in the ledger's requestRejected / plannerHold fields, which is
    // where the attempt audit reads it (P011NaturalObservation ledger).
    this.p011CurrentCommitStatus = 'committed';
    return null;
  }

  /**
   * §20.6.4 preview/commit exact-equality check, run after the authoritative
   * collect (substep) has filled the request ledgers at thetaAccepted. The
   * committed allocation is recomputed with the same shared pure allocator
   * (allocatePreviewPlans) from the real ledgers and compared item by item
   * against the preview plans; -0 is normalized and NaN rejected inside the
   * comparator. Any mismatch freezes the substep as PREVIEW_COMMIT_MISMATCH.
   */
  private verifyPreviewCommitConsistency(dt: number): WaterSandErrorCode | null {
    const previewPlans = this.lastPlanningCommittedPlans;
    if (previewPlans === null || this.lastPlanningStatus === null ||
      this.lastPlanningStatus === 'blocked') {
      return null;
    }
    const committedPlans = this.allocatePreviewPlans(dt);
    const consistent = p011PreviewCommitConsistent(
      {
        thetaRequested: this.lastPlanningThetaRequested,
        thetaAccepted: this.lastPlanningCommittedTheta,
        feasibleThetaMin: 0,
        feasibleThetaMax: 0,
        limitingEdge: this.lastPlanningLimitingEdge,
        plans: previewPlans,
        passCount: this.lastPlanningPassCount === 2 ? 2 : 1,
        fallbackUsed: this.lastPlanningFallbackUsed,
        status: 'accepted',
        blockedReason: null
      },
      this.effectiveTilt,
      committedPlans);
    if (!consistent) {
      this.lastPlanningStatus = 'blocked';
      this.lastPlanningBlockedReason = 'PREVIEW_COMMIT_MISMATCH';
      this.lastError = WaterSandErrorCode.PREVIEW_COMMIT_MISMATCH;
      // GAP-4: the mismatched attempt is rolled back (restoreBackup in the
      // step loop discards its material effects).
      this.p011CurrentCommitStatus = 'rolled_back';
      this.p011CurrentRollbackReason = 'PREVIEW_COMMIT_MISMATCH';
      return WaterSandErrorCode.PREVIEW_COMMIT_MISMATCH;
    }
    return null;
  }

  getPlanningTelemetry(): P011PlanningTelemetry {
    return {
      status: this.lastPlanningStatus,
      blockedReason: this.lastPlanningBlockedReason,
      passCount: this.lastPlanningPassCount,
      fallbackUsed: this.lastPlanningFallbackUsed,
      thetaRequested: this.lastPlanningThetaRequested,
      limitingEdge: this.lastPlanningLimitingEdge
    };
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
        this.recordNaturalSuspensionDiffusion(c, c + 1, c, flux, 0);
      } else if (heavyDifference < 0) {
        const flux = Math.min(this.params.heavyDiffusion * dt, -heavyDifference / 2,
          this.suspensionCapacity(c + 1));
        this.suspH[c] -= flux;
        this.suspH[c + 1] += flux;
        this.recordNaturalSuspensionDiffusion(c, c, c + 1, flux, 0);
      }
      if (lightDifference > 0) {
        const flux = Math.min(this.params.lightDiffusion * dt, lightDifference / 2,
          this.suspensionCapacity(c));
        this.suspL[c + 1] -= flux;
        this.suspL[c] += flux;
        this.recordNaturalSuspensionDiffusion(c, c + 1, c, 0, flux);
      } else if (lightDifference < 0) {
        const flux = Math.min(this.params.lightDiffusion * dt, -lightDifference / 2,
          this.suspensionCapacity(c + 1));
        this.suspL[c] -= flux;
        this.suspL[c + 1] += flux;
        this.recordNaturalSuspensionDiffusion(c, c, c + 1, 0, flux);
      }
    }
  }

  private suspensionCapacity(column: number): number {
    return Math.max(0, this.waterTop[column] - this.sandTopScratch[column] -
      this.suspH[column] - this.suspL[column]);
  }

  private recordNaturalSuspensionDiffusion(interfaceIndex: number, donor: number,
    receiver: number, heavy: number, light: number): void {
    if (this.naturalObservationProbe === null || !this.collectingTotalMotionRequests) {
      return;
    }
    this.naturalObservationProbe.recordSuspensionDiffusion({
      interfaceIndex: interfaceIndex,
      donor: donor,
      receiver: receiver,
      directionSign: donor < receiver ? 1 : -1,
      requested: heavy + light,
      allowed: heavy + light,
      heavy: heavy,
      light: light
    });
  }

  private recordNaturalRelaxSummary(appliedTotal: number): void {
    if (this.naturalObservationProbe === null) {
      return;
    }
    this.naturalObservationProbe.recordRelaxAngle(
      this.naturalObservationRelaxCandidateCount,
      this.naturalObservationRelaxRequestedTotal > EPS,
      this.naturalObservationRelaxRequestedTotal,
      appliedTotal
    );
  }

  /**
   * GAP-3: record one state-machine decision at its sampling point (inside
   * relaxAngle's collect scan, reading the CURRENT material state). The
   * observation mirror (naturalObservationSlidingBefore/After) is updated
   * by the caller-side scan so slip records reflect the machine's output
   * (fixing the v2 requested>EPS proxy).
   */
  private recordRelaxDecision(edge: number, deviation: number, activeBefore: boolean,
    decision: P011RelaxHysteresisNextActiveResult): void {
    const probe = this.naturalObservationProbe;
    if (probe === null) {
      return;
    }
    probe.markHysteresisActive();
    const decisionG = this.sandTopScratch[edge + 1] - this.sandTopScratch[edge];
    const tu = decision.thresholdUsed;
    const sample = new P011RelaxHysteresisDecisionSample(
      edge,
      this.relaxDecisionOrdinals[edge] + 1,
      decisionG,
      deviation,
      Math.atan(Math.abs(deviation)) * 180 / Math.PI,
      activeBefore,
      decision.activeAfter,
      decision.transitionReason,
      {
        branch: tu.branch,
        angleDeg: tu.angleDeg,
        slopeLimit: tu.slopeLimit,
        comparator: tu.comparator
      }
    );
    this.relaxDecisionOrdinals[edge] += 1;
    probe.recordDecision(sample);
  }

  /** Per-substep decision order counters (cleared in beginSubstepMotionBudget). */
  private relaxDecisionOrdinals: Int32Array = new Int32Array(0);

  private recordNaturalRelaxInactiveEdges(): void {
    if (this.naturalObservationProbe === null) {
      return;
    }
    const hysteresisActive = this.relaxHysteresisThresholds !== null;
    for (let index = 0; index < this.cols - 1; index++) {
      if (this.naturalObservationSlipSeen[index] !== 0) {
        continue;
      }
      if (hysteresisActive) {
        // Candidate runs: the machine owns the state. An edge the machine
        // holds active but that produced no correction candidate this
        // substep records holdActive (NOT a stop — budget/stock limits are
        // not stop events, §19.5). Edges the machine released record stop
        // via the slidingBefore/After mirror.
        const activeBefore = this.naturalObservationSlidingBefore[index] !== 0;
        const activeAfter = this.relaxSliding[index] !== 0;
        this.naturalObservationSlidingAfter[index] = activeAfter ? 1 : 0;
        this.naturalObservationProbe.recordSlip({
          interfaceIndex: index,
          donor: index,
          receiver: index + 1,
          directionSign: 0,
          requested: 0,
          allowed: 0,
          heavy: 0,
          light: 0,
          activeBefore: activeBefore,
          activeAfter: activeAfter,
          hardGuardRejected: 0,
          hardGuardClipped: 0,
          hardGuardAvailable: true
        });
        continue;
      }
      if (this.naturalObservationSlidingBefore[index] === 0) {
        continue;
      }
      this.naturalObservationProbe.recordSlip({
        interfaceIndex: index,
        donor: index,
        receiver: index + 1,
        directionSign: 0,
        requested: 0,
        allowed: 0,
        heavy: 0,
        light: 0,
        activeBefore: true,
        activeAfter: false,
        hardGuardRejected: 0,
        hardGuardClipped: 0,
        hardGuardAvailable: true
      });
    }
  }

  private recordNaturalTopSplit(interfaceIndex: number | null, column: number, amount: number,
    donorBh: number, donorBl: number, donorMh: number, donorMl: number,
    split: WaterSandSplitResult): void {
    if (this.naturalObservationProbe === null) {
      return;
    }
    const fromMix = Math.min(amount, donorMh + donorMl);
    const mixTotal = Math.max(donorMh + donorMl, EPS);
    const mixScale = fromMix / mixTotal;
    const fromMixedHeavy = donorMh * mixScale;
    const fromMixedLight = donorMl * mixScale;
    let remaining = amount - fromMix;
    const fromLight = Math.min(remaining, donorBl);
    remaining -= fromLight;
    const fromHeavy = Math.min(remaining, donorBh);
    this.naturalObservationProbe.recordTopSplit({
      channel: 'top',
      interfaceIndex: interfaceIndex,
      column: column,
      amount: amount,
      donorBhBefore: donorBh,
      donorBlBefore: donorBl,
      donorMhBefore: donorMh,
      donorMlBefore: donorMl,
      baseHeavyShare: null,
      biasedHeavyShare: null,
      initialHeavy: fromMixedHeavy,
      initialLight: fromMixedLight,
      fromMixedHeavy: fromMixedHeavy,
      fromMixedLight: fromMixedLight,
      fromLight: fromLight,
      fromHeavy: fromHeavy,
      fallbackHeavy: fromHeavy,
      fallbackLight: fromLight,
      finalHeavy: split.heavy,
      finalLight: split.light
    });
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
      let allowed = this.clipSubstepMotion(c, -1, originalRequested);
      // §19.5 pre-commit guard: settle increases S in this column; admit the
      // increment against the two adjacent edges before writing material.
      let guardClipped = 0;
      if (allowed > EPS) {
        const deltaColumns = new Map<number, number>();
        deltaColumns.set(c, 1);
        const guardOutcome = this.materialGuardAdmitIncrement(allowed, deltaColumns);
        if (guardOutcome.rejected > 0) {
          guardClipped = guardOutcome.rejected;
          this.materialGuardRefund(guardOutcome.rejected, [c], false);
          allowed = guardOutcome.accepted;
        }
      }
      const scale = allowed / Math.max(originalRequested, EPS);
      const heavy = desiredHeavy * scale;
      const light = desiredLight * scale;
      this.suspH[c] -= heavy;
      this.mh[c] += heavy;
      this.suspL[c] -= light;
      this.ml[c] += light;
      if (this.naturalObservationProbe !== null) {
        this.naturalObservationProbe.recordSettle(c, originalRequested, allowed, heavy, light,
          guardClipped);
      }
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
      if (this.naturalObservationProbe !== null) {
        this.naturalObservationProbe.recordPercolate(c, originalRequested, allowed, q, r);
      }
    }
    this.updateSandTop();
  }

  private relaxAngle(dt: number): void {
    const hysteresis = this.relaxHysteresisThresholds;
    const limit = Math.tan(this.params.angleDeg * Math.PI / 180);
    const targetSlope = Math.tan(this.effectiveTilt * Math.PI / 180);
    let maximumDeviation = 0;
    for (let c = 0; c < this.cols - 1; c++) {
      const deviation = this.sandTopScratch[c + 1] - this.sandTopScratch[c] - targetSlope;
      maximumDeviation = Math.max(maximumDeviation, Math.abs(deviation));
    }
    const legacyTrigger = maximumDeviation <= limit;
    if (hysteresis === null) {
      // LEGACY path (no candidate thresholds): pre-v3.1 behavior unchanged.
      if (legacyTrigger) {
        this.recordNaturalRelaxInactiveEdges();
        this.recordNaturalRelaxSummary(0);
        return;
      }
    }
    const maxFlux = this.params.maxAvalancheFlux * dt;
    if (this.collectingTotalMotionRequests) {
      let candidateCount = 0;
      let requestedTotal = 0;
      for (let index = 0; index < this.cols - 1; index++) {
        const c = index;
        const next = c + 1;
        const deviation = this.sandTopScratch[next] - this.sandTopScratch[c] - targetSlope;
        let activeBefore = false;
        let activeAfter = false;
        let naturalAmount = 0;
        // Donor/receiver orientation (shared by both branches):
        // deviation > 0 means the RIGHT column is too high -> donor = next,
        // receiver = c; deviation < 0 -> donor = c, receiver = next.
        const donor = deviation > 0 ? next : c;
        const receiver = deviation > 0 ? c : next;
        if (hysteresis !== null) {
          // GAP-1: the state machine owns eligibility. The decision sampling
          // point is HERE — reading the residual and picking the branch on
          // the CURRENT material state (post settle/percolate/creep within
          // the same collect pass), before any relax material moves.
          activeBefore = this.relaxSliding[index] !== 0;
          const decision = p011RelaxHysteresisNextActive(activeBefore, deviation,
            hysteresis, this.naturalObservationThetaStartDeg as number,
            this.naturalObservationThetaStopDeg as number);
          activeAfter = decision.activeAfter;
          this.recordRelaxDecision(index, deviation, activeBefore, decision);
          this.relaxSliding[index] = activeAfter ? 1 : 0;
          // Mirror the machine state into the observation arrays so slip
          // records and the edge ledger see exactly the machine's output.
          this.naturalObservationSlidingBefore[index] = activeBefore ? 1 : 0;
          this.naturalObservationSlidingAfter[index] = activeAfter ? 1 : 0;
          if (activeAfter) {
            const receiverSuspension = this.suspH[receiver] + this.suspL[receiver];
            const receiverCapacity = this.waterTop[receiver] > EPS
              ? this.waterTop[receiver] - this.sandTopScratch[receiver] - receiverSuspension
              : this.capacity - this.sandTopScratch[receiver];
            naturalAmount = p011RelaxHysteresisRequested(deviation, hysteresis,
              dt, this.params.maxAvalancheFlux, this.sandTopScratch[donor],
              receiverCapacity);
            candidateCount += 1;
          }
        } else {
          if (Math.abs(deviation) <= limit) {
            continue;
          }
          candidateCount += 1;
          const excess = Math.abs(deviation) - limit;
          const receiverSuspension = this.suspH[receiver] + this.suspL[receiver];
          const receiverCapacity = this.waterTop[receiver] > EPS
            ? this.waterTop[receiver] - this.sandTopScratch[receiver] - receiverSuspension
            : this.capacity - this.sandTopScratch[receiver];
          naturalAmount = Math.min(excess / 2, maxFlux,
            this.sandTopScratch[donor], receiverCapacity);
        }
        requestedTotal += Math.max(0, naturalAmount);
        this.totalMotionBudgetRequested += Math.max(0, naturalAmount);
        if (naturalAmount > EPS && this.slipRequestCount < this.slipRequestAmounts.length) {
          this.slipRequestAmounts[this.slipRequestCount] = naturalAmount;
          this.slipRequestDonors[this.slipRequestCount] = donor;
          this.slipRequestReceivers[this.slipRequestCount] = receiver;
          this.slipRequestCount++;
        }
      }
      this.naturalObservationRelaxCandidateCount = candidateCount;
      this.naturalObservationRelaxRequestedTotal = requestedTotal;
      this.recordNaturalRelaxSummary(0);
      return;
    }
    // Apply branch (relaxAngle's own apply — the pre-correction semantics).
    // D-1 route isolation: this branch runs for every run that did NOT
    // explicitly bind P011_CORRECTION_ROUTE_ID (legacy defaults, SLIP
    // verification, observation ①b, and v3 candidate
    // natural-relax-hysteresis-v1 runs); the correction route replaces this
    // slot via applyCorrectionSlot in applyMotionChannels.
    let appliedTotal = 0;
    for (let index = 0; index < this.slipRequestCount; index++) {
      const requested = this.slipRequestAmounts[index];
      const donor = this.slipRequestDonors[index];
      const receiver = this.slipRequestReceivers[index];
      // P-011 observation ①b fix A: slipRequest* slots are compacted candidates;
      // the physical edge identity is min(donor, receiver), never the slot index.
      const edge = donor < receiver ? donor : receiver;
      let amount = this.clipSubstepMotion(donor, receiver, requested);
      const activeBefore = this.naturalObservationSlidingBefore[edge] !== 0;
      // P-011 observation ①b fix B: active state follows the transition contract
      // requested>0 <-> requested=0 (algorithm-P011.md v2.10 edge contract:
      // budget-limited active edges stay active; a missing grant is not a stop).
      const activeAfter = requested > EPS;
      this.naturalObservationSlipSeen[edge] = 1;
      this.naturalObservationSlidingAfter[edge] = activeAfter ? 1 : 0;
      // §19.5 pre-commit guard: relax moves S donor->receiver; the affected
      // edge set (central + both outer neighbors) is derived inside the
      // admission helper from the per-column delta map.
      let guardRejected = 0;
      if (amount > EPS) {
        const relaxDelta = new Map<number, number>();
        relaxDelta.set(donor, -1);
        relaxDelta.set(receiver, 1);
        const relaxGuard = this.materialGuardAdmitIncrement(amount, relaxDelta);
        if (relaxGuard.rejected > 0) {
          guardRejected = relaxGuard.rejected;
          this.materialGuardRefund(relaxGuard.rejected, [donor, receiver], false);
          amount = relaxGuard.accepted;
        }
      }
      if (requested > EPS && amount <= EPS) {
        this.naturalObservationProbe?.recordSlip({
          interfaceIndex: edge,
          donor: donor,
          receiver: receiver,
          directionSign: donor < receiver ? 1 : -1,
          requested: requested,
          allowed: amount,
          heavy: 0,
          light: 0,
          activeBefore: activeBefore,
          activeAfter: activeAfter,
          hardGuardRejected: guardRejected,
          hardGuardClipped: 0,
          hardGuardAvailable: true
        });
      }
      if (amount <= EPS) {
        continue;
      }
      const donorBhBefore = this.bh[donor];
      const donorBlBefore = this.bl[donor];
      const donorMhBefore = this.mh[donor];
      const donorMlBefore = this.ml[donor];
      const split = this.splitTopV2(donor, amount);
      if (this.naturalObservationProbe !== null) {
        this.naturalObservationProbe.recordSlip({
          interfaceIndex: edge,
          donor: donor,
          receiver: receiver,
          directionSign: donor < receiver ? 1 : -1,
          requested: requested,
          allowed: amount,
          heavy: split.heavy,
          light: split.light,
          activeBefore: activeBefore,
          activeAfter: activeAfter,
          hardGuardRejected: guardRejected,
          hardGuardClipped: guardRejected > 0 && amount > EPS ? guardRejected : 0,
          hardGuardAvailable: true
        });
        this.recordNaturalTopSplit(edge, donor, amount, donorBhBefore, donorBlBefore,
          donorMhBefore, donorMlBefore, split);
      }
      this.mh[receiver] += split.heavy;
      this.ml[receiver] += split.light;
      appliedTotal += amount;
      this.updateSandTop();
    }
    this.recordNaturalRelaxInactiveEdges();
    this.recordNaturalRelaxSummary(appliedTotal);
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
      const directionSign = this.bottomCreepDirection > 0 ? 1 : -1;
      const donorDeposited = this.bh[donor] + this.bl[donor];
      const receiverSuspension = this.suspH[receiver] + this.suspL[receiver];
      const receiverCapacity = this.waterTop[receiver] > EPS
        ? this.waterTop[receiver] - this.sandTopScratch[receiver] - receiverSuspension
        : this.capacity - this.sandTopScratch[receiver];
      const independentGrant = requested * independentScale;
      let amount = this.clipBottomCreepMotion(
        Math.max(0, Math.min(independentGrant, donorDeposited, receiverCapacity)));
      // §19.5 pre-commit guard: creepBottom moves S donor->receiver; refund
      // also touches the bottom ledger (bottom is a creep-only channel).
      let guardRejected = 0;
      if (amount > EPS) {
        const creepDelta = new Map<number, number>();
        creepDelta.set(donor, -1);
        creepDelta.set(receiver, 1);
        const creepGuard = this.materialGuardAdmitIncrement(amount, creepDelta);
        if (creepGuard.rejected > 0) {
          guardRejected = creepGuard.rejected;
          this.materialGuardRefund(creepGuard.rejected, [donor, receiver], true);
          amount = creepGuard.accepted;
        }
      }
      if (amount <= EPS) {
        if (requested > EPS && this.naturalObservationProbe !== null) {
          this.naturalObservationProbe.recordBottomCreep({
            interfaceIndex: index,
            donor: donor,
            receiver: receiver,
            directionSign: directionSign,
            requested: requested,
            allowed: 0,
            heavy: 0,
            light: 0,
            hardGuardRejected: guardRejected,
            hardGuardClipped: 0,
            hardGuardAvailable: true
          });
        }
        continue;
      }
      const heavyResidual = this.phaseHeavyIdeal - this.phaseHeavyActual;
      const lightResidual = this.phaseLightIdeal - this.phaseLightActual;
      const donorBhBefore = this.bh[donor];
      const donorBlBefore = this.bl[donor];
      const donorMhBefore = this.mh[donor];
      const donorMlBefore = this.ml[donor];
      const deposited = Math.max(donorBhBefore + donorBlBefore, EPS);
      const baseHeavyShare = Math.max(P011_HEAVY_SHARE_MIN,
        Math.min(P011_HEAVY_SHARE_MAX, donorBhBefore / deposited));
      const biasedHeavyShare = Math.max(P011_HEAVY_SHARE_MIN,
        Math.min(P011_HEAVY_SHARE_MAX,
          baseHeavyShare + P011_HEAVY_SHARE_BIAS * (heavyResidual - lightResidual)));
      const initialHeavy = Math.min(amount * biasedHeavyShare, donorBhBefore, amount);
      const initialLight = Math.min(amount - initialHeavy, donorBlBefore);
      let remainder = amount - initialHeavy - initialLight;
      const fallbackHeavy = Math.min(remainder, donorBhBefore - initialHeavy);
      remainder -= fallbackHeavy;
      const fallbackLight = Math.min(remainder, donorBlBefore - initialLight);
      const split = this.splitBottomPhaseBalanced(donor, amount, heavyResidual, lightResidual);
      if (this.lastBottomSplitError !== null) {
        return this.lastBottomSplitError;
      }
      if (this.naturalObservationProbe !== null) {
        this.naturalObservationProbe.recordBottomCreep({
          interfaceIndex: index,
          donor: donor,
          receiver: receiver,
          directionSign: directionSign,
          requested: requested,
          allowed: amount,
          heavy: split.heavy,
          light: split.light,
          hardGuardRejected: guardRejected,
          hardGuardClipped: guardRejected > 0 && amount > EPS ? guardRejected : 0,
          hardGuardAvailable: true
        });
        this.naturalObservationProbe.recordBottomSplit({
          channel: 'bottom',
          interfaceIndex: index,
          column: donor,
          amount: amount,
          donorBhBefore: donorBhBefore,
          donorBlBefore: donorBlBefore,
          donorMhBefore: donorMhBefore,
          donorMlBefore: donorMlBefore,
          baseHeavyShare: baseHeavyShare,
          biasedHeavyShare: biasedHeavyShare,
          initialHeavy: initialHeavy,
          initialLight: initialLight,
          fromMixedHeavy: 0,
          fromMixedLight: 0,
          fromLight: 0,
          fromHeavy: 0,
          fallbackHeavy: fallbackHeavy,
          fallbackLight: fallbackLight,
          finalHeavy: split.heavy,
          finalLight: split.light
        });
      }
      this.bh[receiver] += split.heavy;
      this.bl[receiver] += split.light;
      // P-011 v2.8: signed global left/right effective displacement.
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
      let amount = this.clipSubstepMotion(c, -1, originalRequested);
      if (amount <= EPS) {
        if (this.naturalObservationProbe !== null) {
          this.naturalObservationProbe.recordLift(c, originalRequested, 0, 0, 0, 0);
        }
        continue;
      }
      // §19.5 pre-commit guard: lift decreases S in this column; admit the
      // increment against the two adjacent edges before writing material.
      let guardClipped = 0;
      const liftDelta = new Map<number, number>();
      liftDelta.set(c, -1);
      const liftGuard = this.materialGuardAdmitIncrement(amount, liftDelta);
      if (liftGuard.rejected > 0) {
        guardClipped = liftGuard.rejected;
        this.materialGuardRefund(liftGuard.rejected, [c], false);
        amount = liftGuard.accepted;
      }
      if (amount <= EPS) {
        if (this.naturalObservationProbe !== null) {
          this.naturalObservationProbe.recordLift(c, originalRequested, 0, 0, 0,
            guardClipped);
        }
        continue;
      }
      const donorBhBefore = this.bh[c];
      const donorBlBefore = this.bl[c];
      const donorMhBefore = this.mh[c];
      const donorMlBefore = this.ml[c];
      const split = this.splitTopV2(c, amount);
      if (this.naturalObservationProbe !== null) {
        this.naturalObservationProbe.recordLift(c, originalRequested, amount,
          split.heavy, split.light, guardClipped);
        this.recordNaturalTopSplit(null, c, amount, donorBhBefore, donorBlBefore,
          donorMhBefore, donorMlBefore, split);
      }
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

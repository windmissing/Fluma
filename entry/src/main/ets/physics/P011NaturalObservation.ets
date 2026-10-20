/**
 * P-011 natural-observation-v2.10 zero-intrusion physics ledger.
 * The probe only copies state and records actual grants; it never mutates physics.
 */

// P-011 v2.12 §19: observation identity is independent from the physics route.
export const P011_NATURAL_OBSERVATION_ROUTE_ID = 'natural-relax-hysteresis-v1';
export const P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION = 3;

export type P011NaturalObservationPhaseName =
  'baseline' | 'positive' | 'positiveReturn' | 'positiveOvershoot' |
  'positiveOvershootReturn' | 'negativeOvershoot' | 'negativeOvershootReturn' | 'unknown';

export type P011NaturalObservationRunRole = 'calibration' | 'holdout';

export type P011NaturalObservationEdgeTransitionType =
  'start' | 'stop' | 'holdActive' | 'holdInactive' | 'unavailable';

export interface P011NaturalObservationState {
  bh: number[];
  bl: number[];
  mh: number[];
  ml: number[];
  suspH: number[];
  suspL: number[];
  S: number[];
  waterTop: number[];
}

export interface P011NaturalObservationSubstepContext {
  observationRouteId: string;
  ledgerSchemaVersion: number;
  runId: string;
  frameId: number;
  substepId: number;
  sequence: number;
  physicsTime: number;
  physicsDt: number;
  commandSequence: number;
  commandPhaseId: number;
  commandPhaseSign: number;
  phaseD: P011NaturalObservationPhaseName;
  targetTilt: number;
  effectiveTilt: number;
  effectiveTiltRate: number;
  shakeEnergy: number;
  runRole: P011NaturalObservationRunRole | null;
  thresholdCalibrationId: string | null;
  thetaStartDeg: number | null;
  thetaStopDeg: number | null;
  transitionTelemetryAvailable: boolean;
}

export interface P011NaturalObservationColumnGrant {
  requested: number[];
  allowed: number[];
  heavy: number[];
  light: number[];
  /** §19.5 material-guard clipped amount per column (0 when not clipped). */
  hardGuardClipped: number[];
}

export interface P011NaturalObservationPercolationGrant {
  requested: number[];
  allowed: number[];
  q: number[];
  r: number[];
  heavyDown: number[];
  lightDown: number[];
}

export interface P011NaturalObservationEdgeGrantBase {
  interfaceIndex: number;
  donor: number;
  receiver: number;
  directionSign: number;
  requested: number;
  allowed: number;
  heavy: number;
  light: number;
  activeBefore?: boolean | null;
  activeAfter?: boolean | null;
  hardGuardRejected?: number | null;
  hardGuardClipped?: number | null;
  hardGuardAvailable?: boolean;
}

export interface P011NaturalObservationEdgeGrant extends P011NaturalObservationEdgeGrantBase {
  activeBefore: boolean | null;
  activeAfter: boolean | null;
  transitionType: P011NaturalObservationEdgeTransitionType;
  effectiveTilt: number;
  d: number;
  r: number;
  dAfter: number;
  rAfter: number;
  hardGuardRejected: number | null;
  hardGuardClipped: number | null;
  hardGuardAvailable: boolean;
}

export interface P011RelaxHysteresisThresholdObservation {
  thetaStartDeg: number | null;
  thetaStopDeg: number | null;
  slopeStart: number | null;
  slopeStop: number | null;
  slopeHardMax: number;
}

export interface P011RelaxHysteresisEdgeObservation {
  interfaceIndex: number;
  d: number;
  r: number;
  dAfter: number;
  rAfter: number;
  activeBefore: boolean;
  activeAfter: boolean;
  transitionType: P011NaturalObservationEdgeTransitionType;
  requested: number;
  granted: number;
  budgetScale: number | null;
  hardGuardRejected: number | null;
  hardGuardClipped: number | null;
  hardGuardAvailable: boolean;
}

export interface P011RelaxHysteresisObservation {
  effectiveTilt: number;
  thresholds: P011RelaxHysteresisThresholdObservation;
  edges: P011RelaxHysteresisEdgeObservation[];
  hardGuardRejected: number | null;
  hardGuardClipped: number | null;
  hardGuardAvailable: boolean;
  maxResidualBefore: number;
  maxResidualAfter: number;
}

export interface P011NaturalObservationSplitRecord {
  channel: 'top' | 'bottom';
  interfaceIndex: number | null;
  column: number | null;
  amount: number;
  donorBhBefore: number;
  donorBlBefore: number;
  donorMhBefore: number;
  donorMlBefore: number;
  baseHeavyShare: number | null;
  biasedHeavyShare: number | null;
  initialHeavy: number;
  initialLight: number;
  fromMixedHeavy: number;
  fromMixedLight: number;
  fromLight: number;
  fromHeavy: number;
  fallbackHeavy: number;
  fallbackLight: number;
  finalHeavy: number;
  finalLight: number;
}

export interface P011NaturalObservationBudgets {
  localLimit: number[];
  localUsed: number[];
  localRemaining: number[];
  bottomRequested: number;
  bottomUsed: number;
  bottomLimit: number;
  bottomClipped: number;
  totalRequested: number;
  totalUsed: number;
  totalLimit: number;
  totalRemaining: number;
  totalClipped: number;
}

export interface P011NaturalObservationAudit {
  finite: boolean;
  range: boolean;
  capacity: boolean;
  stock: boolean;
  conservation: boolean;
  waterTotalBefore: number;
  waterTotalAfter: number;
  heavyTotalBefore: number;
  heavyTotalAfter: number;
  lightTotalBefore: number;
  lightTotalAfter: number;
  maxSubstepBh: number;
  maxSubstepBl: number;
  maxSandTopSubstepDelta: number;
  maxSandTopInvariantError: number;
  bottomBudgetExcess: number;
  totalBudgetExcess: number;
  maxLocalBudgetExcess: number;
  relaxCandidateCount: number;
  relaxTriggered: boolean;
  relaxRequestedTotal: number;
  relaxAppliedTotal: number;
  errorCode: string | null;
}

export interface P011NaturalObservationRecord {
  context: P011NaturalObservationSubstepContext;
  stateBefore: P011NaturalObservationState;
  stateAfter: P011NaturalObservationState;
  settle: P011NaturalObservationColumnGrant;
  percolate: P011NaturalObservationPercolationGrant;
  lift: P011NaturalObservationColumnGrant;
  suspensionDiffusion: P011NaturalObservationEdgeGrant[];
  slip: P011NaturalObservationEdgeGrant[];
  relaxHysteresis: P011RelaxHysteresisObservation;
  bottomCreep: P011NaturalObservationEdgeGrant[];
  splits: P011NaturalObservationSplitRecord[];
  budgets: P011NaturalObservationBudgets;
  audit: P011NaturalObservationAudit;
}

export type P011NaturalObservationSink = (record: P011NaturalObservationRecord) => void;

function emptyColumnGrant(count: number): P011NaturalObservationColumnGrant {
  return {
    requested: new Array<number>(count).fill(0),
    allowed: new Array<number>(count).fill(0),
    heavy: new Array<number>(count).fill(0),
    light: new Array<number>(count).fill(0),
    hardGuardClipped: new Array<number>(count).fill(0)
  };
}

function emptyPercolationGrant(count: number): P011NaturalObservationPercolationGrant {
  return {
    requested: new Array<number>(count).fill(0),
    allowed: new Array<number>(count).fill(0),
    q: new Array<number>(count).fill(0),
    r: new Array<number>(count).fill(0),
    heavyDown: new Array<number>(count).fill(0),
    lightDown: new Array<number>(count).fill(0)
  };
}

function residualForEdge(state: P011NaturalObservationState, edgeIndex: number,
  effectiveTilt: number): number {
  if (edgeIndex < 0 || edgeIndex + 1 >= state.S.length) {
    return Number.NaN;
  }
  return state.S[edgeIndex + 1] - state.S[edgeIndex] -
    Math.tan(effectiveTilt * Math.PI / 180);
}

function transitionTypeFor(activeBefore: boolean, activeAfter: boolean):
  P011NaturalObservationEdgeTransitionType {
  if (!activeBefore && activeAfter) {
    return 'start';
  }
  if (activeBefore && !activeAfter) {
    return 'stop';
  }
  return activeAfter ? 'holdActive' : 'holdInactive';
}

export function p011NaturalObservationPhaseForCommand(
  commandSequence: number,
  targetTilt: number
): P011NaturalObservationPhaseName {
  if (commandSequence === 1 && Math.abs(targetTilt) < 1e-9) {
    return 'baseline';
  }
  if (commandSequence === 2 && Math.abs(targetTilt - 15) < 1e-9) {
    return 'positive';
  }
  if (commandSequence === 3 && Math.abs(targetTilt) < 1e-9) {
    return 'positiveReturn';
  }
  if (commandSequence === 4 && Math.abs(targetTilt - 45) < 1e-9) {
    return 'positiveOvershoot';
  }
  if (commandSequence === 5 && Math.abs(targetTilt) < 1e-9) {
    return 'positiveOvershootReturn';
  }
  if (commandSequence === 6 && Math.abs(targetTilt + 45) < 1e-9) {
    return 'negativeOvershoot';
  }
  if (commandSequence === 7 && Math.abs(targetTilt) < 1e-9) {
    return 'negativeOvershootReturn';
  }
  return 'unknown';
}

export class P011NaturalObservationProbe {
  private readonly cols: number;
  private readonly sink: P011NaturalObservationSink;
  private context: P011NaturalObservationSubstepContext | null = null;
  private stateBefore: P011NaturalObservationState | null = null;
  private settle: P011NaturalObservationColumnGrant;
  private percolate: P011NaturalObservationPercolationGrant;
  private lift: P011NaturalObservationColumnGrant;
  private suspensionDiffusion: P011NaturalObservationEdgeGrant[] = [];
  private slip: P011NaturalObservationEdgeGrant[] = [];
  private bottomCreep: P011NaturalObservationEdgeGrant[] = [];
  private splits: P011NaturalObservationSplitRecord[] = [];
  private relaxCandidateCount = 0;
  private relaxTriggered = false;
  private relaxRequestedTotal = 0;
  private relaxAppliedTotal = 0;
  private lastContext: P011NaturalObservationSubstepContext | null = null;
  private lastAudit: P011NaturalObservationAudit | null = null;

  constructor(cols: number, sink: P011NaturalObservationSink) {
    if (!Number.isInteger(cols) || cols < 2) {
      throw new Error('natural observation cols must be an integer >= 2');
    }
    this.cols = cols;
    this.sink = sink;
    this.settle = emptyColumnGrant(cols);
    this.percolate = emptyPercolationGrant(cols);
    this.lift = emptyColumnGrant(cols);
  }

  beginSubstep(context: P011NaturalObservationSubstepContext,
    stateBefore: P011NaturalObservationState): void {
    this.context = context;
    this.lastContext = context;
    this.stateBefore = stateBefore;
    this.settle = emptyColumnGrant(this.cols);
    this.percolate = emptyPercolationGrant(this.cols);
    this.lift = emptyColumnGrant(this.cols);
    this.suspensionDiffusion = [];
    this.slip = [];
    this.bottomCreep = [];
    this.splits = [];
    this.relaxCandidateCount = 0;
    this.relaxTriggered = false;
    this.relaxRequestedTotal = 0;
    this.relaxAppliedTotal = 0;
  }

  recordSuspensionDiffusion(grant: P011NaturalObservationEdgeGrantBase): void {
    this.suspensionDiffusion.push(this.completeEdgeGrant(grant));
  }

  recordSettle(column: number, requested: number, allowed: number,
    heavy: number, light: number, hardGuardClipped?: number): void {
    if (column < 0 || column >= this.cols) {
      return;
    }
    this.settle.requested[column] = requested;
    this.settle.allowed[column] = allowed;
    this.settle.heavy[column] = heavy;
    this.settle.light[column] = light;
    this.settle.hardGuardClipped[column] = hardGuardClipped === undefined
      ? 0 : hardGuardClipped;
  }

  recordPercolate(column: number, requested: number, allowed: number,
    q: number, r: number): void {
    if (column < 0 || column >= this.cols) {
      return;
    }
    this.percolate.requested[column] = requested;
    this.percolate.allowed[column] = allowed;
    this.percolate.q[column] = q;
    this.percolate.r[column] = r;
    this.percolate.heavyDown[column] = q + r;
    this.percolate.lightDown[column] = r - q;
  }

  recordLift(column: number, requested: number, allowed: number,
    heavy: number, light: number, hardGuardClipped?: number): void {
    if (column < 0 || column >= this.cols) {
      return;
    }
    this.lift.requested[column] = requested;
    this.lift.allowed[column] = allowed;
    this.lift.heavy[column] = heavy;
    this.lift.light[column] = light;
    this.lift.hardGuardClipped[column] = hardGuardClipped === undefined
      ? 0 : hardGuardClipped;
  }

  recordSlip(grant: P011NaturalObservationEdgeGrantBase): void {
    this.slip.push(this.completeEdgeGrant(grant));
  }

  recordBottomCreep(grant: P011NaturalObservationEdgeGrantBase): void {
    this.bottomCreep.push(this.completeEdgeGrant(grant));
  }

  recordTopSplit(split: P011NaturalObservationSplitRecord): void {
    this.splits.push(split);
  }

  recordBottomSplit(split: P011NaturalObservationSplitRecord): void {
    this.splits.push(split);
  }

  recordRelaxAngle(candidateCount: number, triggered: boolean,
    requestedTotal: number, appliedTotal: number): void {
    this.relaxCandidateCount = candidateCount;
    this.relaxTriggered = triggered;
    this.relaxRequestedTotal = requestedTotal;
    this.relaxAppliedTotal = appliedTotal;
  }

  finishSubstep(stateAfter: P011NaturalObservationState,
    budgets: P011NaturalObservationBudgets,
    audit: P011NaturalObservationAudit): void {
    if (this.context === null || this.stateBefore === null) {
      return;
    }
    this.refreshEdgeGrantsAfterApply(this.suspensionDiffusion, stateAfter);
    this.refreshEdgeGrantsAfterApply(this.slip, stateAfter);
    this.refreshEdgeGrantsAfterApply(this.bottomCreep, stateAfter);
    const relaxHysteresis = this.buildRelaxHysteresisObservation(stateAfter);
    const completeAudit: P011NaturalObservationAudit = {
      finite: audit.finite,
      range: audit.range,
      capacity: audit.capacity,
      stock: audit.stock,
      conservation: audit.conservation,
      waterTotalBefore: audit.waterTotalBefore,
      waterTotalAfter: audit.waterTotalAfter,
      heavyTotalBefore: audit.heavyTotalBefore,
      heavyTotalAfter: audit.heavyTotalAfter,
      lightTotalBefore: audit.lightTotalBefore,
      lightTotalAfter: audit.lightTotalAfter,
      maxSubstepBh: audit.maxSubstepBh,
      maxSubstepBl: audit.maxSubstepBl,
      maxSandTopSubstepDelta: audit.maxSandTopSubstepDelta,
      maxSandTopInvariantError: audit.maxSandTopInvariantError,
      bottomBudgetExcess: audit.bottomBudgetExcess,
      totalBudgetExcess: audit.totalBudgetExcess,
      maxLocalBudgetExcess: audit.maxLocalBudgetExcess,
      relaxCandidateCount: this.relaxCandidateCount,
      relaxTriggered: this.relaxTriggered,
      relaxRequestedTotal: this.relaxRequestedTotal,
      relaxAppliedTotal: this.relaxAppliedTotal,
      errorCode: audit.errorCode
    };
    this.lastAudit = completeAudit;
    this.sink({
      context: this.context,
      stateBefore: this.stateBefore,
      stateAfter: stateAfter,
      settle: this.settle,
      percolate: this.percolate,
      lift: this.lift,
      suspensionDiffusion: this.suspensionDiffusion,
      slip: this.slip,
      relaxHysteresis: relaxHysteresis,
      bottomCreep: this.bottomCreep,
      splits: this.splits,
      budgets: budgets,
      audit: completeAudit
    });
    this.context = null;
    this.stateBefore = null;
  }

  private completeEdgeGrant(grant: P011NaturalObservationEdgeGrantBase):
    P011NaturalObservationEdgeGrant {
    const context = this.context;
    const stateBefore = this.stateBefore;
    const d = context !== null && stateBefore !== null
      ? residualForEdge(stateBefore, grant.interfaceIndex, context.effectiveTilt)
      : Number.NaN;
    const hasTransitionTelemetry = grant.activeBefore !== undefined &&
      grant.activeAfter !== undefined &&
      grant.activeBefore !== null && grant.activeAfter !== null;
    return {
      interfaceIndex: grant.interfaceIndex,
      donor: grant.donor,
      receiver: grant.receiver,
      directionSign: grant.directionSign,
      requested: grant.requested,
      allowed: grant.allowed,
      heavy: grant.heavy,
      light: grant.light,
      activeBefore: grant.activeBefore === undefined ? null : grant.activeBefore,
      activeAfter: grant.activeAfter === undefined ? null : grant.activeAfter,
      transitionType: hasTransitionTelemetry
        ? transitionTypeFor(grant.activeBefore as boolean, grant.activeAfter as boolean)
        : 'unavailable',
      effectiveTilt: context === null ? Number.NaN : context.effectiveTilt,
      d: d,
      r: Math.abs(d),
      dAfter: d,
      rAfter: Math.abs(d),
      hardGuardRejected: grant.hardGuardRejected === undefined ? null : grant.hardGuardRejected,
      hardGuardClipped: grant.hardGuardClipped === undefined ? null : grant.hardGuardClipped,
      hardGuardAvailable: grant.hardGuardAvailable === true
    };
  }

  private refreshEdgeGrantsAfterApply(grants: P011NaturalObservationEdgeGrant[],
    stateAfter: P011NaturalObservationState): void {
    const context = this.context;
    if (context === null) {
      return;
    }
    for (const grant of grants) {
      const dAfter = residualForEdge(stateAfter, grant.interfaceIndex, context.effectiveTilt);
      grant.dAfter = dAfter;
      grant.rAfter = Math.abs(dAfter);
    }
  }

  private buildRelaxHysteresisObservation(stateAfter: P011NaturalObservationState):
    P011RelaxHysteresisObservation {
    const context = this.context;
    const stateBefore = this.stateBefore;
    if (context === null || stateBefore === null) {
      throw new Error('NATURAL_OBSERVATION_SUBSTEP_NOT_ACTIVE');
    }
    const slipByEdge = new Map<number, P011NaturalObservationEdgeGrant>();
    for (const grant of this.slip) {
      slipByEdge.set(grant.interfaceIndex, grant);
    }
    const edges: P011RelaxHysteresisEdgeObservation[] = [];
    let maxResidualBefore = 0;
    let maxResidualAfter = 0;
    let allGuardAvailable = this.slip.length > 0;
    for (let index = 0; index < this.cols - 1; index++) {
      const d = residualForEdge(stateBefore, index, context.effectiveTilt);
      const dAfter = residualForEdge(stateAfter, index, context.effectiveTilt);
      const slip = slipByEdge.get(index);
      const activeBefore = slip === undefined ? false : slip.activeBefore === true;
      const activeAfter = slip === undefined ? false : slip.activeAfter === true;
      const requested = slip === undefined ? 0 : slip.requested;
      const granted = slip === undefined ? 0 : slip.allowed;
      maxResidualBefore = Math.max(maxResidualBefore, Math.abs(d));
      maxResidualAfter = Math.max(maxResidualAfter, Math.abs(dAfter));
      allGuardAvailable = allGuardAvailable &&
        (slip !== undefined && slip.hardGuardAvailable === true);
      edges.push({
        interfaceIndex: index,
        d: d,
        r: Math.abs(d),
        dAfter: dAfter,
        rAfter: Math.abs(dAfter),
        activeBefore: activeBefore,
        activeAfter: activeAfter,
        transitionType: transitionTypeFor(activeBefore, activeAfter),
        requested: requested,
        granted: granted,
        budgetScale: requested > 0 ? granted / requested : null,
        hardGuardRejected: slip === undefined ? null : slip.hardGuardRejected,
        hardGuardClipped: slip === undefined ? null : slip.hardGuardClipped,
        hardGuardAvailable: slip !== undefined && slip.hardGuardAvailable === true
      });
    }
    let hardGuardRejected: number | null = null;
    let hardGuardClipped: number | null = null;
    if (allGuardAvailable) {
      hardGuardRejected = 0;
      hardGuardClipped = 0;
      for (const edge of edges) {
        hardGuardRejected += edge.hardGuardRejected === null ? 0 : edge.hardGuardRejected;
        hardGuardClipped += edge.hardGuardClipped === null ? 0 : edge.hardGuardClipped;
      }
    }
    return {
      effectiveTilt: context.effectiveTilt,
      thresholds: {
        thetaStartDeg: context.thetaStartDeg,
        thetaStopDeg: context.thetaStopDeg,
        slopeStart: context.thetaStartDeg === null
          ? null : Math.tan(context.thetaStartDeg * Math.PI / 180),
        slopeStop: context.thetaStopDeg === null
          ? null : Math.tan(context.thetaStopDeg * Math.PI / 180),
        slopeHardMax: Math.tan(30 * Math.PI / 180)
      },
      edges: edges,
      hardGuardRejected: hardGuardRejected,
      hardGuardClipped: hardGuardClipped,
      hardGuardAvailable: allGuardAvailable,
      maxResidualBefore: maxResidualBefore,
      maxResidualAfter: maxResidualAfter
    };
  }

  getLastContext(): P011NaturalObservationSubstepContext | null {
    return this.lastContext;
  }

  getLastAudit(): P011NaturalObservationAudit | null {
    return this.lastAudit;
  }
}

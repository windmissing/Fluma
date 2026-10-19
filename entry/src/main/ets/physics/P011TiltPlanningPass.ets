/**
 * P-011 architecture §20 / v2.14 planning-pass service skeleton.
 * Status: BLOCKED / DESIGN_NOT_CALIBRATED. This module is intentionally not
 * wired into WaterSandField and performs no physics, guard, or budget mutation.
 */

export type P011ExistingChannelId =
  'settle' | 'percolate' | 'bottomCreep' | 'relax' | 'lift' | 'suspensionDiffusion';

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

export interface P011ExistingBudgetSnapshot {
  localRemaining: number[];
  bottomRemaining: number;
  totalRemaining: number;
  channelRateCapRemaining: Record<P011ExistingChannelId, number>;
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

function blockedResult(input: P011TiltFeasibilityInput): P011TiltFeasibilityResult {
  return {
    thetaRequested: input.thetaBefore,
    thetaAccepted: input.thetaBefore,
    feasibleThetaMin: Number.NaN,
    feasibleThetaMax: Number.NaN,
    limitingEdge: null,
    plans: [],
    passCount: 1,
    fallbackUsed: false,
    status: 'blocked',
    blockedReason: 'DESIGN_NOT_CALIBRATED'
  };
}

export class P011TiltPlanningPassService {
  createPreview(input: P011TiltFeasibilityInput): P011PlanningPassPreview {
    const result = blockedResult(input);
    return {
      input: input,
      result: result,
      discard: (): void => {
      },
      commit: (): void => {
        throw new Error('P011_PLANNING_PASS_BLOCKED');
      }
    };
  }
}

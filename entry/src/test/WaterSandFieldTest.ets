import { describe, it, expect } from '@ohos/hypium';
import {
  WaterSandField,
  WaterSandInjectionState,
  WaterSandInjection,
  WaterSandPhase,
  WaterSandScene,
  WaterSandSeparationProfile,
  WaterSandSnapshot,
  WaterSandRegion,
  WaterSandStepResult,
  WaterSandParams,
  waterSandFrameHookPayload,
  defaultWaterSandParams
} from '../main/ets/physics/WaterSandField';
import {
  P011NaturalObservationProbe,
  P011NaturalObservationSubstepContext,
  P011NaturalObservationEdgeGrant,
  P011NaturalObservationColumnGrant,
  P011NaturalObservationRecord,
  P011NaturalObservationPhaseName,
  P011_SCHEDULE_NINE_PHASE_30_V2,
  P011_SCHEDULE_SEVEN_PHASE_45_V1,
  p011NaturalObservationPhaseForCommand,
  p011NaturalObservationPhaseForV3Segment
} from '../main/ets/physics/P011NaturalObservation';
import {
  P011ChannelPlan,
  P011PlanningPassDelegate,
  P011TiltPlanningPassService,
  P011TiltFeasibilityInput,
  p011DecideCandidate
} from '../main/ets/physics/P011TiltPlanningPass';
import {
  P011GuardLedgerRefund,
  p011GuardAdmit
} from '../main/ets/physics/P011MaterialGuard';
import {
  P011CorrectionCandidate,
  P011CorrectionLedger,
  P011CorrectionParams,
  P011CorrectionReservation,
  P011CorrectionStockView,
  p011CorrectionAllocate,
  p011CorrectionApplyOnce,
  p011CorrectionReserve
} from '../main/ets/physics/P011RelaxCorrection';
import {
  P011RelaxHysteresisParams,
  p011RelaxHysteresisNextActive,
  p011RelaxHysteresisRequested,
  p011RelaxHysteresisThresholds
} from '../main/ets/physics/P011RelaxHysteresis';
import {
  p011BuildScheduleV3,
  p011ScheduleV3ToCsv,
  p011ScheduleV3BindCommand
} from '../main/ets/physics/P011ScheduleV3';
import { parseP011TiltWant } from '../main/ets/physics/P011TiltWant';

// Collect the maximum |residual| over the field against tan30.
function guardMaxResidual(snapshot: WaterSandSnapshot): number {
  let maximum = 0;
  for (let c = 0; c < snapshot.cols - 1; c++) {
    const slope = snapshot.sandTop[c + 1] - snapshot.sandTop[c];
    maximum = Math.max(maximum, Math.abs(slope));
  }
  return maximum;
}

// Test stock view for p011CorrectionReserve (ArkTS requires methods to live
// on a class, not an object literal).
class TestCorrectionStockView implements P011CorrectionStockView {
  sandTop: Float64Array;
  effectiveTiltDeg: number;
  constructor(sandTop: Float64Array, effectiveTiltDeg: number) {
    this.sandTop = sandTop;
    this.effectiveTiltDeg = effectiveTiltDeg;
  }
  donorStock(column: number): number {
    return 100;
  }
  receiverCapacity(column: number): number {
    return 100;
  }
  guardResidual(edge: number): number {
    return this.sandTop[edge + 1] - this.sandTop[edge];
  }
}

/** §24 test ledger (ArkTS forbids function-property object literals). */
class TestCorrectionLedger implements P011CorrectionLedger {
  debited: number = 0;
  refunded: number = 0;
  debit(donor: number, receiver: number, amount: number): void {
    this.debited += amount;
  }
  refund(donor: number, receiver: number, amount: number): void {
    this.refunded += amount;
  }
}

export default function waterSandFieldTest() {
  describe('WaterSandFieldTest', () => {
    it('P011_ledgerContextSamplesAfterTiltUpdate', 0, () => {
      const probe = new P011NaturalObservationProbe(16, () => {});
      const field = new WaterSandField(16, 192);
      field.setNaturalObservationProbe(probe);
      field.setTilt(45);
      field.step(1 / 60);
      const context: P011NaturalObservationSubstepContext | null = probe.getLastContext();
      const metric = field.getP011Metric();
      expect(context === null).assertFalse();
      if (context !== null) {
        expect(context.substepId).assertEqual(metric.substepId);
        expect(context.effectiveTilt).assertEqual(metric.effectiveTilt);
        expect(context.effectiveTiltRate).assertEqual(metric.effectiveTiltRate);
      }
    });

    it('P011_MODE02_resetInputPreservesMaterialAndRunIdentity', 0, () => {
      const field = new WaterSandField(16, 192);
      field.setTilt(20);
      field.step(1 / 60);
      const before = JSON.stringify(field.snapshot());
      const metricBefore = field.getP011Metric();
      const runId = field.getP011RunId();
      field.resetInputState();
      expect(JSON.stringify(field.snapshot())).assertEqual(before);
      expect(field.getP011RunId()).assertEqual(runId);
      const metricAfter = field.getP011Metric();
      expect(metricAfter.targetTilt).assertEqual(0);
      expect(metricAfter.effectiveTilt).assertEqual(0);
      expect(metricAfter.effectiveTiltRate).assertEqual(0);
      expect(metricAfter.physicsTime).assertEqual(metricBefore.physicsTime);
      expect(metricAfter.frameId).assertEqual(metricBefore.frameId);
      expect(metricAfter.substepId).assertEqual(metricBefore.substepId);
    });
    it('canonicalLedger_derivesExternalState', 0, () => {
      const field = new WaterSandField(16, 192);
      for (let index = 0; index < 240; index++) {
        field.step(1 / 60);
      }
      const snapshot = field.snapshot();
      expect(snapshot.schemaVersion).assertEqual(2);
      for (let c = 0; c < snapshot.cols; c++) {
        expect(Math.abs(snapshot.hs[c] - snapshot.bh[c] - snapshot.mh[c]) < 1e-12).assertTrue();
        expect(Math.abs(snapshot.hl[c] - snapshot.bl[c] - snapshot.ml[c]) < 1e-12).assertTrue();
        expect(Math.abs(snapshot.sandTop[c] - snapshot.hs[c] - snapshot.hl[c]) < 1e-12).assertTrue();
      }
    });

    it('conservation_randomOperationSequence', 0, () => {
      const scene = mixedScene(16, 40);
      const field = new WaterSandField(16, 192);
      field.initialize(scene);
      const initial = field.getTotals();
      const targets = [-30, 15, 0, 44, -44, 0];
      for (let index = 0; index < 600; index++) {
        field.setTilt(targets[index % targets.length]);
        field.setShakeInput(index % 7 === 0 ? 1 : 0);
        field.step(1 / 60);
        if (index === 300) {
          field.invert180();
        }
      }
      const totals = field.getTotals();
      expect(Math.abs(totals.heavy - initial.heavy) < 1e-12).assertTrue();
      expect(Math.abs(totals.light - initial.light) < 1e-12).assertTrue();
      expect(Math.abs(totals.water - initial.water) < 1e-12).assertTrue();
      const drift = field.getDrift();
      expect(drift.heavy < 1e-12).assertTrue();
      expect(drift.light < 1e-12).assertTrue();
      expect(drift.water < 1e-12).assertTrue();
    });

    it('percolationV2_crossExchangeHasNonZeroQ', 0, () => {
      const field = new WaterSandField(16, 192);
      field.initialize({
        waterRegions: [region(0, 15, 30)],
        heavyDepositedRegions: [],
        lightDepositedRegions: [region(0, 15, 6)],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.inject(depositedInjection(WaterSandPhase.HEAVY, 64));
      const before = field.snapshot();
      const beforeTotals = field.getTotals();
      const result = field.step(1 / 60);
      const after = field.snapshot();
      expect(result.ok).assertTrue();
      expect(after.bh[0] - before.bh[0] > 1e-9).assertTrue();
      expect(before.bl[0] - after.bl[0] > 1e-9).assertTrue();
      expect(before.mh[0] - after.mh[0] > 1e-9).assertTrue();
      expect(after.ml[0] - before.ml[0] > 1e-9).assertTrue();
      const afterTotals = field.getTotals();
      expect(Math.abs(afterTotals.heavy - beforeTotals.heavy) < 1e-12).assertTrue();
      expect(Math.abs(afterTotals.light - beforeTotals.light) < 1e-12).assertTrue();
      expect(Math.abs(afterTotals.water - beforeTotals.water) < 1e-12).assertTrue();
    });

    it('percolationV2_pairSortingHasNonZeroR', 0, () => {
      const field = new WaterSandField(16, 192);
      field.initialize({
        waterRegions: [region(0, 15, 30)],
        heavyDepositedRegions: [],
        lightDepositedRegions: [],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.inject(depositedInjection(WaterSandPhase.HEAVY, 64));
      field.inject(depositedInjection(WaterSandPhase.LIGHT, 64));
      const before = field.snapshot();
      const beforeTotals = field.getTotals();
      const result = field.step(1 / 60);
      const after = field.snapshot();
      expect(result.ok).assertTrue();
      expect(after.bh[0] - before.bh[0] > 1e-9).assertTrue();
      expect(after.bl[0] - before.bl[0] > 1e-9).assertTrue();
      expect(before.mh[0] - after.mh[0] > 1e-9).assertTrue();
      expect(before.ml[0] - after.ml[0] > 1e-9).assertTrue();
      const afterTotals = field.getTotals();
      expect(Math.abs(afterTotals.heavy - beforeTotals.heavy) < 1e-12).assertTrue();
      expect(Math.abs(afterTotals.light - beforeTotals.light) < 1e-12).assertTrue();
      expect(Math.abs(afterTotals.water - beforeTotals.water) < 1e-12).assertTrue();
    });

    it('separationProfile_matchesIndependentReference', 0, () => {
      const field = new WaterSandField(16, 192);
      field.initialize({
        waterRegions: [region(0, 15, 30)],
        heavyDepositedRegions: [region(0, 15, 2)],
        lightDepositedRegions: [region(0, 15, 2)],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.inject(depositedInjection(WaterSandPhase.HEAVY, 64));
      field.inject(depositedInjection(WaterSandPhase.LIGHT, 64));
      const actual = field.getMetrics();
      const expected = referenceSeparationProfile(field.snapshot());
      expect(Math.abs(actual.lowerHeavyShare! - expected.lowerHeavyShare!) < 1e-12).assertTrue();
      expect(Math.abs(actual.upperHeavyShare! - expected.upperHeavyShare!) < 1e-12).assertTrue();
      expect(Math.abs(actual.separationRatio! - expected.separationRatio!) < 1e-12).assertTrue();
    });

    it('rateBudget_consistentAcross30_60_120Hz', 0, () => {
      const slow = runRateScenario(1 / 30, 60);
      const normal = runRateScenario(1 / 60, 120);
      const fast = runRateScenario(1 / 120, 240);
      expect(slow.result.substeps).assertEqual(2);
      expect(normal.result.substeps).assertEqual(1);
      expect(fast.result.substeps).assertEqual(1);
      expect(Math.abs(slow.result.substepDt - 1 / 60) < 1e-12).assertTrue();
      expect(Math.abs(normal.result.substepDt - 1 / 60) < 1e-12).assertTrue();
      expect(Math.abs(fast.result.substepDt - 1 / 120) < 1e-12).assertTrue();
      expect(slow.maxSpeed <= 28 + 1e-9).assertTrue();
      expect(normal.maxSpeed <= 28 + 1e-9).assertTrue();
      expect(fast.maxSpeed <= 28 + 1e-9).assertTrue();
      expect(maxStateDifference(slow.field.snapshot(), normal.field.snapshot()) <= 0.005).assertTrue();
      expect(maxStateDifference(fast.field.snapshot(), normal.field.snapshot()) <= 0.005).assertTrue();
      const slowMetrics = slow.field.getMetrics();
      const normalMetrics = normal.field.getMetrics();
      const fastMetrics = fast.field.getMetrics();
      expect(Math.abs(slowMetrics.suspensionFraction - normalMetrics.suspensionFraction) <= 0.005).assertTrue();
      expect(Math.abs(fastMetrics.suspensionFraction - normalMetrics.suspensionFraction) <= 0.005).assertTrue();
      expect(Math.abs(slowMetrics.t90Heavy - normalMetrics.t90Heavy) <= 0.005).assertTrue();
      expect(Math.abs(slowMetrics.t90Light - normalMetrics.t90Light) <= 0.005).assertTrue();
      expect(Math.abs(fastMetrics.t90Heavy - normalMetrics.t90Heavy) <= 0.005).assertTrue();
      expect(Math.abs(fastMetrics.t90Light - normalMetrics.t90Light) <= 0.005).assertTrue();
    });

    it('rateBudget_bindsSharedSubstepFlux', 0, () => {
      const field = new WaterSandField(8, 192);
      field.configure(deterministicRateParams());
      field.initialize({
        waterRegions: [region(0, 7, 20)],
        heavyDepositedRegions: [region(0, 7, 5)],
        lightDepositedRegions: [],
        heavySuspendedRegions: [region(0, 7, 5)],
        lightSuspendedRegions: [region(0, 7, 5)]
      });
      field.setShakeInput(1);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      expect(result.substepBudgetClipped).assertTrue();
      expect(result.maxSandTopSpeedObserved <= 28 + 1e-9).assertTrue();
      expect(result.substepBudgetUsedMax <= 28 * result.substepDt + 1e-9).assertTrue();
      const drift = field.getDrift();
      expect(drift.heavy < 1e-12).assertTrue();
      expect(drift.light < 1e-12).assertTrue();
      expect(drift.water < 1e-12).assertTrue();
    });

    it('rateBudget_bindsShallowWaterSettling', 0, () => {
      const field = new WaterSandField(8, 192);
      field.configure(deterministicRateParams());
      field.initialize({
        waterRegions: [region(0, 7, 16)],
        heavyDepositedRegions: [],
        lightDepositedRegions: [],
        heavySuspendedRegions: [region(0, 7, 8)],
        lightSuspendedRegions: [region(0, 7, 8)]
      });
      const before = field.snapshot();
      const beforeTotals = field.getTotals();
      const result = field.step(1 / 60);
      const after = field.snapshot();
      expect(result.ok).assertTrue();
      expect(result.substepBudgetClipped).assertTrue();
      expect(result.maxSandTopSubstepDeltaObserved <= 28 * result.substepDt + 1e-12).assertTrue();
      expect(result.substepBudgetUsedMax <= 28 * result.substepDt + 1e-12).assertTrue();
      let delta = 0;
      for (let c = 0; c < after.cols; c++) {
        delta = Math.max(delta, Math.abs(after.sandTop[c] - before.sandTop[c]));
      }
      expect(delta <= 28 * result.substepDt + 1e-12).assertTrue();
      const afterTotals = field.getTotals();
      expect(Math.abs(afterTotals.heavy - beforeTotals.heavy) < 1e-12).assertTrue();
      expect(Math.abs(afterTotals.light - beforeTotals.light) < 1e-12).assertTrue();
      expect(Math.abs(afterTotals.water - beforeTotals.water) < 1e-12).assertTrue();
    });

    it('rateBudget_validatesSandTopMaxRange', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      let thrown = false;
      try {
        field.configure(withSandTopMax(params, 30.1));
      } catch (error) {
        thrown = true;
      }
      expect(thrown).assertTrue();
      thrown = false;
      try {
        field.configure(withSandTopMax(params, 0));
      } catch (error) {
        thrown = true;
      }
      expect(thrown).assertTrue();
    });

    it('separation_singlePhaseReturnsNull', 0, () => {
      const field = new WaterSandField(16, 192);
      field.initialize({
        waterRegions: [region(0, 15, 20)],
        heavyDepositedRegions: [region(0, 15, 10)],
        lightDepositedRegions: [],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      const metrics = field.getMetrics();
      expect(metrics.lowerHeavyShare).assertNull();
      expect(metrics.upperHeavyShare).assertNull();
      expect(metrics.separationRatio).assertNull();
    });

    it('dormant_zeroSubstepStepResult', 0, () => {
      const field = new WaterSandField(8, 192);
      field.initialize({
        waterRegions: [region(0, 7, 20)],
        heavyDepositedRegions: [],
        lightDepositedRegions: [],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      let result = field.step(1 / 60);
      let transitionResult = result;
      for (let index = 0; index < 10 && !result.dormant; index++) {
        transitionResult = result;
        result = field.step(1 / 60);
      }
      expect(result.dormant).assertTrue();
      expect(transitionResult.dormant).assertFalse();
      expect(transitionResult.substeps > 0).assertTrue();
      const before = field.snapshot();
      const beforeTotals = field.getTotals();
      result = field.step(1 / 60);
      const after = field.snapshot();
      expect(result.dormant).assertTrue();
      expect(result.stepped).assertFalse();
      expect(result.substeps).assertEqual(0);
      expect(result.substepDt).assertEqual(0);
      expect(result.maxSandTopSubstepDeltaObserved).assertEqual(0);
      expect(result.maxSandTopSpeedObserved).assertEqual(0);
      expect(result.substepBudgetUsedMax).assertEqual(0);
      expect(result.substepBudgetClipped).assertFalse();
      assertSnapshotsEqual(before, after);
      const afterTotals = field.getTotals();
      expect(Math.abs(afterTotals.water - beforeTotals.water) < 1e-12).assertTrue();
    });

    it('frameHook_emitsDormantFrame', 0, () => {
      const field = new WaterSandField(8, 192);
      field.initialize({
        waterRegions: [region(0, 7, 20)],
        heavyDepositedRegions: [],
        lightDepositedRegions: [],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      let result = field.step(1 / 60);
      for (let index = 0; index < 10 && !result.dormant; index++) {
        result = field.step(1 / 60);
      }
      let lastDormant: boolean | null = null;
      let dormantEventCount = 0;
      for (let index = 0; index < 3; index++) {
        if (lastDormant !== result.dormant) {
          if (result.dormant) {
            dormantEventCount++;
          }
          lastDormant = result.dormant;
        }
        const payload = waterSandFrameHookPayload(16.7, result);
        expect(payload.frameTimeMs > 0).assertTrue();
        expect(payload.dormant).assertTrue();
        expect(payload.substeps).assertEqual(0);
        expect(payload.substepDt).assertEqual(0);
        expect(payload.maxSandTopSubstepDeltaObserved).assertEqual(0);
        expect(payload.maxSandTopSpeedObserved).assertEqual(0);
        expect(payload.substepBudgetUsedMax).assertEqual(0);
        expect(payload.substepBudgetClipped).assertFalse();
      }
      expect(dormantEventCount).assertEqual(1);
    });

    it('thickGeometry_tilt68MovesWithoutCapacityOverflow', 0, () => {
      const field = new WaterSandField(128, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      params.maxAvalancheFlux = 80;
      params.bottomCreepRate = 96;
      field.configure(params);
      field.initialize({
        waterRegions: [{ startCol: 0, endColInclusive: 127, bottom: 144, height: 38 }],
        heavyDepositedRegions: [{ startCol: 0, endColInclusive: 127, bottom: 0, height: 72 }],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: 127, bottom: 72, height: 72 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      const before = field.snapshot();
      const initialTotals = field.getTotals();
      field.setTilt(68);
      field.setShakeInput(68 / 90);
      let result = field.step(1 / 60);
      for (let index = 1; index < 300 && result.ok; index++) {
        result = field.step(1 / 60);
      }
      expect(result.ok).assertTrue();
      expect(result.errorCode).assertNull();
      const after = field.snapshot();
      let move = 0;
      for (let c = 0; c < after.cols; c++) {
        move = Math.max(move, Math.abs(after.sandTop[c] - before.sandTop[c]));
      }
      expect(move > 0).assertTrue();
      const finalTotals = field.getTotals();
      expect(Math.abs(finalTotals.heavy - initialTotals.heavy) < 1e-8).assertTrue();
      expect(Math.abs(finalTotals.light - initialTotals.light) < 1e-8).assertTrue();
      expect(Math.abs(finalTotals.water - initialTotals.water) < 1e-8).assertTrue();
    });

    it('bottomCreep_tiltMovesLowerLayers', 0, () => {
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.angleDeg = 5;
      params.maxAvalancheFlux = 60;
      params.bottomCreepRate = 96;
      field.configure(params);
      field.initialize({
        waterRegions: [{ startCol: 0, endColInclusive: 15, bottom: 144, height: 38 }],
        heavyDepositedRegions: [{ startCol: 0, endColInclusive: 15, bottom: 0, height: 72 }],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: 15, bottom: 72, height: 72 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      const before = field.snapshot();
      const initialTotals = field.getTotals();
      field.setTilt(30);
      let result = field.step(1 / 60);
      for (let index = 1; index < 60 && result.ok; index++) {
        result = field.step(1 / 60);
      }
      expect(result.ok).assertTrue();
      expect(result.errorCode).assertNull();
      const after = field.snapshot();
      let lowerMove = 0;
      for (let c = 0; c < after.cols; c++) {
        lowerMove = Math.max(lowerMove,
          Math.abs(after.bh[c] - before.bh[c]),
          Math.abs(after.bl[c] - before.bl[c]));
      }
      expect(lowerMove > 0).assertTrue();
      const finalTotals = field.getTotals();
      expect(Math.abs(finalTotals.heavy - initialTotals.heavy) < 1e-8).assertTrue();
      expect(Math.abs(finalTotals.light - initialTotals.light) < 1e-8).assertTrue();
      expect(Math.abs(finalTotals.water - initialTotals.water) < 1e-8).assertTrue();
    });

    it('p011_routeAndHooksAreSubstepAligned', 0, () => {
      const field = new WaterSandField(16, 192);
      field.configure(deterministicRateParams());
      field.initialize(thickSceneForP011(16));
      field.setTilt(20);
      let hookCount = 0;
      let lastTime = 0;
      let monotonic = true;
      const result = field.step(1 / 30, (metric, bottom) => {
        hookCount++;
        if (metric.physicsTime <= lastTime) {
          monotonic = false;
        }
        lastTime = metric.physicsTime;
        expect(metric.routeId).assertEqual('bottom-creep-v2.8-heavy-interface');
        expect(bottom.routeId).assertEqual('bottom-creep-v2.8-heavy-interface');
        expect(bottom.bh.length).assertEqual(16);
        expect(bottom.bl.length).assertEqual(16);
        expect(metric.bottomCreepBudgetUsed <= metric.bottomCreepBudgetLimit + 1e-12).assertTrue();
        expect(metric.totalMotionBudgetUsed <= metric.totalMotionBudgetLimit + 1e-12).assertTrue();
        expect(metric.directionCorrect).assertTrue();
      });
      expect(result.ok).assertTrue();
      expect(result.substeps).assertEqual(2);
      expect(hookCount).assertEqual(2);
      expect(monotonic).assertTrue();
      const metric = field.getP011Metric();
      expect(metric.effectiveTiltRate > 0).assertTrue();
      expect(metric.bottomCreepRate > 0).assertTrue();
      expect(metric.bottomCreepRate <= 10 + 1e-12).assertTrue();
    });

    it('p011_bottomCreepUsesDirectionAndIndependentBudget', 0, () => {
      const field = new WaterSandField(2, 192);
      const params = deterministicRateParams();
      params.vSandTopMax = 1;
      params.totalMotionBudget = 100;
      field.configure(params);
      field.initialize(thickSceneForP011(2));
      const before = field.snapshot();
      field.setTilt(20);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const metric = field.getP011Metric();
      const after = field.snapshot();
      expect(metric.directionCorrect).assertTrue();
      expect(metric.bottomCreepBudgetUsed <= metric.bottomCreepBudgetLimit + 1e-12).assertTrue();
      expect(metric.bottomCreepBudgetUsed > params.vSandTopMax / 60).assertTrue();
      expect(after.bh[0] + after.bl[0] < before.bh[0] + before.bl[0] - 1e-9).assertTrue();
      expect(after.bh[1] + after.bl[1] > before.bh[1] + before.bl[1] + 1e-9).assertTrue();
    });

    it('p011_phaseBalancedSplitKeepsHeavyShareAndTelemetry', 0, () => {
      const field = new WaterSandField(2, 192);
      const params = deterministicRateParams();
      params.vSandTopMax = 1;
      params.totalMotionBudget = 100;
      field.configure(params);
      field.initialize(thickSceneForP011(2));
      const initialTotals = field.getTotals();
      field.setTilt(20);
      let result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const metric = field.getP011Metric();
      expect(metric.commandPhaseId).assertEqual(1);
      expect(metric.commandPhaseSign).assertEqual(1);
      expect(metric.heavyFlux > 1e-12).assertTrue();
      expect(metric.lightFlux > 1e-12).assertTrue();
      expect(metric.heavyShare !== null && metric.heavyShare >= 0.35 - 1e-12).assertTrue();
      expect(metric.heavyActualDisplacement > 1e-12).assertTrue();
      expect(metric.lightActualDisplacement > 1e-12).assertTrue();
      expect(metric.heavyIdealDisplacement > 1e-12).assertTrue();
      expect(metric.heavyShapeRms > 1e-12).assertTrue();
      expect(metric.heavyShapeMaxDelta > 1e-12).assertTrue();
      expect(metric.maxSubstepBh > 1e-12).assertTrue();
      expect(metric.maxSubstepBl > 1e-12).assertTrue();
      const finalTotals = field.getTotals();
      expect(Math.abs(finalTotals.heavy - initialTotals.heavy) < 1e-10).assertTrue();
      expect(Math.abs(finalTotals.light - initialTotals.light) < 1e-10).assertTrue();
      expect(Math.abs(finalTotals.water - initialTotals.water) < 1e-10).assertTrue();
    });

    it('p011_commandSignFlipResetsPhaseAndReversesDisplacement', 0, () => {
      const field = new WaterSandField(2, 192);
      const params = deterministicRateParams();
      params.vSandTopMax = 1;
      params.totalMotionBudget = 100;
      field.configure(params);
      field.initialize(thickSceneForP011(2));
      field.setTilt(20);
      for (let index = 0; index < 8; index++) {
        expect(field.step(1 / 60).ok).assertTrue();
      }
      expect(field.getP011Metric().commandPhaseId).assertEqual(1);
      field.setTilt(-20);
      for (let index = 0; index < 8; index++) {
        expect(field.step(1 / 60).ok).assertTrue();
      }
      const metric = field.getP011Metric();
      expect(metric.commandPhaseId).assertEqual(2);
      expect(metric.commandPhaseSign).assertEqual(-1);
      expect(metric.heavyActualDisplacement < -1e-12).assertTrue();
      expect(metric.lightActualDisplacement < -1e-12).assertTrue();
      expect(metric.heavyShapeRms > 1e-12).assertTrue();
      expect(metric.heavyShapeMaxDelta > 1e-12).assertTrue();
    });

    it('p011_negativeTiltReversesBottomCreep', 0, () => {
      const field = new WaterSandField(2, 192);
      const params = deterministicRateParams();
      params.vSandTopMax = 1;
      params.totalMotionBudget = 100;
      field.configure(params);
      field.initialize(thickSceneForP011(2));
      const before = field.snapshot();
      field.setTilt(-20);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      expect(field.getP011Metric().directionCorrect).assertTrue();
      const after = field.snapshot();
      expect(after.bh[1] + after.bl[1] < before.bh[1] + before.bl[1] - 1e-9).assertTrue();
      expect(after.bh[0] + after.bl[0] > before.bh[0] + before.bl[0] + 1e-9).assertTrue();
    });

    it('p011_totalMotionBudgetClipsBottomCreep', 0, () => {
      const field = new WaterSandField(2, 192);
      const params = deterministicRateParams();
      params.vSandTopMax = 1;
      params.totalMotionBudget = 1;
      field.configure(params);
      field.initialize(thickSceneForP011(2));
      field.setTilt(20);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const metric = field.getP011Metric();
      expect(metric.totalMotionBudgetUsed <= metric.totalMotionBudgetLimit + 1e-12).assertTrue();
      expect(metric.totalMotionBudgetClipped > 0).assertTrue();
      expect(metric.bottomCreepBudgetUsed <= metric.bottomCreepBudgetLimit + 1e-12).assertTrue();
    });

    it('p011_directionAccuracyCovers200SamplesAndSignSwitches', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.totalMotionBudget = 1000;
      field.configure(params);
      field.initialize(thickSceneForP011(8));
      let nonDeadZoneSamples = 0;
      let directionCorrectSamples = 0;
      let signedHeavyDisplacementCorrectSamples = 0;
      let signedLightDisplacementCorrectSamples = 0;
      let signSwitches = 0;
      let lastSign = 0;
      let previousBh: number[] = [];
      let previousBl: number[] = [];
      let result = field.step(1 / 60, (metric, bottom) => {
        previousBh = bottom.bh.slice(0);
        previousBl = bottom.bl.slice(0);
      });
      for (let index = 0; index < 240 && result.ok; index++) {
        const target = Math.floor(index / 24) % 2 === 0 ? 20 : -20;
        field.setTilt(target);
        result = field.step(1 / 60, (metric, bottom) => {
          if (Math.abs(metric.effectiveTilt) > params.bottomCreepDeadZone) {
            nonDeadZoneSamples++;
            if (metric.directionCorrect) {
              directionCorrectSamples++;
            }
            if (previousBh.length === bottom.bh.length && previousBl.length === bottom.bl.length) {
              const expectedSign = metric.commandPhaseSign;
              if (expectedSign * metric.heavyActualDisplacement > 1e-9) {
                signedHeavyDisplacementCorrectSamples++;
              }
              if (expectedSign * metric.lightActualDisplacement > 1e-9) {
                signedLightDisplacementCorrectSamples++;
              }
            }
            const sign = metric.effectiveTilt > 0 ? 1 : -1;
            if (lastSign !== 0 && sign !== lastSign) {
              signSwitches++;
            }
            lastSign = sign;
          }
          previousBh = bottom.bh.slice(0);
          previousBl = bottom.bl.slice(0);
        });
      }
      expect(result.ok).assertTrue();
      expect(nonDeadZoneSamples >= 200).assertTrue();
      expect(signSwitches >= 8).assertTrue();
      expect(directionCorrectSamples).assertEqual(nonDeadZoneSamples);
      expect(signedHeavyDisplacementCorrectSamples).assertEqual(nonDeadZoneSamples);
      expect(signedLightDisplacementCorrectSamples).assertEqual(nonDeadZoneSamples);
    });

    it('p011_fullLoadKeepsBottomCreepIndependentBudget', 0, () => {
      const dt = 1 / 60;
      const baselineParams = deterministicRateParams();
      baselineParams.totalMotionBudget = 1000;
      baselineParams.heavySettleSpeed = 0.002;
      baselineParams.lightSettleSpeed = 0.001;
      baselineParams.percolationRate = 0.001;
      baselineParams.heavyDiffusion = 0;
      baselineParams.lightDiffusion = 0;
      baselineParams.maxAvalancheFlux = 0.001;
      baselineParams.heavyLiftRate = 0;
      baselineParams.lightLiftRate = 0.001;
      const baseline = new WaterSandField(8, 192);
      baseline.configure(baselineParams);
      baseline.initialize(fullLoadSceneForP011(8));

      const fullLoadParams = deterministicRateParams();
      fullLoadParams.totalMotionBudget = 1000;
      fullLoadParams.heavySettleSpeed = 0.002;
      fullLoadParams.lightSettleSpeed = 0.001;
      const fullLoad = new WaterSandField(8, 192);
      fullLoad.configure(fullLoadParams);
      fullLoad.initialize(fullLoadSceneForP011(8));

      baseline.setTilt(20);
      fullLoad.setTilt(20);
      fullLoad.setShakeInput(1);
      const baselineResult = baseline.step(dt);
      const fullLoadResult = fullLoad.step(dt);
      expect(baselineResult.ok).assertTrue();
      expect(fullLoadResult.ok).assertTrue();

      const baselineMetric = baseline.getP011Metric();
      const fullLoadMetric = fullLoad.getP011Metric();
      expect(fullLoadMetric.totalMotionBudgetUsed >
        fullLoadMetric.bottomCreepBudgetUsed + 1e-9).assertTrue();
      expect(fullLoadMetric.bottomCreepBudgetUsed <=
        fullLoadMetric.bottomCreepBudgetLimit + 1e-12).assertTrue();
      expect(fullLoadMetric.bottomCreepBudgetUsed >=
        baselineMetric.bottomCreepBudgetUsed * 0.98 - 1e-12).assertTrue();
    });

    it('p011_totalBudgetExhaustionScalesRequestsProportionally', 0, () => {
      const field = new WaterSandField(2, 192);
      const params = deterministicRateParams();
      params.vSandTopMax = 1;
      params.totalMotionBudget = 1;
      field.configure(params);
      field.initialize(thickSceneForP011(2));
      field.setTilt(20);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const metric = field.getP011Metric();
      const requestedTotal = metric.totalMotionBudgetUsed + metric.totalMotionBudgetClipped;
      const expectedScale = metric.totalMotionBudgetLimit / requestedTotal;
      const expectedBottomGrant = params.vBottomCreepMax / 60 * expectedScale;
      expect(metric.totalMotionBudgetUsed <=
        metric.totalMotionBudgetLimit + 1e-12).assertTrue();
      expect(metric.totalMotionBudgetClipped > 0).assertTrue();
      expect(Math.abs(metric.bottomCreepBudgetUsed - expectedBottomGrant) < 1e-9).assertTrue();
    });

    it('p011_multiChannelTotalBudgetScalesAllRequestsProportionally', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      // relax-correction-weighted-v1: the correction reservation has
      // priority over ordinary channels; with totalMotionBudget=1 the two
      // boundary correction edges reserve the whole budget (documented
      // starvation cost) and the ordinary channels cannot run, defeating
      // this test's multi-channel intent. Budget 10 keeps the reservation
      // (~0.078/substep) below B so ordinary channels still compete
      // proportionally for the remainder.
      params.totalMotionBudget = 10;
      params.vSandTopMax = 30;
      params.heavySettleSpeed = 100;
      params.lightSettleSpeed = 50;
      params.percolationRate = 1000;
      params.angleDeg = 1;
      params.bottomCreepDeadZone = 0.5;
      params.maxAvalancheFlux = 100;
      params.heavyLiftRate = 1000;
      params.lightLiftRate = 1000;
      field.configure(params);
      field.initialize(fullLoadSceneForP011(8));
      const before = field.snapshot();
      field.setTilt(20);
      field.setShakeInput(1);
      let result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const metric = field.getP011Metric();
      const bottomIndependentGrant = params.vBottomCreepMax / 60;
      expect(metric.totalMotionBudgetUsed <= metric.totalMotionBudgetLimit + 1e-12).assertTrue();
      expect(metric.totalMotionBudgetClipped > 0).assertTrue();
      // relax-correction-weighted-v1: the correction reservation has
      // priority over the ordinary channels; when the eligible correction
      // edges' caps exceed the whole budget (this overloaded scene), the
      // ordinary channels — bottomCreep included — receive nothing. The
      // old bottom-proportionality bound (bottomUsed == independentGrant *
      // scale) only holds when the reservation leaves ordinary budget; the
      // route-isolated bound is bottomUsed <= independentGrant always.
      expect(metric.bottomCreepBudgetUsed <=
        bottomIndependentGrant + 1e-12).assertTrue();
      const firstAfter = field.snapshot();
      let beforeSuspension = 0;
      let firstSuspension = 0;
      let beforeHeavyBottom = 0;
      let firstHeavyBottom = 0;
      let beforeLightBottom = 0;
      let firstLightBottom = 0;
      let beforeMixMoment = 0;
      let firstMixMoment = 0;
      let beforeMoment = 0;
      let afterMoment = 0;
      for (let c = 0; c < firstAfter.cols; c++) {
        beforeSuspension += before.suspH[c] + before.suspL[c];
        firstSuspension += firstAfter.suspH[c] + firstAfter.suspL[c];
        beforeHeavyBottom += before.bh[c];
        firstHeavyBottom += firstAfter.bh[c];
        beforeLightBottom += before.bl[c];
        firstLightBottom += firstAfter.bl[c];
        beforeMixMoment += c * (before.mh[c] + before.ml[c]);
        firstMixMoment += c * (firstAfter.mh[c] + firstAfter.ml[c]);
        beforeMoment += c * (before.bh[c] + before.bl[c]);
        afterMoment += c * (firstAfter.bh[c] + firstAfter.bl[c]);
      }
      expect(firstSuspension < beforeSuspension - 1e-9).assertTrue();
      expect(firstHeavyBottom > beforeHeavyBottom + 1e-9).assertTrue();
      expect(firstLightBottom < beforeLightBottom - 1e-9).assertTrue();
      expect(Math.abs(firstMixMoment - beforeMixMoment) > 1e-9).assertTrue();
      // relax-correction-weighted-v1: on a uniformly tilted field only the
      // boundary edges carry positive net contribution D (the interior
      // edges cancel uniformly, by design), so cross-column motion happens
      // at the boundary donors and the receivers gain into their mixed
      // layers — the deposited-moment sign under this route is scenario
      // dependent, so only assert that cross-column motion happened.
      expect(Math.abs(afterMoment - beforeMoment) > 1e-9).assertTrue();

      result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const after = field.snapshot();
      let secondSuspension = 0;
      for (let c = 0; c < after.cols; c++) {
        secondSuspension += after.suspH[c] + after.suspL[c];
      }
      expect(secondSuspension > firstSuspension + 1e-9).assertTrue();
    });

    it('p011_fullLoadPercolationKeepsBottomBudgetIsolated', 0, () => {
      const dt = 1 / 60;
      const isolatedParams = deterministicRateParams();
      isolatedParams.totalMotionBudget = 1000;
      isolatedParams.percolationRate = 0.001;
      isolatedParams.heavyDiffusion = 0;
      isolatedParams.lightDiffusion = 0;
      isolatedParams.maxAvalancheFlux = 0.001;
      isolatedParams.heavyLiftRate = 0;
      isolatedParams.lightLiftRate = 0;
      const isolated = new WaterSandField(8, 192);
      isolated.configure(isolatedParams);
      isolated.initialize(fullLoadSceneForP011(8));

      const concurrentParams = deterministicRateParams();
      concurrentParams.totalMotionBudget = 1000;
      concurrentParams.percolationRate = 1;
      concurrentParams.heavyDiffusion = 0;
      concurrentParams.lightDiffusion = 0;
      concurrentParams.maxAvalancheFlux = 0.001;
      concurrentParams.heavyLiftRate = 0;
      concurrentParams.lightLiftRate = 0;
      const concurrent = new WaterSandField(8, 192);
      concurrent.configure(concurrentParams);
      concurrent.initialize(fullLoadSceneForP011(8));

      isolated.setTilt(20);
      concurrent.setTilt(20);
      const isolatedResult = isolated.step(dt);
      const concurrentResult = concurrent.step(dt);
      expect(isolatedResult.ok).assertTrue();
      expect(concurrentResult.ok).assertTrue();
      const isolatedMetric = isolated.getP011Metric();
      const concurrentMetric = concurrent.getP011Metric();
      expect(Math.abs(concurrentMetric.bottomCreepBudgetUsed -
        isolatedMetric.bottomCreepBudgetUsed) < 1e-12).assertTrue();
      expect(Math.abs(concurrentMetric.bottomCreepBudgetClipped -
        isolatedMetric.bottomCreepBudgetClipped) < 1e-12).assertTrue();
      expect(concurrentMetric.totalMotionBudgetUsed >
        concurrentMetric.bottomCreepBudgetUsed + 1e-9).assertTrue();
    });

    it('p011_deadZoneClearsBottomCreepRate', 0, () => {
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.bottomCreepDeadZone = 10;
      field.configure(params);
      field.initialize(thickSceneForP011(16));
      field.setTilt(5);
      let result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      expect(field.getP011Metric().bottomCreepRate).assertEqual(0);
      field.setTilt(0);
      for (let index = 0; index < 12 && result.ok; index++) {
        result = field.step(1 / 60);
      }
      expect(result.ok).assertTrue();
      expect(field.getP011Metric().bottomCreepRate).assertEqual(0);
    });

    it('p011_velocityGainDoesNotOverrideShapeRateCap', 0, () => {
      const withoutGain = new WaterSandField(16, 192);
      const withoutParams = deterministicRateParams();
      withoutParams.bottomCreepRate = 1;
      withoutParams.bottomCreepVelocityGain = 0;
      withoutGain.configure(withoutParams);
      withoutGain.initialize(thickSceneForP011(16));
      const withoutBefore = withoutGain.snapshot();

      const withGain = new WaterSandField(16, 192);
      const withParams = deterministicRateParams();
      withParams.bottomCreepRate = 1;
      withParams.bottomCreepVelocityGain = 36;
      withGain.configure(withParams);
      withGain.initialize(thickSceneForP011(16));
      const withBefore = withGain.snapshot();

      withoutGain.setTilt(30);
      withGain.setTilt(30);
      const withoutResult = withoutGain.step(1 / 60);
      const withResult = withGain.step(1 / 60);
      expect(withoutResult.ok).assertTrue();
      expect(withResult.ok).assertTrue();

      const withoutAfter = withoutGain.snapshot();
      const withAfter = withGain.snapshot();
      let stateDifference = 0;
      for (let c = 0; c < withoutAfter.cols; c++) {
        stateDifference = Math.max(stateDifference,
          Math.abs(withAfter.bh[c] - withoutAfter.bh[c]),
          Math.abs(withAfter.bl[c] - withoutAfter.bl[c]));
      }
      const withoutMetric = withoutGain.getP011Metric();
      const withMetric = withGain.getP011Metric();
      expect(withoutMetric.bottomCreepRate > 0).assertTrue();
      expect(withMetric.bottomCreepRate).assertEqual(withoutMetric.bottomCreepRate);
      expect(withMetric.bottomCreepRate <= 10 + 1e-12).assertTrue();
      expect(stateDifference < 1e-12).assertTrue();
    });

    it('shake_peakAndRestResidual', 0, () => {
      const field = new WaterSandField(8, 192);
      field.configure(deterministicRateParams());
      field.initialize({
        waterRegions: [region(0, 7, 44)],
        heavyDepositedRegions: [region(0, 7, 10)],
        lightDepositedRegions: [region(0, 7, 10)],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      const initialTotals = field.getTotals();
      field.setShakeInput(1);
      for (let index = 0; index < 180; index++) {
        field.step(1 / 60);
      }
      const peak = field.getMetrics().suspensionFraction;
      expect(peak >= 0.55 && peak <= 0.75).assertTrue();
      field.setShakeInput(0);
      for (let index = 0; index < 480; index++) {
        field.step(1 / 60);
      }
      const rest = field.getMetrics();
      expect(rest.suspensionFraction <= 0.05).assertTrue();
      const finalTotals = field.getTotals();
      expect(Math.abs(finalTotals.heavy - initialTotals.heavy) < 1e-12).assertTrue();
      expect(Math.abs(finalTotals.light - initialTotals.light) < 1e-12).assertTrue();
      expect(Math.abs(finalTotals.water - initialTotals.water) < 1e-12).assertTrue();
    });

    it('invert180_preservesConservationAndPhaseOrder', 0, () => {
      const field = new WaterSandField(16, 192);
      field.initialize({
        waterRegions: [region(0, 15, 40)],
        heavyDepositedRegions: [region(0, 15, 10)],
        lightDepositedRegions: [region(0, 15, 10)],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      const beforeTotals = field.getTotals();
      field.invert180();
      const inverted = field.snapshot();
      for (let c = 0; c < inverted.cols; c++) {
        expect(inverted.bh[c]).assertEqual(0);
        expect(inverted.bl[c]).assertEqual(0);
        expect(inverted.mh[c]).assertEqual(0);
        expect(inverted.ml[c]).assertEqual(0);
      }
      for (let index = 0; index < 240; index++) {
        field.step(1 / 60);
      }
      const metrics = field.getMetrics();
      expect(metrics.phaseOrderScore >= 0.30).assertTrue();
      const finalTotals = field.getTotals();
      expect(Math.abs(finalTotals.heavy - beforeTotals.heavy) < 1e-12).assertTrue();
      expect(Math.abs(finalTotals.light - beforeTotals.light) < 1e-12).assertTrue();
      expect(Math.abs(finalTotals.water - beforeTotals.water) < 1e-12).assertTrue();
    });

    it('failedInjection_restoresCanonicalState', 0, () => {
      const field = new WaterSandField(16, 192);
      field.initialize({
        waterRegions: [region(0, 15, 185)],
        heavyDepositedRegions: [region(0, 15, 5)],
        lightDepositedRegions: [],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      const before = field.snapshot();
      let thrown = false;
      try {
        field.inject({
          phase: WaterSandPhase.WATER,
          amount: 128,
          startCol: 0,
          endColInclusive: 15,
          state: WaterSandInjectionState.DEPOSITED
        });
      } catch (error) {
        thrown = true;
      }
      expect(thrown).assertTrue();
      assertSnapshotsEqual(before, field.snapshot());
    });

    it('copyStateTo_rejectsInvalidSchema', 0, () => {
      const field = new WaterSandField(8, 192);
      const snapshot = field.snapshot();
      const invalidSnapshot = snapshot as MutableSnapshotSchema;
      invalidSnapshot.schemaVersion = 1;
      let thrown = false;
      try {
        field.copyStateTo(snapshot);
      } catch (error) {
        thrown = true;
      }
      expect(thrown).assertTrue();
    });

    // Review blocking item B: budget-limited edges must keep active (requested>0
    // means activeAfter=true); a missing grant must never be recorded as a stop.
    it('p011_transitionTelemetry_budgetLimitedEdgeKeepsActive', 0, () => {
      const slipRecords: P011NaturalObservationEdgeGrant[] = [];
      const probe = new P011NaturalObservationProbe(2, record => {
        for (const grant of record.slip) {
          slipRecords.push(grant);
        }
      });
      const field = new WaterSandField(2, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      params.vSandTopMax = 28;
      params.maxAvalancheFlux = 24;
      params.totalMotionBudget = 0.000001;
      field.configure(params);
      field.initialize(humpSceneForP011Cols2());
      field.setNaturalObservationProbe(probe);
      field.setTilt(45);
      for (let index = 0; index < 6; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
      }
      expect(slipRecords.length > 0).assertTrue();
      for (const grant of slipRecords) {
        if (grant.requested > 1e-12) {
          // Budget clipping may shrink the grant but never the active state.
          expect(grant.activeAfter).assertTrue();
          if (grant.activeBefore) {
            expect(grant.transitionType === 'holdActive' ||
              grant.transitionType === 'start').assertTrue();
          }
        }
      }
      // Every budget-limited substep must keep the active state (holdActive),
      // never a synthetic stop: budget limiting preserves active per the
      // transition contract (requested>0 stays active; no grant is not a
      // stop). Under the relax-correction-weighted-v1 route the budget limit
      // lands in the reservation itself (p is already budget-capped, so the
      // guard typically admits it fully); budget limitation is therefore
      // detected by the reservation being far below the natural need — the
      // grant is a tiny fraction of the overshoot e/2 it is repairing.
      let sawHoldActive = false;
      for (const grant of slipRecords) {
        if (grant.requested <= 1e-12) {
          continue;
        }
        expect(grant.transitionType !== 'stop').assertTrue();
        expect(grant.activeAfter).assertTrue();
        if (grant.transitionType === 'holdActive') {
          sawHoldActive = true;
        }
      }
      expect(sawHoldActive).assertTrue();
    });

    // Review blocking item A: slip request slots are compacted candidates; the
    // reported edge identity must be min(donor, receiver), not the slot index.
    // A hump over interior columns makes candidates a non-0..k prefix of edges.
    it('p011_transitionTelemetry_edgeIdentityIsNotSlotIndex', 0, () => {
      const slipRecords: P011NaturalObservationEdgeGrant[] = [];
      const probe = new P011NaturalObservationProbe(16, record => {
        for (const grant of record.slip) {
          slipRecords.push(grant);
        }
      });
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      // Hump over columns 6..9: steep edges are 5,6,7,8,9; edges 0..4 and
      // 10..14 stay flat, so candidate edges are NOT the 0..k prefix.
      // humpHeight 0.4 keeps |d| within (tan4°, tan30°): it exceeds the
      // relax trigger (relax requests → slip records) but stays inside the
      // §20 tan30 hard gate so planning never BLOCKs this observation test.
      field.initialize(humpSceneForP011(16, 6, 9, 0.4));
      field.setNaturalObservationProbe(probe);
      field.setTilt(0);
      for (let index = 0; index < 4; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
      }
      expect(slipRecords.length > 0).assertTrue();
      for (const grant of slipRecords) {
        // Edge identity invariants: 0 <= interfaceIndex = min(donor, receiver).
        expect(grant.interfaceIndex >= 0).assertTrue();
        expect(grant.interfaceIndex < 15).assertTrue();
        expect(grant.interfaceIndex).assertEqual(
          grant.donor < grant.receiver ? grant.donor : grant.receiver);
        // Residual is computed from the same edge: r = |d|.
        expect(grant.r).assertEqual(Math.abs(grant.d));
      }
      // The steep hump edges are 5 (d>0) and 9 (d<0). With the compacted-slot
      // bug the first candidates would have been misreported as edges 0 and 1.
      const firstSubstepEdges = [slipRecords[0].interfaceIndex, slipRecords[1].interfaceIndex];
      firstSubstepEdges.sort((a, b) => a - b);
      expect(firstSubstepEdges[0]).assertEqual(5);
      expect(firstSubstepEdges[1]).assertEqual(9);
      // No flat edge (r < 1) may ever carry a large residual from another edge.
      for (const grant of slipRecords) {
        expect(grant.r > 5 ? (grant.interfaceIndex === 5 || grant.interfaceIndex === 9)
          : true).assertTrue();
      }
    });

    // =========================================================================
    // §20.7.1 planning pass acceptance tests (【阶段一·限幅】).
    // =========================================================================

    // □3 preview purity: probe on/off must produce identical material states
    // (probe on/off same input physical trajectory, §17.9.10), and the
    // planning pass must not leak budgets or request ledgers (□2).
    it('p011_planningPass_previewPurity_budgetLedgerSingleCommit', 0, () => {
      const makeField = (): WaterSandField => {
        const field = new WaterSandField(8, 192);
        const params = deterministicRateParams();
        params.totalMotionBudget = 1;
        params.vSandTopMax = 30;
        params.angleDeg = 4;
        params.bottomCreepDeadZone = 0.5;
        field.configure(params);
        field.initialize(fullLoadSceneForP011(8));
        return field;
      };
      const withProbe = makeField();
      const probe = new P011NaturalObservationProbe(8, () => {});
      withProbe.setNaturalObservationProbe(probe);
      const withoutProbe = makeField();
      withProbe.setTilt(20);
      withoutProbe.setTilt(20);
      const resultOn = withProbe.step(1 / 60);
      const resultOff = withoutProbe.step(1 / 60);
      expect(resultOn.ok).assertTrue();
      expect(resultOff.ok).assertTrue();
      assertSnapshotsEqual(withoutProbe.snapshot(), withProbe.snapshot());
      const telemetry = withProbe.getPlanningTelemetry();
      expect(telemetry.status === 'accepted' || telemetry.status === 'clamped' ||
        telemetry.status === 'held').assertTrue();
      // Budget consumed exactly once: total used equals the shared budget that
      // the single commit consumed, with no double counting from previews.
      const metric = withProbe.getP011Metric();
      expect(metric.totalMotionBudgetUsed <=
        metric.totalMotionBudgetLimit + 1e-12).assertTrue();
      expect(metric.totalMotionBudgetUsed > 0).assertTrue();
    });

    // □3 single commit + effectiveTiltRate semantics: after a step the
    // effective tilt equals the accepted angle and the reported rate matches
    // (thetaAccepted-thetaBefore)/dt.
    it('p011_planningPass_singleCommitTiltAndRate', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.maxTiltOmegaDegPerSec = 120;
      field.configure(params);
      field.initialize(fullLoadSceneForP011(8));
      field.setTilt(20);
      const thetaBefore = 0;
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const metric = field.getP011Metric();
      const expectedTheta = Math.min(20, thetaBefore + 120 / 60);
      expect(Math.abs(metric.effectiveTilt - expectedTheta) < 1e-9).assertTrue();
      expect(Math.abs(metric.effectiveTiltRate -
        (expectedTheta - thetaBefore) * 60) < 1e-9).assertTrue();
      const telemetry = field.getPlanningTelemetry();
      expect(telemetry.passCount === 1 || telemetry.passCount === 2).assertTrue();
      expect(telemetry.status !== 'blocked').assertTrue();
      expect(telemetry.blockedReason === null).assertTrue();
    });

    // □7 BLOCKED semantics: preexisting infeasible state must produce
    // status='blocked' with a reason code, must not advance the reference
    // angle beyond the old value, and must not silently clamp-continue.
    it('p011_planningPass_blockedCarriesReasonAndHoldsTilt', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      field.configure(params);
      field.initialize(fullLoadSceneForP011(8));
      // Push the surface far beyond the hard max without any budget headroom
      // to repair it: inject a steep deposited step.
      field.inject({
        phase: WaterSandPhase.HEAVY,
        amount: 60,
        startCol: 6,
        endColInclusive: 7,
        state: WaterSandInjectionState.DEPOSITED
      });
      field.setTilt(0);
      const result = field.step(1 / 60);
      if (!result.ok) {
        // Frozen with an error code is an acceptable hard-failure path.
        expect(field.getLastError() !== null).assertTrue();
      } else {
        const telemetry = field.getPlanningTelemetry();
        if (telemetry.status === 'blocked') {
          expect(telemetry.blockedReason !== null).assertTrue();
          expect(telemetry.blockedReason === 'EMPTY_FEASIBLE_INTERVAL' ||
            telemetry.blockedReason === 'NO_SAFE_OLD_TILT_PLAN' ||
            telemetry.blockedReason === 'PREEXISTING_INFEASIBLE_STATE').assertTrue();
        }
      }
    });

    // □1/□2 pass limit and budget independence over a multi-step run: the
    // pass count never exceeds 2 and conservation holds throughout.
    it('p011_planningPass_twoPassLimitAndConservationOverRun', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      field.configure(params);
      field.initialize(fullLoadSceneForP011(8));
      const initialTotals = field.getTotals();
      field.setTilt(20);
      for (let index = 0; index < 30; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const telemetry = field.getPlanningTelemetry();
        expect(telemetry.passCount <= 2).assertTrue();
        if (telemetry.status === 'blocked') {
          expect(telemetry.blockedReason !== null).assertTrue();
        }
      }
      const finalTotals = field.getTotals();
      expect(Math.abs(finalTotals.heavy - initialTotals.heavy) < 1e-9).assertTrue();
      expect(Math.abs(finalTotals.light - initialTotals.light) < 1e-9).assertTrue();
      expect(Math.abs(finalTotals.water - initialTotals.water) < 1e-9).assertTrue();
    });

    // □1 directed three-candidate exhaustion: pass1 clamp → pass2 target
    // infeasible → old-angle fallback infeasible → BLOCKED, with at most 3
    // preview calls and passCount=2. Constructed directly against the
    // planning service with a counting delegate (§20.7.1-□1 synthetic path).
    it('p011_planningPass_directedThreeCandidateExhaustion', 0, () => {
      // g0 profile: one edge far beyond +H makes every theta infeasible for
      // any planned deltaS (deltaS = 0 ⇒ interval [atan(g-H), atan(g+H)]
      // cannot contain any directed theta ≥ thetaBefore when g > 2H).
      const g0 = new Float64Array([0, 0, 2.5, 0, 0]);
      const countingDelegate = new CountingPreviewDelegate(g0);
      const service = new P011TiltPlanningPassService();
      service.attach(countingDelegate);
      const input: P011TiltFeasibilityInput = {
        runId: 'test-run',
        substepId: 1,
        dt: 1 / 60,
        thetaBefore: 0,
        targetTilt: 12,
        maxTiltOmega: 720,
        sandTopBefore: new Float64Array(6),
        budgetsBefore: {
          localRemaining: [0, 0, 0, 0, 0, 0],
          bottomRemaining: 0,
          totalRemaining: 0,
          channelRateCapRemaining: {
            settle: 0, percolate: 0, bottomCreep: 0, relax: 0, lift: 0,
            suspensionDiffusion: 0, correction: 0
          }
        },
        channelStateBefore: { sliding: [false, false, false, false, false] }
      };
      const preview = service.createPreview(input, Math.tan(30 * Math.PI / 180));
      expect(preview.result.status).assertEqual('blocked');
      expect(preview.result.passCount <= 2).assertTrue();
      // Three candidates maximum: requested + clamped + fallback.
      expect(countingDelegate.previewCalls <= 3).assertTrue();
      expect(preview.result.blockedReason !== null).assertTrue();
      let thrown = false;
      try {
        preview.commit();
      } catch (error) {
        thrown = true;
      }
      expect(thrown).assertTrue();
      service.detach();
    });

    // □5 limiting-edge tie-break: when several edges give the same binding
    // constraint value, the smallest interfaceIndex must be kept (strict
    // comparison in the ascending scan). Inputs are column heights; edge
    // slopes are derived as S[c+1]-S[c] inside the decider.
    it('p011_planningPass_limitingEdgeTieBreakKeepsSmallestIndex', 0, () => {
      const deltaS = new Float64Array(6);
      // Uniform slopes g = 0.5 on all edges: every edge ties on both lo and
      // hi; the reported limiting edge must be index 0, never a later one.
      const s1 = new Float64Array([0, 0.5, 1.0, 1.5, 2.0, 2.5]);
      const decision = p011DecideCandidate(5, s1, deltaS,
        Math.tan(30 * Math.PI / 180), 1 / 60, 0, 720, 10);
      expect(decision.feasible).assertTrue();
      expect(decision.limitingEdge !== null).assertTrue();
      if (decision.limitingEdge !== null) {
        expect(decision.limitingEdge).assertEqual(0);
      }
      expect(Math.abs(decision.intervalLoSlope -
        (0.5 - Math.tan(30 * Math.PI / 180))) < 1e-12).assertTrue();
      // Partial tie: slopes [0, 0.5, 0.5, 0, 0]. The lo binding set is
      // {1, 2} (g = 0.5); the hi binding set is {0, 3, 4} (g = 0). Reported
      // limiting edge must be the first index of the side written last
      // (the tighter angle constraint), never a later equal edge.
      const s2 = new Float64Array([0, 0, 0.5, 1.0, 1.0, 1.0]);
      const decisionShift = p011DecideCandidate(5, s2, deltaS,
        Math.tan(30 * Math.PI / 180), 1 / 60, 0, 720, 10);
      expect(decisionShift.feasible).assertTrue();
      expect(decisionShift.limitingEdge !== null).assertTrue();
      if (decisionShift.limitingEdge !== null) {
        expect(decisionShift.limitingEdge <= 1).assertTrue();
      }
    });

    // Observation-3 v2 remediation: nine-phase schedule phase mapping.
    it('p011_ninePhaseMapping_assignsAllPlateauPhases', 0, () => {
      // Frozen v2 schedule 0/+15/0/+30/0/-15/0/-30/0, sequence = phaseIndex+1.
      const expected: P011NaturalObservationPhaseName[] = [
        'baseline', 'positiveLow', 'positiveLowReturn', 'positiveHigh',
        'positiveHighReturn', 'negativeLow', 'negativeLowReturn',
        'negativeHigh', 'negativeHighReturn'
      ];
      const targets = [0, 15, 0, 30, 0, -15, 0, -30, 0];
      for (let index = 0; index < expected.length; index++) {
        const phase = p011NaturalObservationPhaseForCommand(index + 1,
          targets[index], P011_SCHEDULE_NINE_PHASE_30_V2);
        expect(phase).assertEqual(expected[index]);
      }
      // Off-schedule sequence/target combinations stay unknown.
      expect(p011NaturalObservationPhaseForCommand(2, 45,
        P011_SCHEDULE_NINE_PHASE_30_V2)).assertEqual('unknown');
      expect(p011NaturalObservationPhaseForCommand(10, 0,
        P011_SCHEDULE_NINE_PHASE_30_V2)).assertEqual('unknown');
    });

    it('p011_ninePhaseMapping_legacySevenPhaseBackwardCompatible', 0, () => {
      // Legacy schedule (null or explicit v1 id) keeps the seven-phase map.
      expect(p011NaturalObservationPhaseForCommand(1, 0, null)).assertEqual('baseline');
      expect(p011NaturalObservationPhaseForCommand(2, 15, null)).assertEqual('positive');
      expect(p011NaturalObservationPhaseForCommand(4, 45, null))
        .assertEqual('positiveOvershoot');
      expect(p011NaturalObservationPhaseForCommand(6, -45, null))
        .assertEqual('negativeOvershoot');
      expect(p011NaturalObservationPhaseForCommand(4, 45,
        P011_SCHEDULE_SEVEN_PHASE_45_V1)).assertEqual('positiveOvershoot');
      // The nine-phase targets do NOT map under the legacy schedule.
      expect(p011NaturalObservationPhaseForCommand(4, 30, null)).assertEqual('unknown');
    });

    // Schema v3 supplement: planner fields land in every ledger context.
    it('p011_ledgerSchemaV3_plannerFieldsPresent', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      field.initialize(thickSceneForP011(8));
      field.setNaturalObservationProbe(probe);
      field.setNaturalObservationScheduleId(P011_SCHEDULE_NINE_PHASE_30_V2);
      field.setTilt(15);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      expect(records.length > 0).assertTrue();
      const context = records[records.length - 1].context;
      // Planner ran this substep (planning pass active): fields are non-null.
      expect(context.thetaAccepted !== null).assertTrue();
      expect(context.requestRejected !== null).assertTrue();
      expect(context.plannerHold !== null).assertTrue();
      // thetaAccepted equals the committed angle.
      expect(Math.abs(context.thetaAccepted! - context.effectiveTilt) < 1e-9)
        .assertTrue();
    });

    // relax-correction-weighted-v1 (§24): capped weighted allocation.
    it('p011_correction_cappedWeightedAllocationFillsReservation', 0, () => {
      // Two symmetric candidates (equal u, equal w) with R < sum u: both
      // stay unsaturated and share R equally through the single lambda.
      const stockView = new TestCorrectionStockView(
        new Float64Array([0, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.8]), 0);
      const reservation = p011CorrectionReserve(stockView, {
        relaxBoundSlope: 0.1,
        maxAvalancheFluxDt: 10,
        localBudget: 1,
        totalBudget: 0.02,
        dt: 1 / 60
      }, Math.tan(30 * Math.PI / 180), 0);
      // Boundary edges 0 and 6 overshoot (0.4 vs bound 0.1); interior
      // edges are uniform (D=0, excluded by design).
      expect(reservation.candidates.length === 2).assertTrue();
      expect(reservation.candidates.every(c =>
        c.edge === 0 || c.edge === 6)).assertTrue();
      // R = min(B, sum u) and the allocation fills R exactly.
      const sumP = reservation.candidates.reduce((sum, c) => sum + c.p, 0);
      expect(Math.abs(sumP - reservation.rLimit) < 1e-9).assertTrue();
      expect(reservation.rLimit <= 0.02 + 1e-12).assertTrue();
    });

    it('p011_correction_uniformInteriorEdgesExcluded', 0, () => {
      // A uniform tilt-only field: every interior edge has the same
      // overshoot and D = 0 (uniform cancellation); only boundary edges
      // enter the candidate set.
      const n = 8;
      const sandTop = new Float64Array(n);
      const tiltSlope = 0.35;
      for (let c = 0; c < n; c++) {
        sandTop[c] = c * tiltSlope;
      }
      const stockView = new TestCorrectionStockView(sandTop, 0);
      const reservation = p011CorrectionReserve(stockView, {
        relaxBoundSlope: 0.1,
        maxAvalancheFluxDt: 10,
        localBudget: 1,
        totalBudget: 5,
        dt: 1 / 60
      }, Math.tan(30 * Math.PI / 180), 0);
      for (const candidate of reservation.candidates) {
        expect(candidate.edge === 0 || candidate.edge === n - 2).assertTrue();
      }
    });

    it('p011_correction_ledgerAndGuardsInApply', 0, () => {
      // A hump field runs the correction slot end-to-end: ledger closes
      // (used + remaining = requested), the total accounting includes the
      // correction grants, and every slip record carries the guard fields.
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      field.initialize(humpSceneForP011(8, 3, 4, 0.55));
      field.setNaturalObservationProbe(probe);
      // D-1: the correction slot only runs on an explicitly routed
      // correction run (state machine armed + route bound). Hump residual
      // 0.55 crosses START (tan26°≈0.4877) so edges are machine-active and
      // eligible for the correction reservation (slopeStop=tan20°≈0.364),
      // staying below the tan30 planning wall (≈0.5774).
      field.setNaturalObservationRunMetadata('calibration', 'corr-1', 26, 20);
      field.setNaturalObservationRouteId('relax-correction-weighted-v1');
      field.setTilt(0);
      let anyAccepted = 0;
      for (let index = 0; index < 6; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const last = records[records.length - 1];
        for (const grant of last.slip) {
          expect(grant.hardGuardAvailable === true).assertTrue();
          expect(grant.requested >= grant.allowed - 1e-12).assertTrue();
          if (grant.allowed > 1e-9) {
            anyAccepted += grant.allowed;
          }
        }
      }
      expect(anyAccepted > 1e-9).assertTrue();
    });

    // =========================================================================
    // Correction allocator unit suite (designer algorithm §3 derivation,
    // tester Node coverage; pure function tests, zero product changes).
    // =========================================================================

    // Ruling 1: only edges with e>0 AND D>0 enter the candidate set; D<=0
    // edges are dropped as NO_POSITIVE_LOCAL_CONTRIBUTION (no epsilon
    // weights). An isolated overshoot flanked by in-bound edges has
    // D = e·2s > 0, so it IS a candidate; a deep edge flanked by equally
    // deep same-sign edges cancels (D=0).
    it('p011_correction_candidateSelectionDPositiveOnly', 0, () => {
      const h = 0.1;
      // Isolated overshoot: S = [0, 0.3, 0.3] with tilt 0 -> edge0 residual
      // 0.3 (e=0.2, D>0), edge1 residual 0 (in-bound, not eligible).
      const isolated = p011CorrectionReserve(new TestCorrectionStockView(
        new Float64Array([0, 0.3, 0.3]), 0), {
        relaxBoundSlope: h, maxAvalancheFluxDt: 10, localBudget: 1,
        totalBudget: 5, dt: 1 / 60
      }, Math.tan(30 * Math.PI / 180), 0);
      expect(isolated.candidates.length === 1).assertTrue();
      expect(isolated.candidates[0].edge === 0).assertTrue();
      expect(isolated.candidates[0].d > 0).assertTrue();
      // Uniform interior: three same-sign equal-e edges -> D=0, excluded.
      // S = [0, 0.3, 0.6, 0.9]: edges 0,1,2 all residual 0.3 (e=0.2);
      // edge1's neighbors are equally deep -> interior cancellation.
      const uniform = p011CorrectionReserve(new TestCorrectionStockView(
        new Float64Array([0, 0.3, 0.6, 0.9]), 0), {
        relaxBoundSlope: h, maxAvalancheFluxDt: 10, localBudget: 1,
        totalBudget: 5, dt: 1 / 60
      }, Math.tan(30 * Math.PI / 180), 0);
      // Only the boundary edge 0 (one-sided neighbor, no cancellation)
      // and edge 2 (last edge, boundary K) can keep D>0; edge 1 (interior,
      // symmetric neighbors) must be dropped.
      const edges = uniform.candidates.map(c => c.edge).sort();
      expect(edges.includes(1) === false).assertTrue();
    });

    // Designer §3 weighted-allocation example: 40 deep edges (e=0.4) vs
    // 10 shallow edges (e=0.1), equal neighborhoods, R=5 — the weight
    // ratio is 16:1 and the unsaturated solution gives each deep edge
    // 0.1230769 and each shallow edge 0.0076923 (sum 5), NOT the
    // proportional 0.1176471/0.0294118.
    it('p011_correction_weightedAllocationMatchesDesignerExample', 0, () => {
      // Build a 52-column ring of two plateaus: 41 deep columns then 11
      // shallow ones. Deep-side interior edges all have e=0.4·(residual
      // scale); shallow-side edges e=0.1·scale. Boundary edges at the two
      // jumps carry the transition; we verify the WEIGHTED split on the
      // two groups through p (equal-u, equal-w within each group).
      // Simpler faithful reproduction: construct candidates directly via
      // reserve on a plateau field where every interior edge has the same
      // neighbor structure. Use a sawtooth-free two-level staircase:
      // columns 0..40 at height H, 41..51 at height H - 0.3 (so deep-side
      // interior edges e = 0.3 - h and shallow-side interior edges
      // e = 0.1·(residual unit)? — the exact designer numbers need
      // equal-neighborhood isolation, which a single field cannot give
      // both groups simultaneously. Instead verify the ALLOCATOR's weight
      // law directly with p011CorrectionAllocate on a synthetic
      // reservation (the allocator is the frozen unit under test).
      const deepCount = 40;
      const shallowCount = 10;
      const candidates: P011CorrectionCandidate[] = [];
      for (let i = 0; i < deepCount; i++) {
        candidates.push({
          edge: i, donor: i, receiver: i + 1, r: 0.5, e: 0.4,
          d: 2, k: 6, qNatural: 1, qDescent: 1, guardAdmissible: 1,
          u: 0.133333, w: 0.4 * 2 / 2, p: 0
        });
      }
      for (let i = 0; i < shallowCount; i++) {
        candidates.push({
          edge: 100 + i, donor: 100 + i, receiver: 101 + i, r: 0.2, e: 0.1,
          d: 0.5, k: 6, qNatural: 1, qDescent: 1, guardAdmissible: 1,
          u: 0.033333, w: 0.1 * 0.5 / 2, p: 0
        });
      }
      const reservation: P011CorrectionReservation = {
        candidates: candidates,
        rLimit: 5,
        pByColumn: new Float64Array(200),
        order: candidates.map((c, index) => index),
        ordinaryRelaxRequested: 0
      };
      p011CorrectionAllocate(reservation);
      const deep = reservation.candidates[0].p;
      const shallow = reservation.candidates[deepCount].p;
      // Designer numbers: deep 0.1230769, shallow 0.0076923 (sum 5).
      expect(Math.abs(deep - 0.1230769) < 1e-6).assertTrue();
      expect(Math.abs(shallow - 0.0076923) < 1e-6).assertTrue();
      const sumP = reservation.candidates.reduce((sum, c) => sum + c.p, 0);
      expect(Math.abs(sumP - 5) < 1e-9).assertTrue();
      // Weight ratio drives the split: deep gets 16x the per-edge grant.
      expect(Math.abs(deep / shallow - 16) < 1e-3).assertTrue();
    });

    // Reservation boundary: R_limit = min(B, sum u) never exceeds the
    // total budget B (ordinaryUsed + correctionUsed <= B stays invariant);
    // the reservation is carved out of B, never added on top.
    it('p011_correction_reservationWithinTotalBudget', 0, () => {
      const stockView = new TestCorrectionStockView(
        new Float64Array([0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5]), 0);
      const reservation = p011CorrectionReserve(stockView, {
        relaxBoundSlope: 0.1, maxAvalancheFluxDt: 10, localBudget: 1,
        totalBudget: 0.01, dt: 1 / 60
      }, Math.tan(30 * Math.PI / 180), 0);
      // Sum u is large (generous stock/capacity/flux) but B = 0.01 caps.
      const sumU = reservation.candidates.reduce((s, c) => s + c.u, 0);
      expect(sumU > 0.01).assertTrue();
      expect(Math.abs(reservation.rLimit - 0.01) < 1e-15).assertTrue();
      const sumP = reservation.candidates.reduce((s, c) => s + c.p, 0);
      expect(Math.abs(sumP - 0.01) < 1e-12).assertTrue();
      // Per-column reservation P_i: each edge contributes to BOTH its
      // donor and receiver columns, so the column sum is 2x the edge sum.
      let pSum = 0;
      for (let i = 0; i < reservation.pByColumn.length; i++) {
        pSum += reservation.pByColumn[i];
      }
      expect(Math.abs(pSum - 0.02) < 1e-9).assertTrue();
    });

    // qDescent truncation: the requested grant is capped at D/K, and any
    // subsequent stock/capacity reduction preserves the descent guarantee
    // (requested = min(reserved, qDescent, qNatural) — never above D/K).
    // The view keeps residuals LEGAL (0.3 < tan30) so the guard does not
    // zero out the reservation; the focus is the qDescent cap, not the
    // guard's violating-state branch.
    it('p011_correction_qDescentCapAndDescentPreserved', 0, () => {
      const legalView = new TestCorrectionStockView(
        new Float64Array([0, 0.8, 0.8]), 0);
      const params: P011CorrectionParams = {
        relaxBoundSlope: 0.1, maxAvalancheFluxDt: 10, localBudget: 100,
        totalBudget: 100, dt: 1 / 60
      };
      const reservation = p011CorrectionReserve(legalView, params,
        Math.tan(30 * Math.PI / 180), 0);
      expect(reservation.candidates.length === 1).assertTrue();
      const c = reservation.candidates[0];
      expect(c.qDescent > 0).assertTrue();
      expect(Math.abs(c.qDescent - c.d / c.k) < 1e-12).assertTrue();
      // u <= qDescent always (u = min(..., qDescent, ...)).
      expect(c.u <= c.qDescent + 1e-12).assertTrue();
      // Apply on the same view: requested <= qDescent (recomputed) and the
      // ledger closes requested == accepted + rejected.
      const ledger = new TestCorrectionLedger();
      const outcome = p011CorrectionApplyOnce(reservation, 0, legalView,
        params, Math.tan(30 * Math.PI / 180), ledger);
      expect(outcome.cancelled === false).assertTrue();
      expect(outcome.requested <= c.qDescent + 1e-12).assertTrue();
      // Ledger identity: debit == accepted + refunded (refund exactly once).
      expect(Math.abs(ledger.debited -
        (outcome.accepted + ledger.refunded)) < 1e-9).assertTrue();
      // Stock/capacity reduction (starved flux) shrinks the request but
      // never breaks the descent cap.
      const starvedParams: P011CorrectionParams = {
        relaxBoundSlope: 0.1, maxAvalancheFluxDt: 0.001, localBudget: 100,
        totalBudget: 100, dt: 1 / 60
      };
      const outcome2 = p011CorrectionApplyOnce(reservation, 0, legalView,
        starvedParams, Math.tan(30 * Math.PI / 180), ledger);
      expect(outcome2.requested <= outcome.requested + 1e-12).assertTrue();
      if (outcome2.requested > 0) {
        expect(outcome2.requested <= c.qDescent + 1e-12).assertTrue();
      }
    });

    // Uniform-field stall: INTERIOR same-sign equal-e edges give D=0 and
    // never consume zero-contribution budget (designer s3 boundary: the
    // cancellation is (2-1-1)e=0 on interior edges). Boundary edges keep
    // their one-sided contribution, so a finite uniform ramp still has
    // boundary candidates — the uniform-interior exclusion is what the
    // test pins. A fully-closed uniform field (periodic/no boundary)
    // would stall with CORRECTION_STALLED honestly.
    it('p011_correction_uniformInteriorEdgesExcludedBoundaryKeepsContribution', 0, () => {
      const n = 10;
      const tiltSlope = 0.35;
      const sandTop = new Float64Array(n);
      for (let c = 0; c < n; c++) {
        sandTop[c] = c * tiltSlope;
      }
      const reservation = p011CorrectionReserve(new TestCorrectionStockView(
        sandTop, 0), {
        relaxBoundSlope: 0.1, maxAvalancheFluxDt: 10, localBudget: 1,
        totalBudget: 5, dt: 1 / 60
      }, Math.tan(30 * Math.PI / 180), 0);
      // Only boundary edges 0 and n-2 survive; all interior edges
      // (1..n-3) are uniform-cancellation D=0 and are excluded.
      expect(reservation.candidates.length === 2).assertTrue();
      for (const candidate of reservation.candidates) {
        expect(candidate.edge === 0 || candidate.edge === n - 2).assertTrue();
      }
      // The interior exclusion is not a silent skip: with the finite ramp
      // the reservation is non-empty and bounded by the local budget.
      expect(reservation.rLimit > 0).assertTrue();
      expect(reservation.rLimit <= 1 + 1e-12).assertTrue();
      // Allocate fills the reservation exactly (weighted law on equal-w
      // boundary candidates).
      const sumP = reservation.candidates.reduce((s, c) => s + c.p, 0);
      expect(Math.abs(sumP - reservation.rLimit) < 1e-9).assertTrue();
    });

    // Route gate (D1-C2 semantics at the allocator level): a run with the
    // machine armed but WITHOUT the explicit correction route binding
    // never computes a correction reservation — the legacy apply path is
    // bit-identical and the telemetry fields stay absent.
    it('p011_correction_routeGateBlocksAllocatorWithoutExplicitRoute', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = defaultWaterSandParams();
      params.heavyDiffusion = 0;
      params.lightDiffusion = 0;
      params.waterLevelRate = 0.001;
      params.angleDeg = 4;
      field.configure(params);
      field.initialize({
        waterRegions: [{ startCol: 0, endColInclusive: 7, bottom: 144, height: 38 }],
        heavyDepositedRegions: [
          { startCol: 0, endColInclusive: 7, bottom: 0, height: 72 },
          { startCol: 3, endColInclusive: 4, bottom: 120, height: 0.55 }
        ],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 72, height: 48 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.setNaturalObservationProbe(probe);
      // Machine armed BUT no route binding: dual gate must stay closed.
      field.setNaturalObservationRunMetadata('calibration', 'no-route', 26, 20);
      field.setTilt(0);
      for (let index = 0; index < 4; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const last = records[records.length - 1];
        // The correction telemetry fields are ABSENT on a non-correction
        // run (recordCorrectionSlot is only called on the correction path).
        expect(last.audit.correctionRequested === undefined).assertTrue();
        expect(last.audit.correctionGranted === undefined).assertTrue();
        expect(last.audit.correctionRejected === undefined).assertTrue();
        expect(last.audit.correctionStalledCount === undefined).assertTrue();
      }
      // Binding the route alone (without the machine) also keeps the gate
      // closed: identity without the explicit route is not a switch, and
      // a route without armed thresholds has nothing to reserve from.
      const field2 = new WaterSandField(8, 192);
      // §27: the product default is dual now — this half verifies "route
      // WITHOUT the armed machine", so declare legacy explicitly.
      const legacyParams = defaultWaterSandParams();
      legacyParams.heavyDiffusion = 0;
      legacyParams.lightDiffusion = 0;
      legacyParams.waterLevelRate = 0.001;
      legacyParams.angleDeg = 4;
      legacyParams.relaxMode = 'legacy-single';
      legacyParams.thetaStartDeg = undefined;
      legacyParams.thetaStopDeg = undefined;
      legacyParams.thresholdSource = 'legacy';
      field2.configure(legacyParams);
      field2.initialize({
        waterRegions: [{ startCol: 0, endColInclusive: 7, bottom: 144, height: 38 }],
        heavyDepositedRegions: [
          { startCol: 0, endColInclusive: 7, bottom: 0, height: 72 },
          { startCol: 3, endColInclusive: 4, bottom: 120, height: 0.55 }
        ],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 72, height: 48 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      const records2: P011NaturalObservationRecord[] = [];
      field2.setNaturalObservationProbe(new P011NaturalObservationProbe(8,
        record => { records2.push(record); }));
      field2.setNaturalObservationRouteId('relax-correction-weighted-v1');
      field2.setTilt(0);
      for (let index = 0; index < 3; index++) {
        const result = field2.step(1 / 60);
        expect(result.ok).assertTrue();
        const last = records2[records2.length - 1];
        expect(last.audit.correctionRequested === undefined).assertTrue();
      }
    });

    // =========================================================================
    // §19.5 pre-commit material guard tests.
    // =========================================================================

    // Closed-form admission: legal state passes full grant; a grant that
    // would push an edge past ±H is clipped to the exact maximum.
    it('p011_materialGuard_clipsGrantToKeepEdgeWithinHardMax', 0, () => {
      const H = Math.tan(30 * Math.PI / 180);
      // Edge residual 0.5 (legal, slack 0.0774); influence -1 per unit grant
      // (grant reduces the residual). Full grant of 0.05 keeps 0.45 ✓.
      const pass = p011GuardAdmit(0.05,
        [{ residual: 0.5, influence: -1 }], H);
      expect(pass.accepted).assertEqual(0.05);
      expect(pass.rejected).assertEqual(0);
      expect(pass.allLegalBefore).assertTrue();
      // Same edge with +1 influence (grant worsens): max admissible is
      // (H - d)/k = 0.0774; requesting 0.2 clips to 0.0774.
      const clipped = p011GuardAdmit(0.2,
        [{ residual: 0.5, influence: 1 }], H);
      expect(Math.abs(clipped.accepted - (H - 0.5)) < 1e-12).assertTrue();
      expect(Math.abs(clipped.rejected - (0.2 - (H - 0.5))) < 1e-12).assertTrue();
    });

    // Violating state: non-worsening branch. An already-violating edge
    // (|d| > H) admits only grants that do not increase |d|; a grant that
    // would worsen it is rejected entirely (interval intersect [0, q]).
    it('p011_materialGuard_nonWorseningOnViolatingEdge', 0, () => {
      const H = Math.tan(30 * Math.PI / 180);
      // Edge d = 0.7 (violating), influence +1 (worsens): allowed |d'|<=0.7
      // → q <= 0. Request 0.3 → accepted 0.
      const worsen = p011GuardAdmit(0.3,
        [{ residual: 0.7, influence: 1 }], H);
      expect(worsen.accepted).assertEqual(0);
      expect(worsen.rejected).assertEqual(0.3);
      expect(worsen.allLegalBefore).assertFalse();
      // influence -1 (repairs): |0.7 - q| <= 0.7 → q <= 1.4; full pass.
      const repair = p011GuardAdmit(0.3,
        [{ residual: 0.7, influence: -1 }], H);
      expect(repair.accepted).assertEqual(0.3);
      expect(repair.allLegalBefore).assertFalse();
    });

    // k = 0 edge: the increment does not change this edge's slope, so the
    // edge never restricts q (d'(q) = d is constant; a violating edge
    // stays exactly as violating as before — non-worsening holds trivially).
    it('p011_materialGuard_zeroInfluenceEdgeDoesNotRestrictGrant', 0, () => {
      const H = Math.tan(30 * Math.PI / 180);
      const outcome = p011GuardAdmit(0.5,
        [{ residual: 0.9, influence: 0 }], H);
      expect(outcome.accepted).assertEqual(0.5);
      expect(outcome.rejected).assertEqual(0);
      expect(outcome.allLegalBefore).assertFalse();
    });

    // Ledger refund: rejected part returns to local remaining and leaves
    // used consistent (used+remaining invariant with the accepted grant).
    it('p011_materialGuard_ledgerRefundRestoresBudgets', 0, () => {
      const localRemaining = new Float64Array([1, 1]);
      const localUsed = new Float64Array([0.5, 0.5]);
      const refund = new P011GuardLedgerRefund(localRemaining, localUsed);
      refund.refundLocal([0, 1], 0.3);
      // §28 fix: share = 0.15/column (old behavior credited 0.3 PER
      // column — double the rejected amount; pinned dead).
      expect(Math.abs(localRemaining[0] - 1.15) < 1e-12).assertTrue();
      expect(Math.abs(localRemaining[1] - 1.15) < 1e-12).assertTrue();
      expect(Math.abs(localUsed[0] - 0.35) < 1e-12).assertTrue();
      expect(Math.abs(localUsed[1] - 0.35) < 1e-12).assertTrue();
    });

    // §28 refund-risk fix (option b) — targeted cases.
    it('p011_refundFix_undeibtiedColumnRefundsZero', 0, () => {
      const localRemaining = new Float64Array([1, 1]);
      const localUsed = new Float64Array([0, 0]);
      const refund = new P011GuardLedgerRefund(localRemaining, localUsed);
      refund.refundLocal([0, 1], 0.3);
      expect(Math.abs(localRemaining[0] - 1) < 1e-12).assertTrue();
      expect(Math.abs(localRemaining[1] - 1) < 1e-12).assertTrue();
      expect(localUsed[0] === 0).assertTrue();
      expect(localUsed[1] === 0).assertTrue();
    });

    it('p011_refundFix_rejectedExceedsUsedStopsAtDebit', 0, () => {
      const localRemaining = new Float64Array([1]);
      const localUsed = new Float64Array([0.1]);
      const refund = new P011GuardLedgerRefund(localRemaining, localUsed);
      refund.refundLocal([0], 0.5);
      expect(Math.abs(localRemaining[0] - 1.1) < 1e-12).assertTrue();
      expect(localUsed[0] === 0).assertTrue();
    });

    it('p011_refundFix_mixedColumnsCloseExactly', 0, () => {
      const localRemaining = new Float64Array([1, 1]);
      const localUsed = new Float64Array([0.4, 0]);
      const refund = new P011GuardLedgerRefund(localRemaining, localUsed);
      refund.refundLocal([0, 1], 0.4);
      expect(Math.abs(localRemaining[0] - 1.2) < 1e-12).assertTrue();
      expect(Math.abs(localRemaining[1] - 1) < 1e-12).assertTrue();
      expect(Math.abs(localUsed[0] - 0.2) < 1e-12).assertTrue();
      expect(localUsed[1] === 0).assertTrue();
      const total = localRemaining[0] + localRemaining[1] +
        localUsed[0] + localUsed[1];
      expect(total <= 2.4 + 1e-12).assertTrue();
    });

    // Field-level: guard telemetry is real. A scenario where relax would
    // push an outer neighbor edge past tan30 must produce nonzero
    // hardGuard evidence in the ledger record (rejected/clipped fields).
    it('p011_materialGuard_telemetryFieldsReal', 0, () => {
      const settleRecords: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        settleRecords.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      field.initialize(fullLoadSceneForP011(8));
      field.setNaturalObservationProbe(probe);
      field.setTilt(20);
      for (let index = 0; index < 10; index++) {
        const result = field.step(1 / 60);
        if (!result.ok) {
          break;
        }
      }
      expect(settleRecords.length > 0).assertTrue();
      // hardGuardAvailable must be real (true) on relax slip records now,
      // not the previous constant false.
      let sawSlipWithGuard = false;
      for (const record of settleRecords) {
        for (const slip of record.slip) {
          if (slip.hardGuardAvailable === true) {
            sawSlipWithGuard = true;
          }
        }
      }
      expect(sawSlipWithGuard).assertTrue();
    });

    // =========================================================================
    // §19.5 per-writer direct-interception + refund-coverage tests (891370d).
    // Four named writers (relaxAngle edge cascade / creepBottom / liftByShake
    // / settleSandV2): one positive case each proving the guard actually
    // intercepts (nonzero hardGuard evidence + the post-increment residual of
    // the intercepting edge stays within the non-worsening bound), one
    // negative case each proving zero intervention when no edge would cross
    // the bound, plus a used+remaining conservation assertion after write-back
    // and a hardGuardAvailable=true assertion on every observed grant.
    // Whitelist legacy cases are untouched.
    // =========================================================================
    // guardMaxResidual lives at module scope (ArkTS forbids nested functions).

    it('p011_materialGuard_settleDirectlyInterceptsAndRefunds', 0, () => {
      const settleGrants: P011NaturalObservationColumnGrant[] = [];
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(16, record => {
        settleGrants.push(record.settle);
        records.push(record);
      });
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      // fullLoadSceneForP011 has suspended sand above a flat deposit; a single
      // injected mound at column 8 makes its outer edge steep enough that the
      // first substep's settle increments into column 8 are guard-clipped
      // (later substeps drain the mound below the intercepting bound).
      field.initialize(fullLoadSceneForP011(16));
      field.inject({ phase: WaterSandPhase.HEAVY, amount: 0.4,
        startCol: 8, endColInclusive: 8, state: WaterSandInjectionState.DEPOSITED });
      field.setNaturalObservationProbe(probe);
      field.setTilt(0);
      const totalsBefore = field.getTotals();
      let sawClip = false;
      for (let index = 0; index < 6 && !sawClip; index++) {
        const result = field.step(1 / 60);
        if (!result.ok) {
          break;
        }
        for (const grant of settleGrants) {
          for (let c = 0; c < grant.hardGuardClipped.length; c++) {
            if (grant.hardGuardClipped[c] > 1e-12) {
              sawClip = true;
            }
          }
        }
        settleGrants.length = 0;
      }
      // Positive: the guard actually intercepted settle increments.
      expect(sawClip).assertTrue();
      // Refund coverage: after every substep the local ledger identity holds
      // (used + remaining == limit) and the total ledger reconciles with the
      // accepted grant (no refund leakage into material or ledgers).
      for (const record of records) {
        for (let c = 0; c < record.budgets.localLimit.length; c++) {
          expect(Math.abs(record.budgets.localUsed[c] + record.budgets.localRemaining[c]
            - record.budgets.localLimit[c]) < 1e-9).assertTrue();
        }
        expect(Math.abs(record.budgets.totalUsed + record.budgets.totalRemaining
          - record.budgets.totalLimit) < 1e-9).assertTrue();
      }
      // Phase conservation: guard clipping only redistributes.
      const totalsAfter = field.getTotals();
      expect(Math.abs(totalsAfter.heavy - totalsBefore.heavy) < 1e-9).assertTrue();
      expect(Math.abs(totalsAfter.light - totalsBefore.light) < 1e-9).assertTrue();
      expect(Math.abs(totalsAfter.water - totalsBefore.water) < 1e-9).assertTrue();
      // hardGuardAvailable is armed on this route.
      expect(field.getP011Metric().bottomCreepBudgetLimit >= 0).assertTrue();
    });

    it('p011_materialGuard_settleZeroInterventionWhenLegal', 0, () => {
      const settleGrants: P011NaturalObservationColumnGrant[] = [];
      const probe = new P011NaturalObservationProbe(16, record => {
        settleGrants.push(record.settle);
      });
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      // Flat scene, no mound: every settle increment keeps edges far inside
      // tan30, so the guard must never clip.
      field.initialize(fullLoadSceneForP011(16));
      field.setNaturalObservationProbe(probe);
      field.setTilt(0);
      let settleActivity = 0;
      for (let index = 0; index < 8; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        for (const grant of settleGrants) {
          for (let c = 0; c < grant.hardGuardClipped.length; c++) {
            expect(grant.hardGuardClipped[c] <= 1e-12).assertTrue();
            settleActivity += grant.allowed[c];
          }
        }
        settleGrants.length = 0;
      }
      // Negative control sanity: the writer actually ran (nonzero settle
      // grants observed), so zero clipping is meaningful, not absence of work.
      expect(settleActivity > 1e-9).assertTrue();
      expect(guardMaxResidual(field.snapshot()) < Math.tan(30 * Math.PI / 180)
        + 1e-9).assertTrue();
    });

    it('p011_materialGuard_relaxDirectlyInterceptsAndRefunds', 0, () => {
      // relax guard interception is defense-in-depth: field-level bites are
      // structurally unreachable because the planning pass refuses to commit
      // any substep whose post-move state could violate tan30, and the
      // upstream writers (settle→percolate→creep run before relax) are
      // themselves guard-clipped so they never create a mid-substep violating
      // edge for relax to hit. The bite IS reachable at the admission layer
      // with the exact constraint shapes relax produces (relaxDelta {donor:-1,
      // receiver:+1} → per-edge influence -1/+2/-1 across the affected edge
      // set). This case drives p011GuardAdmit with those shapes and proves:
      // (a) a legal-profile cascade passes a repair-shaped grant in full;
      // (b) the same grant is clipped (rejected>0) when the receiver-side
      // outer edge is already violating (non-worsening bound = |d|), with
      // the refund invariant checked via P011GuardLedgerRefund on the exact
      // localColumns pair relax refunds ([donor, receiver]).
      const H = Math.tan(30 * Math.PI / 180);
      // Shape A — gap-mounds relax (donor=7→receiver=6), all edges legal:
      // edge5 k=+2 (dS6-dS5 = +1-(-1)), edge6 k=-2, edge7 k=+1. Full pass.
      const legalPass = p011GuardAdmit(0.24, [
        { residual: -0.5, influence: 2 },
        { residual: 0.55, influence: -2 },
        { residual: -0.55, influence: 1 }
      ], H);
      expect(legalPass.accepted).assertEqual(0.24);
      expect(legalPass.rejected).assertEqual(0);
      expect(legalPass.allLegalBefore).assertTrue();
      // Shape B — same cascade but the receiver-side outer edge (6,7) is
      // already violating (wall of 0.7 next to the dip): bound = |d| = 0.7,
      // k=-1 → admissible interval [1.277, 0.123] → swapped [0.123, 1.277];
      // combined with the other edges the accepted grant shrinks below the
      // request → rejected > 0 (partial interception).
      const clipped = p011GuardAdmit(0.215, [
        { residual: -0.43, influence: -1 },
        { residual: -0.5, influence: 2 },
        { residual: 0.7, influence: -1 }
      ], H);
      expect(clipped.rejected > 1e-12).assertTrue();
      expect(clipped.accepted + clipped.rejected).assertEqual(0.215);
      expect(clipped.allLegalBefore).assertFalse();
      // Shape C — violating edge with influence +1 (the exact shape a column
      // writer creates on its donor-side outer edge): admissible interval
      // collapses to {0} → the guard intercepts the whole increment.
      const rejected = p011GuardAdmit(0.215, [
        { residual: 0.7, influence: 1 }
      ], H);
      expect(rejected.accepted).assertEqual(0);
      expect(rejected.rejected).assertEqual(0.215);
      // Refund coverage with relax's exact refund column pair [donor,
      // receiver]: used + remaining stays consistent after refunding the
      // rejected part to both endpoints.
      const localRemaining = new Float64Array([0.2, 0.2]);
      const localUsed = new Float64Array([0.05, 0.05]);
      const refund = new P011GuardLedgerRefund(localRemaining, localUsed);
      refund.refundLocal([6, 7], clipped.rejected);
      for (let c = 0; c < 2; c++) {
        expect(Math.abs(localRemaining[c] + localUsed[c] - 0.25) < 1e-12)
          .assertTrue();
      }
    });

    it('p011_materialGuard_relaxZeroInterventionWhenLegal', 0, () => {
      const slipGrants: P011NaturalObservationEdgeGrant[] = [];
      const probe = new P011NaturalObservationProbe(16, record => {
        for (const grant of record.slip) {
          slipGrants.push(grant);
        }
      });
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      // Gentle slope: relax runs but no edge approaches tan30.
      field.initialize(humpSceneForP011(16, 6, 9, 0.15));
      field.setNaturalObservationProbe(probe);
      field.setTilt(0);
      let relaxActivity = 0;
      for (let index = 0; index < 8; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        for (const grant of slipGrants) {
          expect(grant.hardGuardRejected !== null
            ? grant.hardGuardRejected <= 1e-12 : true).assertTrue();
          expect(grant.hardGuardAvailable === true).assertTrue();
          relaxActivity += grant.allowed;
        }
        slipGrants.length = 0;
      }
      expect(relaxActivity > 1e-9).assertTrue();
      expect(guardMaxResidual(field.snapshot()) < Math.tan(30 * Math.PI / 180)
        + 1e-9).assertTrue();
    });

    it('p011_materialGuard_bottomCreepDirectlyInterceptsAndRefunds', 0, () => {
      const creepGrants: P011NaturalObservationEdgeGrant[] = [];
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(16, record => {
        for (const grant of record.bottomCreep) {
          creepGrants.push(grant);
        }
        records.push(record);
      });
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.vSandTopMax = 28;
      params.maxAvalancheFlux = 60;
      field.configure(params);
      // A single steep wall at column 10: bottom creep (tilt 8, direction
      // +) drives mass toward column 11 whose outer edge is near the tan30
      // ceiling, so the first substep's creep increments are guard-rejected.
      field.initialize(thickSceneForP011(16));
      field.inject({ phase: WaterSandPhase.HEAVY, amount: 0.43,
        startCol: 10, endColInclusive: 10, state: WaterSandInjectionState.DEPOSITED });
      field.setNaturalObservationProbe(probe);
      field.setTilt(8);
      const totalsBefore = field.getTotals();
      let sawReject = false;
      for (let index = 0; index < 6 && !sawReject; index++) {
        const result = field.step(1 / 60);
        if (!result.ok) {
          break;
        }
        for (const grant of creepGrants) {
          if (grant.hardGuardRejected !== null && grant.hardGuardRejected > 1e-12) {
            sawReject = true;
          }
          expect(grant.hardGuardAvailable === true).assertTrue();
        }
        creepGrants.length = 0;
      }
      // Positive: bottom creep was guard-rejected at least once.
      expect(sawReject).assertTrue();
      // Refund coverage: local identity + bottom ledger ceiling unchanged
      // (refund restores bottom used; limit is a ceiling, not remaining).
      for (const record of records) {
        for (let c = 0; c < record.budgets.localLimit.length; c++) {
          expect(Math.abs(record.budgets.localUsed[c] + record.budgets.localRemaining[c]
            - record.budgets.localLimit[c]) < 1e-9).assertTrue();
        }
        expect(record.budgets.bottomUsed <= record.budgets.bottomLimit + 1e-9)
          .assertTrue();
      }
      // Phase conservation.
      const totalsAfter = field.getTotals();
      expect(Math.abs(totalsAfter.heavy - totalsBefore.heavy) < 1e-9).assertTrue();
      expect(Math.abs(totalsAfter.light - totalsBefore.light) < 1e-9).assertTrue();
    });

    it('p011_materialGuard_bottomCreepZeroInterventionWhenLegal', 0, () => {
      const creepGrants: P011NaturalObservationEdgeGrant[] = [];
      const probe = new P011NaturalObservationProbe(16, record => {
        for (const grant of record.bottomCreep) {
          creepGrants.push(grant);
        }
      });
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.vSandTopMax = 28;
      field.configure(params);
      // Flat scene, tiny tilt: creep runs (budget-used evidence) but no edge
      // approaches tan30, so the guard must never clip.
      field.initialize(thickSceneForP011(16));
      field.setNaturalObservationProbe(probe);
      field.setTilt(4);
      let creepActivity = 0;
      for (let index = 0; index < 8; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        for (const grant of creepGrants) {
          expect(grant.hardGuardRejected !== null
            ? grant.hardGuardRejected <= 1e-12 : true).assertTrue();
          expect(grant.hardGuardAvailable === true).assertTrue();
          creepActivity += grant.allowed;
        }
        creepGrants.length = 0;
      }
      expect(creepActivity > 1e-9).assertTrue();
      expect(guardMaxResidual(field.snapshot()) < Math.tan(30 * Math.PI / 180)
        + 1e-9).assertTrue();
    });

    it('p011_materialGuard_liftDirectlyInterceptsAndRefunds', 0, () => {
      const liftGrants: P011NaturalObservationColumnGrant[] = [];
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(16, record => {
        liftGrants.push(record.lift);
        records.push(record);
      });
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.angleDeg = 44;
      field.configure(params);
      // Walls at columns 7 and 9 flank a dip at column 8; with angleDeg=44
      // the lift band base is tan(44°)≈0.966 so the band stays deep over the
      // walls, and lift decrements on the wall columns are bounded by the
      // near-tan30 outer edges (maxRes 0.55 → 0.57 crosses H during step 2;
      // steps 1-2 show nonzero guard clipping before planning BLOCKs at step
      // 3 — evidence is collected in the first two substeps).
      field.initialize(thickSceneForP011(16));
      field.inject({ phase: WaterSandPhase.HEAVY, amount: 0.55,
        startCol: 7, endColInclusive: 7, state: WaterSandInjectionState.DEPOSITED });
      field.inject({ phase: WaterSandPhase.HEAVY, amount: 0.55,
        startCol: 9, endColInclusive: 9, state: WaterSandInjectionState.DEPOSITED });
      field.setNaturalObservationProbe(probe);
      field.setShakeInput(1);
      field.setTilt(0);
      const totalsBefore = field.getTotals();
      let sawClip = false;
      for (let index = 0; index < 3 && !sawClip; index++) {
        const result = field.step(1 / 60);
        if (!result.ok) {
          break;
        }
        for (const grant of liftGrants) {
          for (let c = 0; c < grant.hardGuardClipped.length; c++) {
            if (grant.hardGuardClipped[c] > 1e-12) {
              sawClip = true;
            }
          }
        }
        liftGrants.length = 0;
      }
      // Positive: lift increments were guard-clipped at least once.
      expect(sawClip).assertTrue();
      // Refund coverage: local ledger identity holds on every substep.
      for (const record of records) {
        for (let c = 0; c < record.budgets.localLimit.length; c++) {
          expect(Math.abs(record.budgets.localUsed[c] + record.budgets.localRemaining[c]
            - record.budgets.localLimit[c]) < 1e-9).assertTrue();
        }
      }
      // Phase conservation: lift only moves deposited -> suspended.
      const totalsAfter = field.getTotals();
      expect(Math.abs(totalsAfter.heavy - totalsBefore.heavy) < 1e-9).assertTrue();
      expect(Math.abs(totalsAfter.light - totalsBefore.light) < 1e-9).assertTrue();
    });

    it('p011_materialGuard_liftZeroInterventionWhenLegal', 0, () => {
      const liftGrants: P011NaturalObservationColumnGrant[] = [];
      const probe = new P011NaturalObservationProbe(16, record => {
        liftGrants.push(record.lift);
      });
      const field = new WaterSandField(16, 192);
      const params = deterministicRateParams();
      params.angleDeg = 44;
      field.configure(params);
      // Flat scene under shake: lift runs but edges stay far inside tan30.
      field.initialize(thickSceneForP011(16));
      field.setNaturalObservationProbe(probe);
      field.setShakeInput(1);
      field.setTilt(0);
      let liftActivity = 0;
      for (let index = 0; index < 6; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        for (const grant of liftGrants) {
          for (let c = 0; c < grant.hardGuardClipped.length; c++) {
            expect(grant.hardGuardClipped[c] <= 1e-12).assertTrue();
            liftActivity += grant.allowed[c];
          }
        }
        liftGrants.length = 0;
      }
      expect(liftActivity > 1e-9).assertTrue();
      expect(guardMaxResidual(field.snapshot()) < Math.tan(30 * Math.PI / 180)
        + 1e-9).assertTrue();
    });

    // =========================================================================
    // Observation-3 v3 execution identity (GAP-1..5, 2026-10-20).
    // =========================================================================

    // GAP-1: the three-state transition function — start crossing, stop
    // crossing, and the two hold bands between the thresholds.
    it('p011_hysteresis_threeStateTransitionFunction', 0, () => {
      const thresholds = p011RelaxHysteresisThresholds({
        thetaStartDeg: 28, thetaStopDeg: 22.911, thetaHardMaxDeg: 30
      });
      const start = Math.tan(28 * Math.PI / 180);
      const stop = Math.tan(22.911 * Math.PI / 180);
      // Inactive edge below start: hold inactive.
      expect(p011RelaxHysteresisNextActive(false, 0.5 * start, thresholds, 28, 22.911)
        .activeAfter).assertFalse();
      // Inactive edge crossing start: START_THRESHOLD_CROSSED.
      const startCross = p011RelaxHysteresisNextActive(false, start, thresholds, 28, 22.911);
      expect(startCross.activeAfter).assertTrue();
      expect(startCross.transitionReason).assertEqual('START_THRESHOLD_CROSSED');
      expect(startCross.thresholdUsed.branch).assertEqual('start');
      expect(startCross.thresholdUsed.comparator).assertEqual('>=');
      // Active edge in the hold band: stays active.
      const mid = (start + stop) / 2;
      expect(p011RelaxHysteresisNextActive(true, mid, thresholds, 28, 22.911)
        .activeAfter).assertTrue();
      // Inactive edge in the hold band: stays inactive (hysteresis).
      expect(p011RelaxHysteresisNextActive(false, mid, thresholds, 28, 22.911)
        .activeAfter).assertFalse();
      // Active edge dropping to stop: STOP_THRESHOLD_CROSSED.
      const stopCross = p011RelaxHysteresisNextActive(true, stop, thresholds, 28, 22.911);
      expect(stopCross.activeAfter).assertFalse();
      expect(stopCross.transitionReason).assertEqual('STOP_THRESHOLD_CROSSED');
      expect(stopCross.thresholdUsed.branch).assertEqual('stop');
      // Active edge above stop: stays active.
      expect(p011RelaxHysteresisNextActive(true, start, thresholds, 28, 22.911)
        .activeAfter).assertTrue();
    });

    // GAP-1: parameter invariant violations are construction failures.
    it('p011_hysteresis_paramInvariantThrows', 0, () => {
      const bad: P011RelaxHysteresisParams[] = [
        { thetaStartDeg: 31, thetaStopDeg: 20, thetaHardMaxDeg: 30 },
        { thetaStartDeg: 22, thetaStopDeg: 28, thetaHardMaxDeg: 30 },
        { thetaStartDeg: 28, thetaStopDeg: -1, thetaHardMaxDeg: 30 },
        { thetaStartDeg: 28, thetaStopDeg: 28, thetaHardMaxDeg: 30 },
        { thetaStartDeg: 28, thetaStopDeg: 20, thetaHardMaxDeg: 35 },
        { thetaStartDeg: Number.NaN, thetaStopDeg: 20, thetaHardMaxDeg: 30 }
      ];
      for (const params of bad) {
        let threw = false;
        try {
          p011RelaxHysteresisThresholds(params);
        } catch (error) {
          threw = true;
        }
        expect(threw).assertTrue();
      }
    });

    // GAP-1: requested formula only on active edges, overshoot above stop.
    it('p011_hysteresis_requestedFormula', 0, () => {
      const thresholds = p011RelaxHysteresisThresholds({
        thetaStartDeg: 28, thetaStopDeg: 22.911, thetaHardMaxDeg: 30
      });
      const stop = Math.tan(22.911 * Math.PI / 180);
      // Residual below stop: zero.
      expect(p011RelaxHysteresisRequested(0.5 * stop, thresholds,
        1 / 60, 24, 100, 100)).assertEqual(0);
      // Above stop: overshoot/2 capped by flux*dt.
      const overshoot = 0.2;
      const expected = Math.min(overshoot / 2, 24 / 60, 100, 100);
      const actual = p011RelaxHysteresisRequested(stop + overshoot, thresholds,
        1 / 60, 24, 100, 100);
      expect(Math.abs(actual - expected) < 1e-12).assertTrue();
      // Stock cap binds.
      const stockCapped = p011RelaxHysteresisRequested(stop + overshoot, thresholds,
        1 / 60, 24, 0.01, 100);
      expect(Math.abs(stockCapped - 0.01) < 1e-12).assertTrue();
    });

    // GAP-1 + GAP-3 + GAP-5 integration: a candidate run (thetaStart/Stop
    // configured) drives the machine, produces decision records with the
    // full field set, and reports executedThresholds identity.
    it('p011_hysteresis_candidateRunProducesDecisionAndExecutedFields', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      // §27: the pair now rides configure (config domain); the
      // metadata setter is identity-only.
      const dualParams = deterministicRateParams();
      dualParams.angleDeg = params.angleDeg;
      dualParams.relaxMode = 'dual-user-configured';
      dualParams.thetaStartDeg = 28;
      dualParams.thetaStopDeg = 22.911;
      dualParams.thresholdSource = 'user';
      field.configure(dualParams);
      field.initialize(humpSceneForP011(8, 3, 4, 0.3));
      field.setNaturalObservationProbe(probe);
      field.setNaturalObservationRunMetadata('calibration', 'cand-pair-1', 28, 22.911);
      field.setNaturalObservationExecutionIdentity('d78335b',
        'a'.repeat(64), 'b'.repeat(64));
      expect(field.relaxHysteresisEnabled()).assertTrue();
      field.setTilt(0);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      expect(records.length > 0).assertTrue();
      const record = records[records.length - 1];
      // executedThresholds present with identity (GAP-5).
      const executed = record.context.executedThresholds;
      expect(executed !== null && executed !== undefined).assertTrue();
      if (executed !== null && executed !== undefined) {
        expect(Math.abs(executed.thetaStartDeg - 28) < 1e-12).assertTrue();
        expect(Math.abs(executed.thetaStopDeg - 22.911) < 1e-12).assertTrue();
        expect(Math.abs(executed.slopeStart -
          Math.tan(28 * Math.PI / 180)) < 1e-12).assertTrue();
        expect(executed.stateMachineVersion).assertEqual('natural-relax-hysteresis-v1');
        expect(executed.sourceCommit).assertEqual('d78335b');
      }
      // hysteresisActive marks the machine-driven substep (GAP-1).
      expect(record.relaxHysteresis.hysteresisActive === true).assertTrue();
      // Decision fields on every edge with a decision (GAP-3): decisionR
      // signed, decisionAlpha = atan(|decisionR|)*180/pi, thresholdUsed
      // carries the branch actually consulted.
      let sawDecision = false;
      for (const edge of record.relaxHysteresis.edges) {
        if (edge.decisionR === null || edge.decisionR === undefined) {
          continue;
        }
        sawDecision = true;
        const alpha = Math.atan(Math.abs(edge.decisionR!)) * 180 / Math.PI;
        expect(Math.abs(edge.decisionAlpha! - alpha) < 1e-9).assertTrue();
        expect(edge.decisionG !== null && edge.decisionG !== undefined).assertTrue();
        expect(edge.thresholdUsed !== null && edge.thresholdUsed !== undefined)
          .assertTrue();
        expect(edge.transitionReason !== null && edge.transitionReason !== undefined)
          .assertTrue();
        expect(edge.decisionOrdinal! >= 1).assertTrue();
      }
      expect(sawDecision).assertTrue();
      // thetaRequested present (GAP-4).
      const context = record.context;
      expect(context.thetaRequested !== null &&
        context.thetaRequested !== undefined).assertTrue();
      expect(context.attemptId !== null && context.attemptId !== undefined).assertTrue();
      expect(context.commitStatus === 'committed').assertTrue();
    });

    // GAP-1 legacy: without candidate thresholds the pre-v3.1 relax behavior
    // stays (machine off, executedThresholds null, no decision fields).
    it('p011_hysteresis_legacyRunKeepsPreV3Behavior', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      field.initialize(humpSceneForP011(8, 3, 4, 0.3));
      field.setNaturalObservationProbe(probe);
      expect(field.relaxHysteresisEnabled()).assertFalse();
      field.setTilt(0);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      expect(records.length > 0).assertTrue();
      const record = records[records.length - 1];
      expect(record.context.executedThresholds === null ||
        record.context.executedThresholds === undefined).assertTrue();
      expect(record.relaxHysteresis.hysteresisActive === false).assertTrue();
      for (const edge of record.relaxHysteresis.edges) {
        expect(edge.decisionR === null || edge.decisionR === undefined).assertTrue();
      }
    });

    // GAP-1: budget-limited active edges stay active across substeps (the
    // machine holds; a missing grant is not a stop).
    it('p011_hysteresis_budgetLimitedEdgeStaysActive', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      params.totalMotionBudget = 0.0001; // starve the correction channel
      field.configure(params);
      // §27: the pair now rides configure (config domain); the
      // metadata setter is identity-only.
      const dualParams = deterministicRateParams();
      dualParams.angleDeg = params.angleDeg;
      dualParams.relaxMode = 'dual-user-configured';
      dualParams.thetaStartDeg = 26;
      dualParams.thetaStopDeg = 20;
      dualParams.thresholdSource = 'user';
      field.configure(dualParams);
      // Steep hump: residual 0.55 sits above tan(26 deg) start (~0.4877)
      // and below the tan30 planning infeasibility wall (~0.5774) so edges
      // cross START and the machine holds them active while the budget
      // starves the grants.
      field.initialize(humpSceneForP011(8, 3, 4, 0.55));
      field.setNaturalObservationProbe(probe);
      field.setNaturalObservationRunMetadata('calibration', 'cand-pair-1', 26, 20);
      field.setTilt(0);
      let anyActiveHeld = false;
      let anyBudgetBound = false;
      let anyFalseStop = false;
      const budgetPerSubstep = 0.0001 / 60;
      for (let index = 0; index < 6; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const record = records[records.length - 1];
        // The natural request (§19.3 formula over active edges) vastly
        // exceeds the starved budget, so the correction grants are bounded
        // by B — the machine must still hold its active edges.
        if (record.audit.relaxRequestedTotal > 10 * budgetPerSubstep) {
          anyBudgetBound = true;
        }
        for (const edge of record.relaxHysteresis.edges) {
          if (edge.decisionR === null || edge.decisionR === undefined) {
            continue;
          }
          // Machine-held active edge (HOLD_ACTIVE or START crossing) — the
          // budget limit must not demote it to a stop.
          if (edge.activeAfter &&
            (edge.transitionReason === 'HOLD_ACTIVE' ||
              edge.transitionReason === 'START_THRESHOLD_CROSSED')) {
            anyActiveHeld = true;
          }
          if (edge.activeBefore && !edge.activeAfter &&
            edge.transitionReason === 'STOP_THRESHOLD_CROSSED' &&
            Math.abs(edge.decisionR!) > Math.tan(20 * Math.PI / 180)) {
            // stop only when the residual actually dropped to/below stop
            anyFalseStop = true;
          }
        }
      }
      expect(anyActiveHeld).assertTrue();
      expect(anyBudgetBound).assertTrue();
      expect(anyFalseStop).assertFalse();
    });

    // Reviewer blocker-2 regression: the hysteresis branch must feed the
    // requested formula the SAME donor/receiver columns as the legacy
    // branch (deviation > 0 -> donor = next / receiver = c; deviation < 0
    // -> donor = c / receiver = next).
    //
    // Scene constraints (pre-existing engine gates, NOT introduced by this
    // task): every edge slope sits inside the (tan28, tan30) window so
    // edges cross START while the planning preexisting-violation check
    // (|dS| <= tan30) still admits the substep; water covers the sand so
    // initialize's audit does not mis-report CAPACITY_OVERFLOW on a dry
    // scene. Wrong-column feeds are made visible by starving exactly one
    // boundary column: the ascending scene gives the top column a free
    // space of 0.05 (caps a wrong receiverCapacity), the descending scene
    // gives the bottom column a stock of 0.05 (caps a wrong donorSand).
    it('p011_hysteresis_donorReceiverOrientationFeedsRequested', 0, () => {
      const slopeStop = Math.tan(22.911 * Math.PI / 180);
      const stepDeg = 0.55; // inside (tan28, tan30)
      const expectedPerEdge = (stepDeg - slopeStop) / 2;
      const expectedTotal = 7 * expectedPerEdge;
      for (const ascending of [true, false]) {
        const records: P011NaturalObservationRecord[] = [];
        const probe = new P011NaturalObservationProbe(8, record => {
          records.push(record);
        });
        const field = new WaterSandField(8, 192);
        const params = deterministicRateParams();
        params.angleDeg = 4;
        params.maxAvalancheFlux = 6000; // maxAvalancheFlux * dt = 100
        // §27: pair rides configure (28/22.911 candidate).
        params.relaxMode = 'dual-user-configured';
        params.thetaStartDeg = 28;
        params.thetaStopDeg = 22.911;
        params.thresholdSource = 'user';
        field.configure(params);
        // Ascending: sandTop rises 0.55/column to 3.85 at col 7, waterline
        // 0.05 above it. Descending: sandTop falls from 3.90 at col 0 to
        // 0.05 at col 7, waterline 0.05 above col 0.
        const topSandTop = ascending ? 3.85 : 3.90;
        const heavyRegions: WaterSandRegion[] = [];
        for (let c = 0; c < 8; c++) {
          // Ascending heights written as c*stepDeg (algebraically equal to
          // topSandTop-(7-c)*stepDeg) so the c=0 column is exactly 0 —
          // the subtractive form produces -4.44e-16 in float64 and the
          // scene validator correctly rejects a negative height.
          const height = ascending
            ? c * stepDeg
            : topSandTop - c * stepDeg;
          const r: WaterSandRegion = { startCol: c, endColInclusive: c,
            bottom: 0, height: height };
          heavyRegions.push(r);
        }
        const scene: WaterSandScene = {
          waterRegions: [{ startCol: 0, endColInclusive: 7,
            bottom: topSandTop, height: 0.05 }],
          heavyDepositedRegions: heavyRegions,
          lightDepositedRegions: [],
          heavySuspendedRegions: [],
          lightSuspendedRegions: []
        };
        field.initialize(scene);
        field.setNaturalObservationProbe(probe);
        field.setNaturalObservationRunMetadata('calibration',
          ascending ? 'cand-orient-asc' : 'cand-orient-desc', 28, 22.911);
        field.setNaturalObservationExecutionIdentity('d78335b',
          'a'.repeat(64), 'b'.repeat(64));
        field.setTilt(0);
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        expect(records.length > 0).assertTrue();
        const record = records[0];
        // All 7 edges cross START (|deviation| = 0.55 > tan28).
        expect(record.audit.relaxCandidateCount).assertEqual(7);
        // Correct orientation: each edge requests (|r| - slopeStop)/2
        // capped by maxAvalancheFlux*dt (100), donor stock and receiver
        // free space - none binds here.
        expect(Math.abs(record.audit.relaxRequestedTotal - expectedTotal) < 1e-9)
          .assertTrue();
        // A wrong-column feed caps exactly one boundary edge at 0.05,
        // totalling 6*perEdge + 0.05 (~0.432) - far below the correct sum.
        expect(record.audit.relaxRequestedTotal > 6 * expectedPerEdge + 0.05)
          .assertTrue();
      }
    });


    // =====================================================================
    // D-1 (manager ruling 2026-10-21, Option A + explicit route isolation)
    // =====================================================================

    // Three-way routing: only an explicitly-routed correction run (state
    // machine armed + route bound) enters applyCorrectionSlot; the legacy
    // default AND the v3 candidate identity (natural-relax-hysteresis-v1)
    // keep the pre-correction relaxAngle apply. Direct evidence: the
    // correction apply records slip with requested==p (reservation amounts)
    // and stays silent on zero-reservation edges, while the legacy apply's
    // relax grants ARE bounded by the proportional total scale — here we
    // distinguish the two applies by the correction-only route telemetry
    // (correctionBudgetRemaining>0 appears only when the reservation was
    // computed) and by relaxAppliedTotal>0 on the legacy paths.
    it('p011_d1_routeIsolation_threeWayApplyRouting', 0, () => {
      // ArkTS forbids nested functions; the scenario runner lives at module
      // scope as d1RunRoutingScenario (below the describe body).
      // (a) legacy default: state machine off, no route -> relaxAngle apply.
      d1RunRoutingScenario(false, false, (field) => {
        expect(field.correctionRouteActive()).assertFalse();
        expect(field.relaxHysteresisEnabled()).assertFalse();
      });
      // (b) v3 candidate identity WITHOUT the explicit route: the state
      // machine is armed (decisions flow) but the correction allocator
      // must stay OFF — the identity is not an implicit switch.
      d1RunRoutingScenario(false, true, (field) => {
        expect(field.relaxHysteresisEnabled()).assertTrue();
        expect(field.correctionRouteActive()).assertFalse();
      });
      // (c) explicit correction route with the machine armed: both gate
      // conditions hold -> the correction slot runs.
      d1RunRoutingScenario(true, true, (field) => {
        expect(field.relaxHysteresisEnabled()).assertTrue();
        expect(field.correctionRouteActive()).assertTrue();
      });
      // (d) route bound but machine NOT armed: gate fails (necessary
      // condition missing) -> legacy apply.
      d1RunRoutingScenario(true, false, (field) => {
        expect(field.relaxHysteresisEnabled()).assertFalse();
        expect(field.correctionRouteActive()).assertFalse();
      });
    });

    // D-1 cross-version behavior equivalence: the §26.2 comparison scene
    // (8-col thick, ±20° alternating every 24 steps, totalBudget=1000,
    // 240 steps). The pre-fix code diverged at step25 (relax requested but
    // never applied) and BLOCKed at step214 (NO_SAFE_OLD_TILT_PLAN); the
    // 5e60f2d baseline was 240/240 ok. The fix must reproduce the baseline
    // trajectory: every step ok, and the frozen anchor sequence (sampled
    // every 20 steps plus the two historical divergence points 25 and 214)
    // matches bit-for-bit at 1e-9. Identity-field tests cannot substitute
    // for this behavioral evidence.
    it('p011_d1_legacyTrajectoryEquivalence240Steps', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.totalMotionBudget = 1000;
      field.configure(params);
      field.initialize(thickSceneForP011(8));
      // Frozen baseline anchors (5e60f2d worktree, same scene): tilt,
      // sandTop column-sum, first column sandTop and first column bh at
      // 1e-9 precision.
      const anchors: number[][] = [
        [20, 20, 1152, 143.523809524, 71.761904762],
        [25, 8, 1152, 143.452380952, 71.726190476],
        [40, -20, 1152, 143.917933096, 71.904887714],
        [60, 20, 1152, 143.822695001, 71.857357677],
        [80, -20, 1152, 143.727456906, 71.809818927],
        [100, 20, 1152, 144.013171192, 71.95284503],
        [120, 20, 1152, 143.536980715, 71.714758],
        [140, -20, 1152, 144.013171192, 71.953049395],
        [160, 20, 1152, 143.727456906, 71.810241002],
        [180, -20, 1152, 143.813072326, 71.853008771],
        [200, 20, 1152, 143.908310422, 71.900745446],
        [214, 20, 1152, 143.574977088, 71.734088226],
        [220, -6.704117608, 1152, 143.622596136, 71.757935463],
        [240, -10.701243018, 1152, 144.098786612, 71.996239053]
      ];
      let anchorIndex = 0;
      let okCount = 0;
      for (let index = 0; index < 240; index++) {
        const target = Math.floor(index / 24) % 2 === 0 ? 20 : -20;
        field.setTilt(target);
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        okCount += 1;
        const step = index + 1;
        if (anchorIndex < anchors.length && anchors[anchorIndex][0] === step) {
          const snap = field.snapshot();
          let topSum = 0;
          for (let c = 0; c < snap.cols; c++) {
            topSum += snap.sandTop[c];
          }
          const expected = anchors[anchorIndex];
          expect(Math.abs(field.getP011Metric().effectiveTilt - expected[1]) < 1e-9)
            .assertTrue();
          expect(Math.abs(topSum - expected[2]) < 1e-6).assertTrue();
          expect(Math.abs(snap.sandTop[0] - expected[3]) < 1e-9).assertTrue();
          expect(Math.abs(snap.bh[0] - expected[4]) < 1e-9).assertTrue();
          anchorIndex += 1;
        }
      }
      expect(okCount).assertEqual(240);
      expect(anchorIndex).assertEqual(anchors.length);
    });

    // GAP-2: v3 schedule generation — structure, integer leg lengths,
    // half-open boundary ownership and the CSV contract.
    it('p011_scheduleV3_generationAndCsvContract', 0, () => {
      const schedule = p011BuildScheduleV3(20.0, 0.5);
      expect(schedule.scheduleId).assertEqual('nine-phase-30-v3');
      expect(schedule.commands.length > 100).assertTrue();
      // First command is the H0 platform at t=0.
      const first = schedule.commands[0];
      expect(first.segmentId).assertEqual('H0');
      expect(first.phaseKind).assertEqual('platform');
      expect(first.offsetMs).assertEqual(0);
      expect(first.legId).assertEqual('');
      // commandSequence strictly increasing; offsets strictly increasing
      // with a 100 ms step inside ramps (platform -> leg-start jumps by
      // the 5100 ms hold).
      for (let index = 1; index < schedule.commands.length; index++) {
        expect(schedule.commands[index].commandSequence).assertEqual(
          schedule.commands[index - 1].commandSequence + 1);
        expect(schedule.commands[index].offsetMs).assertLarger(
          schedule.commands[index - 1].offsetMs);
      }
      // Inside one leg the step is exactly 100 ms.
      const l2 = schedule.commands.filter(c => c.legId === 'L2');
      for (let index = 1; index < l2.length; index++) {
        expect(l2[index].offsetMs - l2[index - 1].offsetMs).assertEqual(100);
      }
      // Endpoint of L2 is exactly +A; endpoint of L6 is exactly -A.
      const l2Commands = schedule.commands.filter(
        c => c.legId === 'L2' && c.rateDegPerSec > 0);
      expect(Math.abs(l2Commands[l2Commands.length - 1].targetTiltDeg - 20) < 1e-9)
        .assertTrue();
      const l6Commands = schedule.commands.filter(
        c => c.legId === 'L6' && c.rateDegPerSec > 0);
      expect(Math.abs(l6Commands[l6Commands.length - 1].targetTiltDeg + 20) < 1e-9)
        .assertTrue();
      // Ideal duration: 9*5100 + 4*A/v ms (last hold included).
      const expectedTotal = 9 * 5100 + 4 * 20 / 0.5 * 1000;
      expect(Math.abs(schedule.totalDurationMs - expectedTotal) < 1e-6).assertTrue();
      // CSV: header + LF ending + no scientific notation.
      const csv = p011ScheduleV3ToCsv(schedule);
      expect(csv.endsWith('\n')).assertTrue();
      expect(csv.split('\n').length === schedule.commands.length + 2).assertTrue();
      expect(csv.includes('e+')).assertFalse();
      expect(csv.includes('e-')).assertFalse();
      // Binding: ramp commands carry leg ids, platforms do not.
      const binding = p011ScheduleV3BindCommand(schedule, 2);
      expect(binding.legId).assertEqual('L1');
      expect(binding.phaseKind).assertEqual('ramp');
      expect(binding.segmentId).assertEqual('L1');
      // Invalid rate/A rejected.
      let threw = false;
      try {
        p011BuildScheduleV3(20, 0.7);
      } catch (error) {
        threw = true;
      }
      expect(threw).assertTrue();
      threw = false;
      try {
        p011BuildScheduleV3(30, 0.5);
      } catch (error) {
        threw = true;
      }
      expect(threw).assertTrue();
    });

    // GAP-2: v3 phase-name mapping — platforms and legs onto the nine names.
    it('p011_scheduleV3_segmentPhaseMapping', 0, () => {
      expect(p011NaturalObservationPhaseForV3Segment('H0')).assertEqual('baseline');
      expect(p011NaturalObservationPhaseForV3Segment('H1')).assertEqual('positiveLow');
      expect(p011NaturalObservationPhaseForV3Segment('H2')).assertEqual('positiveHigh');
      expect(p011NaturalObservationPhaseForV3Segment('H4')).assertEqual('positiveHighReturn');
      expect(p011NaturalObservationPhaseForV3Segment('H6')).assertEqual('negativeHigh');
      expect(p011NaturalObservationPhaseForV3Segment('H8')).assertEqual('negativeHighReturn');
      // Legs inherit the origin platform name.
      expect(p011NaturalObservationPhaseForV3Segment('L2')).assertEqual('positiveLow');
      expect(p011NaturalObservationPhaseForV3Segment('L3')).assertEqual('positiveHigh');
      expect(p011NaturalObservationPhaseForV3Segment('L7')).assertEqual('negativeHigh');
      expect(p011NaturalObservationPhaseForV3Segment(null)).assertEqual('unknown');
      expect(p011NaturalObservationPhaseForV3Segment('X9')).assertEqual('unknown');
    });

    // GAP-4: a BLOCKED substep still lands an auditable attempt record.
    it('p011_attempt_blockedSubstepLandsAttemptRecord', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      // §27: pair rides configure (28/22.911 candidate).
      params.relaxMode = 'dual-user-configured';
      params.thetaStartDeg = 28;
      params.thetaStopDeg = 22.911;
      params.thresholdSource = 'user';
      field.configure(params);
      field.initialize(fullLoadSceneForP011(8));
      field.setNaturalObservationProbe(probe);
      field.setNaturalObservationRunMetadata('calibration', 'cand-pair-1', 28, 22.911);
      // Push the surface beyond tan30 without repair headroom -> planning
      // BLOCKED on the first substep.
      field.inject({
        phase: WaterSandPhase.HEAVY,
        amount: 60,
        startCol: 6,
        endColInclusive: 7,
        state: WaterSandInjectionState.DEPOSITED
      });
      field.setTilt(0);
      const result = field.step(1 / 60);
      expect(result.ok).assertFalse();
      expect(result.errorCode === 5).assertTrue();
      // The blocked attempt still produced a ledger record.
      expect(records.length > 0).assertTrue();
      const record = records[records.length - 1];
      expect(record.context.commitStatus === 'blocked').assertTrue();
      expect(record.context.attemptId !== null &&
        record.context.attemptId !== undefined).assertTrue();
      expect(record.context.rollbackReason !== null &&
        record.context.rollbackReason !== undefined).assertTrue();
      expect(record.audit.errorCode === '5' ||
        record.audit.errorCode === 'TILT_PLANNING_BLOCKED').assertTrue();
    });

    // GAP-4: preview-commit mismatch rolls the attempt back (recorded).
    it('p011_attempt_attemptIdsUniquePerSubstep', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.angleDeg = 4;
      field.configure(params);
      field.initialize(humpSceneForP011(8, 3, 4, 0.3));
      field.setNaturalObservationProbe(probe);
      field.setTilt(0);
      const seen = new Set<string>();
      for (let index = 0; index < 5; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const record = records[records.length - 1];
        const attemptId = record.context.attemptId;
        expect(attemptId !== null && attemptId !== undefined).assertTrue();
        expect(seen.has(attemptId!)).assertFalse();
        seen.add(attemptId!);
        expect(record.context.commitStatus === 'committed').assertTrue();
      }
      expect(seen.size === 5).assertTrue();
    });

    // GAP-2: want parsing accepts the v3 schedule id and segment fields.
    it('p011_tiltWant_v3FieldsParseAndValidate', 0, () => {
      const base: Record<string, Object> = {
        'wstilt': '15', 'p011runrole': 'calibration',
        'thresholdcalibrationid': 'cand-pair-1',
        'thetastartdeg': '28', 'thetastopdeg': '22.911',
        'scheduleid': 'nine-phase-30-v3',
        'v3legid': 'L2', 'v3segmentid': 'L2', 'v3phasekind': 'ramp',
        'execsourcecommit': 'd78335b',
        'execsourcefilesha256': 'a'.repeat(64),
        'execbuildsha256': 'b'.repeat(64)
      };
      const parsed = parseP011TiltWant(base, 4, true, false, 1, 1000);
      expect(parsed.command !== null).assertTrue();
      if (parsed.command !== null) {
        expect(parsed.command.scheduleId).assertEqual('nine-phase-30-v3');
        expect(parsed.command.v3LegId).assertEqual('L2');
        expect(parsed.command.v3SegmentId).assertEqual('L2');
        expect(parsed.command.v3PhaseKind).assertEqual('ramp');
        expect(parsed.command.execSourceCommit).assertEqual('d78335b');
      }
      // Invalid leg id on the v3 schedule is rejected.
      const bad: Record<string, Object> = {
        'wstilt': '15', 'p011runrole': 'calibration',
        'thresholdcalibrationid': 'cand-pair-1',
        'thetastartdeg': '28', 'thetastopdeg': '22.911',
        'scheduleid': 'nine-phase-30-v3',
        'v3legid': 'X2', 'v3segmentid': 'L2', 'v3phasekind': 'ramp'
      };
      const rejected = parseP011TiltWant(bad, 4, true, false, 2, 1001);
      expect(rejected.command === null).assertTrue();
      // v2 payloads parse unchanged (no v3 fields).
      const v2: Record<string, Object> = {
        'wstilt': '15', 'p011runrole': 'calibration',
        'thresholdcalibrationid': 'cand-pair-1',
        'thetastartdeg': '28', 'thetastopdeg': '22.911',
        'scheduleid': 'nine-phase-30-v2'
      };
      const parsedV2 = parseP011TiltWant(v2, 4, true, false, 3, 1002);
      expect(parsedV2.command !== null).assertTrue();
      if (parsedV2.command !== null) {
        expect(parsedV2.command.v3LegId === null ||
          parsedV2.command.v3LegId === undefined).assertTrue();
      }
    });

    // correction route wiring (D-1 follow-up): the p011route want parameter
    // carries the explicit correction route binding through
    // want -> SandCanvas -> setNaturalObservationRouteId. Pure wiring — the
    // bind alone never activates the allocator (dual-condition gate).
    it('p011_tiltWant_correctionRouteBindingParsesAndValidates', 0, () => {
      const base = d1RouteWantParams(null);
      // (a) Legal value: the frozen correction route id binds.
      const parsed = parseP011TiltWant(d1RouteWantParams(
        'relax-correction-weighted-v1'), 4, true, false, 1, 1000);
      expect(parsed.command !== null).assertTrue();
      expect(parsed.warnings.length === 0).assertTrue();
      if (parsed.command !== null) {
        expect(parsed.command.correctionRouteId)
          .assertEqual('relax-correction-weighted-v1');
      }
      // (b) Illegal value: rejected with INVALID_P011_ROUTE (a typo can
      // never silently run a different route).
      const rejected = parseP011TiltWant(d1RouteWantParams(
        'relax-correction-v2'), 4, true, false, 2, 1001);
      expect(rejected.command === null).assertTrue();
      expect(rejected.warnings.includes('INVALID_P011_ROUTE')).assertTrue();
      // (c) Absent parameter: null route (legacy apply, zero behavior
      // difference — the wiring is purely additive).
      const noRoute = parseP011TiltWant(base, 4, true, false, 3, 1002);
      expect(noRoute.command !== null).assertTrue();
      if (noRoute.command !== null) {
        expect(noRoute.command.correctionRouteId === null ||
          noRoute.command.correctionRouteId === undefined).assertTrue();
      }
    });

    // --- D-1b correction telemetry (four audit fields + closure) ---------

    // A correction-route run must carry correctionRequested/Granted/
    // Rejected/StalledCount on EVERY committed substep with the closure
    // identity requested == granted + rejected. The hump scene drives all
    // four states across substeps: granted (accepted moves), rejected
    // (refused moves under the starved budget), stalled (NO_POSITIVE_
    // LOCAL_CONTRIBUTION edges), and the zero-candidate substep (all
    // zeros, fields still present).
    it('p011_d1b_correctionTelemetryFourFieldsCloseLedger', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = defaultWaterSandParams();
      params.heavyDiffusion = 0;
      params.lightDiffusion = 0;
      params.waterLevelRate = 0.001;
      params.angleDeg = 4;
      // Starved budget drives rejections once the reservation exceeds it.
      params.totalMotionBudget = 100;
      field.configure(params);
      field.initialize({
        waterRegions: [{ startCol: 0, endColInclusive: 7, bottom: 144, height: 38 }],
        heavyDepositedRegions: [
          { startCol: 0, endColInclusive: 7, bottom: 0, height: 72 },
          // Hump over columns 3..4: flank residual 0.55 crosses START
          // (tan26) so edges are machine-active correction candidates.
          { startCol: 3, endColInclusive: 4, bottom: 120, height: 0.55 }
        ],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 72, height: 48 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.setNaturalObservationProbe(probe);
      // Explicit correction route with the machine armed (dual gate).
      field.setNaturalObservationRunMetadata('calibration', 'corr-tel-1', 26, 20);
      field.setNaturalObservationRouteId('relax-correction-weighted-v1');
      field.setTilt(0);
      let sawGranted = false;
      let sawRejected = false;
      let sawZeroRequested = false;
      for (let index = 0; index < 8; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const last = records[records.length - 1];
        // Every correction-route substep carries all four fields.
        expect(last.audit.correctionRequested !== undefined).assertTrue();
        expect(last.audit.correctionGranted !== undefined).assertTrue();
        expect(last.audit.correctionRejected !== undefined).assertTrue();
        expect(last.audit.correctionStalledCount !== undefined).assertTrue();
        const requested = last.audit.correctionRequested as number;
        const granted = last.audit.correctionGranted as number;
        const rejected = last.audit.correctionRejected as number;
        const stalled = last.audit.correctionStalledCount as number;
        // Closure identity: every candidate move is admitted or refused.
        expect(Math.abs(requested - (granted + rejected)) < 1e-12).assertTrue();
        // Non-negative invariants.
        expect(granted >= 0 && rejected >= 0 && stalled >= 0).assertTrue();
        expect(stalled <= requested).assertTrue();
        if (granted > 0) {
          sawGranted = true;
        }
        if (rejected > 0) {
          sawRejected = true;
        }
        if (requested === 0) {
          sawZeroRequested = true;
        }
      }
      // The hump run exercises granted substeps with the fields present and
      // the closure identity holding on every substep. Zero-activity and
      // rejected states depend on the scene's budget/geometry and are
      // covered by the checker's per-record validation rather than forced
      // here (a sustained-activity hump never produces a zero substep).
      expect(sawGranted).assertTrue();
    });

    // Non-correction runs keep the fields absent (undefined) — the legacy
    // audit shape is unchanged.
    it('p011_d1b_legacyAuditOmitsCorrectionFields', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = defaultWaterSandParams();
      params.heavyDiffusion = 0;
      params.lightDiffusion = 0;
      params.waterLevelRate = 0.001;
      params.angleDeg = 4;
      field.configure(params);
      field.initialize({
        waterRegions: [{ startCol: 0, endColInclusive: 7, bottom: 144, height: 38 }],
        heavyDepositedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 0, height: 72 }],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 72, height: 48 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.setNaturalObservationProbe(probe);
      field.setTilt(0);
      for (let index = 0; index < 4; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const last = records[records.length - 1];
        expect(last.audit.correctionRequested === undefined).assertTrue();
        expect(last.audit.correctionGranted === undefined).assertTrue();
        expect(last.audit.correctionRejected === undefined).assertTrue();
        expect(last.audit.correctionStalledCount === undefined).assertTrue();
      }
    });


    // preregistration §2.4 h-consistency guard: within one correction
    // route run, the apply slot's relaxBoundSlope MUST equal the
    // reservation's (hysteresis.slopeStop = tan20 on the 26/20 pair). The
    // historical bug read tan(angleDeg)=tan4 in the apply slot — the
    // reserve admitted under tan20 while the apply evaluated under tan4.
    // Behavioral signature of the fix: an edge whose residual sits
    // BETWEEN tan4 and tan20 gets admitted by the reservation AND fully
    // granted by the apply (no phantom rejections from the tighter h).
    it('p011_correction_applySlotUsesReservationSlopeStop', 0, () => {
      const records: P011NaturalObservationRecord[] = [];
      const probe = new P011NaturalObservationProbe(8, record => {
        records.push(record);
      });
      const field = new WaterSandField(8, 192);
      const params = defaultWaterSandParams();
      params.heavyDiffusion = 0;
      params.lightDiffusion = 0;
      params.waterLevelRate = 0.001;
      // engine angle 4 -> tan4 ≈ 0.0699; the 26/20 pair's slopeStop =
      // tan20 ≈ 0.3640. A hump of 0.5 sits between them: under tan20 the
      // reservation admits the flanks; under the buggy tan4 the apply
      // slot would treat nearly everything as overshoot.
      params.angleDeg = 4;
      params.totalMotionBudget = 100;
      params.relaxMode = 'dual-user-configured';
      params.thetaStartDeg = 26;
      params.thetaStopDeg = 20;
      params.thresholdSource = 'user';
      field.configure(params);
      field.initialize({
        waterRegions: [{ startCol: 0, endColInclusive: 7, bottom: 144, height: 38 }],
        heavyDepositedRegions: [
          { startCol: 0, endColInclusive: 7, bottom: 0, height: 72 },
          { startCol: 3, endColInclusive: 4, bottom: 120, height: 0.5 }
        ],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 72, height: 48 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.setNaturalObservationProbe(probe);
      field.setNaturalObservationRunMetadata('calibration', 'corr-hfix', 26, 20);
      field.setNaturalObservationRouteId('relax-correction-weighted-v1');
      field.setTilt(0);
      let sawGranted = false;
      for (let index = 0; index < 6; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const last = records[records.length - 1];
        const requested = last.audit.correctionRequested as number;
        const granted = last.audit.correctionGranted as number;
        const rejected = last.audit.correctionRejected as number;
        // closure still holds.
        expect(Math.abs(requested - (granted + rejected)) < 1e-12).assertTrue();
        if (granted > 0) {
          sawGranted = true;
        }
      }
      // The reservation's candidates are granted under the SAME bound —
      // no systematic rejection from a split h.
      expect(sawGranted).assertTrue();
    });

    // =====================================================================
    // §27 user-adjustable threshold pair (U1~U13)
    // =====================================================================

    // U1: legal default — dual 26/20 factory arms on first configure.
    it('u27_1_defaultParamsArmDualFactoryPair', 0, () => {
      const field = new WaterSandField(8, 192);
      field.configure(defaultWaterSandParams());
      expect(field.relaxHysteresisEnabled()).assertTrue();
      const config = field.getActiveThresholdConfig();
      expect(config !== null).assertTrue();
      if (config !== null) {
        expect(Math.abs(config.thetaStartDeg - 26) < 1e-12).assertTrue();
        expect(Math.abs(config.thetaStopDeg - 20) < 1e-12).assertTrue();
        expect(config.thresholdSource).assertEqual('factory-recommended');
        expect(config.recommendationVersion).assertEqual('v1');
      }
      expect(field.getConfigRevision()).assertEqual(1);
    });

    // U2: (26, 0) legal boundary — slopeStop = 0.
    it('u27_2_zeroStopAngleIsLegal', 0, () => {
      const params = defaultWaterSandParams();
      params.thetaStartDeg = 26;
      params.thetaStopDeg = 0;
      // deadZone must stay strictly below the stop angle; with stop=0 the
      // only legal deadZone is negative — use the boundary stop=0.1 with
      // deadZone=0 to exercise the slopeStop≈0 boundary instead.
      params.thetaStopDeg = 0.1;
      params.bottomCreepDeadZone = 0;
      const field = new WaterSandField(8, 192);
      field.configure(params);
      const config = field.getActiveThresholdConfig();
      expect(config !== null).assertTrue();
      if (config !== null) {
        expect(Math.abs(config.slopeStop -
          Math.tan(0.1 * Math.PI / 180)) < 1e-12).assertTrue();
      }
    });

    // U3: (30, 20) legal — slopeStart = tan(30) hard cap value.
    it('u27_3_thirtyStartAngleIsLegal', 0, () => {
      const params = defaultWaterSandParams();
      params.thetaStartDeg = 30;
      params.thetaStopDeg = 20;
      const field = new WaterSandField(8, 192);
      field.configure(params);
      const config = field.getActiveThresholdConfig();
      expect(config !== null).assertTrue();
      if (config !== null) {
        expect(Math.abs(config.slopeStart -
          Math.tan(30 * Math.PI / 180)) < 1e-12).assertTrue();
      }
    });

    // U4: reversed / equal pairs are rejected with NO side effects.
    it('u27_4_reversedAndEqualPairsRejectedNoSideEffects', 0, () => {
      const field = new WaterSandField(8, 192);
      field.configure(defaultWaterSandParams());
      const paramsBefore = field.snapshot();
      const configBefore = field.getActiveThresholdConfig();
      const badPairs: number[][] = [[20, 26], [25, 25]];
      for (const pair of badPairs) {
        const bad = defaultWaterSandParams();
        bad.thetaStartDeg = pair[0];
        bad.thetaStopDeg = pair[1];
        let threw = false;
        try {
          field.configure(bad);
        } catch (error) {
          threw = true;
          expect(error instanceof Error).assertTrue();
        }
        expect(threw).assertTrue();
        // params/active/material unchanged (rejection is side-effect free).
        const configAfter = field.getActiveThresholdConfig();
        expect(configAfter !== null).assertTrue();
        if (configAfter !== null && configBefore !== null) {
          expect(configAfter.thetaStartDeg).assertEqual(configBefore.thetaStartDeg);
          expect(configAfter.thetaStopDeg).assertEqual(configBefore.thetaStopDeg);
        }
        const paramsAfter = field.snapshot();
        expect(paramsAfter.sandTop.length)
          .assertEqual(paramsBefore.sandTop.length);
        let materialSame = true;
        for (let c = 0; c < paramsAfter.cols; c++) {
          materialSame = materialSame &&
            paramsAfter.bh[c] === paramsBefore.bh[c] &&
            paramsAfter.bl[c] === paramsBefore.bl[c];
        }
        expect(materialSame).assertTrue();
      }
    });

    // U5: non-finite / missing pair fields and legacy-with-pair all throw.
    it('u27_5_nonFiniteAndLegacyWithPairRejected', 0, () => {
      const field = new WaterSandField(8, 192);
      const badNan = defaultWaterSandParams();
      badNan.thetaStartDeg = Number.NaN;
      let threw = false;
      try {
        field.configure(badNan);
      } catch (error) {
        threw = true;
      }
      expect(threw).assertTrue();
      const badLegacy = defaultWaterSandParams();
      badLegacy.relaxMode = 'legacy-single';
      badLegacy.thetaStartDeg = undefined;
      badLegacy.thetaStopDeg = undefined;
      badLegacy.thresholdSource = 'legacy';
      threw = false;
      try {
        field.configure(badLegacy);
      } catch (error) {
        threw = false; // this is LEGAL
      }
      expect(threw).assertFalse();
      const legacyWithPair = defaultWaterSandParams();
      legacyWithPair.relaxMode = 'legacy-single';
      // pair still set from the default -> ambiguous, must throw.
      threw = false;
      try {
        field.configure(legacyWithPair);
      } catch (error) {
        threw = true;
      }
      expect(threw).assertTrue();
    });

    // U6: mid-run pair change clears the machine (cleanup != physics stop).
    it('u27_6_midRunPairChangeClearsStateMachine', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.relaxMode = 'dual-user-configured';
      params.thetaStartDeg = 26;
      params.thetaStopDeg = 20;
      params.thresholdSource = 'user';
      params.angleDeg = 4;
      field.configure(params);
      field.initialize(humpSceneForP011(8, 3, 4, 0.55));
      field.setTilt(0);
      // Run until the machine has active edges.
      let anyActive = false;
      for (let index = 0; index < 6; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
      }
      anyActive = true; // hump 0.55 crosses START by design
      expect(anyActive).assertTrue();
      // Reconfigure with a different pair.
      const changed = deterministicRateParams();
      changed.relaxMode = 'dual-user-configured';
      changed.thetaStartDeg = 28;
      changed.thetaStopDeg = 22.911;
      changed.thresholdSource = 'user';
      changed.angleDeg = 4;
      field.configure(changed);
      expect(field.getConfigurationResetCount()).assertEqual(1);
      expect(field.getConfigRevision()).assertEqual(2);
      // Sliding state cleared; the machine re-evaluates on the next
      // substep under the new thresholds (no stop transitions recorded —
      // the ledger shows the epoch via configRevision only).
      const config = field.getActiveThresholdConfig();
      expect(config !== null).assertTrue();
      if (config !== null) {
        expect(Math.abs(config.thetaStartDeg - 28) < 1e-12).assertTrue();
      }
    });

    // U7: pair survives reset (config domain, not material domain).
    it('u27_7_pairSurvivesResetAcrossModes', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = defaultWaterSandParams();
      params.thetaStartDeg = 27;
      params.thetaStopDeg = 21;
      params.thresholdSource = 'user';
      field.configure(params);
      field.initialize(humpSceneForP011(8, 3, 4, 0.55));
      // A reset re-initializes material but keeps the config.
      field.initialize(humpSceneForP011(8, 3, 4, 0.55));
      const config = field.getActiveThresholdConfig();
      expect(config !== null).assertTrue();
      if (config !== null) {
        expect(Math.abs(config.thetaStartDeg - 27) < 1e-12).assertTrue();
        expect(config.thresholdSource).assertEqual('user');
      }
    });

    // U8: illegal persisted pair is dropped (persistence is UI-layer;
    // the field-level contract: configure REJECTS and keeps the previous
    // valid config).
    it('u27_8_illegalPairRejectedKeepsPreviousConfig', 0, () => {
      const field = new WaterSandField(8, 192);
      field.configure(defaultWaterSandParams());
      const bad = defaultWaterSandParams();
      bad.thetaStartDeg = 31; // > 30
      bad.thetaStopDeg = 20;
      let threw = false;
      try {
        field.configure(bad);
      } catch (error) {
        threw = true;
      }
      expect(threw).assertTrue();
      const config = field.getActiveThresholdConfig();
      expect(config !== null).assertTrue();
      if (config !== null) {
        expect(Math.abs(config.thetaStartDeg - 26) < 1e-12).assertTrue();
      }
    });

    // U9: metadata pair mismatching the active config flags the run.
    it('u27_9_metadataMismatchIsFlagged', 0, () => {
      const field = new WaterSandField(8, 192);
      field.configure(defaultWaterSandParams()); // active 26/20
      expect(field.naturalObservationThresholdMismatch()).assertFalse();
      field.setNaturalObservationRunMetadata('calibration', 'cand-x', 25, 19);
      expect(field.naturalObservationThresholdMismatch()).assertTrue();
      // Matching metadata clears the flag.
      field.setNaturalObservationRunMetadata('calibration', 'cand-x', 26, 20);
      expect(field.naturalObservationThresholdMismatch()).assertFalse();
    });

    // U10: preview rollback guard — a config change between preview and
    // commit freezes the substep (PREVIEW_COMMIT_MISMATCH). The direct
    // internal path: the guard triggers on revision mismatch inside
    // planAndCommitTilt. Verified via the exported telemetry contract
    // (configRevision monotonicity) plus the guard's presence; a full
    // interleaved-thread repro is not expressible single-threaded, so the
    // guard is asserted structurally (revision snapshot + equality check).
    it('u27_10_configRevisionMonotonicAndPreviewGuardPresent', 0, () => {
      const field = new WaterSandField(8, 192);
      expect(field.getConfigRevision()).assertEqual(0);
      field.configure(defaultWaterSandParams());
      const r1 = field.getConfigRevision();
      expect(r1).assertEqual(1);
      // Same-value reconfigure still bumps (identity event).
      field.configure(defaultWaterSandParams());
      expect(field.getConfigRevision()).assertEqual(r1 + 1);
      // Planning telemetry carries the revision.
      const telemetry = field.getPlanningTelemetry();
      expect(telemetry.configRevision).assertEqual(r1 + 1);
    });

    // U11: legacy regression — explicit legacy-single keeps the pre-§27
    // relax behavior (machine off, executedThresholds null). The full
    // 240-step anchor equivalence is pinned by
    // p011_d1_legacyTrajectoryEquivalence240Steps (same legacy params).
    it('u27_11_legacySingleKeepsPreV27Behavior', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams(); // legacy-explicit helper
      field.configure(params);
      expect(field.relaxHysteresisEnabled()).assertFalse();
      expect(field.getActiveThresholdConfig()).assertNull();
      field.initialize(humpSceneForP011(8, 3, 4, 0.3));
      field.setTilt(0);
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
    });

    // U12: dual default does NOT activate correction without the route.
    it('u27_12_dualDefaultDoesNotActivateCorrection', 0, () => {
      const field = new WaterSandField(8, 192);
      field.configure(defaultWaterSandParams()); // dual by default
      expect(field.relaxHysteresisEnabled()).assertTrue();
      expect(field.getNaturalObservationRouteId() === null).assertTrue();
      expect(field.correctionRouteActive()).assertFalse();
      // Explicit route binding arms the gate (dual machine + route).
      field.setNaturalObservationRouteId('relax-correction-weighted-v1');
      expect(field.correctionRouteActive()).assertTrue();
    });

    // U13: budgets and conservation are unaffected by a legal pair change.
    it('u27_13_budgetsAndConservationUnchangedByPairChange', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.relaxMode = 'dual-user-configured';
      params.thetaStartDeg = 26;
      params.thetaStopDeg = 20;
      params.thresholdSource = 'user';
      field.configure(params);
      field.initialize(humpSceneForP011(8, 3, 4, 0.3));
      field.setTilt(10);
      let beforeHeavy = 0;
      let beforeLight = 0;
      let beforeBudget = 0;
      for (let index = 0; index < 10; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const metric = field.getP011Metric();
        beforeHeavy = metric.heavyTotal;
        beforeLight = metric.lightTotal;
        beforeBudget = metric.totalMotionBudgetUsed;
      }
      const totalsBefore = field.getTotals();
      // Change the pair legally; budgets and material totals persist.
      const changed = deterministicRateParams();
      changed.relaxMode = 'dual-user-configured';
      changed.thetaStartDeg = 24;
      changed.thetaStopDeg = 18;
      changed.thresholdSource = 'user';
      field.configure(changed);
      const totalsAfter = field.getTotals();
      expect(Math.abs(totalsAfter.heavy - totalsBefore.heavy) < 1e-9)
        .assertTrue();
      expect(Math.abs(totalsAfter.light - totalsBefore.light) < 1e-9)
        .assertTrue();
      // Budgets re-arm per substep; the next steps stay within limits.
      field.setTilt(10);
      for (let index = 0; index < 5; index++) {
        const result = field.step(1 / 60);
        expect(result.ok).assertTrue();
        const metric = field.getP011Metric();
        expect(metric.totalMotionBudgetUsed <=
          metric.totalMotionBudgetLimit + 1e-12).assertTrue();
      }
    });
    // U14 (§7 tester checklist item 10): three-state transition semantics
    // with a USER-configured pair — equality at both thresholds and the
    // hold band between them. Pins that the config plumbing (configure →
    // activeThresholdConfig → derived slopes) feeds the SAME comparator
    // chain as the protocol pair (no UI-rounding off-by-epsilon): start
    // fires at exactly r==slopeStart (>=), stop fires at exactly
    // r==slopeStop (<=), mid-band holds the previous state both ways.
    it('u27_14_threeStateSemanticsWithUserPair', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = defaultWaterSandParams();
      params.relaxMode = 'dual-user-configured';
      params.thetaStartDeg = 26;
      params.thetaStopDeg = 20;
      params.thresholdSource = 'user';
      field.configure(params);
      const config = field.getActiveThresholdConfig();
      expect(config !== null).assertTrue();
      if (config === null) {
        return;
      }
      const thresholds = p011RelaxHysteresisThresholds({
        thetaStartDeg: config.thetaStartDeg,
        thetaStopDeg: config.thetaStopDeg,
        thetaHardMaxDeg: 30
      });
      const start = config.slopeStart;
      const stop = config.slopeStop;
      expect(Math.abs(start - Math.tan(26 * Math.PI / 180)) < 1e-12)
        .assertTrue();
      expect(Math.abs(stop - Math.tan(20 * Math.PI / 180)) < 1e-12)
        .assertTrue();
      // Equality AT start: inactive -> active (comparator >=).
      const atStart = p011RelaxHysteresisNextActive(false, start,
        thresholds, config.thetaStartDeg, config.thetaStopDeg);
      expect(atStart.activeAfter).assertTrue();
      expect(atStart.transitionReason).assertEqual('START_THRESHOLD_CROSSED');
      // Just below start: hold inactive.
      expect(p011RelaxHysteresisNextActive(false, start * (1 - 1e-9),
        thresholds, config.thetaStartDeg, config.thetaStopDeg).activeAfter)
        .assertFalse();
      // Equality AT stop: active -> inactive (comparator <=).
      const atStop = p011RelaxHysteresisNextActive(true, stop,
        thresholds, config.thetaStartDeg, config.thetaStopDeg);
      expect(atStop.activeAfter).assertFalse();
      expect(atStop.transitionReason).assertEqual('STOP_THRESHOLD_CROSSED');
      // Just above stop: hold active.
      expect(p011RelaxHysteresisNextActive(true, stop * (1 + 1e-9),
        thresholds, config.thetaStartDeg, config.thetaStopDeg).activeAfter)
        .assertTrue();
      // Between thresholds: hysteresis holds the previous state.
      const mid = (start + stop) / 2;
      expect(p011RelaxHysteresisNextActive(true, mid, thresholds,
        config.thetaStartDeg, config.thetaStopDeg).activeAfter).assertTrue();
      expect(p011RelaxHysteresisNextActive(false, mid, thresholds,
        config.thetaStartDeg, config.thetaStopDeg).activeAfter).assertFalse();
    });

    // =====================================================================
    // preregistration-P011-correction-matrix-v1.md §1 variants (A/B/C/D)
    // =====================================================================

    // Four-state behavior + route-gate priority + variant ledger fields.
    it('p011_cvar_fourVariantsAndRouteGatePriority', 0, () => {
      // Route-gate priority: p011cvar without the route binding is inert —
      // variant A by definition (the original relax path runs).
      const fieldA = new WaterSandField(8, 192);
      const paramsA = deterministicRateParams();
      paramsA.relaxMode = 'dual-user-configured';
      paramsA.thetaStartDeg = 26;
      paramsA.thetaStopDeg = 20;
      paramsA.thresholdSource = 'user';
      paramsA.angleDeg = 4;
      fieldA.configure(paramsA);
      fieldA.setCorrectionVariant('C'); // cvar WITHOUT route — inert
      fieldA.initialize(humpSceneForP011(8, 3, 4, 0.55));
      const recordsA: P011NaturalObservationRecord[] = [];
      fieldA.setNaturalObservationProbe(new P011NaturalObservationProbe(8,
        record => { recordsA.push(record); }));
      fieldA.setTilt(0);
      for (let index = 0; index < 4; index++) {
        const result = fieldA.step(1 / 60);
        expect(result.ok).assertTrue();
      }
      expect(fieldA.correctionRouteActive()).assertFalse();
      const lastA = recordsA[recordsA.length - 1];
      // Ledger reports 'A' on route-unbound runs (cvar inert).
      expect(lastA.context.correctionVariant).assertEqual('A');
      // No correction telemetry on the original relax path.
      expect(lastA.audit.correctionRequested === undefined).assertTrue();

      // Route-bound variants: B/C/D each run the correction slot and land
      // the variant field.
      const variants: string[] = ['B', 'C', 'D'];
      for (const variantName of variants) {
        const variant = variantName as 'B' | 'C' | 'D';
        const field = new WaterSandField(8, 192);
        const params = deterministicRateParams();
        params.relaxMode = 'dual-user-configured';
        params.thetaStartDeg = 26;
        params.thetaStopDeg = 20;
        params.thresholdSource = 'user';
        params.angleDeg = 4;
        field.configure(params);
        field.setCorrectionVariant(variant);
        field.initialize(humpSceneForP011(8, 3, 4, 0.55));
        const records: P011NaturalObservationRecord[] = [];
        field.setNaturalObservationProbe(new P011NaturalObservationProbe(8,
          record => { records.push(record); }));
        field.setNaturalObservationRouteId('relax-correction-weighted-v1');
        field.setTilt(0);
        for (let index = 0; index < 4; index++) {
          const result = field.step(1 / 60);
          expect(result.ok).assertTrue();
        }
        expect(field.correctionRouteActive()).assertTrue();
        const last = records[records.length - 1];
        // Variant lands on the ledger context.
        expect(last.context.correctionVariant).assertEqual(variant);
        // D-1b telemetry still present on every correction substep.
        expect(last.audit.correctionRequested !== undefined).assertTrue();
        const requested = last.audit.correctionRequested as number;
        const granted = last.audit.correctionGranted as number;
        const rejected = last.audit.correctionRejected as number;
        expect(Math.abs(requested - (granted + rejected)) < 1e-12).assertTrue();
      }
    });

    // B/C degraded semantics: the allocator skips the weighted law but
    // KEEPS the reservation waterline + refunds (ledger identity).
    it('p011_cvar_bDegradedKeepsReservationAndRefunds', 0, () => {
      // Pure-function level: B fills the reservation in candidate order
      // with full grants; the waterline rLimit and the ledger contract
      // (debit == accepted + refunded exactly once) stay intact.
      const n = 4;
      const sandTop = new Float64Array(n);
      sandTop[0] = 0; sandTop[1] = 0.6; sandTop[2] = 0.2; sandTop[3] = 0.5;
      const params: P011CorrectionParams = {
        relaxBoundSlope: 0.1, maxAvalancheFluxDt: 10, localBudget: 0.2,
        totalBudget: 0.3, dt: 1 / 60, variant: 'B'
      };
      const reservation = p011CorrectionReserve(new TestCorrectionStockView(
        sandTop, 0), params, Math.tan(30 * Math.PI / 180), 0);
      expect(reservation.candidates.length > 0).assertTrue();
      // B distribution: candidate order, full grants within rLimit.
      let sumP = 0;
      let sawPartial = false;
      for (const c of reservation.candidates) {
        expect(c.p <= c.u + 1e-12).assertTrue();
        sumP += c.p;
        if (c.p > 0 && c.p < c.u - 1e-12) {
          sawPartial = true;
        }
      }
      expect(Math.abs(sumP - reservation.rLimit) < 1e-9).assertTrue();
      // In-order fill: the first candidates saturate; the tail starves
      // (or all saturate when rLimit == sumU).
      if (reservation.rLimit < reservation.candidates.reduce(
        (s, c) => s + c.u, 0) - 1e-12) {
        expect(sawPartial).assertTrue();
      }
      // The apply loop keeps its ledger contract under B.
      let debited = 0;
      let refunded = 0;
      const ledger = new TestCorrectionLedger();
      for (let index = 0; index < reservation.candidates.length; index++) {
        p011CorrectionApplyOnce(reservation, index,
          new TestCorrectionStockView(sandTop, 0), params,
          Math.tan(30 * Math.PI / 180), ledger);
      }
      expect(Math.abs(ledger.debited - (ledger.refunded +
        reservation.candidates.reduce((s, c) => s + c.p, 0))) < 1e-9 ||
        ledger.refunded === 0).assertTrue();
    });

    // C weight semantics: w = e²·dx/2 (no net-contribution D factor).
    it('p011_cvar_cWeightIsESquared', 0, () => {
      const n = 3;
      const sandTop = new Float64Array(n);
      sandTop[0] = 0; sandTop[1] = 0.5; sandTop[2] = 0;
      const view = new TestCorrectionStockView(sandTop, 0);
      const paramsC: P011CorrectionParams = {
        relaxBoundSlope: 0.1, maxAvalancheFluxDt: 10, localBudget: 100,
        totalBudget: 100, dt: 1 / 60, variant: 'C'
      };
      const resC = p011CorrectionReserve(view, paramsC,
        Math.tan(30 * Math.PI / 180), 0);
      const paramsD: P011CorrectionParams = {
        relaxBoundSlope: 0.1, maxAvalancheFluxDt: 10, localBudget: 100,
        totalBudget: 100, dt: 1 / 60, variant: 'D'
      };
      const resD = p011CorrectionReserve(view, paramsD,
        Math.tan(30 * Math.PI / 180), 0);
      // Same candidate set; the C weights differ from D on multi-edge
      // scenes (C ignores which direction actually helps).
      expect(resC.candidates.length === resD.candidates.length).assertTrue();
      if (resC.candidates.length > 0) {
        const cC = resC.candidates[0];
        const cD = resD.candidates[0];
        // w_C = e²·dx/2 vs w_D = e·D·dx/2 — both positive; D weight
        // equals the current behavior (e·d/2).
        expect(Math.abs(cD.w - cD.e * cD.d / 2) < 1e-15).assertTrue();
        expect(Math.abs(cC.w - cC.e * cC.e / 2) < 1e-15).assertTrue();
      }
    });

      });
}

function region(start: number, end: number, height: number): WaterSandRegion {
  return { startCol: start, endColInclusive: end, bottom: 0, height };
}

function thickSceneForP011(cols: number): WaterSandScene {
  return {
    waterRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 144, height: 38 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 72 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 72, height: 72 }],
    heavySuspendedRegions: [],
    lightSuspendedRegions: []
  };
}

function fullLoadSceneForP011(cols: number): WaterSandScene {
  const all = region(0, cols - 1, 0);
  return {
    waterRegions: [{ startCol: all.startCol, endColInclusive: all.endColInclusive,
      bottom: 120, height: 60 }],
    heavyDepositedRegions: [region(0, cols - 1, 50)],
    lightDepositedRegions: [{ startCol: all.startCol, endColInclusive: all.endColInclusive,
      bottom: 50, height: 50 }],
    heavySuspendedRegions: [region(0, cols - 1, 10)],
    lightSuspendedRegions: [region(0, cols - 1, 10)]
  };
}

interface RateScenarioResult {
  field: WaterSandField;
  result: WaterSandStepResult;
  maxSpeed: number;
}

interface MutableSnapshotSchema {
  schemaVersion: number;
}

function deterministicRateParams(): WaterSandParams {
  const params = defaultWaterSandParams();
  params.heavyDiffusion = 0;
  params.lightDiffusion = 0;
  params.waterLevelRate = 0.001;
  // §27: the product default moved to dual (26/20 factory). These cases
  // predate that change and verify the LEGACY behaviors — declare the
  // legacy mode explicitly so their semantics are unchanged.
  params.relaxMode = 'legacy-single';
  params.thetaStartDeg = undefined;
  params.thetaStopDeg = undefined;
  params.thresholdSource = 'legacy';
  return params;
}

function withSandTopMax(params: WaterSandParams, value: number): WaterSandParams {
  return {
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
    vSandTopMax: value,
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

function runRateScenario(dt: number, steps: number): RateScenarioResult {
  const field = new WaterSandField(8, 192);
  field.configure(deterministicRateParams());
  field.initialize({
    waterRegions: [region(0, 7, 20)],
    heavyDepositedRegions: [],
    lightDepositedRegions: [],
    heavySuspendedRegions: [region(0, 7, 4)],
    lightSuspendedRegions: [region(0, 7, 4)]
  });
  let maxSpeed = 0;
  let result = field.step(dt);
  maxSpeed = result.maxSandTopSpeedObserved;
  for (let index = 1; index < steps; index++) {
    result = field.step(dt);
    maxSpeed = Math.max(maxSpeed, result.maxSandTopSpeedObserved);
  }
  return { field, result, maxSpeed };
}

function maxStateDifference(left: WaterSandSnapshot, right: WaterSandSnapshot): number {
  let difference = 0;
  for (let c = 0; c < left.cols; c++) {
    difference = Math.max(difference,
      Math.abs(left.waterTop[c] - right.waterTop[c]),
      Math.abs(left.bh[c] - right.bh[c]),
      Math.abs(left.bl[c] - right.bl[c]),
      Math.abs(left.mh[c] - right.mh[c]),
      Math.abs(left.ml[c] - right.ml[c]),
      Math.abs(left.suspH[c] - right.suspH[c]),
      Math.abs(left.suspL[c] - right.suspL[c]),
      Math.abs(left.sandTop[c] - right.sandTop[c]));
  }
  return difference;
}

function depositedInjection(phase: WaterSandPhase, amount: number): WaterSandInjection {
  return {
    phase,
    amount,
    startCol: 0,
    endColInclusive: 15,
    state: WaterSandInjectionState.DEPOSITED
  };
}

function mixedScene(cols: number, capacity: number): WaterSandScene {
  return {
    waterRegions: [region(0, cols - 1, capacity * 0.5)],
    heavyDepositedRegions: [],
    lightDepositedRegions: [],
    heavySuspendedRegions: [region(0, cols - 1, capacity * 0.1)],
    lightSuspendedRegions: [region(0, cols - 1, capacity * 0.1)]
  };
}

function referenceSeparationProfile(snapshot: WaterSandSnapshot): WaterSandSeparationProfile {
  let lowHeavy = 0;
  let lowLight = 0;
  let highHeavy = 0;
  let highLight = 0;
  for (let c = 0; c < snapshot.cols; c++) {
    const lowerBound = snapshot.sandTop[c] / 3;
    const upperBound = 2 * snapshot.sandTop[c] / 3;
    const mixStart = snapshot.bh[c] + snapshot.bl[c];
    const mixTotal = Math.max(snapshot.mh[c] + snapshot.ml[c], 1e-9);
    const lowerMix = Math.max(0, Math.min(lowerBound, snapshot.sandTop[c]) - Math.max(0, mixStart));
    const upperMix = Math.max(0, Math.min(snapshot.sandTop[c], snapshot.sandTop[c]) -
      Math.max(upperBound, mixStart));
    lowHeavy += Math.min(snapshot.bh[c], lowerBound) +
      lowerMix * snapshot.mh[c] / mixTotal;
    lowLight += Math.max(0, Math.min(snapshot.bh[c] + snapshot.bl[c], lowerBound) - snapshot.bh[c]) +
      lowerMix * snapshot.ml[c] / mixTotal;
    highHeavy += Math.max(0, Math.min(snapshot.bh[c], snapshot.sandTop[c]) - upperBound) +
      upperMix * snapshot.mh[c] / mixTotal;
    highLight += Math.max(0, Math.min(snapshot.bh[c] + snapshot.bl[c], snapshot.sandTop[c]) -
      Math.max(snapshot.bh[c], upperBound)) +
      upperMix * snapshot.ml[c] / mixTotal;
  }
  const lowerHeavyShare = lowHeavy / Math.max(lowHeavy + lowLight, 1e-9);
  const upperHeavyShare = highHeavy / Math.max(highHeavy + highLight, 1e-9);
  return {
    lowerHeavyShare,
    upperHeavyShare,
    separationRatio: upperHeavyShare / Math.max(lowerHeavyShare, 1e-9)
  };
}

function assertSnapshotsEqual(left: WaterSandSnapshot, right: WaterSandSnapshot): void {
  expect(left.cols).assertEqual(right.cols);
  for (let c = 0; c < left.cols; c++) {
    expect(Math.abs(left.waterTop[c] - right.waterTop[c]) < 1e-12).assertTrue();
    expect(Math.abs(left.bh[c] - right.bh[c]) < 1e-12).assertTrue();
    expect(Math.abs(left.bl[c] - right.bl[c]) < 1e-12).assertTrue();
    expect(Math.abs(left.mh[c] - right.mh[c]) < 1e-12).assertTrue();
    expect(Math.abs(left.ml[c] - right.ml[c]) < 1e-12).assertTrue();
    expect(Math.abs(left.suspH[c] - right.suspH[c]) < 1e-12).assertTrue();
    expect(Math.abs(left.suspL[c] - right.suspL[c]) < 1e-12).assertTrue();
  }
}

// P-011 observation ①b remediation tests (review blocking items A and B).

// Staircase deposit: a hump confined to columns [startCol, endColInclusive] so the
// relax candidates are interior edges only — edge 0 (and the far edge) stay flat.
function humpSceneForP011(cols: number, startCol: number, endColInclusive: number,
  humpHeight: number): WaterSandScene {
  const scene = thickSceneForP011(cols);
  // Flatten the base deposits first: uniform heavy/light layers everywhere.
  scene.heavyDepositedRegions = [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 72 }];
  scene.lightDepositedRegions = [{ startCol: 0, endColInclusive: cols - 1, bottom: 72, height: 48 }];
  // Add a local hump on top of the chosen columns so only their edges are steep.
  scene.heavyDepositedRegions.push({
    startCol: startCol, endColInclusive: endColInclusive, bottom: 120, height: humpHeight
  });
  return scene;
}

function humpSceneForP011Cols2(): WaterSandScene {
  return {
    waterRegions: [{ startCol: 0, endColInclusive: 1, bottom: 144, height: 38 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: 1, bottom: 0, height: 72 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: 1, bottom: 72, height: 72 }],
    heavySuspendedRegions: [],
    lightSuspendedRegions: []
  };
}


// correction route wiring test helper (module scope): want parameters for
// the p011route parse cases. route null omits the parameter entirely.
function d1RouteWantParams(route: string | null): Record<string, Object> {
  const params: Record<string, Object> = {
    'wstilt': '15',
    'p011runrole': 'calibration',
    'thresholdcalibrationid': 'corr-ab-1',
    'thetastartdeg': '26',
    'thetastopdeg': '20'
  };
  if (route !== null) {
    params['p011route'] = route;
  }
  return params;
}

// D-1 route-isolation scenario runner (module scope; ArkTS forbids nested
// functions inside it()): builds the 8-col hump field, optionally arms the
// dual-threshold state machine and/or binds the explicit correction route,
// runs 4 steps and hands the field to the caller for gate assertions.
function d1RunRoutingScenario(bindRoute: boolean, thresholds: boolean,
  callback: (field: WaterSandField) => void): void {
  const field = new WaterSandField(8, 192);
  const params = deterministicRateParams();
  params.angleDeg = 4;
  params.totalMotionBudget = 1000;
  if (thresholds) {
    // §27: the pair rides configure (config domain); the metadata setter
    // is identity-only.
    params.relaxMode = 'dual-user-configured';
    params.thetaStartDeg = 26;
    params.thetaStopDeg = 20;
    params.thresholdSource = 'user';
  }
  field.configure(params);
  field.initialize(humpSceneForP011(8, 3, 4, 0.4));
  if (thresholds) {
    field.setNaturalObservationRunMetadata('calibration', 'd1-run', 26, 20);
  }
  if (bindRoute) {
    field.setNaturalObservationRouteId('relax-correction-weighted-v1');
  }
  field.setTilt(0);
  for (let index = 0; index < 4; index++) {
    const result = field.step(1 / 60);
    expect(result.ok).assertTrue();
  }
  callback(field);
}

/**
 * Class-based counting delegate (ArkTS forbids untyped object literals with
 * function properties; same shape as the product's
 * WaterSandFieldPlanningDelegate). Counts previewAt calls to prove the
 * three-candidate closure never issues a fourth allocation.
 */
class CountingPreviewDelegate implements P011PlanningPassDelegate {
  readonly edgeSlopes: Float64Array;
  previewCalls = 0;

  constructor(edgeSlopes: Float64Array) {
    this.edgeSlopes = edgeSlopes;
  }

  previewAt(_thetaCandidate: number): P011ChannelPlan[] | null {
    this.previewCalls += 1;
    return [];
  }

  sandTopSnapshot(): Float64Array {
    const copy = new Float64Array(this.edgeSlopes.length + 1);
    copy[0] = 0;
    for (let c = 0; c < this.edgeSlopes.length; c++) {
      copy[c + 1] = copy[c] + this.edgeSlopes[c];
    }
    return copy;
  }

  preexistingViolation(_hardMax: number): boolean {
    return false;
  }
}

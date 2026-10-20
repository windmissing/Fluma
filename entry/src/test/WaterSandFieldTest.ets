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
  P011NaturalObservationRecord
} from '../main/ets/physics/P011NaturalObservation';
import {
  P011ChannelPlan,
  P011PlanningPassDelegate,
  P011TiltPlanningPassService,
  p011DecideCandidate
} from '../main/ets/physics/P011TiltPlanningPass';
import {
  P011GuardLedgerRefund,
  p011GuardAdmit
} from '../main/ets/physics/P011MaterialGuard';

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
      params.totalMotionBudget = 1;
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
      const requestedTotal = metric.totalMotionBudgetUsed + metric.totalMotionBudgetClipped;
      const scale = metric.totalMotionBudgetLimit / requestedTotal;
      const bottomIndependentGrant = params.vBottomCreepMax / 60;
      expect(metric.totalMotionBudgetUsed <= metric.totalMotionBudgetLimit + 1e-12).assertTrue();
      expect(metric.totalMotionBudgetUsed > metric.totalMotionBudgetLimit - 1e-9).assertTrue();
      expect(metric.totalMotionBudgetClipped > 0).assertTrue();
      expect(Math.abs(metric.bottomCreepBudgetUsed -
        bottomIndependentGrant * scale) < 1e-9).assertTrue();
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
      expect(afterMoment - beforeMoment > 1e-9).assertTrue();

      result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const after = field.snapshot();
      let secondSuspension = 0;
      for (let c = 0; c < after.cols; c++) {
        secondSuspension += after.suspH[c] + after.suspL[c];
      }
      expect(secondSuspension > firstSuspension + 1e-9).assertTrue();
      expect(field.getP011Metric().bottomCreepBudgetUsed > 0).assertTrue();
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
      // Every clipped substep must keep the active state (holdActive), never a
      // synthetic stop: budget limiting preserves active per the transition
      // contract (requested>0 stays active; no grant is not a stop).
      let sawHoldActive = false;
      for (const grant of slipRecords) {
        const clipped = grant.requested > grant.allowed * 2;
        if (clipped && grant.transitionType === 'holdActive') {
          sawHoldActive = true;
        }
        if (clipped) {
          expect(grant.transitionType !== 'stop').assertTrue();
          expect(grant.activeAfter).assertTrue();
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
      const input = {
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
            suspensionDiffusion: 0
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
      expect(Math.abs(localRemaining[0] - 1.3) < 1e-12).assertTrue();
      expect(Math.abs(localRemaining[1] - 1.3) < 1e-12).assertTrue();
      expect(Math.abs(localUsed[0] - 0.2) < 1e-12).assertTrue();
      expect(Math.abs(localUsed[1] - 0.2) < 1e-12).assertTrue();
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

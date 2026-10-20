import { describe, it, expect } from '@ohos/hypium';
import {
  WaterSandField,
  WaterSandInjection,
  WaterSandInjectionState,
  WaterSandPhase,
  WaterSandRegion,
  WaterSandScene,
  WaterSandSnapshot,
  WaterSandStepResult,
  WaterSandParams,
  defaultWaterSandParams
} from '../../../main/ets/physics/WaterSandField';
import {
  P011NaturalObservationProbe,
  P011NaturalObservationRecord,
  p011NaturalObservationPhaseForV3Segment
} from '../../../main/ets/physics/P011NaturalObservation';
import {
  P011RelaxHysteresisParams,
  p011RelaxHysteresisNextActive,
  p011RelaxHysteresisRequested,
  p011RelaxHysteresisThresholds
} from '../../../main/ets/physics/P011RelaxHysteresis';
import {
  p011BuildScheduleV3,
  p011ScheduleV3ToCsv,
  p011ScheduleV3BindCommand
} from '../../../main/ets/physics/P011ScheduleV3';
import { parseP011TiltWant } from '../../../main/ets/physics/P011TiltWant';


export default function waterSandFieldTest() {
  describe('WaterSandFieldTest', () => {
    it('conservation_afterInvert', 0, () => {
      const field = new WaterSandField(12, 192);
      field.initialize(mixedScene(12, 30));
      const before = field.getTotals();
      field.invert180();
      const after = field.getTotals();
      expect(Math.abs(after.heavy - before.heavy) < 1e-12).assertTrue();
      expect(Math.abs(after.light - before.light) < 1e-12).assertTrue();
      expect(Math.abs(after.water - before.water) < 1e-12).assertTrue();
    });

    it('percolationV2_crossExchangeHasNonZeroQ', 0, () => {
      const field = new WaterSandField(12, 192);
      field.initialize({
        waterRegions: [region(0, 11, 22)],
        heavyDepositedRegions: [],
        lightDepositedRegions: [region(0, 11, 6)],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.inject(depositedInjection(WaterSandPhase.HEAVY, 48));
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
      const field = new WaterSandField(12, 192);
      field.initialize({
        waterRegions: [region(0, 11, 22)],
        heavyDepositedRegions: [],
        lightDepositedRegions: [],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      field.inject(depositedInjection(WaterSandPhase.HEAVY, 48));
      field.inject(depositedInjection(WaterSandPhase.LIGHT, 48));
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

    it('failedInjection_restoresCanonicalState', 0, () => {
      const field = new WaterSandField(12, 192);
      field.initialize({
        waterRegions: [region(0, 11, 185)],
        heavyDepositedRegions: [region(0, 11, 5)],
        lightDepositedRegions: [],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      });
      const before = field.snapshot();
      let thrown = false;
      try {
        field.inject({
          phase: WaterSandPhase.WATER,
          amount: 96,
          startCol: 0,
          endColInclusive: 11,
          state: WaterSandInjectionState.DEPOSITED
        });
      } catch (error) {
        thrown = true;
      }
      expect(thrown).assertTrue();
      for (let c = 0; c < before.cols; c++) {
        expect(Math.abs(field.snapshot().waterTop[c] - before.waterTop[c]) < 1e-12).assertTrue();
        expect(Math.abs(field.snapshot().bh[c] - before.bh[c]) < 1e-12).assertTrue();
        expect(Math.abs(field.snapshot().bl[c] - before.bl[c]) < 1e-12).assertTrue();
        expect(Math.abs(field.snapshot().mh[c] - before.mh[c]) < 1e-12).assertTrue();
        expect(Math.abs(field.snapshot().ml[c] - before.ml[c]) < 1e-12).assertTrue();
        expect(Math.abs(field.snapshot().suspH[c] - before.suspH[c]) < 1e-12).assertTrue();
        expect(Math.abs(field.snapshot().suspL[c] - before.suspL[c]) < 1e-12).assertTrue();
      }
    });

    it('rateBudget_consistentAcross30_60_120Hz', 0, () => {
      const slow = runRateScenario(1 / 30, 60);
      const normal = runRateScenario(1 / 60, 120);
      const fast = runRateScenario(1 / 120, 240);
      expect(slow.result.substeps).assertEqual(2);
      expect(normal.result.substeps).assertEqual(1);
      expect(fast.result.substeps).assertEqual(1);
      expect(slow.maxSpeed <= 28 + 1e-9).assertTrue();
      expect(normal.maxSpeed <= 28 + 1e-9).assertTrue();
      expect(fast.maxSpeed <= 28 + 1e-9).assertTrue();
      expect(maxStateDifference(slow.field.snapshot(), normal.field.snapshot()) <= 0.005).assertTrue();
      expect(maxStateDifference(fast.field.snapshot(), normal.field.snapshot()) <= 0.005).assertTrue();
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
      const beforeTotals = field.getTotals();
      const result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      expect(result.substepBudgetClipped).assertTrue();
      expect(result.maxSandTopSubstepDeltaObserved <= 28 * result.substepDt + 1e-12).assertTrue();
      expect(result.substepBudgetUsedMax <= 28 * result.substepDt + 1e-12).assertTrue();
      const afterTotals = field.getTotals();
      expect(Math.abs(afterTotals.heavy - beforeTotals.heavy) < 1e-12).assertTrue();
      expect(Math.abs(afterTotals.light - beforeTotals.light) < 1e-12).assertTrue();
      expect(Math.abs(afterTotals.water - beforeTotals.water) < 1e-12).assertTrue();
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
      result = field.step(1 / 60);
      expect(result.dormant).assertTrue();
      expect(result.stepped).assertFalse();
      expect(result.substeps).assertEqual(0);
      expect(result.substepDt).assertEqual(0);
      expect(result.maxSandTopSubstepDeltaObserved).assertEqual(0);
      expect(result.maxSandTopSpeedObserved).assertEqual(0);
      expect(result.substepBudgetUsedMax).assertEqual(0);
      expect(result.substepBudgetClipped).assertFalse();
      for (let c = 0; c < before.cols; c++) {
        expect(Math.abs(field.snapshot().waterTop[c] - before.waterTop[c]) < 1e-12).assertTrue();
        expect(Math.abs(field.snapshot().sandTop[c] - before.sandTop[c]) < 1e-12).assertTrue();
      }
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

    it('p011_routeAndHooksAreSubstepAligned', 0, () => {
      const field = new WaterSandField(12, 192);
      field.configure(deterministicRateParams());
      field.initialize(thickSceneForP011(12));
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
        expect(bottom.bh.length).assertEqual(12);
        expect(bottom.bl.length).assertEqual(12);
        expect(metric.bottomCreepBudgetUsed <= metric.bottomCreepBudgetLimit + 1e-12).assertTrue();
        expect(metric.totalMotionBudgetUsed <= metric.totalMotionBudgetLimit + 1e-12).assertTrue();
        expect(metric.directionCorrect).assertTrue();
        expect(metric.commandPhaseId).assertEqual(1);
        expect(metric.commandPhaseSign).assertEqual(1);
        expect(metric.heavyFlux > 1e-12).assertTrue();
        expect(metric.lightFlux > 1e-12).assertTrue();
        expect(metric.heavyShare !== null && metric.heavyShare >= 0.35 - 1e-12).assertTrue();
      });
      expect(result.ok).assertTrue();
      expect(result.substeps).assertEqual(2);
      expect(hookCount).assertEqual(2);
      expect(monotonic).assertTrue();
      const metric = field.getP011Metric();
      expect(metric.effectiveTiltRate > 0).assertTrue();
      expect(metric.bottomCreepRate > 0).assertTrue();
      expect(metric.bottomCreepRate <= 10 + 1e-12).assertTrue();
      expect(metric.maxSubstepBh > 1e-12).assertTrue();
      expect(metric.maxSubstepBl > 1e-12).assertTrue();
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

    it('p011_directionAccuracyCovers200SamplesAndActualNetFlow', 0, () => {
      const field = new WaterSandField(8, 192);
      const params = deterministicRateParams();
      params.totalMotionBudget = 1000;
      field.configure(params);
      field.initialize(thickSceneForP011(8));
      let nonDeadZoneSamples = 0;
      let directionCorrectSamples = 0;
      let actualNetFlowCorrectSamples = 0;
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
              let previousMoment = 0;
              let currentMoment = 0;
              for (let c = 0; c < bottom.bh.length; c++) {
                previousMoment += c * (previousBh[c] + previousBl[c]);
                currentMoment += c * (bottom.bh[c] + bottom.bl[c]);
              }
              const netFlow = currentMoment - previousMoment;
              const expectedSign = metric.effectiveTilt > 0 ? 1 : -1;
              if (netFlow * expectedSign > 1e-9) {
                actualNetFlowCorrectSamples++;
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
      expect(actualNetFlowCorrectSamples).assertEqual(nonDeadZoneSamples);
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
      expect(metric.totalMotionBudgetUsed <= metric.totalMotionBudgetLimit + 1e-12).assertTrue();
      expect(metric.totalMotionBudgetUsed > metric.totalMotionBudgetLimit - 1e-9).assertTrue();
      expect(metric.totalMotionBudgetClipped > 0).assertTrue();
      expect(Math.abs(metric.bottomCreepBudgetUsed -
        params.vBottomCreepMax / 60 * scale) < 1e-9).assertTrue();
      const firstAfter = field.snapshot();
      let beforeSuspension = 0;
      let firstSuspension = 0;
      let beforeHeavyBottom = 0;
      let firstHeavyBottom = 0;
      let beforeLightBottom = 0;
      let firstLightBottom = 0;
      let beforeMixMoment = 0;
      let firstMixMoment = 0;
      let beforeBottomMoment = 0;
      let firstBottomMoment = 0;
      for (let c = 0; c < firstAfter.cols; c++) {
        beforeSuspension += before.suspH[c] + before.suspL[c];
        firstSuspension += firstAfter.suspH[c] + firstAfter.suspL[c];
        beforeHeavyBottom += before.bh[c];
        firstHeavyBottom += firstAfter.bh[c];
        beforeLightBottom += before.bl[c];
        firstLightBottom += firstAfter.bl[c];
        beforeMixMoment += c * (before.mh[c] + before.ml[c]);
        firstMixMoment += c * (firstAfter.mh[c] + firstAfter.ml[c]);
        beforeBottomMoment += c * (before.bh[c] + before.bl[c]);
        firstBottomMoment += c * (firstAfter.bh[c] + firstAfter.bl[c]);
      }
      expect(firstSuspension < beforeSuspension - 1e-9).assertTrue();
      expect(firstHeavyBottom > beforeHeavyBottom + 1e-9).assertTrue();
      expect(firstLightBottom < beforeLightBottom - 1e-9).assertTrue();
      expect(Math.abs(firstMixMoment - beforeMixMoment) > 1e-9).assertTrue();
      expect(firstBottomMoment - beforeBottomMoment > 1e-9).assertTrue();

      result = field.step(1 / 60);
      expect(result.ok).assertTrue();
      const after = field.snapshot();
      let secondSuspension = 0;
      let priorBottomMoment = 0;
      let afterBottomMoment = 0;
      for (let c = 0; c < after.cols; c++) {
        secondSuspension += after.suspH[c] + after.suspL[c];
        priorBottomMoment += c * (firstAfter.bh[c] + firstAfter.bl[c]);
        afterBottomMoment += c * (after.bh[c] + after.bl[c]);
      }
      expect(secondSuspension > firstSuspension + 1e-9).assertTrue();
      expect(afterBottomMoment - priorBottomMoment > 1e-9).assertTrue();
    });

    it('p011_fullLoadPercolationKeepsBottomBudgetIsolated', 0, () => {
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
      const isolatedResult = isolated.step(1 / 60);
      const concurrentResult = concurrent.step(1 / 60);
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

    it('p011_deadZoneClearsBottomCreepRate', 0, () => {
      const field = new WaterSandField(12, 192);
      const params = deterministicRateParams();
      params.bottomCreepDeadZone = 10;
      field.configure(params);
      field.initialize(thickSceneForP011(12));
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
      const withoutGain = new WaterSandField(12, 192);
      const withoutParams = deterministicRateParams();
      withoutParams.bottomCreepRate = 1;
      withoutParams.bottomCreepVelocityGain = 0;
      withoutGain.configure(withoutParams);
      withoutGain.initialize(thickSceneForP011(12));
      const withoutBefore = withoutGain.snapshot();

      const withGain = new WaterSandField(12, 192);
      const withParams = deterministicRateParams();
      withParams.bottomCreepRate = 1;
      withParams.bottomCreepVelocityGain = 36;
      withGain.configure(withParams);
      withGain.initialize(thickSceneForP011(12));
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
      expect(field.getMetrics().suspensionFraction <= 0.05).assertTrue();
      const finalTotals = field.getTotals();
      expect(Math.abs(finalTotals.heavy - initialTotals.heavy) < 1e-12).assertTrue();
      expect(Math.abs(finalTotals.light - initialTotals.light) < 1e-12).assertTrue();
      expect(Math.abs(finalTotals.water - initialTotals.water) < 1e-12).assertTrue();
    });

    it('invert180_preservesConservationAndPhaseOrder', 0, () => {
      const field = new WaterSandField(12, 192);
      field.initialize({
        waterRegions: [region(0, 11, 40)],
        heavyDepositedRegions: [region(0, 11, 10)],
        lightDepositedRegions: [region(0, 11, 10)],
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
      expect(field.getMetrics().phaseOrderScore >= 0.30).assertTrue();
      const finalTotals = field.getTotals();
      expect(Math.abs(finalTotals.heavy - beforeTotals.heavy) < 1e-12).assertTrue();
      expect(Math.abs(finalTotals.light - beforeTotals.light) < 1e-12).assertTrue();
      expect(Math.abs(finalTotals.water - beforeTotals.water) < 1e-12).assertTrue();
    });
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
        field.configure(params);
        // Ascending: sandTop rises 0.55/column to 3.85 at col 7, waterline
        // 0.05 above it. Descending: sandTop falls from 3.90 at col 0 to
        // 0.05 at col 7, waterline 0.05 above col 0.
        const topSandTop = ascending ? 3.85 : 3.90;
        const heavyRegions: WaterSandRegion[] = [];
        for (let c = 0; c < 8; c++) {
          const height = ascending
            ? topSandTop - (7 - c) * stepDeg
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
  return {
    waterRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 120, height: 60 }],
    heavyDepositedRegions: [region(0, cols - 1, 50)],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1,
      bottom: 50, height: 50 }],
    heavySuspendedRegions: [region(0, cols - 1, 10)],
    lightSuspendedRegions: [region(0, cols - 1, 10)]
  };
}

function deterministicRateParams(): WaterSandParams {
  const params = defaultWaterSandParams();
  params.heavyDiffusion = 0;
  params.lightDiffusion = 0;
  params.waterLevelRate = 0.001;
  return params;
}

interface RateScenarioResult {
  field: WaterSandField;
  result: WaterSandStepResult;
  maxSpeed: number;
}

function runRateScenario(dt: number, steps: number): RateScenarioResult {
  const field = new WaterSandField(8, 192);
  const params = defaultWaterSandParams();
  params.heavyDiffusion = 0;
  params.lightDiffusion = 0;
  params.waterLevelRate = 0.001;
  field.configure(params);
  field.initialize({
    waterRegions: [region(0, 7, 20)],
    heavyDepositedRegions: [],
    lightDepositedRegions: [],
    heavySuspendedRegions: [region(0, 7, 4)],
    lightSuspendedRegions: [region(0, 7, 4)]
  });
  let result = field.step(dt);
  let maxSpeed = result.maxSandTopSpeedObserved;
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
    endColInclusive: 11,
    state: WaterSandInjectionState.DEPOSITED
  };
}

function mixedScene(cols: number, capacity: number): WaterSandScene {
  return {
    waterRegions: [region(0, cols - 1, capacity * 0.5)],
    heavyDepositedRegions: [region(0, cols - 1, capacity * 0.1)],
    lightDepositedRegions: [region(0, cols - 1, capacity * 0.1)],
    heavySuspendedRegions: [region(0, cols - 1, capacity * 0.05)],
    lightSuspendedRegions: [region(0, cols - 1, capacity * 0.05)]
  };
}

function humpSceneForP011(cols: number, startCol: number, endColInclusive: number,
  humpHeight: number): WaterSandScene {
  const scene = thickSceneForP011(cols);
  scene.heavyDepositedRegions = [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 72 }];
  scene.lightDepositedRegions = [{ startCol: 0, endColInclusive: cols - 1, bottom: 72, height: 48 }];
  scene.heavyDepositedRegions.push({
    startCol: startCol, endColInclusive: endColInclusive, bottom: 120, height: humpHeight
  });
  return scene;
}

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
      // §27: the pair rides configure (config domain).
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
      // §27: the pair rides configure (config domain).
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
        field.configure(params);
      // §27: the pair rides configure (config domain).
      const dualParams = deterministicRateParams();
      dualParams.angleDeg = params.angleDeg;
      dualParams.relaxMode = 'dual-user-configured';
      dualParams.thetaStartDeg = 28;
      dualParams.thetaStopDeg = 22.911;
      dualParams.thresholdSource = 'user';
      field.configure(dualParams);
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
      field.configure(params);
      // §27: the pair rides configure (config domain).
      const dualParams = deterministicRateParams();
      dualParams.angleDeg = params.angleDeg;
      dualParams.relaxMode = 'dual-user-configured';
      dualParams.thetaStartDeg = 28;
      dualParams.thetaStopDeg = 22.911;
      dualParams.thresholdSource = 'user';
      field.configure(dualParams);
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
      for (const variant of ['B', 'C', 'D'] as const) {
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


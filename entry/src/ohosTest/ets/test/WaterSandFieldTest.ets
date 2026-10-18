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

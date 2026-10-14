import { describe, it, expect } from '@ohos/hypium';
import {
  WaterSandField,
  WaterSandInjection,
  WaterSandInjectionState,
  WaterSandPhase,
  WaterSandRegion,
  WaterSandScene
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
  });
}

function region(start: number, end: number, height: number): WaterSandRegion {
  return { startCol: start, endColInclusive: end, bottom: 0, height };
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

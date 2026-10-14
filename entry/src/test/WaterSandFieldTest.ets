import { describe, it, expect } from '@ohos/hypium';
import {
  WaterSandField,
  WaterSandInjectionState,
  WaterSandInjection,
  WaterSandPhase,
  WaterSandScene,
  WaterSandSeparationProfile,
  WaterSandSnapshot,
  WaterSandRegion
} from '../main/ets/physics/WaterSandField';

export default function waterSandFieldTest() {
  describe('WaterSandFieldTest', () => {
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
      (snapshot as { schemaVersion: number }).schemaVersion = 1;
      let thrown = false;
      try {
        field.copyStateTo(snapshot);
      } catch (error) {
        thrown = true;
      }
      expect(thrown).assertTrue();
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

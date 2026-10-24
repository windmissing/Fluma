// P-011 D1-B: ensure isolated M1/reference sinks are compiled by ArkTS.
// Device E2E remains a separate downstream task.
import { describe, it, expect } from '@ohos/hypium';
import { P011ParticleField, particleRouteEnabled, P011ParticleParams,
  P011ParticleStepIdentity } from '../../../main/ets/physics/P011ParticleField';
import { P011ParticlePrototypeBudget, P011ParticlePrototypeSink } from '../../../main/ets/physics/P011ParticlePrototype';

export default function p011ParticleM1Test(): void {
  describe('P011ParticleM1', () => {
    it('routeOffAndRemainder', 0, () => {
      expect(particleRouteEnabled(null, null)).assertFalse();
      const pool = new P011ParticleField(128, 4);
      const slot = pool.allocate({ mass: 1, x: 60.5, y: 100, vx: 0, vy: 0,
        phase: 0, sourceKind: 0, sourceId: 1 });
      expect(pool.retainAfterDeposit(slot, 0.25, 100.25, 1e-6)).assertEqual(0.75);
      expect(pool.totals().resting).assertEqual(1);
    });
    it('referenceSinkConstructsWithoutFieldOrRenderer', 0, () => {
      const pool = new P011ParticleField(128, 4);
      const params: P011ParticleParams = { massPackage: 0.1, nmax: 4,
        terminalSpeedHeavy: 64, terminalSpeedLight: 38, dragHeavy: 2, dragLight: 2,
        liftSpeedHeavy: 8, liftSpeedLight: 8, liftSpread: 2, mergeColRadius: 1,
        mergeHeightRadius: 2, contactEps: 1e-6, channelRateCap: 0 };
      const identity: P011ParticleStepIdentity = { runId: 'ark-m1', substepId: 1,
        dt: 1 / 60, effectiveTiltRad: 0, seed: 1, paramsRevision: 1, inputRevision: 1 };
      const budget = new P011ParticlePrototypeBudget(128, identity.dt, 1, 1);
      const sink = new P011ParticlePrototypeSink(pool, params, identity, budget);
      sink.audit();
      expect(pool.totals().count).assertEqual(0);
    });
  });
}

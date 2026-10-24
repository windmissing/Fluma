import { describe, it, expect } from '@ohos/hypium';
import p011ParticleM1Test from './P011ParticleM1Test';
import particleTest from './ParticleTest';
import physicsConfigTest from './PhysicsConfigTest';
import spatialHashTest from './SpatialHashTest';
import boundsTest from './BoundsTest';
import collisionSolverTest from './CollisionSolverTest';
import sandGridTest from './SandGridTest';
import heightFieldTest from './HeightFieldTest';
import waterSandFieldTest from './WaterSandFieldTest';
import liquidCAFieldTest from './LiquidCAFieldTest';
import p011SchedulePlayerTest from '../../../test/P011SchedulePlayerTest';
import p012WaterSandAdapterTest from '../../../test/P012WaterSandAdapterTest';

export default function abilityTest() {
  p011ParticleM1Test();
  describe('ActsAbilityTest', () => {
    it('assertContain', 0, () => {
      let a: string = 'abc';
      let b: string = 'b';
      expect(a).assertContain(b);
    });
  });

  particleTest();
  physicsConfigTest();
  spatialHashTest();
  boundsTest();
  collisionSolverTest();
  sandGridTest();
  heightFieldTest();
  waterSandFieldTest();
  liquidCAFieldTest();
  p011SchedulePlayerTest();
  p012WaterSandAdapterTest();
}

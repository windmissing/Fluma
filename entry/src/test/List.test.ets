import { describe, it, expect } from '@ohos/hypium';
import particleTest from './ParticleTest';
import physicsConfigTest from './PhysicsConfigTest';
import spatialHashTest from './SpatialHashTest';
import boundsTest from './BoundsTest';
import collisionSolverTest from './CollisionSolverTest';
import sandGridTest from './SandGridTest';
import heightFieldTest from './HeightFieldTest';
import waterSandFieldTest from './WaterSandFieldTest';
import liquidCAFieldTest from './LiquidCAFieldTest';
import p011InputModeTest from './P011InputModeTest';

export default function abilityTest() {
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
  p011InputModeTest();
}
